#define _GNU_SOURCE /* ;O */
#include <fcntl.h>
#include <poll.h>
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
GSList *ostric_client_list = NULL;
bool gw_token_mode = false;
GSList *next_token_recp = NULL;

void read_frames();
int process_command(uint8_t *buffer, int len);
void read_arguments(int argc, char **argv);
void try_reading_stuff();
void try_parsing_stuff();

int
main(int argc, char **argv)
{
	struct pollfd fd;
	int timeout;

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

	fd.fd = ostric_pty_fd;
	timeout = 10;
	while (1) {
		fd.events = POLLIN;
		fd.revents = 0;
		if (poll(&fd, 1, timeout) < 0) {
			perror("Couldn't poll");
			exit(1);
		}

		if (fd.revents & (POLLERR | POLLHUP)) {
			fprintf(stderr, "Error/Hangup reading from pty\n");
			exit(1);
		}

		if (fd.revents & POLLIN) {
			read_frames();
			continue;
		}

		/* Otherwise, wait a little and prod the token situation */
		//circulate_token();
	}

	close(ostric_pty_fd);
	return 0;
}

int pty_buf_len;
uint8_t pty_buf[128];

void
read_frames()
{

	try_reading_stuff();
	try_parsing_stuff();
}

void
try_reading_stuff()
{
	int ret;

	ret = read(ostric_pty_fd, &pty_buf[pty_buf_len],
				sizeof(pty_buf)- pty_buf_len);
	if (ret < 0) {
		perror("Couldn't read from pty");
		exit(1);
	}

	pty_buf_len += ret;
	return;
}


void
try_parsing_stuff()
{
	uint8_t tmp_buf[128];
	int src_len;
	uint8_t decode_len;

and_again:
	if (pty_buf_len == 0)
		return;

	if (pty_buf[0] != 0x7E && pty_buf[0] != 0x8E) {
		/* Not a start-of-frame:  shift forwards */
		memcpy(pty_buf, &pty_buf[1], pty_buf_len - 1);
		pty_buf_len--;

		/* And try to process a frame again */
		goto and_again;
	}

	tmp_buf[0] = pty_buf[0];
	src_len = unescape(&pty_buf[1], pty_buf_len - 1, &tmp_buf[1],
			sizeof(tmp_buf) - 1, &decode_len);
	src_len++; /* unescape returns position, not length, of src */

	if (!process_command(tmp_buf, decode_len + 1)){
		/* Shift used data out; if there's more, then start
		 * this function again to read another frame */
		memcpy(pty_buf, &pty_buf[src_len], pty_buf_len - src_len);
		pty_buf_len -= src_len;
		if (pty_buf_len != 0)
			goto and_again;
	}

	return;
}

int
process_command(uint8_t *buffer, int len)
{
	int ret;
	uint16_t crc, sentcrc, cmdlen;

	/* Is there actually enough space for this? */
	cmdlen = (buffer[3] & 0x3F) + 6;
	if (cmdlen > len)
		return 1;

	crc = crc16(&buffer[0], cmdlen - 2);
	sentcrc = buffer[cmdlen-2];
	sentcrc |= buffer[cmdlen-1] << 8;
	if (crc != sentcrc) {
		fprintf(stderr, "Bad CRC in received frame\n");
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

	for (i = 1; i < argc; i++) {
		for (j = 0; client_types[j].name != NULL; j++) {
			if (!strcmp(client_types[j].name, argv[i])) {
				client = malloc(sizeof(*client));
				client->address = -1;
				client->msg_callback =
						client_types[j].msg_callback;
				client->priv = NULL;
				client->has_token = false;

				ostric_client_list =
				g_slist_append(ostric_client_list, client);
				break;
			}
		}
	}

	return;
}
