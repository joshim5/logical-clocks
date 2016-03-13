#define MAX_QUEUE 128

struct queue {
	unsigned elements[MAX_QUEUE];
	unsigned front;
	unsigned rear;
	unsigned capacity;
	unsigned size;
};

unsigned queue_enqueue(struct queue *q, unsigned element);

unsigned queue_dequeue(struct queue *q);

unsigned queue_size(struct queue *q);

struct queue *queue_create(unsigned capacity);
