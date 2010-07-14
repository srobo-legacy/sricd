#include "queue.h"
#include <assert.h>

queue* queue_create(unsigned objsize)
{
	pool* p;
	queue* q;
	assert(objsize);
	p = pool_create_extra(sizeof(void*) + objsize, sizeof(queue), (void**)&q);
	q->head = NULL;
	q->tail = NULL;
	q->last = NULL;
	q->pool = p;
}

void queue_destroy(queue* q)
{
	assert(q);
	pool_destroy(q->pool);
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
