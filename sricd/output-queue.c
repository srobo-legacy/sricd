#include "output-queue.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "log.h"
#include "sric-if.h"

/* Transmit queue itself.
   [0] is highest priority, and is only used for ACKs and enumeration. */
static GQueue* tx_queue[2];

void txq_init(void)
{
	uint8_t i;
	for( i=0; i<2; i++ )
		tx_queue[i] = g_queue_new();
}

void txq_push(const frame_t* tx, uint8_t pri)
{
	void* e = g_memdup(tx, sizeof(*tx));
	g_assert( pri < 2 );
	g_queue_push_head(tx_queue[pri], e);

	sric_if_tx_ready();
}

const frame_t* txq_next(void)
{
	uint8_t i;

	for( i=0; i<2; i++ ) {
		const frame_t *r = g_queue_pop_tail(tx_queue[i]);
		if( r != NULL )
			return r;
	}

	return NULL;
}

bool txq_empty(void)
{
	uint8_t i;

	for( i=0; i<2; i++ )
		if ( !g_queue_is_empty(tx_queue[i]) )
			return FALSE;

	return TRUE;
}

