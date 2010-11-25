#ifndef _INCLUDED_SRIC_ENUM
#define _INCLUDED_SRIC_ENUM
/* State machine for enumerating the SRIC bus  */
#include <stdbool.h>

/* Start enumerating the bus */
 void sric_enum_start( void );

/* Called when a frame is received */
bool sric_enum_rx( void );

#endif	/* _INCLUDED_SRIC_ENUM */
