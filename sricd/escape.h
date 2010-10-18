#ifndef _INCLUDED_ESCAPE
#define _INCLUDED_ESCAPE

#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

int16_t escape_frame(uint8_t* data, unsigned length, unsigned maxlength);

#ifdef __cplusplus
}
#endif

#endif
