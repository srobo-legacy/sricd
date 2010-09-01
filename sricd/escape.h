#ifndef _INCLUDED_ESCAPE
#define _INCLUDED_ESCAPE

#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool escape_frame(uint8_t* data, unsigned length, unsigned maxlength);
bool unescape_frame(uint8_t* data, unsigned length, unsigned* outlength);

#ifdef __cplusplus
}
#endif

#endif
