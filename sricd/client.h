#ifndef _INCLUDED_CLIENT
#define _INCLUDED_CLIENT

#include "queue.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PAYLOAD_MAX 64

typedef struct _client client;
typedef struct _client_rx client_rx;
typedef struct _client_note client_note;

struct _client_rx {
	int address;
	int payload_length;
	char payload[PAYLOAD_MAX];
};

struct _client_note {
	int address;
	int note;
	int payload_length;
	char payload[PAYLOAD_MAX];
};

struct _client {
	queue* note_q;
	queue* rx_q;
	int fd;
};

client* client_create(int fd);
void client_push_rx(client* c, const client_rx* rx);
void client_push_note(client* c, const client_note* rx);
const client_rx* client_pop_rx(client* c);
const client_note* client_pop_note(client* c);

#ifdef __cplusplus
}
#endif

#endif
