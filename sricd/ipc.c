#include "ipc.h"
#include "client.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "log.h"
#include "output-queue.h"
#include "sched.h"
#include <stdbool.h>
#include <arpa/inet.h>
#include <glib.h>

static const char* sock_path;

static GIOChannel* ipc_gio = NULL;

static gboolean ipc_new( GIOChannel *src, GIOCondition cond, gpointer data );
static gboolean ipc_death( GIOChannel *gio, GIOCondition cond, gpointer data );
static gboolean ipc_client_incoming( GIOChannel *gio, GIOCondition cond, gpointer _client );
static gboolean ipc_client_death( GIOChannel *gio, GIOCondition cond, gpointer _client );

static void ipc_boot(void)
{
	int                s, rv;
	struct sockaddr_un addr;
	socklen_t          len;
	s               = socket(PF_UNIX, SOCK_STREAM, 0);
	assert(s);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sock_path);
	len             = sizeof (addr);
	rv              = unlink(sock_path);
	assert(rv == 0 || errno == ENOENT);
	rv              = bind(s, (struct sockaddr*)&addr, len);
	assert(rv == 0);
	rv              = listen(s, 5);
	assert(rv == 0);
	wlog("created IPC socket as fd %d", s);

	ipc_gio = g_io_channel_unix_new(s);
	g_io_channel_set_close_on_unref(ipc_gio, TRUE);

	g_io_add_watch(ipc_gio, G_IO_IN, ipc_new, NULL);
	g_io_add_watch(ipc_gio, G_IO_HUP, ipc_death, NULL);
}

static gboolean ipc_new( GIOChannel *src, GIOCondition cond, gpointer data )
{
	client*            c;
	struct sockaddr_un addr;
	socklen_t          len = sizeof (addr);
	int                new;
	int sock;
	assert( src == ipc_gio );
	assert( cond == G_IO_IN );
	sock = g_io_channel_unix_get_fd(ipc_gio);

	new = accept(sock, (struct sockaddr*)&addr, &len);
	wlog("accepted new connection");

	c = client_create(new);
	g_io_add_watch( c->gio, G_IO_IN, ipc_client_incoming, c );
	g_io_add_watch( c->gio, G_IO_HUP, ipc_client_death, c );

	return TRUE;
}

#define SRICD_TX           0
#define SRICD_NOTE_FLAGS   1
#define SRICD_POLL_RX      2
#define SRICD_POLL_NOTE    3
#define SRICD_NOTE_CLEAR   4
#define SRICD_LIST_DEVICES 5

#define SRIC_E_SUCCESS    0
#define SRIC_E_BADADDR    1
#define SRIC_E_TIMEOUT    2
#define SRIC_E_BADREQUEST 3

static bool read_data(int fd, void* target, unsigned length)
{
	int rc = read(fd, target, length);
	return rc == length;
}

static bool write_data(int fd, client* c, const void* data, unsigned length)
{
	int rc = write(fd, data, length);
	return rc == length;
}

static bool write_result(int fd, client* c, unsigned char result)
{
	return write_data(fd, c, &result, 1);
}

static void handle_tx(int fd, client* c)
{
	unsigned char buf[4];
	tx            frame;
	if (!read_data(fd, buf, 4)) {
		write_result(fd, c, SRIC_E_BADREQUEST);
		return;
	}
	frame.address        = (int)buf[0] | ((int)buf[1] << 8);
	frame.payload_length = (int)buf[2] | ((int)buf[3] << 8);
	if (!read_data(fd, frame.payload, frame.payload_length)) {
		write_result(fd, c, SRIC_E_BADREQUEST);
		return;
	}
	frame.tag = c;
	txq_push(&frame);
	write_result(fd, c, SRIC_E_SUCCESS);
}

static void send_rx(int fd, client* c, const client_rx* rx)
{
	char response_header[7];
	response_header[0] = SRIC_E_SUCCESS;
	response_header[1] = (rx->address >> 0) & 0xFF;
	response_header[2] = (rx->address >> 8) & 0xFF;
	response_header[3] = 0xFF;
	response_header[4] = 0xFF;
	response_header[5] = (rx->payload_length >> 0) & 0xFF;
	response_header[6] = (rx->payload_length >> 8) & 0xFF;
	write_data(fd, c, response_header, 7);
	write_data(fd, c, rx->payload,     rx->payload_length);
}

static void rx_timeout(int timer, void* ptr)
{
	// client* c = (client*)ptr;
	// TODO: stuff here
}

static void rx_ping(client* c)
{
	client_rx* rx;
	if (c->rx_timer != -1) {
		sched_cancel(c->rx_timer);
	}
	c->rx_ping = NULL;
	rx         = client_pop_rx(c);
	assert(rx);
	send_rx(c->fd, c, rx);

	g_free(rx);
}

static void handle_poll_rx(int fd, client* c)
{
	client_rx* rx;
	int32_t          timeout;
	if (!read_data(fd, &timeout, 4)) {
		write_result(fd, c, SRIC_E_BADREQUEST);
		return;
	}
	timeout = ntohl(timeout);
	if ((rx = client_pop_rx(c))) {
		// RX was immediately available, return it
		send_rx(fd, c, rx);
		g_free(rx);
	} else if (timeout == 0) {
		// immediately return
		write_result(fd, c, SRIC_E_TIMEOUT);
	} else {
		// schedule for timeout
		if (timeout != -1) {
			c->rx_timer = sched_event(timeout, rx_timeout, (void*)c);
		}
		c->rx_ping = rx_ping;
	}
}

static gboolean ipc_client_incoming( GIOChannel *gio, GIOCondition cond, gpointer _client )
{
	unsigned char cmd;
	client* c= (client*)_client;
	int fd = g_io_channel_unix_get_fd(gio);
	assert(c);

	// do R/W stuff here
	if (read(fd, &cmd, 1) == 0) {
		// EOF:
		return FALSE;
	}
	switch (cmd) {
	case SRICD_TX:
		handle_tx(fd, c);
		break;

	case SRICD_POLL_RX:
		handle_poll_rx(fd, c);
		break;

	default:
		cmd = SRIC_E_BADREQUEST;
		write(fd, &cmd, 1);
		break;
	}

	return TRUE;
}

static gboolean ipc_client_death( GIOChannel *gio, GIOCondition cond, gpointer _client )
{
	client* c = (client*)_client;
	assert(c);
	client_destroy(c);
	wlog("IPC client died");
	return FALSE;
}

static gboolean ipc_death( GIOChannel *gio, GIOCondition cond, gpointer data )
{
	g_io_channel_unref(gio);

	wlog("IPC socket died, restarting");
	ipc_boot();

	return FALSE;
}

void ipc_init(const char* socket_path)
{
	sock_path = socket_path;
	ipc_boot();
}

