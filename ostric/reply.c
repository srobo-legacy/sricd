#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ostric.h"

#include "../sricd/escape.h"
#include "../sricd/crc16/crc16.h"

int
send_msg(uint8_t src, uint8_t dst, const void *data, int len)
{
	uint8_t resp[256];
	int escaped_len, ret;
	uint16_t crc;

	resp[0] = 0x7E;
	resp[1] = dst;
	resp[2] = src & 0x7F;
	resp[3] = len;
	memcpy(&resp[4], data, len);
	crc = crc16(&resp[0], len + 4);
	resp[4+len] = crc & 0xFF;
	resp[5+len] = crc >> 8;

	/* Escape everything *but* the SOF byte */
	escaped_len = escape_frame(&resp[1], 5 + len, sizeof(resp) - 1);

	/* Write everything including SOF */
	ret = write(ostric_pty_fd, resp, escaped_len + 1);
	if (ret < 0) {
		perror("Writing a response failed");
		return 1;
	} else if (ret != escaped_len) {
		fprintf(stderr, "Short write when emitting a response\n");
		return 1;
	}

	return 0;
}
