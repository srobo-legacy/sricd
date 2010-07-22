#include "device.h"
#include <stdlib.h>
#include "pool.h"
#include <assert.h>

#define NO_DEVICE -1
#define MASTER_DEVICE 0
#define BROADCAST_DEVICE -2

typedef struct _device_note_target device_note_target;

struct _device_note_target {
	client* c;
	device_note_target* next;
	uint64_t flags;
};

static char device_types[DEVICE_HIGH_ADDRESS];
static device_note_target* targets[DEVICE_HIGH_ADDRESS];
static pool* note_target_pool = NULL;

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
	targets[0] = NULL;
	targets[1] = NULL;
	for (i = 2; i < DEVICE_HIGH_ADDRESS; ++i) {
		targets[i] = NULL;
		device_types[i] = NO_DEVICE;
	}
	pool_destroy(note_target_pool);
	note_target_pool = pool_create(sizeof(device_note_target));
}

static void device_drop_client_notes(int address, client* c)
{
	device_note_target** ptr;
	device_note_target* cur;
	if (!targets[address])
		return;
	ptr = &(targets[address]);
	cur = targets[address];
	while (cur) {
		if (cur->c == c) {
			*ptr = cur->next;
			pool_free(note_target_pool, ptr);
			return;
		}
		cur = cur->next;
	}
}

static void device_update_client_notes(int address, client* c, uint64_t notes)
{
	device_note_target* cur;
	if (!targets[address]) {
		targets[address] = pool_alloc(note_target_pool);
		targets[address]->c = c;
		targets[address]->next = NULL;
		targets[address]->flags = notes;
	} else {
		cur = targets[address];
		do {
			if (cur->c == c) {
				cur->flags = notes;
				return;
			} else if (cur->next) {
				cur = cur->next;
			} else {
				cur->next = pool_alloc(note_target_pool);
				cur = cur->next;
				cur->c = c;
				cur->next = NULL;
				cur->flags = notes;
			}
		} while(1);
	}
}

void device_set_client_notes(int address, client* c, uint64_t notes)
{
	uint64_t* client_flag_pointer;
	int bit = address;
	if (bit > 64) {
		bit -= 64;
		client_flag_pointer = &(c->device_note_data[1]);
	} else {
		client_flag_pointer = &(c->device_note_data[0]);
	}
	if (notes) {
		*client_flag_pointer |= ((uint64_t)1 << bit);
		device_update_client_notes(address, c, notes);
	} else {
		*client_flag_pointer &= ~((uint64_t)1 << bit);
		device_drop_client_notes(address, c);
	}
}

void device_clear_client_notes(client* c)
{
	int i;
	// TODO: replace this with something more efficient
	for (i = 2; i < DEVICE_HIGH_ADDRESS; ++i) {
		device_set_client_notes(i, c, 0);
	}
}

void device_dispatch_note(const client_note* note)
{
	device_note_target* cur = targets[note->address];
	uint64_t flag = ((uint64_t)1 << note->note);
	while (cur) {
		if (cur->flags & flag)
			client_push_note(cur->c, note);
		cur = cur->next;
	}
}
