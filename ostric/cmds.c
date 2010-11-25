#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "ostric.h"
#include "cmds.h"

struct bus_msg {
	int len;
	uint8_t *buffer;
};

void
bus_cmd_despatch(void *self, void *msg)
{
	struct ostric_client *client;
	struct bus_msg *bus_msg;

	if (self == NULL)
		return;

	client = self;
	bus_msg = msg;

	/* Ahem */
	client->msg_callback(client, bus_msg->buffer, bus_msg->len, NULL, NULL);
	return;
}

void
bus_command(uint8_t *buffer, int len)
{
	struct bus_msg bus_msg;

	bus_msg.len = len;
	bus_msg.buffer = buffer;

	/* Send this bus message to all clients */
	g_slist_foreach(ostric_client_list, bus_cmd_despatch, &bus_msg);
	return;
}       

void
gateway_command(uint8_t *buffer, int len)
{

	if (len < 5) {
		fprintf(stderr, "Gateway command has invalid length %d\n", len);
		return;
	}

	switch (buffer[4]) {
	case GW_CMD_USE_TOKEN:
	case GW_CMD_REQ_TOKEN:
	case GW_CMD_HAVE_TOKEN:
	case GW_CMD_GEN_TOKEN:
	default:
		fprintf(stderr, "Unimplemented gateway cmd %d\n", buffer[4]);
		break;
	}

	return;
}
