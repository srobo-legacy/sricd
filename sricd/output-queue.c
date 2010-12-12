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
	for (i = 0; i < 2; i++) {
		tx_queue[i] = g_queue_new();
	}
}

void txq_push(frame_t* tx, uint8_t pri)
{
	void* e = g_memdup(tx, sizeof (*tx));
	g_assert(pri < 2);
	g_queue_push_head(tx_queue[pri], e);

	sric_if_tx_ready();
}

frame_t* txq_next(uint8_t max_pri)
{
	uint8_t i;
	g_assert(max_pri < 2);

	for (i = 0; i <= max_pri; i++) {
		frame_t* r = g_queue_pop_tail(tx_queue[i]);
		if (r != NULL) {
			return r;
		}
	}

	return NULL;
}

bool txq_empty(void)
{
	uint8_t i;

	for (i = 0; i < 2; i++) {
		if (!g_queue_is_empty(tx_queue[i])) {
			return FALSE;
		}
	}

	return TRUE;
}

struct queue_cancel_data {
	GQueue *q;
	void *tag;
};

static void rm_tag_frames(gpointer f, gpointer data)
{
	struct queue_cancel_data *rm_data;
	frame_t *frame;

	frame = f;
	rm_data = data;
	if (frame->tag == rm_data->tag) {
		g_queue_remove(rm_data->q, frame);
		free(frame);
	}

	return;
}

void txq_cancel(void *tag)
{
	struct queue_cancel_data rm_data;
	int i;

	for (i = 0; i < 2; i++) {
		rm_data.q = tx_queue[i];
		rm_data.tag = tag;
		g_queue_foreach(tx_queue[i], rm_tag_frames, &rm_data);
	}

	return;
}
