#include "queue.h"
#include <assert.h>

static pool* queue_pool;

void queue_init(void)
{
	queue_pool = pool_create(sizeof(queue));
}

queue* queue_create(unsigned objsize)
{
	queue* q;
	assert(objsize);
	q = pool_alloc(queue_pool);
	assert(q);
	q->head = NULL;
	q->tail = NULL;
	q->last = NULL;
	q->pool = pool_create(sizeof(void*) + objsize);
}

void queue_destroy(queue* q)
{
	assert(q);
	pool_destroy(q->pool);
	pool_free(queue_pool, q);
}

void* queue_push(queue* q)
{
	unsigned char* buf;
	unsigned char* data;
	assert(q);
	buf = pool_alloc(q->pool);
	*(void**)buf = NULL;
	data = buf + sizeof(void*);
	if (q->tail) {
		*(void**)q->tail = buf;
	} else {
		q->head = q->tail = buf;
	}
	return data;
}

void* queue_pop(queue* q)
{
	assert(q);
	pool_free(q->pool, q->last);
	q->last = q->head;
	q->head = *(void**)q->head;
	if (!q->head) q->tail = NULL;
	return q->last;
}
