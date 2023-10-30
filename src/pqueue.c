/**
 * @file pqueue.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functions related to the creation and maintenance of a binary heatp.
 * @version 1.0
 * @date 2022-07-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "pqueue.h"
#include "register.h"

int pq_parent(int);
int left_child(int);
int right_child(int);
void pq_swap(struct p_node *, struct p_node *);
void heapify_down(struct p_queue *, int);

#define left_child(i) ((i * 2) + 1)
#define right_child(i) ((i * 2) + 2)
#define pq_parent(i) ((i - 1) / 2)

/* In theory based on https://www.geeksforgeeks.org/priority-queue-using-binary-heap/ */

void pq_swap(struct p_node *a, struct p_node *b) {
    struct p_node temp = *a;
    *a = *b;
    *b = temp;
}

void pq_push(struct p_queue *queue, int heat, int x, int y) {
    queue->size++;
    int size = queue->size;

    queue->heap[size].heat = heat;
    queue->heap[size].x = x;
    queue->heap[size].y = y;
    /* Heapify up */
    while (queue->heap[size].heat < queue->heap[pq_parent(size)].heat) {
        pq_swap(&(queue->heap[size]), &(queue->heap[pq_parent(size)]));
        size = pq_parent(size);
    }
}

struct p_node pq_pop(struct p_queue *queue) {
    int max_index, max_heat, l, r;
    int index = 0;

    struct p_node node = queue->heap[0];
    queue->heap[0] = queue->heap[queue->size];
    queue->size--;
    /* Heapify down */
    while (1) {
        max_index = index;
        max_heat = queue->heap[index].heat;
        l = left_child(index);
        r = right_child(index);

        if (l <= queue->size) {
            if (queue->heap[l].heat < max_heat)
                max_index = l;
        }
        if (r <= queue->size) {
            if (queue->heap[r].heat < max_heat)
                max_index = r;
        }

        if (index != max_index) {
            pq_swap(&(queue->heap[index]), &(queue->heap[max_index]));
            index = max_index;
        } else {
            break;
        }
    }

    return node;
}