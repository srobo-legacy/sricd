#include "pool.h"
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

// some maths utility functions
// next power of two
static unsigned npot(unsigned x)
{
	--x;
	x = (x >>  1) | x;
	x = (x >>  2) | x;
	x = (x >>  4) | x;
	x = (x >>  8) | x;
	x = (x >> 16) | x;
	++x;
	return x;
}

// calculate x*y fast, given that y is a power of two
static inline unsigned mbpot(unsigned x, unsigned y)
{
	return x << (__builtin_ctz(y));
}

// calculate x/y fast, given that y is a power of two
static inline unsigned dbpot(unsigned x, unsigned y)
{
	return x >> (__builtin_ctz(y));
}

// slot set
typedef unsigned long ss_t;

// find an unmarked location
static inline int ss_find(ss_t x)
{
	return __builtin_ctz(x);
}

static inline bool ss_test(ss_t x, int idx)
{
	ss_t mask = 1;
	mask <<= idx;
	return (x & mask) == 0;
}

static inline ss_t ss_mark(ss_t x, int idx)
{
	ss_t mask = 1;
	mask <<= idx;
	return x & ~mask;
}

static inline ss_t ss_unmark(ss_t x, int idx)
{
	ss_t mask = 1;
	mask <<= idx;
	return x | mask;
}

const static ss_t SS_EMPTY = -1UL;
const static ss_t SS_FULL = 0UL;

#define SS_SIZE (sizeof(ss_t)*8)

struct _pool {
	unsigned objsize;
	ss_t slots;
	unsigned char* source;
	pool* successor;
};

pool* pool_init(unsigned objsize)
{
	pool* pl;
	unsigned char* buf;
	assert(objsize);
	objsize = npot(objsize);
	buf = malloc(SS_SIZE*objsize + sizeof(pool));
	assert(buf);
	pl = (pool*)(buf + SS_SIZE*objsize);
	pl->objsize = objsize;
	pl->slots = SS_EMPTY;
	pl->source = buf;
	pl->successor = 0;
	return pl;
}

void pool_destroy(pool* pl)
{
	pool* next;
	assert(pl);
	next = pl;
	while (next) {
		pl = next;
		next = next->successor;
		free(pl->source);
	}
}

void* pool_alloc(pool* pl)
{
	assert(pl);
	if (pl->slots == SS_FULL) {
		if (!pl->successor) {
			pl->successor = pool_init(pl->objsize);
		}
		return pool_alloc(pl->successor);
	} else {
		int pos = ss_find(pl->slots);
		unsigned char* loc = pl->source + mbpot(pos, pl->objsize);
		pl->slots = ss_mark(pl->slots, pos);
		return loc;
	}
}

void pool_free(pool* pl, void* ptr)
{
	unsigned char* p = (unsigned char*)ptr;
	assert(pl);
	if (!ptr) return;
	if (p >= pl->source && p < (unsigned char*)pl) {
		// in range - calculate slot
		int slot = dbpot((unsigned)(p - pl->source), pl->objsize);
		assert(ss_test(pl->slots, slot)); // check we had it marked
		pl->slots = ss_unmark(pl->slots, slot);
	} else {
		if (pl->successor)
			pool_free(pl->successor, ptr);
		else {
			assert(0 && "trying to free a non-allocated pointer");
		}
	}
}
