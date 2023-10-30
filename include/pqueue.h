#ifndef PQUEUE_H
#define PQUEUE_H

#include "register.h"

struct p_node {
    int heat;
    int x;
    int y;
};

struct p_queue {
    int size;
    struct p_node heap[MAPH * MAPW + 1];
};

/* Function Prototypes */
void pq_push(struct p_queue *, int, int, int);
struct p_node pq_pop(struct p_queue *);

#endif