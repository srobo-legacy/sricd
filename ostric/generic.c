#include <stdio.h>

#include "ostric.h"
#include "generic.h"
#include "reply.h"

/* That is, the code for some generic sric bus client */

void
generic_msg(struct ostric_client *this, uint8_t *buffer, int len,
			uint8_t **resp, int *rlen)
{

	printf("Generic client received command %X for addr %X, from %X\n",
					buffer[4], buffer[1], buffer[2]);

	/* Emit acknowledgement of receipt, even if we don't know what
	 * something is. */
	emit_response(buffer[1], buffer[2], NULL, 0);
	return;
}

void
generic_token(struct ostric_client *this)
{

	printf("Generic client aquired token. Except I don't want it.\n");
	this->keep_token = false;
	return;
}
