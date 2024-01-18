/**
 * @file combat.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Combat-related functionality.
 * @version 1.0
 * @date 2022-05-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <math.h>

#include "actor.h"
#include "ai.h"
#include "combat.h"
#include "register.h"
#include "gameover.h"
#include "message.h"
#include "random.h"
#include "invent.h"
#include "action.h"

#define BHIT 0
#define BBLOCK 1
#define BTECH 2

void apply_knockback(struct actor*, int, int, int);
int attack_roll(struct actor *, struct actor *, struct attack *);

/**
 * @brief Perform an attack.
 * 
 * @param aggressor The actor initiating the attack.
 * @param target The target of the attack.
 * @param multiplier The damage multiplier on the attack.
 * @return int The cost in energy of making the attack.
 */
int do_attack(struct actor *aggressor, struct actor *target, int multiplier) {
    struct attack attack = choose_attack(aggressor, target);
    int damage = attack.dam * multiplier;
    int result = !is_aware(target, aggressor) ? BHIT : weak_res(attack.hitdescs, target->stance);
    int cost = attack.recovery;
    int color = (target == g.player ? RED : GREEN);
    /* Feedback color */
    if (result)
        color = (target == g.player) ? BRIGHT_GREEN : BRIGHT_RED;
    else
        color = (target == g.player) ? BRIGHT_RED : BRIGHT_GREEN;
    /* Using a grab automatically lets one tech any incoming throws. */
    if (attack.hitdescs == GRAB)
        change_stance(aggressor, STANCE_TECH, 1);
    /* Hit or miss */
    if (!attack_roll(aggressor, target, &attack)) {
        if (aggressor == g.player) {
            g.target = target;
        } 
        logm("%s misses %s.", actor_name(aggressor, NAME_CAP | NAME_THE), actor_name(target, NAME_THE));
        return cost;
    }
    /* Feedback */
    if (result == BHIT) {
        if (aggressor == g.player)
            g.target = target;
        target->combo_counter++;
        logma(color, "%s %s %s! x%d Combo!", 
                actor_name(aggressor, NAME_CAP | NAME_THE),
                attack.hitdescs & GRAB ? "throws" : "hits",
                actor_name(target, NAME_THE),
                target->combo_counter);
        change_stance(target, STANCE_STUN, (target->stance == STANCE_STUN));
        if (target->combo_counter >= HITSTUN_DETERIORATION)
            target->energy = -1 * attack.stun / (target->combo_counter - HITSTUN_DETERIORATION + 1);
        else
            target->energy = -1 * attack.stun;
    } else if (result == BBLOCK) {
        damage /= 2; /* TEMPORARY */
        // target->energy = -0.5 * attack.stun;
        logma(color, "%s blocks %s's strike. (-%d)",
                actor_name(target, NAME_CAP | NAME_THE),
                actor_name(aggressor, NAME_THE), damage);
    } else if (result == BTECH) {
        logma(color, "%s techs %s's throw!",
                actor_name(target, NAME_CAP | NAME_THE),
                actor_name(aggressor, NAME_THE));
        damage = 0;
    }
    /* Make them aware */
    if (!is_aware(target, aggressor))
        make_aware(target, aggressor, 1);

    /* Apply damage and knockback */
    if  (target->combo_counter > 1)
        target->hp -= (damage * powf(DAMAGE_SCALING, (target->combo_counter - 1)));
    else
        target->hp -= damage;
    if (target == g.player && target->hp <= 0) {
        g.target = aggressor;
        logma(BRIGHT_RED, "%s is KO'd...", actor_name(target, NAME_CAP | NAME_THE));
        if ((g.explore || g.debug) && !yn_prompt("Stay knocked out?", 0)) {
            logm("%s randomly regains consciousness.", actor_name(target, NAME_CAP | NAME_THE));
            g.player->hp = g.player->hpmax;
            return cost;
        }
        end_game(0);
    } else if (target != g.player && target->hp <= 0) {
        logma(BRIGHT_YELLOW, "%s is KO'd.", actor_name(target, NAME_CAP | NAME_THE));
        identify_actor(target, 0);
        remove_actor(target);
        free_actor(target);
    } else if (target && attack.kb > 0 && result == BHIT) {
        if (target->x ==  aggressor->x && target->y == aggressor->y) {
            /* Thrown items can share a cell with an opponent while knocking
               them back. Why not lean into this with a special state? */
            target->energy -= TURN_FULL;
            logm("%s is knocked into the air!", actor_name(target, NAME_CAP | NAME_THE));
        } else
            apply_knockback(target, attack.kb, target->x - aggressor->x, target->y - aggressor->y);
    }
    return cost;
}

/**
 * @brief Apply knockback momentum to a monster
 * 
 * @param target the actor to apply knockback to.
 * @param velocity the number of tiles to be knocked back.
 * @param x horizontal direction, between -1 and 1 inclusive.
 * @param y vertical direction, between -1 and 1 inclusive.
 */
void apply_knockback(struct actor* target, int velocity, int x, int y) {
    struct actor *mon;
    int nx, ny;

    if (target == g.player) {
        f.update_fov = 1;
    }

    while (velocity > 0) {
        nx = target->x + x;
        ny = target->y + y;

        /* Ideally, this should not happen. */
        if (!in_bounds(nx, ny)) {
            logm_warning("Attempting to knock target out of bounds?");
            break;
        }

        if (is_blocked(nx, ny)) {
            if (target->can_tech) {
                logma(target == g.player ? BRIGHT_GREEN : BRIGHT_RED, "%s performs a breakfall againat the %s.", 
                            actor_name(target, NAME_THE),
                            g.levmap[nx][ny].pt->name);
                target->energy = TURN_FULL;
            } else {
                logma(target == g.player ? BRIGHT_RED : BRIGHT_GREEN, "%s bounces off the %s!",
                          actor_name(target, NAME_CAP | NAME_THE), g.levmap[nx][ny].pt->name);
                target->energy -= TURN_FULL;
                target->can_tech = 1;
            }
            return;
        }
        /* If there is a monster present, transfer energy to that monster. */
        mon = MON_AT(nx, ny);
        if (mon && target != mon) {
            if (target == g.player)
                logm("You collide with %s!", actor_name(mon, NAME_A));
            else if (mon == g.player)
                logm("%s collides with you. Momentum gained!",
                     actor_name(target, NAME_THE));
            else
                logm("%s crashes into %s.", actor_name(target, NAME_THE), 
                     actor_name(mon, NAME_A));
            mon->energy += TURN_FULL;
            target->energy = 0;
            return;
        }
        /* Perform movement */
        push_actor(target, nx, ny);
        velocity--;
    }
}

/**
 * @brief 
 * 
 * @param hitdesc The hitdesc to check.
 * @param stance The stance to be checked against.
 * @return int Attack result.
 * hit = 0
 * block = 1
 * tech = 2
 */
int weak_res(short hitdesc, short stance) {
    logm("%d, %d", hitdesc, stance);
    if ((stance & STANCE_STUN)) { /* If stunned, all attacks hit. */
        logm("STUN");
        return BHIT;
    }
    if ((hitdesc & GRAB) && (stance & GRAB))
        return BTECH;
    else if (stance & hitdesc)
        return BBLOCK;
    return BHIT;
}

/**
 * @brief Perform an attack roll.
 * 
 * @param aggressor The actor making the attack.
 * @param target The actor defending.
 * @param attack A pointer to the attack being made.
 * @return int Returns 1 if the attack should hit. Otherwise, returns zero.
 */
int attack_roll(struct actor *aggressor, struct actor *target, struct attack *attack) {
    int goal = 0;
    /* TODO: Check status effects and equips on target and aggressor. */
    goal += calculate_accuracy(aggressor, attack);
    goal -= calculate_evasion(target);

    /* An attack against an unaware or stunned opponent is an automatic hit. */
    if (!is_aware(target, aggressor) || (target->stance == STANCE_STUN)) {
        goal = 100;
    }

    return (rndrng(1, 101) <= goal);
}

/**
 * @brief Calculate the evasion percentage of an actor.
 * 
 * @param actor The actor to calculate evasion for.
 * @return int The evasion percentage in integer form.
 */
int calculate_evasion(struct actor *actor) {
    return actor->evasion + actor->temp_evasion;
}

/**
 * @brief Calculate the accuracy percentage of an actor.
 * 
 * @param actor The actor to calculate accuracy for.
 * @param attack The attack the actor is using.
 * @return int The accuracy percentage in integer form.
 */
int calculate_accuracy(struct actor *actor, struct attack *attack) {
    return actor->temp_accuracy + actor->accuracy + attack->accuracy;
}

/**
 * @brief Get the active attack
 * 
 * @param actor the actor whose attacks we are indexing
 * @param index the index of the attack to get
 * @return struct attack* The pointer to the active attack.
 */
struct attack *get_active_attack(struct actor *actor, int index) {
    if (EWEP(actor) && !EOFF(actor)) return &EWEP(actor)->attacks[index % MAX_ATTK];
    else if (EWEP(actor) && EOFF(actor)) return index < MAX_ATTK ? &EWEP(actor)->attacks[index] : &EOFF(actor)->attacks[index % MAX_ATTK];
    else if (EOFF(actor)) return &EOFF(actor)->attacks[index % MAX_ATTK];
    else return &actor->attacks[index % MAX_ATTK];
    return NULL;
}

/**
 * @brief Change an actor's stance. Often called as an action, but not always.
 * 
 * @param actor The actor changing stances.
 * @param stance The stance to change to.
 * @return int The cost in points of changing stances.
 */
int change_stance(struct actor *actor, short stance, int silent) {
    int changed = !(stance == actor->stance);
    if (!silent && actor->stance == STANCE_STUN && changed) {
        logm("%s is no longer stunned.", actor_name(actor, NAME_CAP | NAME_THE));
    } else {
        switch(stance) {
            case STANCE_STAND:
                if (!silent && actor == g.player && changed)
                    logm("%s stands up.", actor_name(actor, NAME_CAP | NAME_THE));
                break;
            case STANCE_CROUCH:
                if (!silent && actor == g.player && changed)
                    logm("%s crouches.", actor_name(actor, NAME_CAP | NAME_THE));
                break;
            case STANCE_TECH:
                if (!silent && actor == g.player && changed)
                    logm("%s prepares to tech a throw.", actor_name(actor, NAME_CAP | NAME_THE));
                break;
            case STANCE_STUN:
                if (!silent)
                    logm("%s is stunnded!", actor_name(actor, NAME_CAP | NAME_THE));
                break;
            default:
                logm_warning("%s shifted to unknown stance %d?", actor_name(actor, NAME_EX), stance);
                break;
        }
    }
    if (changed && actor == g.player)
        stop_running();
    actor->stance = stance;
    return 50;
}