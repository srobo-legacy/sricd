#ifndef _INCLUDED_OUTPUT_QUEUE
#define _INCLUDED_OUTPUT_QUEUE

#include "client.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	/* Frames destined for the bus */
	FRAME_SRIC    = 0x7e,
	/* Frames that configure the SRIC interface */
	FRAME_GW_CONF = 0x8e,
} frame_type_t;

/* Entries in the tx queue: */
typedef struct _tx {
	/* The type of frame */
	frame_type_t type;

	/* The destination address */
	int          address;

	/* The client from which this frame originated. */
	void*        tag; // if tag == NULL, this is a response

	/* The payload for this frame: */
	int          payload_length;
	char         payload[PAYLOAD_MAX];
} frame_t;

/* Initialise the transmit queue */
void txq_init(void);

/* Push a frame onto the transmit queue.
   Frame is copied into freshly allocated memory.
   pri is the priority of the frame -- for normal frames, this should be 1. */
void txq_push(const frame_t* tx, uint8_t pri);

/* Pop the next frame off the transmit queue.
   max_pri is the maximum priority that this may look at.
   The caller must free the result when they are done with it.
   Returns NULL if the queue is empty. */
const frame_t* txq_next(uint8_t max_pri);

/* Returns true if the queue is empty. */
bool txq_empty(void);

#ifdef __cplusplus
}
#endif

#endif
