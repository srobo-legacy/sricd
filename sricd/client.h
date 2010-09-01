#ifndef _INCLUDED_CLIENT
#define _INCLUDED_CLIENT

#include <stdint.h>
#include <stdbool.h>
#include <glib.h>
#include "frame.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PAYLOAD_MAX 64

// A client is a process, connected over a socket, talking to sric devices.
typedef struct _client client;

typedef void (*client_ping_callback)(client* c);

struct _client {
	GQueue* note_q;
	GQueue* rx_q;
	GIOChannel *gio;
	int                  fd;
	guint rx_timer, note_timer;
	client_ping_callback rx_ping, note_ping;
	uint64_t             device_note_data[2];
};

client* client_create(int fd);
void client_destroy(client* c);
void client_push_rx(client* c, const frame* rx);
void client_push_note(client* c, const frame* note);

/* Caller must free return values of the pop functions */
frame* client_pop_rx(client* c);
frame* client_pop_note(client* c);

#ifdef __cplusplus
}
#endif

#endif
