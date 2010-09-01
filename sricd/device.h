#ifndef _INCLUDED_DEVICE
#define _INCLUDED_DEVICE

#include <stdint.h>
#include <stdbool.h>
#include "frame.h"
#include "client.h"

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
{
	device_reset();
}

void device_set_client_notes(int address, client* c, uint64_t notes);
void device_clear_client_notes(client* c);
void device_dispatch_note(const frame* note);

#ifdef __cplusplus
}
#endif

#endif
