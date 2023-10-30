/**
 * @file tile.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functionality related to map tiles.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "tile.h"
#include "register.h"
#include "map.h"
#include "message.h"
#include "windows.h"
#include "render.h"


struct permtile permtiles[] = {
    PERMTILES
};

/**
 * @brief Initialize the tile struct.
 * 
 * @param intile The tile to be initialized. Mutated by this function.
 * @param tindex The index of the permtile to initialize the tile as.
 * @return A pointer to the modified tile.
 */
struct tile *init_tile(struct tile *intile, int tindex) {
    #if 0
    if (tindex == T_WALL || tindex == T_EARTH) {
        intile->color = dgn.wall_color;
    } else {
        intile->color = permtiles[tindex].color;
    }
    #endif
    intile->color = permtiles[tindex].color;
    intile->pt = &permtiles[tindex];
    intile->actor = NULL;
    intile->item_actor = NULL;
    intile->refresh = 1;
    return intile;
}

/**
 * @brief Open a door.
 * 
 * @param actor The actor performing the action.
 * @param x x coordinate of the door.
 * @param y y coordinate of the door.
 * @return int The cost in energy of opening the door.
 */
int open_door(struct actor *actor, int x, int y) {
    struct tile *intile;
    int tindex;
    struct coord new_dir;

    if (actor == g.player && g.player->x == x && g.player->y == y) {
        new_dir = get_direction("open");
        x = new_dir.x + g.player->x;
        y = new_dir.y + g.player->y;
    }
    if (!in_bounds(x, y)) return 0;
    intile = &g.levmap[x][y];
    tindex = intile->pt->id;

    if (tindex != T_DOOR_CLOSED) {
        logm("There is nothing to open in that direction.");
        return 0;
    }

    init_tile(intile, T_DOOR_OPEN); // init tile handles the refresh mark.
    if (is_visible(x, y)) {
        f.update_fov = 1;
    }

    if (actor != g.player && is_visible(actor->x, actor->y)) {
        logm("%s opens a door.", actor_name(actor, NAME_THE));
    } else if (is_visible(x, y))
        logm("The door opens.");
    return 100;
}

/**
 * @brief Close a door.
 * 
 * @param actor The actor performing the action.
 * @param x x coordinate of the door.
 * @param y y coordinate of the door.
 * @return int The cost in energy of opening the door.
 */
int close_door(struct actor *actor, int x, int y) {
    struct tile *intile;
    int tindex;
    struct coord new_dir;

    if (actor == g.player && g.player->x == x && g.player->y == y) {
        new_dir = get_direction("close");
        x = new_dir.x + g.player->x;
        y = new_dir.y + g.player->y;
    }
    if (!in_bounds(x, y)) return 0;
    intile = &g.levmap[x][y];
    tindex = intile->pt->id;

    if ((!new_dir.x && !new_dir.y) || tindex != T_DOOR_OPEN) {
        logm("There is nothing to close in that direction.");
        return 0;
    }

    init_tile(intile, T_DOOR_CLOSED);
    if (is_visible(x, y)) {
        map_put_tile(x - g.cx, y - g.cy, x, y, intile->color);
        f.update_fov = 1;
        f.update_map = 1;
    }

    if (actor != g.player && is_visible(actor->x, actor->y)) {
        logm("%s closes a door.", actor_name(actor, NAME_THE));
    } else if (is_visible(x, y))
        logm("The door closes.");
    return 100;
}