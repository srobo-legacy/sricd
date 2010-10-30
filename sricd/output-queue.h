#ifndef _INCLUDED_OUTPUT_QUEUE
#define _INCLUDED_OUTPUT_QUEUE

#include "client.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Entries in the tx queue: */
typedef struct _tx {
	/* The destination address */
	int   address;

	/* The client from which this frame originated. */
	void* tag; // if tag == NULL, this is a response

	/* The payload for this frame: */
	int   payload_length;
	char  payload[PAYLOAD_MAX];
} tx;

/* Initialise the transmit queue */
void txq_init(void);

/* Push a frame onto the transmit queue.
   Frame is copied into freshly allocated memory. */
void txq_push(const tx* tx);

/* Pop the next frame off the transmit queue.
   The caller must free the result when they are done with it.
   Returns NULL if the queue is empty. */
const tx* txq_next(void);

/* Returns true if the queue is empty. */
bool txq_empty(void);

#ifdef __cplusplus
}
#endif

#endif
