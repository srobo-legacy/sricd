#include "device.h"
#include <stdlib.h>
#include <assert.h>

#define NO_DEVICE -1
#define MASTER_DEVICE 0
#define BROADCAST_DEVICE -2

typedef struct _device_note_target device_note_target;

struct _device_note_target {
	client* c;
	uint64_t flags;
};

static char device_types[DEVICE_HIGH_ADDRESS];
static GList *targets[DEVICE_HIGH_ADDRESS];

bool device_exists(int address)
{
	return device_types[address] != NO_DEVICE;
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
}

static void device_drop_client_notes(int address, client* c)
{
	device_note_target*  cur;
	GList *iter;

	iter = g_list_first(targets[address]);
	while (iter != NULL) {
		cur = iter->data;
		if (cur->c == c) {
			targets[address] = g_list_remove(targets[address], cur);
			free(cur);
			return;
		}
	}

	return;
}

static void device_update_client_notes(int address, client* c, uint64_t notes)
{
	device_note_target* cur;
	GList *iter;

	/* Has client already registered for notifications? */
	iter = g_list_first(targets[address]);
	while (iter != NULL) {
		cur = iter->data;
		if (cur->c == c) {
			/* Client has already registered; update notes */
			cur->flags = notes;
			return;
		}

		iter = g_list_next(iter);
	}

	/* Nope; create a new record, put in list */
	cur = malloc(sizeof (device_note_target));
	cur->c = c;
	cur->flags = notes;
	targets[address] = g_list_append(targets[address], cur);
}

void device_set_client_notes(int address, client* c, uint64_t notes)
{
	uint64_t* client_flag_pointer;
	int       bit = address;
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

void device_dispatch_note(const frame* note)
{
	uint64_t flag;
	device_note_target *cur;
	GList *iter;

	iter = g_list_first(targets[note->source_address]);
	flag = (((uint64_t)1) << note->note);
	while (iter != NULL) {
		cur = iter->data;
		if (cur->flags & flag) {
			client_push_note(cur->c, note);
		}
		iter = g_list_next(iter);
	}

	return;
}

