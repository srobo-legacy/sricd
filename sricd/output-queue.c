#include "output-queue.h"
#include "pool.h"
#include <stdlib.h>
#include <string.h>
#include "log.h"
#define NPRIOS 4
static queue* tx_queues[NPRIOS];
static int    count = 0;
void txq_init(void)
{
	int i;
	wlog("starting %d tx priority levels", NPRIOS);
	for (i = 0; i < NPRIOS; ++i) {
		tx_queues[i] = queue_create(sizeof (tx));
	}
}
void txq_push(const tx* tx, int prio)
{
	void* e;
	prio = (NPRIOS - 1) - prio;
	e    = queue_push(tx_queues[prio]);
	// TODO: shorten this based on payload size
	memcpy(e, tx, sizeof (*tx));
	++count;
}
const tx* txq_next(void)
{
	int i;
	for (i = 0; i < NPRIOS; ++i) {
		if (!queue_empty(tx_queues[i])) {
			--count;
			return queue_pop(tx_queues[i]);
		}
	}
	return NULL;
}
bool txq_empty(void)
{
	return count == 0;
}
