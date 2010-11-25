#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ostric.h"
#include "cmds.h"

void
bus_command(uint8_t *buffer, int len)
{

        printf("Unimplemented: client commands\n");
        abort();
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
