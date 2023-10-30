#ifndef MAP_H
#define MAP_H

/* Not a Melty Blood reference, I swear. */
#define MAX_HEAT 999
#define IMPASSABLE MAX_HEAT + 1

/* Coord struct. May move elsewhere later. */
struct coord {
    int x, y;
};

/* Heatmaps */
enum hm_enum {
    HM_PLAYER,
    HM_EXPLORE,
    HM_DOWNSTAIR,
    HM_GENERIC,
    HM_GOAL
};
#define NUM_HEATMAPS HM_GOAL + 1

struct hm_def {
    int id;
    short field;
    const char *hm_name;
};
extern struct hm_def heatmaps[NUM_HEATMAPS];

/* MACROS */

/* bounds */
#define in_bounds(x, y) \
    (x >= 0 && x < MAPW && y >= 0 && y < MAPH)
/* permtile attributes */
#define is_opaque(x, y) \
    (g.levmap[x][y].pt->opaque)
#define is_blocked(x, y) \
    (g.levmap[x][y].pt->blocked)
#define is_wall(x, y) \
    (g.levmap[x][y].pt->blocked && g.levmap[x][y].pt->id != T_DOOR_CLOSED)
#define is_stairs(x, y) \
    (g.levmap[x][y].pt->id == T_STAIR_DOWN || g.levmap[x][y].pt->id == T_STAIR_UP)
/* tile attributes */
#define is_visible(x, y) \
    (g.levmap[x][y].visible)
#define is_explored(x, y) \
    (g.levmap[x][y].explored)
#define is_lit(x, y) \
    (g.levmap[x][y].lit)
#define needs_refresh(x, y) \
    (g.levmap[x][y].refresh)

/* lookup */
#define MON_AT(x, y) \
    (g.levmap[x][y].actor)
#define ITEM_AT(x, y) \
    (g.levmap[x][y].item_actor)
#define TILE_AT(x, y) \
    (g.levmap[x][y].pt->id)

/* Function Prototypes */
struct coord get_direction(const char *);
int make_visible(int, int);
struct coord rand_open_coord(void);
int magic_mapping(void);
int change_depth(int);
void do_heatmaps(short, int);
void generic_heatmap(int, int, int);
struct coord best_adjacent_tile(int, int, int, int, int);

#endif