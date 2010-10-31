/* Copyright 2010 - Robert Spanton */
#include "crc16/crc16.h"
#include "client.h"
#include "escape.h"
#include "sric-if.h"
#include "log.h"
#include "output-queue.h"
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* Offsets of fields in the tx buffer */
enum {
        SRIC_DEST = 1,
        SRIC_SRC = 2,
        SRIC_LEN = 3,
        SRIC_DATA = 4
        /* CRC is last two bytes */
};

/* The number of bytes in a SRIC header, including framing byte */
#define SRIC_HEADER_SIZE 4

/* The number of bytes in the header and footer of a SRIC frame */
#define SRIC_OVERHEAD (SRIC_HEADER_SIZE + 2)

#define is_framing_byte(x) ( ((x)==0x7E) || ((x)==0x8E) )

/* Serial port file descriptor */
static int fd = -1;
static GIOChannel *if_gio = NULL;

/* The write glib source ID (0 = invalid/not-registered) */
static guint write_srcid = 0;

/* Frame we're currently working on transmitting */
static const frame_t *tx_frame = NULL;
/* Output buffer that we're currently transmitting -- allow enough space for escaping */
static uint8_t txbuf[ (SRIC_OVERHEAD + PAYLOAD_MAX) * 2 ];
/* Number of bytes currently in txbuf */
static uint8_t txlen = 0;
/* Offset of next byte in txbuf to go out  */
static uint8_t txpos = 0;

/* Receive buffer for data straight from the serial port */
static uint8_t rxbuf[ (SRIC_OVERHEAD + PAYLOAD_MAX) * 2 ];
static uint8_t rxpos = 0;

/* Receive buffer for a single unescaped frame */
static uint8_t unesc_rx[ SRIC_OVERHEAD + PAYLOAD_MAX ];
static uint8_t unesc_pos = 0;

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

/* Shift the bytes in rxbuf left by n bytes.
   (Losing n bytes from the beginning of the buffer) */
static void rxbuf_shift( uint8_t n )
{
	g_assert( n <= rxpos );

	memmove( rxbuf, rxbuf + n, rxpos - n );
	rxpos -= n;
}

/* Find the start of a frame in rxbuf, and shift it to be aligned with the
   beginning of the buffer -- throwing away all junk that came before it.

   Returns TRUE if it found the start of a frame. */
static gboolean rxbuf_shift_frame_start( void )
{
	uint8_t i;
	for( i=0; i<rxpos; i++ )
		if( is_framing_byte( rxbuf[i] ) )
			break;

	if( i == rxpos ) {
		/* No frame start in input buffer - throw it all away */
		rxpos = 0;
		return FALSE;
	}

	rxbuf_shift(i);
	return TRUE;
}

/* Read available stuff from serial port into rxbuf */
static void read_incoming( void )
{
	int r;
	int size = sizeof(rxbuf) - rxpos;

	r = read( fd, rxbuf + rxpos, size );

	if( r == -1 ) {
		wlog( "Error reading from serial port: %s", strerror(errno) );
		exit(1);
	} else if( r == 0 && size != 0 ) {
		wlog( "EOF on serial port.  Aborting." );
		exit(1);
	}
	rxpos += r;
}

/* Assemble as much of a frame as possible from rxbuf into unesc_rx.
   Assumes that if there was a valid frame in unesc_rx, it has already been processed.
   Returns TRUE if this results in something work considering in unesc_rx. */
static gboolean advance_rx( void )
{
	uint8_t used;

	if( is_framing_byte(rxbuf[0]) || unesc_pos == sizeof(unesc_rx) )
		/* Move onto the next frame immediately */
		unesc_pos = 0;

	if( unesc_pos == 0 ) {
		/* Not even a smidgen of frame in unesc_rx yet */
		if( !rxbuf_shift_frame_start() )
			/* No frame to process */
			return FALSE;

		/* Copy framing byte across */
		unesc_rx[0] = rxbuf[0];
		unesc_pos = 1;

		/* Lose the escape byte */
		rxbuf_shift(1);
	}

	/* Unescape bytes from rxbuf into unesc_rx */
	rxbuf_shift( unescape( rxbuf, rxpos,
			       unesc_rx + unesc_pos, sizeof(unesc_rx) - unesc_pos,
			       &used ) );
	unesc_pos += used;

	return used ? TRUE : FALSE;
}

/* Data ready on serial port callback */
static gboolean rx( GIOChannel *src, GIOCondition cond, gpointer data )
{
	read_incoming();

	while( advance_rx() ) {
		uint8_t i;
		uint8_t len;

		if( unesc_pos < SRIC_OVERHEAD )
			continue;

		len = unesc_rx[SRIC_LEN];
		if( unesc_pos < (SRIC_OVERHEAD + len) )
			continue;

		printf( "Frame:" );
		for( i=0; i<unesc_pos; i++ )
			printf( "%2.2x ", unesc_rx[i] );
		printf( "\n" );
	}

	return TRUE;
}

void sric_if_init(const char* fname)
{
	fd = open( fname, O_RDWR | O_NONBLOCK );
	if( fd == -1 ) {
		wlog( "Failed to open serial device \"%s\"", fname );
		exit(1);
	}

	serial_conf();
	if_gio = g_io_channel_unix_new(fd);

	g_io_add_watch( if_gio, G_IO_IN, rx, NULL );
}

/* Set up the next frame for transmission
   Returns true if there's a frame to transmit */
static gboolean next_tx( void )
{
	uint16_t crc;
	uint8_t len;
	int16_t esclen;
	tx_frame = txq_next(1);

	if( tx_frame == NULL )
		/* Queue's empty */
		return FALSE;

	/*** Build up the frame in the output buffer ***/

	/* Alias payload length for convenience */
	len = tx_frame->payload_length;

	txbuf[0] = tx_frame->type;
	txbuf[SRIC_DEST] = tx_frame->address;
	txbuf[SRIC_SRC] = 1;
	txbuf[SRIC_LEN] = len;
	memcpy( txbuf + SRIC_DATA, tx_frame->payload, len );

	crc = crc16( txbuf, SRIC_HEADER_SIZE + len );
	txbuf[SRIC_DATA + len] = crc;
	txbuf[SRIC_DATA + len + 1] = crc >> 8;

	/* Don't escape the initial framing byte */
	esclen = escape_frame( txbuf + 1, SRIC_OVERHEAD - 1 + len, sizeof(txbuf) - 1);
	g_assert( esclen > 0 );

	/* Remember the framing byte: */
	txlen = esclen + 1;
	txpos = 0;

	return TRUE;
}

static gboolean if_tx( GIOChannel *src, GIOCondition cond, gpointer data )
{
	int w;

	if( tx_frame == NULL && !next_tx() )
		/* Nothing to transmit at this time */
		goto empty;

	w = write( fd, txbuf + txpos, txlen - txpos );
	if( w == -1 ) {
		wlog( "Error writing to serial port: %s", strerror(errno) );
		exit( 1 );
	}

	txpos += w;
	if( txpos == txlen && !next_tx() )
		/* Queue has been emptied */
		goto empty;

	return TRUE;

empty:
	write_srcid = 0;
	return FALSE;
}

void sric_if_tx_ready( void )
{
	if( write_srcid == 0 )
		write_srcid = g_io_add_watch( if_gio, G_IO_OUT, if_tx, NULL );
}
