#include <queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Simple implementation of a circular queue. Capacity must be at most
 * MAX_QUEUE. */


/* Enqueues an element. Returns 0 on success, 1 on failure (out of
 * space). */
unsigned queue_enqueue(struct queue *q, unsigned element) {
	if (q->size == q->capacity)
		return -1;
	
	q->size++;

	q->elements[q->rear] = element;

	q->rear++;
	if (q->rear == q->capacity)
		q->rear = 0;

	return 0;
}

/* Dequeues an element. Returns the element on success, or -1 on failure.
 * Notice that the elements are all unsigned, so this is safe to do! */
unsigned queue_dequeue(struct queue *q) {
	if (q->size == 0) 
		return -1;

	unsigned ret = q->elements[q->front];
	
	q->size--;
	q->front++;
	
	if (q->front == q->capacity) 
		q->front = 0;

	return ret;
}

/* Returns the capacity of the queue. */
unsigned queue_capacity(struct queue *q) {
	return q->capacity;
}

/* Returns the current size of the queue. */
unsigned queue_size(struct queue *q) {
	return q->size;
}

/* Allocate space and start up queue */
struct queue *queue_create(unsigned capacity) {
	if (capacity > MAX_QUEUE) {
		errno = ENOMEM;
		return NULL;
	}

	struct queue *q = malloc(sizeof(*q));
	if (q == NULL) {
		perror("malloc");
		return NULL;
	}

	q->capacity = capacity;
	q->size = 0;
	q->front = 0;
	q->rear = 0;
	memset(q->elements, 0, q->capacity * sizeof(unsigned));
	
	return q;
}
