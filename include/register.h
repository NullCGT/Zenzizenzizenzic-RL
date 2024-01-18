#ifndef REGISTER_H
#define REGISTER_H

#include "actor.h"
#include "color.h"
#include "tile.h"
#include "map.h"

/* Map and window constants */
#define MAPW 80
#define MAPH 40
#define MIN_TERM_H 20
#define MIN_TERM_W 104

/* Size Constants */
#define MAX_USERSZ 32

/* Common functions */
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define signum(x) ((x > 0) - (x < 0))
#define vowel(x) (x == 'a' || x == 'e' || x == 'i' || x == 'o' || x == 'u')

/* Func Proto */
void setup_term_dimensions(int, int, int, int);

#define MAX_MONSTERS 200
#define MAX_ITEMS 200

/* Persistent data which is saved and loaded. */
typedef struct global {
    char userbuf[MAX_USERSZ];
    struct tile levmap[MAPW][MAPH];
    int heatmap[NUM_HEATMAPS][MAPW][MAPH];
    struct actor *monsters[MAX_MONSTERS];
    struct actor *items[MAX_ITEMS];
    struct actor *player; /* Assume player is first NPC */
    struct actor *target;
    struct actor *active_attacker;
    struct msg *msg_list;
    struct msg *msg_last;
    struct action *prev_action; /* for the moment, only used for runmode */
    unsigned char active_attack_index;
    unsigned char display_heat;
    int turns;
    int depth;
    int max_depth;
    int score;
    int total_monsters;
    int total_items;
    int spawn_countdown;
    int up_x, up_y, down_x, down_y;
    int cx, cy; /* Camera location */
    int cursor_x, cursor_y; /* In-game cursor location */
    int goal_x, goal_y; /* Traveling */
    /* Persistent flags */
    unsigned int debug : 1;
    unsigned int practice : 1;
    /* 5 free bits */
} global;

typedef struct bitflags {
    /* Rendering update flags */
    unsigned int update_msg : 1;
    unsigned int update_map : 1;
    unsigned int update_fov : 1;
    /* Mode flags */
    unsigned int mode_explore : 1;
    unsigned int mode_run : 1;
    unsigned int mode_map : 1;
    unsigned int mode_look : 1;
    unsigned int mode_mapgen : 1;
    /* 0 free bit */
} bitflags;

typedef struct terminal {
    int h;
    int w;
    int mapwin_w;
    int mapwin_h;
    int mapwin_y;
    int mapwin_x;
    int msg_w;
    int msg_h;
    int msg_y;
    int sb_w;
    int sb_x;
    int sb_h;
    char *saved_locale;
    char hudmode;
} terminal;

extern struct global g;
extern struct bitflags f;
extern struct terminal term;

#endif