#ifndef _INCLUDED_SRIC_IF
#define _INCLUDED_SRIC_IF
#include <stdint.h>

/* Initialise the interface */
void sric_if_init(const char* fname);

/* Indicate that there are frames available in the transmit queue */
void sric_if_tx_ready(void);

/* Indicate that the currently transmitted frame, if owned by the provided tag,
 * should be cancelled: not retransmitted, and the ack discarded */
void sric_if_cancel(void *tag);

#endif  /* _INCLUDED_SRIC_IF */
