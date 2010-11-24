#define _GNU_SOURCE /* ;O */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../sricd/escape.h"
#include "../sricd/frame.h"

#include "cmds.h"

void read_frames(int fd);
int process_command(uint8_t sof, uint8_t *buffer, int len);

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
	uint8_t sof, decode_len;

	while (1) {
		sof = 0;
		while (sof != 0x7E && sof != 0x8E) {
			ret = read(fd, &sof, 1);
			if (ret < 0) {
				perror("Couldn't read from terminal");
				return;
			}
		}

		/* Only reachable once we've read a start-of-frame */
		len = 0;
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
				if (!process_command(sof, frame_buf, decode_len)) {
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
process_command(uint8_t sof, uint8_t *buffer, int len)
{

	if (sof == 0x8E)
		/* This is an actual command to the gateway... */
		return gateway_command(buffer, len);
	else
		/* Pump at all clients */
		return bus_command(buffer, len);
}
