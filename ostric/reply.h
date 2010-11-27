#ifndef _SRICD_OSTRIC_REPLY_H_
#define _SRICD_OSTRIC_REPLY_H_
#include <stdint.h>

#include "ostric.h"

int emit_response(uint8_t src, uint8_t dst, const void *data, int len);

#endif /* _SRICD_OSTRIC_REPLY_H_ */
