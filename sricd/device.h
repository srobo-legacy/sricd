#ifndef _INCLUDED_DEVICE
#define _INCLUDED_DEVICE

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DEVICE_HIGH_ADDRESS 128

bool device_exists(int address);
char device_type(int device);
void device_add(int address, char type);
void device_del(int address);
void device_reset(void);
static inline void device_init(void)
	{ device_reset(); }

#ifdef __cplusplus
}
#endif

#endif
