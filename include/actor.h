#ifndef ACTOR_H
#define ACTOR_H

/* Naming bitmasks */
#define NAME_CAP      0x01
#define NAME_THE      0x02
#define NAME_A        0x04
#define NAME_YOUR     0x08
#define NAME_EQ       0x10
#define NAME_EX       0x20

/* Known bitmasks */
#define KNOW_NAME   0x1000
#define KNOW_HEALTH 0x2000

/* Maximums */
#define MAXNAMESIZ 20
#define MAX_ATTK 4

/* Hitdescs */
#define LOW 0x01
#define MID 0x02
#define HIGH 0x04
#define GRAB 0x08

/* Stances */
#define STANCE_CROUCH (LOW | MID)
#define STANCE_STAND (MID | HIGH)
#define STANCE_TECH GRAB
#define STANCE_STUN 0x0
#define MAX_HITDESC 4

struct damage {
    const char *str;
    unsigned char color;
    unsigned long val; 
};

struct hitdesc {
    const char *str;
    unsigned char color;
    unsigned long val;
};

struct attack {
    unsigned char dam;
    unsigned char kb;
    unsigned char accuracy;
    unsigned char stun;
    unsigned char recovery;
    /* bitfields */
    unsigned short hitdescs;
};

struct name {
    char real_name[MAXNAMESIZ];
    char appearance[MAXNAMESIZ];
    char given_name[MAXNAMESIZ];
};

struct actor {
    int id, chr;
    unsigned char color;
    /* Mutable attributes */
    unsigned char x, y, lv;
    int energy;
    int hp, hpmax;
    int speed; /* For creatures, denotes move speed. For items, attack speed. */
    signed char evasion, accuracy;
    signed char temp_evasion, temp_accuracy;
    unsigned char combo_counter;
    /* Attack list */
    struct attack attacks[MAX_ATTK];
    /* Components */
    struct name *name;
    struct actor *next;
    struct ai *ai;
    struct actor *invent;
    struct item *item;
    struct equip *equip;
    /* bitfields */
    unsigned short stance;
    unsigned short old_stance;
    unsigned short known;
    /* bitflags */
    unsigned int unique : 1;
    unsigned int can_tech : 1; /* Can tech a wallslam */
    unsigned int saved : 1; /* Infinite file write loop prevention. */
    /* 5 free bits */
};

#define is_noatk(x) \
    (!x.dam)

/* Function Prototypes */
int can_push(struct actor *, int, int);
int nearest_pushable_cell(struct actor *, int *, int *);
int push_actor(struct actor *, int, int);
struct actor *remove_actor(struct actor *);
void actor_sanity_checks(struct actor *);
char *actor_name(struct actor *, unsigned);
int free_actor(struct actor *);
int free_actor_list(struct actor *);
int in_danger(struct actor *);
const char *describe_health(struct actor *);
void identify_actor(struct actor *, int);

extern struct hitdesc hitdescs_arr[];


#endif