#include <stdio.h>
#include <pthread.h>

#include <queue.h>

#define THREADS 3
#define BUF_SIZE 128
#define CLOCKS_MAX 500

struct server {
	int master_socket;
	int connections[THREADS];
	int port;
};

struct client {
	int ticks;
	int connections[THREADS];
};

struct machine {
	int id;
	unsigned lclock;	
	FILE *log;

	pthread_mutex_t queue_lock;

	struct queue *queue;
	struct server server;
	struct client client;
};

extern struct machine machines[THREADS];

int pre_init_machine(int ticks, int id);
void *server_routine(void *arg);
void *client_routine(void *arg);
int send_message(int dest, unsigned lclock);
void internal_event(void);
