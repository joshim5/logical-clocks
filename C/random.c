#include <random.h>
#include <stdio.h>

int random_int(int a, int b) {
	double mult = (b - a) / (double) RAND_MAX;
	int r = mult * rand() + a;

	return r;
}
