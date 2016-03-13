#include <stdio.h>
#include <unistd.h>

#include <pthread.h>
#include <random.h>
#include <threads.h>
#include <time.h>

#define THREADS_MAX 10

pthread_t threads[THREADS_MAX];

int main(int argc, char** argv) {
	int err = 0;

	srand(time(NULL));
	
	int ticks[THREADS];

	for (int i = 0; i < THREADS; i++) {
		ticks[i] = random_int(1, 6);
		
		/* Does pre initialization, such as allocating the queue,
		 * initializing locks, etc */
		err = pre_init_machine(ticks[i], i);
		if (err) {
			perror("pre_init_machine");
			exit(1);
		}
	}

	for (int i = 0, j = 0; i < THREADS; i++) {
		/* Start up server thread */
		
		int err = pthread_create(&threads[j], NULL, server_routine, 
				(void*) &machines[i]);
		if (err) {
			perror("pthread_create");
			return -1;
		}

		j++;

		/* Start up client thread */
		
		err = pthread_create(&threads[j], NULL, client_routine, 
				(void*) &machines[i]);
		if (err) {
			perror("pthread_create");
			return -1;
		}

		j++;
	}

	/* Sleep for 5 minutes */
	sleep(5 * 60);

	printf("Woke up. Cancelling threads...\n");

	/* Kill the threads */
	for (int i = 0; i < THREADS_MAX; i++) {
		pthread_cancel(threads[i]);	
	}

	/* Clean up after the threads, since we just killed them... */
	for (int i = 0; i < THREADS; i++) {
		free(machines[i].queue);
		pthread_mutex_destroy(&machines[i].queue_lock);
		fclose(machines[i].log);

		for (int j = 0; j < THREADS - 1; j++) {
			close(machines[i].client.connections[j]);
			close(machines[i].server.connections[j]);
		}
	}

	printf("Done!\n");
	return 0;
}
