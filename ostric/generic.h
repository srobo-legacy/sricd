#ifndef _SRICD_OSTRIC_GENERIC_H_
#define _SRICD_OSTRIC_GENERIC_H_
#include <stdint.h>
#include "ostric.h"

void generic_msg (struct ostric_client *this, uint8_t *buffer, int len,
			uint8_t **resp, int *rlen);
void generic_token(struct ostric_client *this);
#endif /* _SRICD_OSTRIC_GENERIC_H_ */
