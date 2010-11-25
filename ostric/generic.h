#include <stdint.h>
#include "ostric.h"

void generic_enum (struct ostric_client *this, uint8_t *buffer, int len,
			uint8_t **resp, int *rlen);
void generic_msg (struct ostric_client *this, uint8_t *buffer, int len,
			uint8_t **resp, int *rlen);
