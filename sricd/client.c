#include "client.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "queue.h"
#include "log.h"
#include "input.h"

client* client_create(int fd)
{
	client* c = malloc(sizeof(client));
	assert(c);
	c->note_q = queue_create(sizeof(client_note));
	c->rx_q   = queue_create(sizeof(client_rx));
	c->fd = fd;
	return c;
}

void client_destroy(client* c)
{
	queue_destroy(c->note_q);
	queue_destroy(c->rx_q);
	free(c);
}

void client_push_rx(client* c, const client_rx* rx)
{
	void* ptr;
	assert(c);
	assert(rx);
	ptr = queue_push(c->rx_q);
	memcpy(ptr, rx, sizeof(*rx));
}

void client_push_note(client* c, const client_note* note)
{
	void* ptr;
	assert(c);
	assert(note);
	ptr = queue_push(c->note_q);
	memcpy(ptr, note, sizeof(*note));
}

const client_rx* client_pop_rx(client* c)
{
	assert(c);
	if (queue_empty(c->rx_q))
		return NULL;
	else
		return queue_pop(c->rx_q);
}

const client_note* client_pop_note(client* c)
{
	assert(c);
	if (queue_empty(c->note_q))
		return NULL;
	else
		return queue_pop(c->note_q);
}
