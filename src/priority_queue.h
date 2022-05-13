#ifndef __PRIOR_QUEUE_H_
#define __PRIOR_QUEUE_H_

#include <stdlib.h>
#include <stdbool.h>

typedef struct priorityQueue PriorityQueue;

// Allocates a new priority queue
// comparator: returns 1 if first is larger, 0 if equal, -1 if first is smaller
PriorityQueue *pq_new(size_t data_size, int(comparator)(void*, void*));

// Destroys priority queue, elements with pointers to allocated memory get leaked
void pq_free(PriorityQueue *pq);

// Enqueues data according to size given on initialization
void pq_enqueue(void *data_in, PriorityQueue *pq);

// Writes data to data_out according to size given on initialization and removes element
// returns false if empty
bool pq_dequeue(void *data_out, PriorityQueue *pq);

// Writes data to data_out according to size given on initialization and keeps element
// returns false if empty
bool pq_peek(void* data_out, PriorityQueue *pq);

bool pq_is_empty(PriorityQueue *pq);

void *pq_get_arr(size_t *arr_len_out, PriorityQueue *pq);

void *pq_iter_start(PriorityQueue *pq);

void *pq_iter_end(PriorityQueue *pq);

#define PQ_FOREACH(var_name, type, prio_queue) \
    for(type *var_name=pq_iter_start(prio_queue);var_name<(type*)pq_iter_end(prio_queue);++var_name)

#endif