#include "device.h"
#include <stdlib.h>
#include "hashtable.h"

static hashtable* device_table = NULL;

static int device_hash(const void* ptr)
{
	const device* dev = (const device*)ptr;
	return dev->address;
}

static int device_cmp(const void* ptrA, const void* ptrB)
{
	const device* devA = (const device*)ptrA;
	const device* devB = (const device*)ptrB;
	if (devA->address < devB->address) return -1;
	else if (devA->address > devB->address) return 1;
	else return 0;
}

void device_init(void)
{
	device_reset();
}

void device_add(const device* dev)
{
	hashtable_insert(device_table, dev);
}

void device_del(int address)
{
	device key;
	key.address = address;
	hashtable_delete(device_table, &key);
}

void device_reset(void)
{
	if (device_table)
		hashtable_destroy(device_table);
	device_table = hashtable_create(sizeof(device), 64,
	                                device_hash,
	                                device_cmp);
}
