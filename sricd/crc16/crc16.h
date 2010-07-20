#ifndef _INCLUDED_CRC16
#define _INCLUDED_CRC16

#include <inttypes.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint16_t crc16(const void* data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
