#ifndef _INCLUDED_SRIC_IF
#define _INCLUDED_SRIC_IF

/* Initialise the interface */
void sric_if_init(const char* fname);

/* Indicate that there are frames available in the transmit queue */
void sric_if_tx_ready( void );

#endif	/* _INCLUDED_SRIC_IF */
