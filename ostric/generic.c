#include <stdio.h>

#include "ostric.h"
#include "generic.h"

/* That is, the code for some generic sric bus client */

void
generic_msg(struct ostric_client *this, uint8_t *buffer, int len,
			uint8_t **resp, int *rlen)
{

	printf("Generic client received command %X for addr %X, from %X\n",
					buffer[4], buffer[1], buffer[2]);
	return;
}
