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
#include "device.h"
#include <stdbool.h>
#include <arpa/inet.h>
#include <glib.h>
#include <signal.h>

static const char* sock_path;

static int sock;
static GIOChannel* ipc_gio = NULL;

static gboolean ipc_new(GIOChannel* src, GIOCondition cond, gpointer data);
static gboolean ipc_death(GIOChannel* gio, GIOCondition cond, gpointer data);
static gboolean ipc_client_incoming(GIOChannel* gio,
                                    GIOCondition cond,
                                    gpointer _client);
static gboolean ipc_client_death(GIOChannel* gio,
                                 GIOCondition cond,
                                 gpointer _client);

static void ipc_boot(void)
{
	int rv;
	struct sockaddr_un addr;
	socklen_t len;
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	assert(sock);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sock_path);
	len = sizeof (addr);
	rv = unlink(sock_path);
	assert(rv == 0 || errno == ENOENT);
	rv = bind(sock, (struct sockaddr*)&addr, len);
	assert(rv == 0);
	wlog("created IPC socket as fd %d", sock);

	rv = listen(sock, 100);
	assert(rv == 0);

	/* Ignore SIGPIPE -- If we don't, the process can be aborted
	 * if we try writing to a client that has disconnected. */
	g_assert( signal( SIGPIPE, SIG_IGN ) != SIG_ERR );
}

static gboolean ipc_new(GIOChannel* src, GIOCondition cond, gpointer data)
{
	client* c;
	struct sockaddr_un addr;
	socklen_t len = sizeof (addr);
	int new;
	int sock;
	assert(src == ipc_gio);
	assert(cond == G_IO_IN);
	sock = g_io_channel_unix_get_fd(ipc_gio);

	new = accept(sock, (struct sockaddr*)&addr, &len);
	wlog("accepted new connection");

	c = client_create(new);
	g_io_add_watch(c->gio, G_IO_IN, ipc_client_incoming, c);
	g_io_add_watch(c->gio, G_IO_HUP, ipc_client_death, c);

	return TRUE;
}

#define SRICD_TX 0
#define SRICD_NOTE_FLAGS 1
#define SRICD_POLL_RX 2
#define SRICD_POLL_NOTE 3
#define SRICD_NOTE_CLEAR 4
#define SRICD_LIST_DEVICES 5

#define SRIC_E_SUCCESS 0
#define SRIC_E_BADADDR 1
#define SRIC_E_TIMEOUT 2
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
	frame_t       frame;
	if (!read_data(fd, buf, 4)) {
		write_result(fd, c, SRIC_E_BADREQUEST);
		return;
	}
	frame.type = FRAME_SRIC;
/* XXX - this isn't really big endian, is it? */
	frame.address = (int)buf[0] | ((int)buf[1] << 8);
	frame.payload_length = (int)buf[2] | ((int)buf[3] << 8);
	if (!read_data(fd, frame.payload, frame.payload_length)) {
		write_result(fd, c, SRIC_E_BADREQUEST);
		return;
	}
	frame.tag = c;

	if (!device_exists(frame.address)) {
		write_result(fd, c, SRIC_E_BADADDR);
		return;
	}

	txq_push(&frame, 1);
	write_result(fd, c, SRIC_E_SUCCESS);
}

static void send_rx(int fd, client* c, const frame* rx)
{
	uint8_t response_header[7];
	response_header[0] = SRIC_E_SUCCESS;
	response_header[1] = 0x0;
	response_header[2] = rx->source_address;
	response_header[3] = rx->note >> 8;
	response_header[4] = rx->note & 0xFF;
	response_header[5] = 0;
	response_header[6] = rx->payload_length;
	write_data(fd, c, response_header, 7);
	write_data(fd, c, rx->payload,     rx->payload_length);
}

static gboolean timeout_callback(gpointer _client)
{
	client* c = (client*)_client;

	/* Dequeue frames the client transmitted - preventing them from being
	 * retransmitted forever. This can lead to unexpected behavior (IE,
	 * send three frames, one times out, all three dequeued), but these
	 * are all situation where strict request-ack ordering isn't honoured */
	txq_cancel(c);

	/* Tell client about it */
	write_result(c->fd, c, SRIC_E_TIMEOUT);

	/* Clear record for this timeout */
	c->rx_timer = 0;
	return FALSE;
}

static void rx_ping(client* c)
{
	frame* rx;
	if (c->rx_timer != 0) {
		g_source_remove(c->rx_timer);
		c->rx_timer = 0;
	}
	c->rx_ping = NULL;
	rx = client_pop_rx(c);
	assert(rx);
	send_rx(c->fd, c, rx);

	g_free(rx);
}

static gboolean note_timeout_callback(gpointer _client)
{
	client *c;

	c = _client;

	/* Inform client of timeout */
	write_result(c->fd, c, SRIC_E_TIMEOUT);
	c->note_timer = 0;
	return FALSE;
}

static void note_ping(client* c)
{
	frame* note;
	if (c->note_timer != 0) {
		g_source_remove(c->note_timer);
		c->note_timer = 0;
	}
	c->note_ping = NULL;
	note = client_pop_note(c);
	assert(note);
	send_rx(c->fd, c, note);

	g_free(note);
}

static void handle_poll_rx(int fd, client* c)
{
	frame*  rx;
	int32_t timeout;
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
			c->rx_timer = g_timeout_add(timeout, timeout_callback, c);
		}
		c->rx_ping = rx_ping;
	}
}

static void handle_poll_note(int fd, client* c)
{
	frame* note;
	int32_t timeout;
	if (!read_data(fd, &timeout, 4)) {
		write_result(fd, c, SRIC_E_BADREQUEST);
		return;
	}
	timeout = ntohl(timeout);
	if ((note = client_pop_note(c))) {
		// RX was immediately available, return it
		send_rx(fd, c, note);
		g_free(note);
	} else if (timeout == 0) {
		// immediately return
		write_result(fd, c, SRIC_E_TIMEOUT);
	} else {
		// schedule for timeout
		if (timeout != -1) {
			c->note_timer = g_timeout_add(timeout,
						note_timeout_callback, c);
		}
		c->note_ping = note_ping;
	}
}

static uint64_t ntohll(uint64_t x)
{
#ifdef __BIG_ENDIAN__
	return x;
#else
	int i;
	union {
		uint64_t intVal;
		uint8_t bytes[8];
	} src, dst;
	src.intVal = x;
	for (i = 0; i < 8; ++i) {
		dst.bytes[7 - i] = src.bytes[i];
	}
	return dst.intVal;
#endif
}

static void handle_note_flags(int fd, client* c)
{
	uint16_t device;
	uint64_t flags;
	if (!read_data(fd, &device, 2)) {
		write_result(fd, c, SRIC_E_BADREQUEST);
		return;
	}
	device = ntohs(device);
	if (!read_data(fd, &flags, 8)) {
		write_result(fd, c, SRIC_E_BADREQUEST);
	}
	flags = ntohll(flags);
	if (!device_exists(device)) {
		write_result(fd, c, SRIC_E_BADADDR);
		return;
	}
	device_set_client_notes(device, c, flags);
	write_result(fd, c, SRIC_E_SUCCESS);
}

static void handle_note_clear(int fd, client* c)
{
	device_clear_client_notes(c);
	write_result(fd, c, SRIC_E_SUCCESS);
}

static void handle_list_devices(int fd, client* c)
{
	int i;
	uint16_t deviceCount = 0;
	for (i = 0; i < DEVICE_HIGH_ADDRESS; ++i) {
		if (device_exists(i)) {
			deviceCount++;
		}
	}
	deviceCount = htons(deviceCount);
	write_result(fd, c, SRIC_E_SUCCESS);
	write_data(fd, c, &deviceCount, 2);
	for (i = 0; i < DEVICE_HIGH_ADDRESS; ++i) {
		if (device_exists(i)) {
			short data = (short)i;
			data = htons(data);
			write_data(fd, c, &data, 2);
			data = (short)device_type(i);
			data = htons(data);
			write_data(fd, c, &data, 2);
		}
	}
}

static gboolean ipc_client_incoming(GIOChannel*  gio,
                                    GIOCondition cond,
                                    gpointer _client)
{
	unsigned char cmd;
	client* c  = (client*)_client;
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

	case SRICD_POLL_NOTE:
		handle_poll_note(fd, c);
		break;

	case SRICD_NOTE_FLAGS:
		handle_note_flags(fd, c);
		break;

	case SRICD_NOTE_CLEAR:
		handle_note_clear(fd, c);
		break;

	case SRICD_LIST_DEVICES:
		handle_list_devices(fd, c);
		break;

	default:
		cmd = SRIC_E_BADREQUEST;
		write(fd, &cmd, 1);
		break;
	}

	return TRUE;
}

static gboolean ipc_client_death(GIOChannel*  gio,
                                 GIOCondition cond,
                                 gpointer     _client)
{
	client* c = (client*)_client;
	assert(c);
	client_destroy(c);
	wlog("IPC client died");
	return FALSE;
}

static gboolean ipc_death(GIOChannel* gio, GIOCondition cond, gpointer data)
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

void ipc_listen( void )
{
	ipc_gio = g_io_channel_unix_new(sock);
	g_io_channel_set_close_on_unref(ipc_gio, TRUE);

	g_io_add_watch(ipc_gio, G_IO_IN, ipc_new, NULL);
	g_io_add_watch(ipc_gio, G_IO_HUP, ipc_death, NULL);

	wlog("Enumeration finished: listening for commands.");
}
