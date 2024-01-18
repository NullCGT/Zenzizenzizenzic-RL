#ifndef COMBAT_H
#define COMBAT_H

/* Function Prototypes */
int do_attack(struct actor *, struct actor *, int);
int weak_res(short, short);
int calculate_evasion(struct actor *);
int calculate_accuracy(struct actor *, struct attack *);
struct attack *get_active_attack(int);
int change_stance(struct actor *, short, int);

#define DAMAGE_SCALING 0.9
#define HITSTUN_DETERIORATION 4

#endif