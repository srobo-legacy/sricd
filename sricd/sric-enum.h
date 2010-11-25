#ifndef _INCLUDED_SRIC_ENUM
#define _INCLUDED_SRIC_ENUM
/* State machine for enumerating the SRIC bus  */
#include <stdbool.h>
#include "frame.h"

/* Start enumerating the bus */
 void sric_enum_start( void );

/* Called when a frame is received */
bool sric_enum_rx( packed_frame_t *f );

#endif	/* _INCLUDED_SRIC_ENUM */
