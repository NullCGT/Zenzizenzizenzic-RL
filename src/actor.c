/**
 * @file actor.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functionality relating to the creation, destruction, and
 description of actors.
 * @version 1.0
 * @date 2022-05-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "actor.h"
#include "color.h"
#include "invent.h"
#include "map.h"
#include "message.h"
#include "random.h"
#include "register.h"
#include "windows.h"
#include "gameover.h"
#include "spawn.h"
#include "ai.h"
#include "render.h"

struct hitdesc hitdescs_arr[MAX_HITDESC] = {
    { "Low",    WHITE,          LOW },
    { "Mid",    CYAN,           MID },
    { "High",   BRIGHT_CYAN,    HIGH },
    { "Grab",   BLUE,           GRAB }
};

/**
 * @brief test whether an actor can be pushed to a given map location.
 * 
 * @param actor The actor to be pushed.
 * @param x the x coordinate to test.
 * @param y the y coordinate to test.
 * @return int returns 1 if the actor can be pushed, 0 if not.
 */
int can_push(struct actor *actor, int x, int y) {
    if (!in_bounds(x, y) || is_blocked(x, y))
        return 0;
    if (actor->item && g.levmap[x][y].item_actor != NULL)
        return 0;
    if (actor->item == NULL && g.levmap[x][y].actor != NULL)
        return 0;
    return 1;
}

/**
 * @brief Find the nearest cell that an actor can be pushed to.
 * 
 * @param actor the actor to push.
 * @param x the x coordinate to begin searching at.
 * @param y the y coordinate to begin searching at.
 * @return int returns 1 if the actor was successfully pushed, 0 if not.
 */
int nearest_pushable_cell(struct actor *actor, int *x, int *y) {
    int nx = *x;
    int ny = *y;
    /* TODO: Replace this with breadth-first search that searches within several tiles. */
    if (can_push(actor, nx, ny)) {
        *x = nx;
        *y = ny;
        return 0;
    }
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            nx = *x + i;
            ny = *y + j;
            if (!in_bounds(nx, ny))
                continue;
            if (can_push(actor, nx, ny)) {
                *x = nx;
                *y = ny;
                return 0;
            }
        }
    }
    return 1;
}

/**
 * @brief Pushes an actor to a new location and updates the levmap
 accordingly.
 * 
 * @param actor The actor to be pushed.
 * @param dx The x coordinate the actor is to be pushed to.
 * @param dy The y coordinate the actor is to be pushed to.
 *
 * @return int Return 1 if the actor could not be pushed, 0 otherwise.
   It is the caller's responsibility to handle this situation.
 */
int push_actor(struct actor *actor, int dx, int dy) {
    mark_refresh(actor->x, actor->y);

    if ((actor->item && g.levmap[dx][dy].item_actor) ||
        (actor->item == NULL && g.levmap[dx][dy].actor)) {
        if (nearest_pushable_cell(actor, &dx, &dy)) {
            return 1;
        }
    }

    if (actor->item) {
        g.levmap[actor->x][actor->y].item_actor = NULL;
        actor->x = dx;
        actor->y = dy;
        g.levmap[actor->x][actor->y].item_actor = actor;
    } else {
        g.levmap[actor->x][actor->y].actor = NULL;
        actor->x = dx;
        actor->y = dy;
        g.levmap[actor->x][actor->y].actor = actor;
    }
    mark_refresh(actor->x, actor->y);
    return 0;
}

/**
 * @brief Removes an actor from both the map and the linked list
 of actors. Does not free memory associated with the actor.
 * 
 * @param actor The actor to be removed.
 * @return struct actor* The actor that has been removed.
 */
struct actor *remove_actor(struct actor *actor) {
    struct actor *cur = g.player;
    struct actor *prev = NULL;
    mark_refresh(actor->x, actor->y);
    if (actor == g.target)
        g.target = NULL;
    if (actor->item)
        g.levmap[actor->x][actor->y].item_actor = NULL;
    else
        g.levmap[actor->x][actor->y].actor = NULL;
    while (cur != NULL) {
        if (cur == actor) {
            if (prev != NULL) prev->next = cur->next;
            else {
                g.player = cur->next;
            }
            actor->next = NULL;
            return actor;
        }
        prev = cur;
        cur = cur->next;
    }
    logm_warning("Attempting to remove actor that is not there?");
    return actor;
}

/**
 * @brief Perform sanity checks to ensure that any sort of redundant data
 remains in sync.
 * 
 * @param actor The actor to perform sanity checks upon.
 */
void actor_sanity_checks(struct actor *actor) {
    if (g.levmap[actor->x][actor->y].actor != actor) {
        logm_warning("Sanity check fail: %s claims to be at (%d, %d), but is not there.",
              actor_name(actor, 0), actor->x, actor->y);
    }
}

/**
 * @brief Frees an actor and all members of the actor struct.
 * 
 * @param actor The actor to be freed.
 * @return The number of actors freed.
 */
int free_actor(struct actor *actor) {
    int count = 1;
    if (actor->name)
        free(actor->name);
    if (actor->invent)
        count += free_actor_list(actor->invent);
    if (actor->ai)
        free(actor->ai);
    if (actor->item)
        free(actor->item);
    if (actor->equip)
        free(actor->equip);
    free(actor);
    actor = NULL;
    return count;
}

/**
 * @brief Frees a contiguous linked list of actors.
 * 
 * @param actor The head of the linked list.
 * @return The number of actors freed.
 */
int free_actor_list(struct actor *actor) {
    struct actor *next;
    int count = 0;
    while (actor != NULL) {
        next = actor->next;
        count += free_actor(actor);
        actor = next;
    }
    return count;
}

/* TODO: Remove magic numbers. */
/* Make use of a rotating set of buffers, just like how NetHack does it. */
static int nbi = -1;
static char namebuf[4][64];

/**
 * @brief Return the name of an actor.
 * 
 * @param actor The actor to be named.
 * @param flags Flags that can modify the characteristics of the returned
 string. For a full list, see actor.h.
 * @return char* The name of the actor.
 */
char *actor_name(struct actor *actor, unsigned flags) {
    struct actor *perm_actor = actor->item ? g.items[actor->id] : g.monsters[actor->id];
    int no_given_name = (actor->name->given_name[0] == '\0');
    /* Increase the namebuffer index. */
    nbi = (nbi + 1) % 4;
    
    /* Reset the name buffer. */
    memset(namebuf[nbi], 0, 64);
    const char *actname;
    if ((!(flags & NAME_EX)) && !no_given_name) actname = actor->name->given_name;
    else if (!(perm_actor->known & KNOW_NAME) && perm_actor->name->appearance[0] != '\0') 
        actname = actor->name->appearance;
    else
        actname = actor->name->real_name;

    if ((flags & NAME_THE) && !actor->unique && no_given_name) {
        snprintf(namebuf[nbi], sizeof(namebuf[nbi]), "the %s", actname);
    } else if ((flags & NAME_YOUR) && !actor->unique && no_given_name) {
        snprintf(namebuf[nbi], sizeof(namebuf[nbi]), "your %s", actname);
    } else if ((flags & NAME_A) && !actor->unique && no_given_name) {
        snprintf(namebuf[nbi], sizeof(namebuf[nbi]), "%s %s", !vowel(namebuf[nbi][0]) ? "a" : "an", actname);
    } else {
        snprintf(namebuf[nbi], sizeof(namebuf[nbi]), "%s", actname);
    }
    /* Explanation */
    if ((flags & NAME_EX) && !no_given_name) {
        snprintf(namebuf[nbi] + strlen(namebuf[nbi]), sizeof(namebuf[nbi]), " named \"%s\"", 
                 actor->name->given_name);
    }
    /* Equip Status */
    if ((flags & NAME_EQ) && actor->item && actor->item->slot >= 0) {
        snprintf(namebuf[nbi] + strlen(namebuf[nbi]), sizeof(namebuf[nbi]), " %s", 
                 slot_types[actor->item->slot].slot_desc);
    }
    /* Capitalization */
    if ((flags & NAME_CAP) && namebuf[nbi][0] > 'Z') {
        namebuf[nbi][0] = namebuf[nbi][0] - 32;
    }
    return namebuf[nbi];
}

/**
 * @brief Determine if an actor is "in danger."
 * 
 * @param actor The actor whose danger needs to be determined.
 * @return int Return 1 if in danger, 0 otherwise.
 */
int in_danger(struct actor *actor) {
    return actor->hpmax / actor->hp >= 2;
}

#define MAX_HEALTH_COND 4
static const char *health_conditions[MAX_HEALTH_COND] = { "Faint", "Battered", "Bruised", "OK"};

/**
 * @brief Describe the health of an actor by way of a string.
 * 
 * @param actor The actor to describe the health of.
 * @return const char* A short description of the actor's health.
 */
const char *describe_health(struct actor *actor) {
    int index = 0;
    int fraction;

    if (actor->hp == actor->hpmax)
        return "Perfect";
    if (actor->hp > actor->hpmax)
        return "Beyond Perfect";
    if (actor->hp <= 0)
        return "Super Dead";

    fraction = actor->hpmax / MAX_HEALTH_COND;

    while (((index + 1) * fraction) < actor->hp) {
        index++;
    }

    return health_conditions[min(MAX_HEALTH_COND - 1, index)];
}

void identify_actor(struct actor *actor, int silent) {
    struct actor *perm_actor = actor->item ? g.items[actor->id] : g.monsters[actor->id];
    if (!(perm_actor->known & KNOW_NAME) && perm_actor->name->appearance[0] != '\0') {
        perm_actor->known |= KNOW_NAME;
        if (!silent)
            logm("%s identifies the %s as %s.",
                 actor_name(g.player, NAME_CAP | NAME_THE),
                 actor->name->appearance, actor_name(actor, NAME_A));
    }
}