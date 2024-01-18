/**
 * @file action.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Contains functionality related to actions. Actions are decisions
 made by the player or other actors which may cost energy.
 * @version 1.0
 * @date 2022-05-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "register.h"
#include "action.h"
#include "message.h"
#include "map.h"
#include "windows.h"
#include "render.h"
#include "gameover.h"
#include "save.h"
#include "combat.h"
#include "invent.h"
#include "spawn.h"
#include "ai.h"

int display_structinfo(void);
int do_nothing(void);
int is_player(struct actor *);
int autoexplore(void);
struct action *travel(void);
int look_down(void);
int lookmode(void);
int display_help(void);
int list_actions_exec(void);
int change_hud_mode(void);
int climb(struct actor *, int);

#define MOV_ACT(name, index, code, alt_code) \
    { name, index, code, alt_code, {.dir_act=move_mon}, 0, 1, 1 }
#define DIR_ACT(name, index, code, alt_code, func, debug_only, movement) \
    { name, index, code, alt_code, {.dir_act=func}, debug_only, 1, movement }
#define VOID_ACT(name, index, code, alt_code, func, debug_only, movement) \
    { name, index, code, alt_code, {.void_act=func}, debug_only, 0, movement }

/* Be VERY careful that you have correctly specified a function as directed
   action versus a void action. If you are not careful, you risk corrupting
   the stack pointer. */
/* The order of this array is largely agnostic, with the notable exception of
   the movement keys. Doing nothing and movement keys must come first, and must
   be in the correct dorder. Besides those, just try to keep things organized so
   that the most common inputs appear early in the list, in order to keep the
   number of function calls as low as possible.*/
struct action actions[ACTION_COUNT] = {
    VOID_ACT("none",        A_NONE,        -1,  -1,  do_nothing, 0, 0),
    MOV_ACT("West",         A_WEST,       'h',  'H'),
    MOV_ACT("East",         A_EAST,       'l',  'L'),
    MOV_ACT("North",        A_NORTH,      'k',  'K'),
    MOV_ACT("South",        A_SOUTH,      'j',  'J'),
    MOV_ACT("Northwest",    A_NORTHWEST,  'y',  'Y'),
    MOV_ACT("Northeast",    A_NORTHEAST,  'u',  'U'),
    MOV_ACT("Southwest",    A_SOUTHWEST,  'b',  'B'),
    MOV_ACT("Southeast",    A_SOUTHEAST,  'n',  'N'),
    MOV_ACT("Rest",         A_REST,       '.',  'z'),
    DIR_ACT("Open",         A_OPEN,       'o',  -1,  open_door, 0, 0),
    DIR_ACT("Close",        A_CLOSE,      'c',  -1,  close_door, 0, 0),
    DIR_ACT("Pick Up",      A_PICK_UP,    ',',  'g', pick_up, 0, 0),
    VOID_ACT("Look",        A_LOOK,       ';',  -1,  lookmode, 0, 0),
    DIR_ACT("Ascend",       A_ASCEND,     '<',  -1,  ascend, 0, 0),
    DIR_ACT("Descend",      A_DESCEND,    '>',  -1,  descend, 0, 0),
    VOID_ACT("Look Down",   A_LOOK_DOWN,  ':',  -1,  look_down, 0, 0),
    VOID_ACT("Explore",     A_EXPLORE,    'x',  -1,  autoexplore, 0, 0),
    VOID_ACT("Inventory",   A_INVENT,     'i',  -1,  display_invent, 0, 0),
    VOID_ACT("Change HUD",  A_TAB_HUD,    '\t', -1,  change_hud_mode, 0, 0),
    VOID_ACT("Full Log",    A_FULLSCREEN, 'M',  -1,  fullscreen_action, 0, 0),
    VOID_ACT("Help",        A_HELP,       '?',  -1,  display_help, 0, 0),
    VOID_ACT("Save",        A_SAVE,       'S',  27,  save_exit, 0, 0),
    VOID_ACT("Quit",        A_QUIT,       'Q',  -1,  do_quit, 0, 0),
    VOID_ACT("Direct Input", A_LIST,      '#',  -1,  list_actions_exec, 0, 0),
// DEBUG ACTIONS
    VOID_ACT("debugmap",    A_MAGICMAP,   '[',  -1,  magic_mapping, 1, 0),
    VOID_ACT("debugheat",   A_HEAT,       ']',  -1,  switch_viewmode, 1, 0),
    VOID_ACT("debugsummon", A_SPAWN,      '\\', -1,  debug_summon, 1, 0),
    VOID_ACT("debugstructs",A_STRUCTINFO,  -1,  -1,  display_structinfo, 1, 0),
    VOID_ACT("debugwish",   A_WISH,       '-',  -1,  debug_wish, 1, 0)
};

/**
 * @brief Display size information about game structs.
 * 
 * @return int The cost of executing this action.
 */
int display_structinfo(void) {
    logm("Size of Actor Struct: %d", sizeof(struct actor));
    logm("Size of Tile Struct: %d", sizeof(struct tile));
    logm("Size of Item Struct: %d", sizeof(struct item));
    return 0;
}

/**
 * @brief Output a string representation of an action and the associated input.
 * 
 * @param index Index of the action in the actions list.
 * @return char* String representing the action. Must be manually freed afterward.
 */
char *stringify_action(int index) {
    char *buf = NULL;
    buf = (char *) malloc(64);
    if (actions[index].code == '\t')
        snprintf(buf, 64, "[Tab] %s", actions[index].name);
    else
        snprintf(buf, 64, "[%c] %s", actions[index].code, actions[index].name);
    return buf;
}

/**
 * @brief Does nothing.
 * 
 * @return int Returns the cost of doing nothing. Hint: It's nothing.
 */
int do_nothing(void) {
    return 0;
}

/**
 * @brief Determine if a given actor is the player
 * 
 * @param actor The actor to check. 
 * @return int True if the actor passed in is the player.
 */
int is_player(struct actor* actor) {
    return (actor == g.player);
}

/**
 * @brief Moves a creature a relative amount in a given direction.
 * 
 * @param mon The creature to be moved.
 * @param x The number of cells to move in along the x axis.
 * @param y The number of cells to move along the y axis.
 * @return int The cost of the action in energy.
 */
int move_mon(struct actor* mon, int x, int y) {
    struct actor *target;
    int nx = mon->x + x;
    int ny = mon->y + y;
    int ret = 0;

    /* Immediately exit if out of bounds */
    if (!in_bounds(nx, ny)) {
        if (is_player(mon)) {
            logm("Run away? Not likely!");
            stop_running();
            return 0;
        } else {
            return TURN_FULL;
        }
    }
    /* If there is someone there, attack them! */
    target = MON_AT(nx, ny);
    if (target && target != mon) {
        return do_attack(mon, target, 1);
    }
    /* Tile-based effects, such as walls and doors. */
    if (g.levmap[nx][ny].pt->func) {
        ret = g.levmap[nx][ny].pt->func(mon, nx, ny);
        if (ret) {
            if (mon == g.player) stop_running();
            return ret;
        }
    }
    /* Resting costs movement and also puts one in a tech state. */
    if (!x && !y) {
        change_stance(mon, STANCE_TECH, 0);
        return mon->speed;
    }
    /* Handle blocked movement */
    if (is_blocked(nx, ny)) {
        if (is_player(mon)) {
            stop_running();
            return 0;
        } else {
            return TURN_FULL;
        }
    }
    /* Perform movement */
    push_actor(mon, nx, ny);
    /* Log any messages about what you stepped over. */
    /* The FOV update here is temporary. In the future, we can modify this. */
    if (is_player(mon)) {
        f.update_fov = 1;
        if (ITEM_AT(mon->x, mon->y)) {
            logm("%s steps over %s.", actor_name(g.player, NAME_CAP), actor_name(ITEM_AT(mon->x, mon->y), NAME_A));
        }
    }
    return mon->speed;
}

/**
 * @brief Describe the current cell the player is located at.
 * 
 * @return int The cost of the action in energy.
 */
int look_down() {
    if (ITEM_AT(g.player->x, g.player->y)) {
        logm("%ss glance down. There is %s resting on the %s here.",
            actor_name(g.player, NAME_CAP),
            actor_name(ITEM_AT(g.player->x, g.player->y), NAME_A),
            g.levmap[g.player->x][g.player->y].pt->name);
    } else {
        logm("%s glances down at the %s.",
            actor_name(g.player, NAME_CAP), 
            g.levmap[g.player->x][g.player->y].pt->name);
    }
    return 0;
}

/**
 * @brief Pick up an item located at a given creature's location.
 * 
 * @param creature The creature picking up an item
 * @param x The x coordinate of the item to be picked up
 * @param y The y coordinate of the item to be picked up
 * @return int The cost of picking up an item in energy.
 Failing to pick up an item costs 100 energy.
 Picking up an item costs 50 energy.
 Fumbling an item in an attempt to pick it up costs 50 energy.
 */
int pick_up(struct actor *creature, int x, int y) {
    struct actor *item = ITEM_AT(x, y);
    if (!item) {
        logm("%s brushes the %s beneath them with their fingers. There is nothing there to pick up.",
            actor_name(creature, NAME_THE),
            g.levmap[x][y].pt->name);
        return 0;
    }
    /* Remove the actor. If we cannot put it in the inventory, put it back. */
    remove_actor(item);
    if (add_to_invent(creature, item)) {
        logm("%s picks up %s. [%c]", 
            actor_name(creature, NAME_THE),
            actor_name(item, NAME_THE), item->item->letter);
        return 50;
    } else {
        push_actor(item, creature->x, creature->y);
        logm("%s is holding too much to pick up %s.",
            actor_name(creature, NAME_THE),
            actor_name(item, NAME_THE));
        return 50;
    }
}

/**
 * @brief Enter into lookmode. Takes in blocking input, wresting control
 of the player away from the user.
 * 
 * @return int The cost in energy. Should always return zero.
 */
int lookmode(void) {
    struct action *act;
    struct coord move_coord;

    f.mode_look = 1;
    g.cursor_x = g.player->x;
    g.cursor_y = g.player->y;
    logm("What should %s examine?", actor_name(g.player, NAME_THE));
    while (1) {
        f.update_map= 1;
        render_all();
        act = get_action();
        if (act->movement) {
            move_coord = action_to_dir(act);
            g.cursor_x += move_coord.x;
            g.cursor_y += move_coord.y;
        } else if (act->index == A_LOOK) {
            look_at(g.cursor_x, g.cursor_y);
            f.mode_look = 0;
            f.update_map = 1;
            render_all();
            return 0;
        }
    }
    return 0;
}

/**
 * @brief Describe a location and the actors at that location.
 * 
 * @param x The x coordinate of the location to be described.
 * @param y The y coordinate of the location to be described.
 * @return int The cost in energy of looking at the location.
 Should always return zero, unless we implement some sort of bizarre monster
 that can steal turns if you examine it.
 */
int look_at(int x, int y) {
    if (!in_bounds(x, y)) {
        logm("There is nothing to see there.");
        return 0;
    } else if (is_visible(x, y)) {
        if (MON_AT(x, y)) {
            g.target = MON_AT(x, y);
            if (MON_AT(x, y) == g.player)
                logm("It's %s, a member of team %s.", actor_name(MON_AT(x, y), NAME_A),
                                                      g.userbuf);
            else
                logm("That is %s.", actor_name(MON_AT(x, y), NAME_A));
        } else if (ITEM_AT(x, y)) {
            logm("That is %s.", actor_name(ITEM_AT(x, y), NAME_A));
        } else {
            logm("That is %s %s.", an(g.levmap[x][y].pt->name), g.levmap[x][y].pt->name);
        }
    } else if (is_explored(x, y)) {
        logm("That is %s %s.", an(g.levmap[x][y].pt->name), g.levmap[x][y].pt->name);
    } else {
        logm("That area is unexplored.");
    }
    return 0;
}

/**
 * @brief Calculates a direction to automatically explore in.
 * 
 * @return int The index of the action that the player will perform.
 */
int autoexplore(void) {
    int lx = IMPASSABLE;
    int ly = IMPASSABLE;
    int lowest = MAX_HEAT;
    
    /* Regenerate the heatmap if exploration is just beginning. */
    if (!f.mode_explore) {
        f.mode_explore = 1;
        do_heatmaps(heatmaps[HM_EXPLORE].field, 0);
    }
    // Do things
    for (int x = -1; x <= 1; x++) {
        if (x + g.player->x < 0 || x + g.player->x >= MAPW) continue;
        for (int y = -1; y <= 1; y++) {
            if (!x && !y) continue;
            if (y + g.player->y < 0 || y + g.player->y >= MAPH) continue;
            if (g.heatmap[HM_EXPLORE][x + g.player->x][y + g.player->y] <= lowest) {
                lowest = g.heatmap[HM_EXPLORE][x + g.player->x][y + g.player->y];
                lx = x;
                ly = y;
            }
        }
    }
    if (lx == IMPASSABLE || ly == IMPASSABLE) {
        stop_running();
        return 0;
    }
    if (lowest < MAX_HEAT) {
        if (!f.mode_explore) {
            logma(YELLOW, "%s begins cautiously exploring the area.", 
                  actor_name(g.player, NAME_CAP));
            f.mode_explore = 1;
        }
        return move_mon(g.player, lx, ly);
    } else {
        logm("This level is all done. Just move on already!");
        stop_running();
        return 0;
    }
}

/**
 * @brief Calculates the next step when traveling to a specific location.
 * 
 * @return struct action* A pointer to the action that the player will perform.
 */
struct action *travel(void) {
    int lx = IMPASSABLE;
    int ly = IMPASSABLE;
    int lowest = MAX_HEAT;
    if (g.goal_x == g.player->x && g.goal_y == g.player->y) {
        stop_running();
        return &actions[A_NONE];
    }

    // Do things
    for (int x = -1; x <= 1; x++) {
        if (x + g.player->x < 0 || x + g.player->x >= MAPW) continue;
        for (int y = -1; y <= 1; y++) {
            if (!x && !y) continue;
            if (y + g.player->y < 0 || y + g.player->y >= MAPH) continue;
            if (g.heatmap[HM_GOAL][x + g.player->x][y + g.player->y] <= lowest) {
                lowest = g.heatmap[HM_GOAL][x + g.player->x][y + g.player->y];
                lx = x;
                ly = y;
            }
        }
    }
    if (lx >= MAX_HEAT || ly == MAX_HEAT) {
        stop_running();
        return &actions[A_NONE];
    }
    return dir_to_action(lx, ly);
}

/**
 * @brief Cease all travel-related movement.
 * 
 */
void stop_running(void) {
    if (f.mode_explore || f.mode_run) {
        f.mode_run = 0;
        f.mode_explore = 0;
        g.goal_x = -1;
        g.goal_y = -1;
        f.update_map = 1;
        f.update_msg = 1;
        render_all();
    }
}

/**
 * @brief Display the help file to the user.
 * 
 * @return int The cost in energy of displaying the help file.
 */
int display_help(void) {
    display_file_text("data/text/help.txt");
    return 0;
}

/**
 * @brief Determine the action that the player will be taking. Blocks input.
 * 
 * @return int The cost of the action to be taken.
 */
struct action *get_action(void) {
    int i;
    /* If we are in runmode or are exploring, don't block input. */
    if (f.mode_explore) {
        return &actions[A_EXPLORE];
    }
    /* If running, move towards the goal location if there is one. Otherwise, move 
       in the previously input direction. */
    if (f.mode_run && in_bounds(g.goal_x, g.goal_y) && is_explored(g.goal_x, g.goal_y)
        && g.heatmap[HM_GOAL][g.player->x][g.player->y] < MAX_HEAT) {
        return travel();
    } else if (f.mode_run) {
        return g.prev_action;
    }
    /* Otherwise, block input all day :) */
    int keycode = handle_keys();
    if (keycode >= 49 && keycode < 57) {
        g.active_attack_index = keycode - 49;
        while (!get_active_attack(g.active_attack_index)->dam)
            g.active_attack_index++;
    }
    for (i = 0; i < ACTION_COUNT; i++) {
        if (actions[i].code == keycode
            || actions[i].alt_code == keycode) {
                if (keycode == actions[i].alt_code && is_movement(i)) {
                    f.mode_run = 1;
                }
                return &actions[i];
            }
    }
    return &actions[A_NONE];
}

static int dir_act_array[3][3] = {
    { A_NORTHWEST, A_NORTH, A_NORTHEAST },
    { A_WEST,      A_REST,  A_EAST },
    { A_SOUTHWEST, A_SOUTH, A_SOUTHEAST }
};

static struct coord act_dir_array[] = {
    { 0, 0 },
    { -1, 0 },
    { 1, 0 },
    { 0, -1 },
    { 0, 1 },
    { -1, -1 },
    { 1, -1 },
    { -1, 1 }, 
    { 1, 1 }
};

/**
 * @brief Given a relative coordinate movement, return an action pointer.
 * 
 * @param x The xcoordinate. Should fall between -1 and 1 inclusive.
 * @param y The y coordinate. Should fall between -1 and 1 inclusive.
 * @return struct action* A pointer to the action that will move in the given direction.
 */
struct action *dir_to_action(int x, int y) {
    int index = dir_act_array[y + 1][x + 1];
    if (index >= A_REST)
        return &actions[A_REST];
    return &actions[index];
}

struct coord action_to_dir(struct action *action) {
    if (action->index >= A_REST)
        return act_dir_array[A_REST];
    return act_dir_array[action->index];
}

/**
 * @brief Direct an actor to execute an action.
 * 
 * @param actor A pointer to the actor that will perform the action.
 * @param action A pointer to the action to perform.
 * @return int The cost of the actor in energy.
 */
int execute_action(struct actor *actor, struct action *action) {
    struct coord move_coord;
    if (action->index != A_NONE && actor == g.player) g.prev_action = action;
    // Autoexploring code goes here????
    if (action->movement) {
        move_coord = action_to_dir(action);
        return action->func.dir_act(actor, move_coord.x, move_coord.y);
    } else if (action->directed) {
        return action->func.dir_act(actor, actor->x, actor->y);
    } else {
        return action->func.void_act();
    }
    return do_nothing();
}

/**
 * @brief Get text input from the player, then execute the appropriate action.
 * 
 * @return int The cost of the action executed.
 */
int list_actions_exec(void) {
    char buf[32] = {'\0'};
    text_entry("Do what?", buf, 32);
    if (buf[0] == '\0') {
        logm("Never mind.");
        return 0;
    }
    for (int i = 0; i < ACTION_COUNT; i++) {
        if (!strncasecmp(actions[i].name, buf, sizeof(buf))) {
            return execute_action(g.player, &actions[i]);
        }
    }
    logm("%s has no idea how to \"%s.\"", actor_name(g.player, NAME_THE), buf);
    return 0;
}

/**
 * @brief Change the HUD mode.
 * 
 * @return int The cost of changing the HUD mode.
 */
int change_hud_mode(void) {
    term.hudmode += 1;
    if (term.hudmode == MAX_HUD_MODE)
        term.hudmode = 0;
    return 0;
}

/**
 * @brief Ascend to a higher z-level.
 * 
 * @param actor The actor ascending.
 * @param x unused
 * @param y unused
 * @return int The cost of ascending.
 */
int ascend(struct actor *actor, int x, int y) {
    (void) x;
    (void) y;
    return climb(actor, -1);
}

/**
 * @brief Descending to a higher z-level.
 * 
 * @param actor The actor descending.
 * @param x unused
 * @param y unused
 * @return int The cost of descending.
 */
int descend(struct actor *actor, int x, int y) {
    (void) x;
    (void) y;
    return climb(actor, 1);
}

/**
 * @brief Climb a set of stairs. Calls change_depth().
 * 
 * @param actor The actor doing the climbing.
 * @param change The number of levels climbed and direction of the climb. 
 Must be -1 or 1.
 * @return int Cost in energy of changing depth.
 */
int climb(struct actor *actor, int change) {
    if (actor == g.player) {
        if (change == 1) {
            if (TILE_AT(g.player->x, g.player->y) == T_STAIR_DOWN) {
                logm("Team %s retreats to a lower floor of the building.", g.userbuf);
                return change_depth(-1 * change);
            } else {
                actor->old_stance = STANCE_CROUCH;
                return change_stance(actor, STANCE_CROUCH, 0);
            }
        } else if (change == -1) {
            if (TILE_AT(g.player->x, g.player->y) == T_STAIR_UP) {
                if (g.depth == g.max_depth)
                    logm("Team %s ascends to an unfamiliar floor.", g.userbuf);
                else
                    logm("Team %s returns to a previously visited floor.", g.userbuf);
                return change_depth(-1 * change);
            } else {
                actor->old_stance = STANCE_STAND;
                return change_stance(actor, STANCE_STAND, 0);
            }
        } else {
            logm_warning("Climbing multiple levels?");
            return change_depth(change);
        }
    } else {
        if (change == 1) {
            logm("%s disappears down the stairs.", actor_name(actor, NAME_CAP | NAME_THE));
        } else if (change == -1) {
            logm("%s disappears up the stairs.", actor_name(actor, NAME_CAP | NAME_THE));
        }
        remove_actor(actor);
        free_actor(actor);
        return ACTOR_GONE;
    }
}