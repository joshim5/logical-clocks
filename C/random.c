#include <random.h>
#include <stdio.h>
#include <stdint.h>

uint32_t x = 27438327;
uint32_t y = 132499;
uint32_t z = 524328978;
uint32_t w = 324893278;
uint32_t v = 2482937594;

double random_double() {
    return xorshift() / 4294967296.0;
}

int random_int(int a, int b) {
	int r = random_double() * (b - a) + a;

	return r;
}

uint32_t xorshift(void) { 
	uint32_t t = (x ^ (x << 2));
	x = y;
	y = z;
	z = w;
	w = v;
	return v = (v ^ (v >> 4)) ^ (t ^ (t >> 1));
}
