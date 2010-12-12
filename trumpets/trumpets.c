/* Copyright 2010 - Robert Spanton */
#define _GNU_SOURCE /* ;O */
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct trumpet_channel;
struct trumpet_channel {
	int fd;
	uint8_t buffer[256];
	int rxpos;
	int tx_watch_id;
	GIOChannel *gio;
	struct trumpet_channel *target_channel;
};

struct trumpet_channel tty_channel;
struct trumpet_channel pty_channel;

int rand_modulus = 0;

gboolean rx_data(GIOChannel *src, GIOCondition cond, gpointer data);
gboolean tx_data(GIOChannel *src, GIOCondition cond, gpointer data);

static void serial_conf(void)
{
	struct termios t;

	if (!isatty(tty_channel.fd)) {
		fprintf(stderr, "Device is not a tty.  Fail.");
		exit(1);
	}

	if (tcgetattr(tty_channel.fd, &t) < 0) {
		fprintf(stderr, "Failed to retrieve term info");
		exit(1);
	}

	/* Parity off */
	t.c_iflag &= ~INPCK;
	t.c_cflag &= ~PARENB;

	/* Ignore bytes with parity or framing errors */
	t.c_iflag |= IGNPAR;

	/* 8 bits per byte, ignore breaks, don't touch carriage returns */
	t.c_iflag &= ~(ISTRIP | IGNBRK | IGNCR | ICRNL);

	/* No flow control please */
	t.c_iflag &= ~(IXOFF | IXON);
	t.c_cflag &= ~CRTSCTS;

	/* Disable character mangling */
	t.c_oflag &= ~OPOST;

	/* No modem disconnect excitement */
	t.c_cflag &= ~(HUPCL | CSIZE);

	/* Use input from the terminal */
	/* Don't use the carrier detect lines  */
	t.c_cflag |= CREAD | CS8 | CLOCAL;

	/* One stop bit */
	t.c_cflag &= ~CSTOPB;

	/* non-canonical, no looping, no erase printing or use,
	   no special character munging */
	t.c_lflag &= ~(ICANON | ECHO | ECHO | ECHOPRT | ECHOK
	               | ECHOKE | ISIG | IEXTEN | TOSTOP);
	t.c_lflag |= ECHONL;

	if (cfsetspeed(&t, 115200) < 0) {
		fprintf(stderr, "Failed to set serial baud rate");
		exit(1);
	}

	if (tcsetattr(tty_channel.fd, TCSANOW, &t) < 0) {
		fprintf(stderr, "Failed to configure terminal");
		exit(1);
	}
}

gboolean
rx_data(GIOChannel *src, GIOCondition cond, gpointer data)
{
	struct trumpet_channel *c, *target;
	int ret, i, bit;

	c = data;

	ret = read(c->fd, &c->buffer[c->rxpos], sizeof(c->buffer) - c->rxpos);
	if (ret < 0) {
		/* It's semi-acceptable to have no data available */
		if (errno == EAGAIN)
			return TRUE;

		perror("Read error");
		exit(1);
	} else if (ret == 0 && c->rxpos != sizeof(c->buffer)) {
		fprintf(stderr, "Read an EOF, shutting down\n");
		exit(1);
	}

	for (i = 0; i < ret; i++) {
		if ((rand() % rand_modulus) == 0) {
			/* Insert a one bit error */
			bit = rand() % 8;
			c->buffer[c->rxpos + i] ^= (1 << bit);
		}
	}

	c->rxpos += ret;

	/* Generate a write event for /other/ channel */
	target = c->target_channel;
	if (target->tx_watch_id == 0)
		target->tx_watch_id = g_io_add_watch(target->gio, G_IO_OUT,
							tx_data, target);

	return TRUE;
}

gboolean
tx_data(GIOChannel *src, GIOCondition cond, gpointer data)
{
	struct trumpet_channel *c, *source;
	int ret;

	c = data;
	source = c->target_channel;

	ret = write(c->fd, &source->buffer[0], source->rxpos);
	if (ret < 0) {
		perror("Write error");
		exit(1);
	}

	/* Shift data over what we just wrote out */
	memcpy(&source->buffer[0], &source->buffer[ret], source->rxpos - ret);
	source->rxpos -= ret;

	/* If we're done, */
	if (source->rxpos == 0) {
		c->tx_watch_id = 0;
		return FALSE;
	}

	return TRUE;
}

int
main(int argc, char **argv)
{
	GMainLoop *ml;

	if (argc != 3) {
		fprintf(stderr, "Usage: trumpets /dev/sric_tty_dev "
						"rand_modulus\n");
		return 1;
	}

	rand_modulus = strtol(argv[2], NULL, 10);
	if (rand_modulus == 0) {
		fprintf(stderr, "Invalid rand modulus\n");
		return 1;
	}

	/* Open tty, attempt to configure */
	tty_channel.fd = open(argv[1], O_RDWR | O_NONBLOCK);
	if (tty_channel.fd < 1) {
		perror("Couldn't open sric tty device");
		return 1;
	}

	serial_conf();

	tty_channel.gio = g_io_channel_unix_new(tty_channel.fd);
	g_io_add_watch(tty_channel.gio, G_IO_IN, rx_data, &tty_channel);

	tty_channel.rxpos = 0;
	tty_channel.tx_watch_id = 0;

	pty_channel.fd = getpt();
	if (pty_channel.fd < 0) {
		perror("Couldn't generate a pty");
		return 1;
	}

	if (unlockpt(pty_channel.fd)) {
		perror("Couldn't unlock pty");
		return 1;
	}

	pty_channel.gio = g_io_channel_unix_new(pty_channel.fd);
	g_io_add_watch(pty_channel.gio, G_IO_IN, rx_data, &pty_channel);

	pty_channel.rxpos = 0;
	pty_channel.tx_watch_id = 0;

	tty_channel.target_channel = &pty_channel;
	pty_channel.target_channel = &tty_channel;

	printf("Pumping tty data to pty \"%s\"\n", ptsname(pty_channel.fd));

	ml = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ml);

	return 0;
}
