#include <stdio.h>
#include <unistd.h>

#include <random.h>
#include <threads.h>
#include <time.h>

int main(int argc, char** argv) {
	srand(time(NULL));
	
	int ticks[THREADS];

	for (int i = 0; i < THREADS; i++) {
		ticks[i] = random_int(1, 6);
		pre_init_machine(ticks[i], i);
	}

	int err = init_machines();
	if (err) {
		perror("init_machines");
		exit(1);
	}

	while (1) {
		sleep(2);
		srand(time(NULL));
	}
}
