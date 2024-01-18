#ifndef AI_H
#define AI_H

struct ai {
    struct actor *parent;
    /* Stats */
    int seekdef;
    /* Mutable values */
    int seekcur;
    /* Various bitfields */
    unsigned long faction;
    unsigned int guardian : 1; /* Guardian monsters do not seek the stairs */
    /* 7 free bits */
};

/* Macros */
#define is_guardian(actor) (actor->ai && actor->ai->guardian)

/* Function Prototypes */
void make_aware(struct actor *, struct actor *, int);
struct ai *init_ai(struct actor *);
void take_turn(struct actor *);
struct attack choose_attack(struct actor *, struct actor *);
int is_aware(struct actor *, struct actor *); 

#define TURN_FULL 40
#define TURN_HALF 20
#define TURN_QUARTER 10

#endif