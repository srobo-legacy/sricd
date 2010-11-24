#define _GNU_SOURCE /* ;O */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../sricd/escape.h"
#include "../sricd/frame.h"

int
main()
{
	int fd;

	fd = getpt();
	if (fd < 0) {
		perror("Couldn't fetch a pseudo-terminal");
		return 1;
	}

	printf("Creating pseudo-terminal with name \"%s\", impersonating a"
		"sric bus on that terminal\n", ptsname(fd));

	while (1) {
		/* The usual - receive frames, process, operate upon */
		; /* XXX - todo */
	}

	close(fd);
	return 0;
}
