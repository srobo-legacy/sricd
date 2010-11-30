#ifndef _SRICD_OSTRIC_REPLY_H_
#define _SRICD_OSTRIC_REPLY_H_
#include <stdint.h>

#include "ostric.h"

int send_msg(uint8_t src, uint8_t dst, const void *data, int len);
#define emit_ack(src, dst) send_msg((src), (dst) | 0x80, NULL, 0)
#define emit_data_ack(src, dst, data, len) \
		send_msg((src), (dst) | 0x80, (data), (len))

#endif /* _SRICD_OSTRIC_REPLY_H_ */
