/* Copyright 2010 - Robert Spanton */
#include "sric-if.h"
#include "log.h"
#include <fcntl.h>
#include <glib.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>

/* Serial port file descriptor */
static int fd = -1;
static GIOChannel *if_gio = NULL;

/* Open and configure the serial port */
static void serial_conf( void )
{
	struct termios t;

	if( !isatty(fd) ) {
		wlog( "Device is not a tty.  Fail." );
		exit(1);
	}
	
	if( tcgetattr(fd, &t) < 0 ) {
		wlog( "Failed to retrieve term info" );
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
	t.c_iflag &= ~( IXOFF | IXON );
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
			| ECHOKE | ISIG | IEXTEN | TOSTOP );
	t.c_lflag |= ECHONL;

	if( cfsetspeed( &t, 115200 ) < 0 ) {
		wlog( "Failed to set serial baud rate" );
		exit(1);
	}

	if( tcsetattr( fd, TCSANOW, &t ) < 0 )
	{
		wlog( "Failed to configure terminal" );
		exit(1);
	}
}

static gboolean rx( GIOChannel *src, GIOCondition cond, gpointer data )
{
	uint8_t b;
	/* TODO: Actually read data... */
	read( fd, &b, 1 );

	return TRUE;
}

void sric_if_init(const char* fname)
{
	fd = open( fname, O_RDWR );
	if( fd == -1 ) {
		wlog( "Failed to open serial device \"%s\"", fname );
		exit(1);
	}

	serial_conf();
	if_gio = g_io_channel_unix_new(fd);

	g_io_add_watch( if_gio, G_IO_IN, rx, NULL );
}
