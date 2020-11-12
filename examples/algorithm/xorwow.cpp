#ifdef __SCINC__
#define uint32_t int
#else
#include <stdint.h>
#include <stdio.h>
#endif

// Xorwow code taken from  https://en.wikipedia.org/wiki/Xorshift
// with minor changes in syntax

struct xorwow_state
{
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t counter;
};

/* The state array must be initialized to not be all zero in the first four
 * words */
uint32_t xorwow(xorwow_state &state)
{
	/* Algorithm "xorwow" from p. 5 of Marsaglia, "Xorshift RNGs" */
	uint32_t t = state.e;
	uint32_t s = state.a;
	state.e = state.d;
	state.d = state.c;
	state.c = state.b;
	state.b = s;
	t ^= (t >> 2) & 0x3fffffff; // mask sign bit after shift
	t ^= t << 1;
	t ^= s ^ (s << 4);
	state.a = t;
	state.counter += 362437;
	return t + state.counter;
}

int main()
{
	puts("xorwow");
	xorwow_state state;
	state.a = 1*11273;
	state.b = 2*11273;
	state.c = 3*11273;
	state.d = 4*11273;
	state.e = 5*11273;
	state.counter = 0;
	for (int i = 0; i < 1000000; i++)
	{
		uint32_t s = xorwow(state);
		printf("0x%08X\n", s);
	}
	return 0;
}