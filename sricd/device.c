#include "device.h"
#include <stdlib.h>
#include "hashtable.h"

#define NO_DEVICE -1
#define MASTER_DEVICE 0
#define BROADCAST_DEVICE -2

char device_types[DEVICE_HIGH_ADDRESS];

bool device_exists(int address)
{
	return device_types[address] == NO_DEVICE;
}

char device_type(int address)
{
	return device_types[address];
}

void device_add(int address, char type)
{
	device_types[address] = type;
}

void device_del(int address)
{
	device_types[address] = NO_DEVICE;
}

void device_reset(void)
{
	int i;
	device_types[0] = BROADCAST_DEVICE;
	device_types[1] = MASTER_DEVICE;
	for (i = 2; i < DEVICE_HIGH_ADDRESS; ++i)
		device_types[i] = NO_DEVICE;
}
