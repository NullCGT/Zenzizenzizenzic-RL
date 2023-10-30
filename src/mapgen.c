/**
 * @file mapgen.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Map generation functions. Currently undocumented due to high
 volatility.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#define WFC_IMPLEMENTATION

#include <stdlib.h>
#include <random.h>
#include <stdio.h>

#include "register.h"
#include "message.h"
#include "parser.h"
#include "map.h"
#include "spawn.h"
#include "parser.h"
#include "mapgen.h"

int wfc_magpen(void);
void tunnel(struct coord, struct coord);
struct coord rand_region_coord(int, int, int, int);
void cellular_automata(int, int, int, int, int, int);
int deisolate(void);
void init_map(int);

#define WFC_SUCCESS 0
#define WFC_ERROR 1
#define WFC_TRIES 10

/**
 * @brief Generate a section of the map using wave function collapse.
 * 
 * @param x1 start x
 * @param y1 start y
 * @param x2 end x (inclusive)
 * @param y2 end y (inclusive)
 * @return int WFC_SUCCESS or WFC_ERROR
 */
int wfc_mapgen(int x1, int y1, int x2, int y2) {
    struct wfc_image image = parse_wfc_json("data/wfc/dungeon.json");
    int img_w = x2 - x1;
    int img_h = y2 - y1;
    struct wfc *wfc = wfc_overlapping(img_w,
                                    img_h,
                                    &image,
                                    2,
                                    2,
                                    1,
                                    1,
                                    1,
                                    1); 

    if (wfc == NULL) {
        logm("Error: cannot create wfc.");
        wfc_destroy(wfc);
        return WFC_ERROR;
    }

    if (!wfc_run(wfc, -1)) {
        logm("Error: Something went wrong with wfc.");
        wfc_destroy(wfc);
        return WFC_ERROR;
    }
    struct wfc_image *output_image = wfc_output_image(wfc);
    if (!output_image) {
        logm("Error: FAILURE.");
        wfc_destroy(wfc);
        return WFC_ERROR;
    }
    for (int y = 0; y <= img_h; y++) {
        for (int x = 0; x <= img_w; x++) {
            unsigned char cell = output_image->data[y * img_w + x];
            if (cell == '.' || (cell >= '1' && cell <= '9')) {
                init_tile(&g.levmap[x + x1][y + y1], T_FLOOR);
            } else if (cell == '+') {
                init_tile(&g.levmap[x + x1][y + y1], T_DOOR_CLOSED);
            } else {
                init_tile(&g.levmap[x + x1][y + y1], T_WALL);
            }
        }
    }
    /* Clean leftover data from xml parser. Technically handled by wfc.h? */
    free(image.data);
    /* Clean other memory. */
    wfc_img_destroy(output_image);
    wfc_destroy(wfc);
    return WFC_SUCCESS;
}

/**
 * @brief Returns a random open coordinate within a region. Assumes that
 a region has at least one open cell.
 * 
 * @param region The region to find a coordiante within.
 * @return struct coord The coordinate.
 */
struct coord rand_region_coord(int x1, int y1, int x2, int y2) {
    struct coord c;
    do {
        c.x = rndrng(x1, x2);
        c.y = rndrng(y1, y2);
    } while (is_blocked(c.x, c.y));
    return c;
}

/**
 * @brief Carve out a portion of the dungeon level using a cellular automata
 algorithm.
 * 
 * @param x1 Upper left x
 * @param x2 Lower right x
 * @param y1 Upper left y
 * @param y2 Lower right y
 * @param filled What percentage of the map should start filled
 * @param iterations How many iterations to run the automata
 */
void cellular_automata(int x1, int y1, int x2, int y2, int filled, int iterations) {
    int x, y, nx, ny;
    int alive;
    int blocked = 1;
    int width = x2 - x1;
    int height = y2 - y1;
    unsigned char cells[width][height];

    /* Initialize cells. */
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            cells[x][y] = (rndmx(100) < filled);
        }
    }
    /* Game of Life */
    for (int i = 0; i < iterations; i++) {
        for (x = 0; x < width; x++) {
            for (y = 0; y < height; y++) {

                alive = 0;
                for (int xx1 = -1; xx1 <= 1; xx1++) {
                    nx = x + xx1;
                    if (nx < 0 || nx >= width) {
                        alive += 3;
                        continue;
                    }
                    for (int yy1 = -1; yy1 <= 1; yy1++) {
                        ny = y + yy1;
                        if (ny < 0 || ny >= height) {
                            alive++;
                        } else if (cells[nx][ny]) {
                            alive++;
                        }
                    }
                }
                if (alive >= 5)
                    cells[x][y] = 1;
                else
                    cells[x][y] = 0;
            }
        }
    }

    /* Transfer to grid */
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            if (!cells[x][y]) {
                init_tile(&g.levmap[x1 + x][y1 + y], T_FLOOR);
                blocked = 0;
            }
        }
    }
    /* Add a single cell if none existed after running (can happen on small inputs) */
    if (blocked) {
        init_tile(&g.levmap[rndrng(x1, x1 + x)][rndrng(y1, y1 + 1)], T_FLOOR);
    }
}

/**
 * @brief Combs the level map for isolated areas. Upon finding one, use a
 dijkstra map in order to connect it. Highly expensive.
 * 
 * @return int Return 1 if changes were made to the level, otherwise return 0.
 */
int deisolate(void) {
    int x, y;
    int dx, dy;
    struct coord c1, c2;
    /* Hacky hack */
    for (dx = 0; dx < MAPW; dx++) {
        for (dy = 0; dy < MAPH; dy++) {
            if (!is_blocked(dx, dy)) {
                break;
            }
        }
        if (!is_blocked(dx, dy)) {
            break;
        }
    }
    g.goal_x = dx;
    g.goal_y = dy;
    do_heatmaps(heatmaps[HM_GENERIC].field, 0);
    /* Loop over everything to see if there is somewhere the player cannot get. */
    for (x = 0; x < MAPW; x++) {
        for (y = 0; y < MAPH; y++) {
            if (g.heatmap[HM_GENERIC][x][y] == MAX_HEAT) {
                c1.x = dx;
                c1.y = dy;
                c2.x = x;
                c2.y = y;
                g.goal_x = x;
                g.goal_y = y;
                do_heatmaps(heatmaps[HM_GENERIC].field, 1);
                tunnel(c1, c2);
                return 1;
            }
        }
    }
    return 0;
}

/* Initialize the map by making sure everything is not visible and
   not explored. */
void init_map(int tile) {
    for (int y = 0; y < MAPH; y++) {
        for (int x = 0; x < MAPW; x++) {
            init_tile(&g.levmap[x][y], tile);
            g.levmap[x][y].lit = 0;
            g.levmap[x][y].visible = 0;
            g.levmap[x][y].explored = 0;
        }
    }
}

void tunnel(struct coord c1, struct coord c2) {
    struct coord nc;
    while (c1.x != c2.x || c1.y != c2.y) {
        nc = best_adjacent_tile(c1.x, c1.y, 0, 0, HM_GENERIC);
        c1.x += nc.x;
        c1.y += nc.y;
        if (is_blocked(c1.x, c1.y))
            init_tile(&g.levmap[c1.x][c1.y], T_FLOOR);
        if (c1.x == 0 && c1.y == 0) return;
    }
}

void place_stairs(void) {
    struct coord stairs_xy;
    stairs_xy = rand_region_coord(0, 0, MAPW, MAPH / 4);
    init_tile(&g.levmap[stairs_xy.x][stairs_xy.y], T_STAIR_UP);
    g.up_x = stairs_xy.x;
    g.up_y = stairs_xy.y;
    if (g.depth) {
        stairs_xy = rand_region_coord(0, MAPH * 3 / 4, MAPW, MAPH);
        init_tile(&g.levmap[stairs_xy.x][stairs_xy.y], T_STAIR_DOWN);
        g.down_x = stairs_xy.x;
        g.down_y = stairs_xy.y;
    }
}

void make_level(void) {
    f.mode_mapgen = 1;
    int tries;

    /* Fill map */
    init_map(T_WALL);
    /* Wave function collapse */
    for (tries = 0; tries < WFC_TRIES; tries++) {
        if (!wfc_mapgen(1, 1, MAPW - 2, MAPH - 2)) {
            break;
        }
    }
    /* Fallback */
    if (tries >= WFC_TRIES) {
        init_map(T_FLOOR);
    }
    place_stairs();

    //while(deisolate());
    do_heatmaps(heatmaps[HM_DOWNSTAIR].field, 0);

    g.goal_x = -1;
    g.goal_y = -1;
    set_spawn_countdown();
    f.update_map = 1;
    f.update_fov = 1;
    f.mode_mapgen = 0;
    return;
}

/**
 * @brief Set the countdown before the next monster or group spawns at the stairs.
 *        The higher the floor, the more rapidly the countdown occurs.
 * 
 */
void set_spawn_countdown(void) {
    g.spawn_countdown = rndrng(min(25, 78 - g.depth), min(50, 128 - g.depth));
}