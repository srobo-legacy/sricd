#define _GNU_SOURCE /* ;O */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#include "../sricd/escape.h"
#include "../sricd/frame.h"
#include "../sricd/crc16/crc16.h"

#include "ostric.h"
#include "cmds.h"
#include "generic.h"

int ostric_pty_fd;
GSList *ostric_client_list;

void read_frames();
int process_command(uint8_t *buffer, int len);
void read_arguments(int argc, char **argv);

int
main(int argc, char **argv)
{

	read_arguments(argc, argv);

	ostric_pty_fd = getpt();
	if (ostric_pty_fd < 0) {
		perror("Couldn't fetch a pseudo-terminal");
		return 1;
	}

	if (unlockpt(ostric_pty_fd)) {
		perror("Couldn't unlock pty");
		close(ostric_pty_fd);
		return 1;
	}

	printf("Creating pseudo-terminal with name \"%s\", impersonating a "
		"sric bus on that terminal\n", ptsname(ostric_pty_fd));

	read_frames();

	close(ostric_pty_fd);
	return 0;
}

void
read_frames()
{
	uint8_t tmp_buf[128], frame_buf[128];;
	int ret, len, src_len;
	uint8_t decode_len;

	while (1) {
		while (tmp_buf[0] != 0x7E && tmp_buf[0] != 0x8E) {
			ret = read(ostric_pty_fd, &tmp_buf[0], 1);
			if (ret < 0) {
				perror("Couldn't read from terminal");
				return;
			}
		}

		/* Only reachable once we've read a start-of-frame */
		len = 1;
		while (1) {
			ret = read(ostric_pty_fd, &tmp_buf[len],
						sizeof(tmp_buf)- len);
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
		gateway_command(buffer, cmdlen);
	else
		/* Pump at all clients */
		bus_command(buffer, cmdlen);

	/* Always return 0; we've always done /something/ with this data */
	return 0;
}

static struct {
	const char *name;
	void (*msg_callback) (struct ostric_client *this, uint8_t *buffer,
				int len, uint8_t **resp, int *rlen);
} client_types[] = {
{	"generic",		generic_msg, 		},
{	NULL,			NULL,			}
};

void
read_arguments(int argc, char **argv)
{
	struct ostric_client *client;
	int i, j;

	ostric_client_list = g_slist_alloc();

	for (i = 1; i < argc; i++) {
		for (j = 0; client_types[j].name != NULL; j++) {
			if (!strcmp(client_types[j].name, argv[i])) {
				client = malloc(sizeof(*client));
				client->address = -1;
				client->msg_callback =
						client_types[j].msg_callback;
				client->priv = NULL;

				ostric_client_list =
				g_slist_append(ostric_client_list, client);
				break;
			}
		}
	}

	return;
}
