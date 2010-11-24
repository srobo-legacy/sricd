#define _GNU_SOURCE /* ;O */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../sricd/escape.h"
#include "../sricd/frame.h"
#include "../sricd/crc16/crc16.h"

#include "cmds.h"

void read_frames(int fd);
int process_command(uint8_t *buffer, int len);

int
main()
{
	int fd;

	fd = getpt();
	if (fd < 0) {
		perror("Couldn't fetch a pseudo-terminal");
		return 1;
	}

	if (unlockpt(fd)) {
		perror("Couldn't unlock pty");
		close(fd);
		return 1;
	}

	printf("Creating pseudo-terminal with name \"%s\", impersonating a "
		"sric bus on that terminal\n", ptsname(fd));

	read_frames(fd);

	close(fd);
	return 0;
}

void
read_frames(int fd)
{
	uint8_t tmp_buf[128], frame_buf[128];;
	int ret, len, src_len;
	uint8_t decode_len;

	while (1) {
		while (tmp_buf[0] != 0x7E && tmp_buf[0] != 0x8E) {
			ret = read(fd, &tmp_buf[0], 1);
			if (ret < 0) {
				perror("Couldn't read from terminal");
				return;
			}
		}

		/* Only reachable once we've read a start-of-frame */
		len = 1;
		while (1) {
			ret = read(fd, &tmp_buf[len], sizeof(tmp_buf)- len);
			if (ret < 0) {
				perror("Couldn't read from terminal");
				return;
			} else {
				len += ret;
				src_len = unescape(tmp_buf, len, frame_buf,
						sizeof(frame_buf),
						&decode_len);
				if (!process_command(frame_buf, decode_len)) {
					memcpy(tmp_buf, &tmp_buf[src_len],
							len - src_len);
					len -= src_len;
				}
			}
		}
	}

	return;
}

int
process_command(uint8_t *buffer, int len)
{
	uint16_t crc, sentcrc, cmdlen;

	/* Is there actually enough space for this? */
	cmdlen = (buffer[3] & 0x3F) + 6;
	if (cmdlen > len)
		return 1;

	crc = crc16(&buffer[1], cmdlen - 3);
	sentcrc = buffer[cmdlen-1];
	sentcrc |= buffer[cmdlen-2] << 8;
	if (crc != sentcrc) {
		fprintf(stderr, "Bad crc in received frame\n");
		/* However, return 0 indicating that we've ingested that data.
		 * After all, we're not going to do anything else with it */
		return 0;
	}

	if (buffer[0] == 0x8E)
		/* This is an actual command to the gateway... */
		return gateway_command(buffer, len);
	else
		/* Pump at all clients */
		return bus_command(buffer, len);
}
