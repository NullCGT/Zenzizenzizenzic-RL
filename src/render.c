/**
 * @file render.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functions related to rendering the map and actors.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>

#include "map.h"
#include "register.h"
#include "fov.h"
#include "render.h"
#include "windows.h"
#include "message.h"
#include "action.h"

int update_camera(void);
void put_heatmap(int, int);
void render_cursor(void);


/**
 * @brief Perform all rendering tasks. Often called from the main loop.
 * 
 */
void render_all(void) {
    if (f.update_fov) {
        clear_fov();
        calculate_fov(g.player->x, g.player->y, 7);
    }

    /* Do not continually render when auto-exploring. Comes after fov updates
       since fov updates potentially affect the game state. */
    if (f.mode_explore) {
        f.update_map = 0;
        return;
    }
    
    render_map();
    render_all_actors();
    if (f.mode_look) {
        render_cursor();
    }
    display_sb();
    draw_lifebars();
    refresh_map();
    if (f.update_msg) {
        draw_msg_window(0);
    }
    update_panels();
    doupdate();
}

/**
 * @brief Render the cursor when in lookmode.
 * 
 */
void render_cursor(void) {
    map_putch(g.cursor_x - g.cx, g.cursor_y - g.cy, '?', GREEN);
}

/**
 * @brief Update the camera's location.
 * 
 * @return int 
 Return 1 if the camera's location changed.
 Return 0 if the camera's location has remained static.
 */
int update_camera(void) {
    int px = g.cx;
    int py = g.cy;

    if (MAPW > term.mapwin_w)
        g.cx = min(max(0, g.player->x - (term.mapwin_w  / 2)), abs(MAPW - term.mapwin_w));
    if (MAPH > term.mapwin_h)
        g.cy = min(max(0, g.player->y - (term.mapwin_h / 2)), abs(MAPH - term.mapwin_h));
    
    if (px == g.cx && py == g.cy)
        return 0;
    return 1;
}

/**
 * @brief Re-render a single cell.
 * 
 * @param x The x coordinate of the cell.
 * @param y The y coordinate of a cell.
 */
void refresh_cell(int x, int y) {
    if (is_visible(x, y)) {
        if (g.levmap[x][y].item_actor)
            map_put_actor(x - g.cx, y - g.cy, g.levmap[x][y].item_actor, g.levmap[x][y].item_actor->color);
        else if (g.levmap[x][y].actor)
            map_put_actor(x - g.cx, y - g.cy, g.levmap[x][y].actor, g.levmap[x][y].actor->color);
        else
            map_put_tile(x - g.cx, y - g.cy, x, y, g.levmap[x][y].pt->color);
    }
}

/**
 * @brief Render the map, tile by tile.
 Loops over the entirety of the map, and works in O(n) time.
 * 
 */
void render_map(void) {
    int refresh_all;

    refresh_all = update_camera();
    for (int i = 0; i < term.mapwin_w; i++) {
        for (int j = 0; j < term.mapwin_h; j++) {
            if (in_bounds(i + g.cx, j + g.cy)
                && (needs_refresh(i + g.cx, j + g.cy) || refresh_all)) {
                if (is_explored(i + g.cx, j + g.cy)) {
                    if (g.display_heat)
                        put_heatmap(i, j);
                    else
                        map_put_tile(i, j, i + g.cx, j + g.cy, 
                            is_visible(i + g.cx, j + g.cy) ? g.levmap[i + g.cx][j + g.cy].color : DARK_GRAY);
                } else {
                    map_putch(i, j, ' ', WHITE);
                }
                g.levmap[i + g.cx][j + g.cy].refresh = 0;
            }
        }
    }
    f.update_map = 0;
    return;
}

/**
 * @brief Render all actors on the map.
 * 
 */
void render_all_actors(void) {
    struct actor *cur = g.player;
    while (cur != NULL) {
        if (is_visible(cur->x, cur->y)) {
            if (cur->item && !MON_AT(cur->x, cur->y))
                map_put_actor(cur->x - g.cx, cur->y - g.cy, cur, cur->color);
            else if (!cur->item)
                map_put_actor(cur->x - g.cx, cur->y - g.cy, cur, cur->color);
        }
        cur = cur->next;
    }
    return;
}

/**
 * @brief Clear all actors on the map.
 * 
 */
void clear_actors(void) {
    struct actor *cur = g.player;
    while (cur != NULL && is_visible(cur->x, cur->y)) {
        map_put_tile(cur->x - g.cx, cur->y - g.cy, cur->x, cur->y, g.levmap[cur->x][cur->y].pt->color);
        cur = cur->next;
    }
    return;
}

/* HEATMAP RENDERING FUNCTIONS BEGIN HERE */
/* TODO: Break out into own file? */

#define MAX_HEATMAP_DISPLAY 36
static const char hm_chars[MAX_HEATMAP_DISPLAY] = "?123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/**
 * @brief Display a heatmap on the main map. For debug purposes only.
 * 
 * @param x x coordinate of the map.
 * @param y y coordinate of the map.
 */
void put_heatmap(int x, int y) {
    int i = g.heatmap[g.display_heat - 1][x + g.cx][y + g.cy];
    if (i == IMPASSABLE) {
        map_putch(x, y, ' ', WHITE);
        return;
    } else if (!i) {
        map_putch(x, y, '.', WHITE);
        return;
    }
    i = i % MAX_HEATMAP_DISPLAY;
    int color = 0xffffff - i * (0xffffff / MAX_HEATMAP_DISPLAY);
    map_putch_truecolor(x, y, hm_chars[i], color);
}

/**
 * @brief Switfch viewmode to viwe a different heatmap.
 * 
 * @return int Cost of switching viewmode in energy.
 */
int switch_viewmode(void) {
    if (!g.debug) {
        logm("It's chilly in here.");
        return 0;
    }
    g.display_heat++;
    if (g.display_heat > NUM_HEATMAPS) g.display_heat = 0;
    f.update_map = 1;
    if (!g.display_heat)
        logm("Debug Output: Disabled heatmap display.");
    else
        logm("Debug Output: Viewing %s heatmap.", heatmaps[g.display_heat - 1].hm_name);
    return 0;
}