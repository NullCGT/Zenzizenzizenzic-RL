
/**
 * @file spawn.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functionality related to spawning a new actor.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ai.h"
#include "spawn.h"
#include "map.h"
#include "register.h"
#include "random.h"
#include "actor.h"
#include "message.h"
#include "invent.h"
#include "parser.h"
#include "windows.h"
#include "action.h"

struct actor *spawn_named_actor(const char *name, int x, int y);
void mod_attributes(struct actor *);
void mod_ai(struct ai *);

/**
 * @brief Initialize the name struct of an actor.
 * 
 * @param actor The actor to initialize the name for.
 * @param permname The real name of the actor.
 * @return A pointer to the newly-created name struct.
 */
struct name *init_permname(struct actor *actor, const char *permname, const char *appearance) {
    actor->name = (struct name *) malloc(sizeof(struct name));
    *actor->name = (struct name) { 0 };
    strcpy(actor->name->real_name, permname);
    if (appearance)
        strcpy(actor->name->appearance, appearance);
    actor->name->given_name[0] = '\0';
    return actor->name;
}

/**
 * @brief Spawn a creature at a location. Wrapper for spawn_named_actor.
 * 
 * @param name The name of the creature to spawn.
 * @param x The x coordinate to spawn at.
 * @param y THe y coordinate to spawn at.
 * @return struct actor* A pointer to the creature spawned.
 */
struct actor *spawn_named_creature(const char *name, int x, int y) {
    struct actor *actor = NULL;
    for (int i = 0; i < g.total_monsters; i++) {
        if (!strncasecmp(name, g.monsters[i]->name->real_name, sizeof(g.monsters[i]->name->real_name))) {
            actor = spawn_actor(g.monsters, i, x, y);
            break;
        }
    }
    return actor;
}

/**
 * @brief Spawn an item at a location. Wrapper for spawn_named_actor.
 * 
 * @param name The name of the item to spawn.
 * @param x The x coordinate to spawn at.
 * @param y THe y coordinate to spawn at.
 * @return struct actor* A pointer to the item spawned.
 */
struct actor *spawn_named_item(const char *name, int x, int y) {
    struct actor *actor = NULL;
    for (int i = 0; i < g.total_items; i++) {
        if (!strncasecmp(name, g.items[i]->name->real_name, sizeof(g.items[i]->name->real_name))
            || !strncasecmp(name, g.items[i]->name->appearance, sizeof(g.items[i]->name->appearance))) {
            actor = spawn_actor(g.items, i, x, y);
            break;
        }
    }
    return actor;
}

struct actor *add_actor_to_main(struct actor *actor) {
    struct actor *cur_actor = g.player;
    struct actor *prev_actor = cur_actor; 

    while (cur_actor != NULL) {
        prev_actor = cur_actor;
        cur_actor = cur_actor->next;
    }
    if (prev_actor == NULL) {
        prev_actor = actor;
    } else {
        prev_actor->next = actor;
    }
    actor->next = NULL;
    return actor;
}

/**
 * @brief Spawn an actor at a location. If an invalid
 location is passed in, then choose a random one.
 * 
 * @param name The filename of the actor's JSON definition.
 * @param x The x coordinate to spawn at.
 * @param y THe y coordinate to spawn at.
 * @return struct actor* A pointer to the actor spawned.
 */
struct actor *spawn_named_actor(const char *name, int x, int y) {
    struct actor *actor = NULL;
    int i = 0;

    for (int j = 0; j < MAX_MONSTERS; j++) {
        if (!g.monsters[j]) break;
        if (!strcmp(actor_name(g.monsters[j], NAME_EX), name))
            actor = g.monsters[j];
    }

    if (!actor)
        return NULL;

    /* Add the actor the list of actors. */
    add_actor_to_main(actor);

    /* Spawn at a given location. */
    if (!in_bounds(x, y)) {
        struct coord c = rand_open_coord();
        x = c.x;
        y = c.y;
    }
    while (push_actor(actor, x, y) && i++ < 10) {
        struct coord c = rand_open_coord();
        x = c.x;
        y = c.y;
    }
    if (i >= 10) free_actor(actor);

    return actor;
}

/**
 * @brief Debug action for summoning a creature.
 * 
 * @return int The cost of summoning.
 */
int debug_summon(void) {
    char buf[MAXNAMESIZ] = {'\0'};
    struct actor *actor = NULL;
    if (!g.debug) {
        logm("Team %s has no one to summon.", g.userbuf);
        return 0;
    }
    text_entry("What creature do you want to summon?", buf, MAXNAMESIZ);
    actor = spawn_named_creature(buf, g.player->x, g.player->y);
    if (actor) {
        logm("Debug Output: Summoned %s.", actor_name(actor, NAME_A));
    } else {
        logm("Error: Unable to summon a creature called \"%s.\"", buf);
    }
    return 0;
}

/**
 * @brief Debug action for wishing for an item.
 * 
 * @return int The cost of wishing.
 */
int debug_wish(void) {
    char buf[MAXNAMESIZ] = {'\0'};
    struct actor *actor = NULL;
    if (!g.debug) {
        logm("If you want to achieve your wish, you're going to have to fight for it.");
        return 0;
    }
    text_entry("What item do you wish for?", buf, MAXNAMESIZ);
    /* TODO: Put it directly in the inventory instead of using this jank. */
    actor = spawn_named_item(buf, g.player->x, g.player->y);
    if (!actor) {
        logm("Error: Unable to create an item called \"%s.\"", buf);
        return 0;
    }
    logm("Created %s.", actor_name(actor, NAME_EX | NAME_A));
    pick_up(g.player, actor->x, actor->y);
    return 0;
}

/**
 * @brief Mutate a given actor's attributes in order to provide some variance.
 * 
 * @param actor The actor to be mutated.
 */
void mod_attributes(struct actor *actor) {
    if (!actor)
        return;
    actor->hpmax += rndmx(1 + g.depth);
    actor->hp = actor->hpmax;
    for (int i = 0; i < MAX_ATTK; i++) {
        if (is_noatk(actor->attacks[i])) continue;
        actor->attacks[i].accuracy += rndrng(-4, 5);
        actor->attacks[i].dam += rndrng(-1, 2);
    }
}

/**
 * @brief Mutate an actor's ai in order to provide some slight variance.
 * 
 * @param ai The ai to be mutated.
 */
void mod_ai(struct ai *ai) {
    if (!ai)
        return;
    ai->seekdef += rndmx(3);
}

/**
 * @brief Spawn the actor from position index in array list at coordinates x, y
 * 
 * @param list the array of actors to spawn from
 * @param index the index into the array
 * @param x the x coordinate to spawn the actor at
 * @param y the y coordinate to spawn the actor at
 * @return struct actor* 
 */
struct actor *spawn_actor(struct actor **list, int index, int x, int y) {
    int i = 0;
    struct actor *actor = malloc(sizeof(struct actor));

    memcpy(actor, list[index], sizeof(struct actor));

    init_permname(actor, list[index]->name->real_name, list[index]->name->appearance);

    if (actor->ai) {
        actor->ai = malloc(sizeof(struct ai));
        memcpy(actor->ai, list[index]->ai, sizeof(struct ai));
        actor->ai->parent = actor;
        /* If a monster is created during level gen, they are a guardian. */
        if (f.mode_mapgen) actor->ai->guardian = 1;
    }
    
    if (actor->item) {
        actor->item = malloc(sizeof(struct item));
        actor->item = memcpy(actor->item, list[index]->item, sizeof(struct item));
        actor->item->parent = actor;
    }

    actor->invent = NULL;
    actor->stance = STANCE_STAND; /* Initial set, not a change! */
    actor->old_stance = STANCE_STAND;
    
    if (actor->equip)
        init_equip(actor);

    actor->next = NULL;
    add_actor_to_main(actor);

    /* Spawn at a given location. */
    if (!in_bounds(x, y)) {
        struct coord c = rand_open_coord();
        x = c.x;
        y = c.y;
    }
    while (push_actor(actor, x, y) && i++ < 10) {
        struct coord c = rand_open_coord();
        x = c.x;
        y = c.y;
    }
    if (i >= 10) {
        free_actor(actor);
        actor = NULL;
    }
    return actor;
}