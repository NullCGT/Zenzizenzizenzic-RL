#ifndef ACTION_H
#define ACTION_H

/* Order is critical for A_REST and everything beforehand.  */
enum actionnum {
    A_NONE,
    A_WEST,
    A_EAST,
    A_NORTH,
    A_SOUTH,
    A_NORTHWEST,
    A_NORTHEAST,
    A_SOUTHWEST,
    A_SOUTHEAST,
    A_REST,
    /* End movement actions */
    A_OPEN,
    A_CLOSE,
    A_PICK_UP,
    A_LOOK,
    A_ASCEND,
    A_DESCEND,
    A_LOOK_DOWN,
    A_EXPLORE,
    A_INVENT,
    A_TAB_HUD,
    A_FULLSCREEN,
    A_HELP,
    A_SAVE,
    A_QUIT,
    A_LIST,
    A_MAGICMAP,
    A_HEAT,
    A_SPAWN,
    A_STRUCTINFO,
    A_WISH
};

#define ACTION_COUNT 30
#define ACTOR_GONE -10000

union act_func {
    int (*dir_act) (struct actor *, int, int);
    int (*void_act) (void);
};

struct action {
    const char *name;
    int index;
    int code, alt_code;
    union act_func func;
    /* bitflags */
    unsigned int debug_only : 1;
    unsigned int directed : 1;
    unsigned int movement : 1;
    /* 5 free bits */
};

extern struct action actions[];

#define is_movement(a) \
    (a > A_NONE && a < A_REST)

/* Function Prototypes */
char *stringify_action(int);
int move_mon(struct actor*, int, int);
struct coord action_to_dir(struct action *);
int pick_up(struct actor *, int, int);
int look_at(int, int);
void stop_running(void);
struct action *get_action(void);
struct action *dir_to_action(int, int);
int execute_action(struct actor*, struct action*);
int ascend(struct actor *, int, int);
int descend(struct actor *, int, int);

#endif