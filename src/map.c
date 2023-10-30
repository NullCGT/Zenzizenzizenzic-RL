/**
 * @file map.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functions related to the level map.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include "map.h"
#include "mapgen.h"
#include "register.h"
#include "message.h"
#include "random.h"
#include "gameover.h"
#include "render.h"
#include "action.h"
#include "save.h"
#include "pqueue.h"

void update_max_depth(void);
void create_heatmap(int, int);

struct hm_def heatmaps[NUM_HEATMAPS] = {
    { HM_PLAYER,    0x0001, "player" },
    { HM_EXPLORE,   0x0002, "explore" },
    { HM_DOWNSTAIR, 0x0004, "downstair" },
    { HM_GENERIC,   0x0008, "generic" },
    { HM_GOAL,      0x0010, "goal" },
};

/**
 * @brief Ask the player to input a direction, and return a coordinate.
 * 
 * @param actstr A string with which to prompt the user.
 * @return struct coord A coordinate representing the movement.
 */
struct coord get_direction(const char *actstr) {
    struct coord c;
    struct action *action;
    
    logma(GREEN, "What direction should I %s in?", actstr);
    render_all(); /* TODO: Figure out why this needs render all. */
    action = get_action();
    c = action_to_dir(action);
    return c;
}

/**
 * @brief Make a point on the map visible and explored.
 * 
 * @param x x coordinate of the location.
 * @param y y coordinate of the location.
 * @return int Denotes whether tile is opaque.
 */
int make_visible(int x, int y) {
    if (!g.levmap[x][y].visible && !g.levmap[x][y].explored && is_stairs(x, y)) {
        logma(BRIGHT_YELLOW, "%s has found a set of stairs.", actor_name(g.player, NAME_CAP | NAME_THE));
        stop_running();
    } else if (g.levmap[x][y].actor && g.levmap[x][y].actor != g.player) {
        /* TODO: Find somewhere less expensive to put this... */
        stop_running();
    }
    g.levmap[x][y].visible = 1;
    g.levmap[x][y].explored = 1;
    if (is_opaque(x, y))
        return 1;
    return 0;
}

/**
 * @brief Return a random open coordinate on the map.
 * 
 * @return struct coord The open coordinate found.
 */
struct coord rand_open_coord(void) {
    int x, y;

    do {
        x = rndmx(MAPW);
        y = rndmx(MAPH);
    } while (is_blocked(x, y) || g.levmap[x][y].actor);

    struct coord c = {x, y};

    return c;
}

/**
 * @brief Marks every cell in the map as explored.
 * 
 * @return The cost in energy of magic maping (always 0).
 */
int magic_mapping(void) {
    if (!g.debug) {
        logm("And miss out on all the fun?");
        return 0;
    }
    for (int y = 0; y < MAPH; y++) {
        for (int x = 0; x < MAPW; x++) {
            g.levmap[x][y].explored = 1;
        }
    }
    logm("Debug Output: Revealed the map.");
    f.update_map = 1;
    return 0;
}

/**
 * @brief Change the player's depth. If this results in a change in maximum depth,
 * then update the player's score.
 * 
 * @param change The number of levels to shift.
 * @return int The cost in energy of climbing.
 */
int change_depth(int change) {
    save_game();
    g.depth += change;
    if (g.depth > g.max_depth)
        update_max_depth();
    if (g.depth >= 128) {
        /* A winner is you. */
        end_game(1);
    }
    free_actor_list(g.player->next);
    g.player->next = NULL;
    make_level();
    if (change > 0)
        push_actor(g.player, g.down_x, g.down_y);
    else
        push_actor(g.player, g.up_x, g.up_y);
    return 50;
}

/**
 * @brief Update the maximum depth and update score and hit points.
 * 
 */
void update_max_depth(void) {
    int diff = g.depth - g.max_depth;
    /* Restore 50% of health and increase max health by 1. */
    g.player->hpmax += diff;
    g.player->hp += (g.player->hpmax * 0.5 * diff);
    if (g.player->hp > g.player->hpmax) g.player->hp = g.player->hpmax;
    logma(GREEN, "The warm glow of progress restores health.");
    /* Increase score */
    if (in_danger(g.player)) {
        g.score += (1200 * (diff));
    } else {
        g.score += (1000 * (diff));
    }
    /* Update the max depth */
    g.max_depth = g.depth;
}

static struct coord cardinal_dirs[] = {
    { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
};

/**
 * @brief Create a heatmap.
 * 
 * @param hm_index Index with which to access the three-dimensional heatmap array.
 * @param tunneling Whether the heatmap should use tile costs for walking
 over tiles or tunneling through tiles.
 */
void create_heatmap(int hm_index, int tunneling) {
    struct p_node cur;
    int nx, ny;
    int *n_heat;
    int cost;
    unsigned char visited[MAPW][MAPH] = { 0 };
    struct p_queue heat_queue = { 0 };
    heat_queue.size = -1;

    /* Populate heap */
    for (int y = 0; y < MAPH; y++) {
        for (int x = 0; x < MAPW; x++) {
            int val = g.heatmap[hm_index][x][y];
            if (val < MAX_HEAT)
                pq_push(&heat_queue, val, x, y);
        }
    }

    /* Dijkstra */
    while (heat_queue.size >= 0) {
        cur = pq_pop(&heat_queue);
        for (int i = 0; i < 4; i++) {
            /* Loop through neighbors of cur */
            nx = cur.x + cardinal_dirs[i].x;
            ny = cur.y + cardinal_dirs[i].y;
            if (!in_bounds(nx, ny) || visited[nx][ny]) continue;
            n_heat = &(g.heatmap[hm_index][nx][ny]);
            cost = tunneling ? g.levmap[nx][ny].pt->walk_cost : g.levmap[nx][ny].pt->tunnel_cost;
            visited[nx][ny] = 1;
            if (*n_heat == IMPASSABLE) continue;
            if (cur.heat + cost < *n_heat) {
                *n_heat = cur.heat + cost;
                pq_push(&heat_queue, *n_heat, nx, ny);
            }
        }
    }
}

/**
 * @brief Set up given heatmaps.
 * 
 * @param hm_bits A bitfield referencing the heatmaps that need to be set up.
 * @param tunneling Whether walls should be ignored.
 */
void do_heatmaps(short hm_bits, int tunneling) {
    int y, x;
    int blocked;
    /* Setup */
    for (y = 0; y < MAPH; y++) {
        for (x = 0; x < MAPW; x++) {
            blocked = (!tunneling && is_wall(x, y));
            for (int i = 0; i < NUM_HEATMAPS; i++) {
                if (!(hm_bits & heatmaps[i].field))
                    continue;
                if (blocked) {
                    g.heatmap[i][x][y] = IMPASSABLE;
                } else {
                    switch(i) {
                        case HM_PLAYER:
                            g.heatmap[i][x][y] = MAX_HEAT;
                            break;
                        case HM_EXPLORE:
                            g.heatmap[i][x][y] = is_explored(x, y) ? MAX_HEAT : 0;
                            break;
                        case HM_DOWNSTAIR:
                            g.heatmap[i][x][y] = TILE_AT(x, y) == T_STAIR_DOWN ? 0 : MAX_HEAT;
                            break;
                        case HM_GOAL:
                            g.heatmap[i][x][y] = (is_explored(x, y) || tunneling) ? MAX_HEAT : IMPASSABLE;
                            break;
                        case HM_GENERIC:
                            g.heatmap[i][x][y] = MAX_HEAT;
                            break;
                    }
                }
            }
        }
    }

    /* Constant-time fixups */
    if (g.player)
        g.heatmap[HM_PLAYER][g.player->x][g.player->y] = 0;
    if (g.goal_x >= 0 && g.goal_y >= 0) {
        g.heatmap[HM_GOAL][g.goal_x][g.goal_y] = 0;
        g.heatmap[HM_GENERIC][g.goal_x][g.goal_y] = 0;
    }
    
    for (int i = 0; i < NUM_HEATMAPS; i++) {
        if (hm_bits & heatmaps[i].field) {
            create_heatmap(i, tunneling);
        }
    }
}


struct coord best_adjacent_tile(int cx, int cy, int diagonals, int avoid_actors, int hm_index) {
    int lx, ly = -99;
    int lowest = MAX_HEAT;
    struct coord ret = { 0, 0 };

    for (int x = -1; x <= 1; x++) {
        if (x + cx < 0 || x + cx >= MAPW) continue;
        for (int y = -1; y <= 1; y++) {
            if ((!x && !y) || (!diagonals && x && y)) continue;
            if (y + cy < 0 || y + cy >= MAPH) continue;
            if (avoid_actors && g.levmap[x + cx][y + cy].actor != g.player && g.levmap[x + cx][y + cy].actor != NULL) continue;
            if (g.heatmap[hm_index][x + cx][y + cy] <= lowest) {
                lowest = g.heatmap[hm_index][x + cx][y + cy];
                lx = x;
                ly = y;
            }
        }
    }
    if (lx != -99 && ly != -99) {
        ret.x = lx;
        ret.y = ly;
    }
    return ret;
}
