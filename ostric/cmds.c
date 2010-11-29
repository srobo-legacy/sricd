#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "ostric.h"
#include "cmds.h"
#include "reply.h"

struct bus_msg {
	int len;
	uint8_t *buffer;
};

void
bus_cmd_despatch(void *self, void *msg)
{
	struct ostric_client *client;
	struct bus_msg *bus_msg;

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
	struct ostric_client *client;
	GSList *scan;

	if (len < 5) {
		fprintf(stderr, "Gateway command has invalid length %d\n", len);
		return;
	}

	switch (buffer[4]) {
	case GW_CMD_USE_TOKEN:
		/* Dictates whether we're to wait for a token when sending data,
		 * or whether to send things out uninhibited. Tokens on or off
		 * is indicated by boolean in first byte after cmd */
		if (len != 8) {
			fprintf(stderr, "Invalid GW_CMD_USE_TOKEN size: %d\n",
									len);
			break;
		}

		if (buffer[5] == 0)
			gw_token_mode = false;
		else
			gw_token_mode = true;

		printf("USE_TOKEN mode set to %s\n", (gw_token_mode) ? "true"
							: "false");

		/* Response acknowledging receipt, no actual data in response */
		emit_response(buffer[1], buffer[2], NULL, 0);
		break;

	case GW_CMD_GEN_TOKEN:
		scan = ostric_client_list;

		while (scan != NULL) {
			client = scan->data;
			if (client->has_token) {
				printf("SRICD issued GEN_TOKEN gw command "
					"while token is still on the bus\n");
			}

			scan = g_slist_next(scan);
		}

		printf("Generating bus token\n");

		/* Record that we've generated a token to spin around the
		 * bus, also acknowledge to sricd */
		gw_has_token = true;
		gw_keep_token = false;
		emit_response(buffer[1], buffer[2], NULL, 0);
		break;

	case GW_CMD_REQ_TOKEN:
		printf("Gateway requesting token\n");
		/* Set flag for us to keep token when it comes around again */
		gw_keep_token = true;
		emit_response(buffer[1], buffer[2], NULL, 0);
		break;

	case GW_CMD_HAVE_TOKEN:
	default:
		fprintf(stderr, "Unimplemented gateway cmd %d\n", buffer[4]);
		break;
	}

	return;
}
