#ifndef _INCLUDED_QUEUE
#define _INCLUDED_QUEUE

#include "pool.h"
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _queue queue;

struct _queue {
	void* head;
	void* tail;
	void* last;
	pool* pool;
};

queue* queue_create(unsigned objsize);
void queue_destroy(queue* q);

inline static bool queue_empty(queue* q)
{
	return q->head == 0;
}

void* queue_push(queue* q);
void* queue_pop(queue* q);

#ifdef __cplusplus
}
#endif

#endif
