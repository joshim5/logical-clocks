#include <queue.h>
#include <stdlib.h>
#include <string.h>

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

unsigned queue_size(struct queue *q) {
	return q->size;
}

struct queue *queue_create(unsigned capacity) {
	if (capacity > MAX_QUEUE)
		return NULL;

	struct queue *q = malloc(sizeof(*q));

	q->capacity = capacity;
	q->size = 0;
	q->front = 0;
	q->rear = 0;
	memset(q->elements, 0, q->capacity * sizeof(unsigned));
	
	return q;
}