#ifndef _INCLUDED_CLIENT
#define _INCLUDED_CLIENT

#include <stdint.h>
#include <stdbool.h>
#include <glib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define PAYLOAD_MAX 64

typedef struct _client client;
typedef struct _client_rx client_rx;
typedef struct _client_note client_note;

typedef void (*client_ping_callback)(client* c);

struct _client_rx {
	int  address;
	int  payload_length;
	char payload[PAYLOAD_MAX];
};

struct _client_note {
	int  address;
	int  note;
	int  payload_length;
	char payload[PAYLOAD_MAX];
};

/* A client to sricd: */
struct _client {
	/* Note frames received from the bus waiting to be sent to the client */
	GQueue* note_q;
	/* Frames received from the bus waiting to be sent to the client */
	GQueue* rx_q;

	/* The socket fd and mainloop munging foo */
	GIOChannel *gio;
	int         fd;

	/* Source IDs for the rx and note poll timeouts */
	guint rx_timer, note_timer;
	/* Callbacks for frame and note rx queue modification notification */
	/* (When NULL, client is not blocked polling on these) */
	client_ping_callback rx_ping, note_ping;

	/* Device note mask */
	uint64_t             device_note_data[2];
};

client* client_create(int fd);
void client_destroy(client* c);
void client_push_rx(client* c, const client_rx* rx);
void client_push_note(client* c, const client_note* note);

/* Caller must free return values of the pop functions */
client_rx* client_pop_rx(client* c);
client_note* client_pop_note(client* c);

#ifdef __cplusplus
}
#endif

#endif
