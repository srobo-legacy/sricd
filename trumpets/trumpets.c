/* Copyright 2010 - Robert Spanton */
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* Serial port file descriptor */
static int fd = -1;
static GIOChannel* if_gio = NULL;

/* The write glib source ID (0 = invalid/not-registered) */
static guint write_srcid = 0;

static void serial_conf(void)
{
	struct termios t;

	if (!isatty(fd)) {
		wlog("Device is not a tty.  Fail.");
		exit(1);
	}

	if (tcgetattr(fd, &t) < 0) {
		wlog("Failed to retrieve term info");
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
		wlog("Failed to set serial baud rate");
		exit(1);
	}

	if (tcsetattr(fd, TCSANOW, &t) < 0) {
		wlog("Failed to configure terminal");
		exit(1);
	}
}
