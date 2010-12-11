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

struct trumpet_channel {
	int fd;
	uint8_t buffer[256];
	int rxpos;
	int txpos;
	int tx_watch_id;
	GIOChannel *gio;
};

struct trumpet_channel tty_rxbuf;
struct trumpet_channel pts_rxbuf;

/* Serial port file descriptor */
static int fd = -1;
/* Fake pty for sricd to attach to */
static int pty_fd = -1;
static GIOChannel* if_gio = NULL;
static GIOChannel* pty_gio = NULL;

/* The write glib source ID (0 = invalid/not-registered) */
static guint write_srcid = 0;

static void serial_conf(void)
{
	struct termios t;

	if (!isatty(fd)) {
		fprintf(stderr, "Device is not a tty.  Fail.");
		exit(1);
	}

	if (tcgetattr(fd, &t) < 0) {
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

	if (tcsetattr(fd, TCSANOW, &t) < 0) {
		fprintf(stderr, "Failed to configure terminal");
		exit(1);
	}
}

int
main(int argc, char **argv)
{
	GMainLoop *ml;

	if (argc != 2) {
		fprintf(stderr, "Usage: trumpets /dev/sric_tty_dev\n");
		return 1;
	}

	/* Open tty, attempt to configure */
	fd = open(argv[1], O_RDWR | O_NONBLOCK);
	if (fd < 1) {
		perror("Couldn't open sric tty device");
		return 1;
	}

	serial_conf();

	if_gio = g_io_channel_unix_new(fd);
	g_io_add_watch(if_gio, G_IO_IN, rx_data, &tty_rxbuf);

	tty_rxbuf.fd = tty_fd;
	tty_rxbuf.rxpos = 0;
	tty_rxbuf.txpos = 0;
	tty_rxbuf.tx_watch_id = 0;
	tty_rxbuf.gio = tty_gio;

	pty_fd = getpt();
	if (pty_fd < 0) {
		perror("Couldn't generate a pty");
		return 1;
	}

	if (unlockpt(pty_fd)) {
		perror("Couldn't unlock pty");
		return 1;
	}

	pty_gio = g_io_channel_unix_new(pty_fd);
	g_io_add_watch(pty_gio, G_IO_IN, rx_data, &pts_rxbuf);

	pty_rxbuf.fd = pty_fd;
	pty_rxbuf.rxpos = 0;
	pty_rxbuf.txpos = 0;
	pty_rxbuf.tx_watch_id = 0;
	pty_rxbuf.gio = pty_gio;

	ml = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ml);

	return 0;
}
