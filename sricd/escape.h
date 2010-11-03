#ifndef _INCLUDED_ESCAPE
#define _INCLUDED_ESCAPE

#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

int16_t escape_frame(uint8_t* data, unsigned length, unsigned maxlength);

/* Unescapes the given source buffer into the destination buffer.
   Will stop un-escaping if it encounters a frame boundary.

   Returns the number of bytes used from the source buffer.
   -       src: The source buffer.
   -    srclen: The length of the source buffer.
   -      dest: The destination buffer.
   -   destlen: The length of the destination buffer.
   - dest_used: Upon return, will be filled with the number of bytes filled
                in by this function in the dest buffer. */
uint8_t unescape(const uint8_t* src,
                 uint8_t        srclen,
                 uint8_t*       dest,
                 uint8_t        destlen,
                 uint8_t*       dest_used);

#ifdef __cplusplus
}
#endif

#endif
