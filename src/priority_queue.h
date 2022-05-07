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

#endif