#include "client.h"
#include "device.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "output-queue.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "log.h"

client* client_create(int fd)
{
	client* c = malloc(sizeof (client));
	assert(c);
	c->note_q = g_queue_new();
	c->rx_q = g_queue_new();
	c->fd = fd;
	c->rx_timer = 0;
	c->note_timer = 0;
	c->rx_ping = NULL;
	c->note_ping = NULL;
	c->device_note_data[0] = 0;
	c->device_note_data[1] = 0;

	c->gio = g_io_channel_unix_new(fd);
	g_io_channel_set_close_on_unref(c->gio, TRUE);
	return c;
}

static void free_qitem(gpointer data, gpointer user_data)
{
	g_free(data);
}

void client_destroy(client* c)
{
	txq_cancel(c);
	device_clear_client_notes(c);
	g_io_channel_unref(c->gio);
	g_queue_foreach(c->note_q, free_qitem, NULL);
	g_queue_foreach(c->rx_q, free_qitem, NULL);
	g_queue_free(c->note_q);
	g_queue_free(c->rx_q);
	free(c);
}

void client_push_rx(client* c, const frame* rx)
{
	void* ptr;
	assert(c);
	assert(rx);

	ptr = g_memdup(rx, sizeof (*rx));
	g_queue_push_head(c->rx_q, ptr);

	if (c->rx_ping) {
		c->rx_ping(c);
	}
}

void client_push_note(client* c, const frame* note)
{
	void* ptr;
	assert(c);
	assert(note);

	ptr = g_memdup(note, sizeof (*note));
	g_queue_push_head(c->note_q, ptr);

	if (c->note_ping) {
		c->note_ping(c);
	}
}

frame* client_pop_rx(client* c)
{
	assert(c);
	return g_queue_pop_tail(c->rx_q);
}

frame* client_pop_note(client* c)
{
	assert(c);
	return g_queue_pop_tail(c->note_q);
}

