#include <stdio.h>

#include "ostric.h"
#include "generic.h"
#include "reply.h"

#include "../sricd/sric-cmds.h"

/* That is, the code for some generic sric bus client */

void
generic_msg(struct ostric_client *this, uint8_t *buffer, int len,
			uint8_t **resp, int *rlen)
{
	uint8_t resp_byte;

	/* Don't handle messages that aren't for us / broadcast */
	if ((buffer[1] & 0x7F) != this->address && (buffer[1] & 0x7F) != 0)
		return;

	switch (buffer[4]) {
	case 0x80 | SRIC_SYSCMD_RESET:
		printf("Generic client received reset\n");
		/* Move to reset state; keep token if it turns up */
		this->address = -1;
		this->has_token = false;
		this->keep_token = true;
		break;

	case 0x80 | SRIC_SYSCMD_TOK_ADVANCE:
		/* We should have received an address by now; there's no real
		 * way to detect whether we've just been reset though. Can
		 * check that we're still sat on the token though: */
		if (!this->has_token && !this->keep_token)
			printf("Generic client 0x%X instructed to advance "
				"token while not retaining it\n",
				this->address);

		/* Stop retaining. This should take effect when the token
		 * rotates, IE immediately after this frame is procesed. */
		this->keep_token = false;
		break;

	case 0x80 | SRIC_SYSCMD_ADDR_ASSIGN:
		/* To take an address we need to be a) reset, b) have token */
		if (this->address != -1 || !this->has_token)
			return;

		this->address = buffer[5] & 0x7F;
		break;

	case 0x80 | SRIC_SYSCMD_ADDR_INFO:
		/* If we're reset and have token, inform controller about what
		 * kind of sric thing we are. Exact format as yet undecided. */
		resp_byte = 0;
		emit_data_ack(0, buffer[2] & 0x7F, &resp_byte, 1);
		break;

	default:
		printf("Generic client received unimplemented command %X for "
				"addr %X, from %X\n", buffer[4], buffer[1],
				buffer[2]);
	}

	return;
}

void
generic_token(struct ostric_client *this)
{

	/* We're in the reset state; don't do anything with token */
	if (this->address == -1)
		return;

	printf("Generic client aquired token. Except I don't want it.\n");
	this->keep_token = false;
	return;
}
