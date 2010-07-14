#include "pool.h"
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

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

#ifdef __ARM__
#define REVERSE_SS
#endif

// find an unmarked location
#ifdef REVERSE_SS

#define SIGNBIT 0x80000000

static inline unsigned ss_find(ss_t x)
{
	return __builtin_clz(x);
}

static inline bool ss_test(ss_t x, unsigned idx)
{
	ss_t mask = SIGNBIT;
	mask >>= idx;
	return (x & mask) == 0;
}

static inline ss_t ss_mark(ss_t x, unsigned idx)
{
	ss_t mask = SIGNBIT;
	mask >>= idx;
	return x & ~mask;
}

static inline ss_t ss_unmark(ss_t x, unsigned idx)
{
	ss_t mask = SIGNBIT;
	mask >>= idx;
	return x | mask;
}
#else
static inline unsigned ss_find(ss_t x)
{
	return __builtin_ctz(x);
}

static inline bool ss_test(ss_t x, unsigned idx)
{
	ss_t mask = 1;
	mask <<= idx;
	return (x & mask) == 0;
}

static inline ss_t ss_mark(ss_t x, unsigned idx)
{
	ss_t mask = 1;
	mask <<= idx;
	return x & ~mask;
}

static inline ss_t ss_unmark(ss_t x, unsigned idx)
{
	ss_t mask = 1;
	mask <<= idx;
	return x | mask;
}
#endif

const static ss_t SS_EMPTY = -1UL;
const static ss_t SS_FULL = 0UL;

#define SS_SIZE (sizeof(ss_t)*8)

// slot set group
#define SSG_COUNT (8/sizeof(ss_t))
#define SSG_SIZE (SSG_COUNT*SS_SIZE)

typedef struct _ssg {
	ss_t s[SSG_COUNT];
} ssg_t;

static inline bool ssg_full(ssg_t x)
{
	int i;
	for (i = 0; i < SSG_COUNT; ++i) {
		if (x.s[i] != SS_FULL) return false;
	}
	return true;
}

static inline int ssg_find(ssg_t x)
{
	int i, pos = 0;
	for (i = 0; i < SSG_COUNT; ++i) {
		if (x.s[i] != SS_FULL) {
			return pos + ss_find(x.s[i]);
		}
		pos += SS_SIZE;
	}
	return 0;
}

static inline unsigned _ssg_bucket(unsigned idx)
{
	return idx / SSG_COUNT;
}

static inline unsigned _ssg_subindex(unsigned idx)
{
	return idx % SSG_COUNT;
}

static inline bool ssg_test(ssg_t x, int idx)
{
	return ss_test(x.s[_ssg_bucket(idx)], _ssg_subindex(idx));
}

static inline ssg_t ssg_mark(ssg_t x, int idx)
{
	x.s[_ssg_bucket(idx)] = ss_mark(x.s[_ssg_bucket(idx)], _ssg_subindex(idx));
	return x;
}

static inline ssg_t ssg_unmark(ssg_t x, int idx)
{
	x.s[_ssg_bucket(idx)] = ss_unmark(x.s[_ssg_bucket(idx)], _ssg_subindex(idx));
	return x;
}

static inline ssg_t ssg_empty()
{
	ssg_t x;
	memset(&x, 0, sizeof(x));
	return x;
}

struct _pool {
	unsigned objsize, l2s; // l2s = log2(objsize)
	ssg_t slots;
	unsigned char* source;
	pool* successor;
};

pool* pool_create_extra(unsigned objsize, unsigned extradata, void** edata)
{
	pool* pl;
	unsigned char* buf;
	assert(objsize);
	objsize = npot(objsize);
	buf = malloc(SSG_SIZE*objsize + sizeof(pool) + extradata);
	assert(buf);
	pl = (pool*)(buf + SSG_SIZE*objsize);
	pl->objsize = objsize;
	pl->l2s = __builtin_ctz(objsize);
	pl->slots = ssg_empty();
	pl->source = buf;
	pl->successor = 0;
	if (edata)
		*edata = buf + SSG_SIZE*objsize + sizeof(pool);
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
	if (ssg_full(pl->slots)) {
		if (!pl->successor) {
			pl->successor = pool_create(pl->objsize);
		}
		return pool_alloc(pl->successor);
	} else {
		int pos = ssg_find(pl->slots);
		// fast multiply
		unsigned char* loc = pl->source + (pos << pl->l2s);
		pl->slots = ssg_mark(pl->slots, pos);
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
		// fast divide
		int slot = (unsigned)(p - pl->source) >> pl->l2s;
		assert(ssg_test(pl->slots, slot)); // check we had it marked
		pl->slots = ssg_unmark(pl->slots, slot);
	} else {
		if (pl->successor) {
			pool_free(pl->successor, ptr);
		} else {
			assert(0 && "trying to free a non-allocated pointer");
		}
	}
}
