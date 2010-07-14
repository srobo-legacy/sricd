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

static void client_rx_out(int fd, void* client_object)
{
	const client_rx* rx;
	client* c = (client*)client_object;
	assert(c);
	rx = client_pop_rx(c);
	if (rx) {
		// write the RX to fd
		if (!queue_empty(c->rx_q)) {
			input_listen(c->rxfd, NULL, NULL, client_rx_out, c);
		}
	}
	c->free_rx = (rx != NULL);
}

static void client_note_out(int fd, void* client_object)
{
	const client_note* note;
	client* c = (client*)client_object;
	assert(c);
	note = client_pop_note(c);
	if (note) {
		// write the note to fd
		if (!queue_empty(c->note_q)) {
			input_listen(c->notefd, NULL, NULL, client_note_out, c);
		}
	}
	c->free_note = (note != NULL);
}

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
	if (c->free_rx)
		client_rx_out(c->rxfd, c);
	else
		input_listen(c->rxfd, NULL, NULL, client_rx_out, c);
}

void client_push_note(client* c, const client_note* note)
{
	void* ptr;
	assert(c);
	assert(note);
	ptr = queue_push(c->note_q);
	memcpy(ptr, note, sizeof(*note));
	if (c->free_note)
		client_note_out(c->notefd, c);
	else
		input_listen(c->notefd, NULL, NULL, client_note_out, c);
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

void client_open_fifos(client* c)
{
	// TODO: put this into a background thread
	char rxbuf[512];
	char notebuf[512];
	int rc, idx;
	ssize_t writelen, truelen;
	char sendbuf[1024];
	c->rxfd = 0;
	c->notefd = 0;
	tmpnam(rxbuf);
	tmpnam(notebuf);
	rc = mkfifo(rxbuf, 0755);
	assert(rc == 0);
	rc = mkfifo(notebuf, 0755);
	assert(rc == 0);
	assert(strlen(rxbuf) < 256);
	assert(strlen(notebuf) < 256);
	sendbuf[0] = strlen(rxbuf);
	memcpy(sendbuf + 1, rxbuf, strlen(rxbuf) + 1);
	sendbuf[strlen(rxbuf) + 2] = strlen(notebuf);
	memcpy(sendbuf + strlen(rxbuf) + 3, notebuf, strlen(notebuf) + 1);
	writelen = sendbuf + 4 + strlen(rxbuf) + strlen(notebuf);
	truelen = write(c->fd, sendbuf, writelen);
	assert(writelen == truelen);
	c->rxfd = open(rxbuf, O_WRONLY);
	c->notefd = open(notebuf, O_WRONLY);
	c->free_rx = true;
	c->free_note = true;
	wlog("connected to client over FIFOs");
}
