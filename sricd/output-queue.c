#include "output-queue.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "log.h"
#include "sric-if.h"

static GQueue* tx_queue;

void txq_init(void)
{
	tx_queue = g_queue_new();
}

void txq_push(const tx* tx)
{
	void* e = g_memdup(tx, sizeof(*tx));
	g_queue_push_head(tx_queue, e);

	sric_if_tx_ready();
}

const tx* txq_next(void)
{
	return g_queue_pop_tail(tx_queue);
}

bool txq_empty(void)
{
	return g_queue_is_empty(tx_queue);
}

