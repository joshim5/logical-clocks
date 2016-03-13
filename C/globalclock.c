#include <sys/time.h>
#include <time.h>

int cmptime(struct timeval *start, struct timeval *end, struct timeval *remains) {
	remains->tv_sec = end->tv_sec - start->tv_sec;
	remains->tv_usec = end->tv_usec - start->tv_usec;

	if (remains->tv_usec < 0) {
		--remains->tv_sec;
		remains->tv_usec += 1000000;
	}

	if (remains->tv_sec >= 1) {
		remains->tv_sec = 0;
		remains->tv_usec = 0;
		return 1;
	}

	return 0;
}
