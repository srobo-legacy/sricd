#define _GNU_SOURCE /* ;O */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../sricd/escape.h"
#include "../sricd/frame.h"

int read_frame(uint8_t *buffer);
void process_command(uint8_t *buffer, int len);

int
main()
{
	uint8_t frame_buffer[128];
	int fd, len;

	fd = getpt();
	if (fd < 0) {
		perror("Couldn't fetch a pseudo-terminal");
		return 1;
	}

	printf("Creating pseudo-terminal with name \"%s\", impersonating a"
		"sric bus on that terminal\n", ptsname(fd));

	while (1) {
		/* The usual - receive frames, process, operate upon */
		/* Start off by reading a start of frame, then a header with a
		 * length, then do some things */
		len = read_frame(frame_buffer);
		process_command(frame_buffer, len);
	}

	close(fd);
	return 0;
}

int
read_frame(uint8_t *buffer)
{

	return 0;
}

void
process_command(uint8_t *buffer, int len)
{

	return;
}
