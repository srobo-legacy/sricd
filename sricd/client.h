#ifndef _INCLUDED_CLIENT
#define _INCLUDED_CLIENT

#include "queue.h"
#include <stdint.h>
#include <stdbool.h>

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

struct _client {
	queue*               note_q;
	queue*               rx_q;
	int                  fd;
	int                  rx_timer, note_timer;
	client_ping_callback rx_ping, note_ping;
	uint64_t             device_note_data[2];
};

client* client_create(int fd);
void client_destroy(client* c);
void client_push_rx(client* c, const client_rx* rx);
void client_push_note(client* c, const client_note* note);
const client_rx* client_pop_rx(client* c);
const client_note* client_pop_note(client* c);

#ifdef __cplusplus
}
#endif

#endif
