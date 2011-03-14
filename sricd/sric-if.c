/* Copyright 2010 - Robert Spanton */
#include "crc16/crc16.h"
#include "client.h"
#include "escape.h"
#include "device.h"
#include "frame.h"
#include "ipc.h"
#include "sric-if.h"
#include "sric-enum.h"
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

/* The number of bytes in a SRIC header, including framing byte */
#define SRIC_HEADER_SIZE 4

/* The number of bytes in the header and footer of a SRIC frame */
#define SRIC_OVERHEAD (SRIC_HEADER_SIZE + 2)

#define is_framing_byte(x) (((x) == 0x7E) || ((x) == 0x8E))
#define frame_for_me(buf) (((buf)[SRIC_DEST] & ~0x80) == 1)
#define frame_is_ack(buf) ((buf)[SRIC_DEST] & 0x80)
#define frame_set_ack(buf) do {(buf)[SRIC_DEST] |= 0x80; } \
    while (0)

/* Number of milliseconds after which to retransmit a packet */
#define RETXMIT_TIMEOUT		50

/* Serial port file descriptor */
static int fd = -1;
static GIOChannel* if_gio = NULL;

/* The write glib source ID (0 = invalid/not-registered) */
static guint write_srcid = 0;

/* Frame we're currently working on transmitting */
static frame_t* tx_frame = NULL;
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

/* Whether we've finished enumerating */
static bool enumerated = false;

/* ID for timeout that will cause sric frame retransmission */
static gint retxmit_timeout_id = 0;

/* Boolean indicating current frame has been cancelled; It should not be
 * retransmitted, and the ack shouldn't be handled either */
static bool frame_cancelled = false;

/* Open and configure the serial port */
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

/* Shift the bytes in rxbuf left by n bytes.
   (Losing n bytes from the beginning of the buffer) */
static void rxbuf_shift(uint8_t n)
{
	g_assert(n <= rxpos);

	memmove(rxbuf, rxbuf + n, rxpos - n);
	rxpos -= n;
}

/* Find the start of a frame in rxbuf, and shift it to be aligned with the
   beginning of the buffer -- throwing away all junk that came before it.

   Returns TRUE if it found the start of a frame. */
static gboolean rxbuf_shift_frame_start(void)
{
	uint8_t i;
	for (i = 0; i < rxpos; i++) {
		if (is_framing_byte(rxbuf[i])) {
			break;
		}
	}

	if (i == rxpos) {
		/* No frame start in input buffer - throw it all away */
		rxpos = 0;
		return FALSE;
	}

	rxbuf_shift(i);
	return TRUE;
}

/* Read available stuff from serial port into rxbuf */
static void read_incoming(void)
{
	int r;
	int size = sizeof (rxbuf) - rxpos;

	r = read(fd, rxbuf + rxpos, size);

	if (r == -1) {
		wlog("Error reading from serial port: %s", strerror(errno));
		exit(1);
	} else if (r == 0 && size != 0) {
		wlog("EOF on serial port.  Aborting.");
		exit(1);
	}
	rxpos += r;
}

/* Assemble as much of a frame as possible from rxbuf into unesc_rx.
   Assumes that if there was a valid frame in unesc_rx, it has already been processed.
   Returns TRUE if this results in something work considering in unesc_rx. */
static gboolean advance_rx(void)
{
	uint8_t used;

	if (is_framing_byte(rxbuf[0]) || unesc_pos == sizeof (unesc_rx)) {
		/* Move onto the next frame immediately */
		unesc_pos = 0;
	}

	if (unesc_pos == 0) {
		/* Not even a smidgen of frame in unesc_rx yet */
		if (!rxbuf_shift_frame_start()) {
			/* No frame to process */
			return FALSE;
		}

		/* Copy framing byte across */
		unesc_rx[0] = rxbuf[0];
		unesc_pos   = 1;

		/* Lose the escape byte */
		rxbuf_shift(1);
	}

	/* Unescape bytes from rxbuf into unesc_rx */
	rxbuf_shift(unescape(rxbuf, rxpos,
	                     unesc_rx + unesc_pos, sizeof (unesc_rx) - unesc_pos,
	                     &used));
	unesc_pos += used;

	return used ? TRUE : FALSE;
}

/* Process the valid frame found in unesc_rx */
static void proc_rx_frame(void)
{
	packed_frame_t *f = (packed_frame_t*) &unesc_rx[1];

	{
		char *foo;
		int i, pos;

		foo = alloca((SRIC_OVERHEAD + unesc_rx[SRIC_LEN] * 3) + 16);

		strcpy(foo, "r:");
		pos = 2;
		for (i = 0; i < unesc_rx[SRIC_LEN] + SRIC_OVERHEAD; i++) {
			sprintf(&foo[pos], " %2.2X", unesc_rx[i]);
			pos += strlen(&foo[pos]);
		}

		wlog_debug(foo);
	}

	if (!frame_for_me(unesc_rx)) {
		/* Ignore frames not for us */
		return;
	}

	if (!frame_is_ack(unesc_rx)) {
		frame note;
		frame_t resp;

		/* Queue up an ACK for it */
		resp.type = FRAME_SRIC;
		resp.address = unesc_rx[SRIC_SRC] | 0x80;
		resp.tag  = NULL;
		resp.payload_length = 0;
		txq_push(&resp, 0);

		/* Need at least one byte of payload to be a note */
		if (unesc_rx[SRIC_LEN] == 0)
			return;

		note.source_address = unesc_rx[SRIC_SRC] & 0x7F;
		note.dest_address = unesc_rx[SRIC_DEST] & 0x7F;
		note.note = unesc_rx[SRIC_DATA];
		note.payload_length = unesc_rx[SRIC_LEN];
		memcpy(&note.payload, &unesc_rx[SRIC_DATA], unesc_rx[SRIC_LEN]);
		device_dispatch_note(&note);
		return;
	}

	if (tx_frame == NULL) {
		wlog("Error: Suddenly, an unsolicited (or extra) ack from "
			"device %X\n", f->source_address);
		return;
	}

	/* We reach this point if we've received an ack, for sricd. If so,
	 * we needn't timeout and retransmit that frame again */
	g_source_remove(retxmit_timeout_id);
	retxmit_timeout_id = 0;

	/* In case there are other frames blocked, check if we can transmit */
	sric_if_tx_ready();

	/* If frame was cancelled, stop further procesing */
	if (frame_cancelled) {
		free(tx_frame);
		tx_frame = NULL;
		return;
	}

	if( !enumerated ) {
		if( !sric_enum_rx(f) ) {
			enumerated = true;

			/* Now start listening for commands */
			ipc_listen();
		}
	} else {
		frame rxed_frame;

		/* Format packed frame into frame_t */
		rxed_frame.dest_address = f->dest_address;
		rxed_frame.source_address = f->source_address;
		rxed_frame.note = 0; /* XXX */
		rxed_frame.payload_length = f->payload_len;
		memcpy(&rxed_frame.payload, &f->payload, f->payload_len);

		rxed_frame.source_address &= 0x7F;
		rxed_frame.dest_address &= 0x7F;

		/* Hand frame to client */
		client_push_rx(tx_frame->tag, &rxed_frame);
	}

	/* Dispose of transmitted (and now retired) frame */
	free(tx_frame);
	tx_frame = NULL;
}

/* Data ready on serial port callback */
static gboolean rx(GIOChannel* src, GIOCondition cond, gpointer data)
{
	read_incoming();

	while (advance_rx()) {
		uint8_t  len;
		uint16_t crc, recv_crc;

		if (unesc_pos < SRIC_OVERHEAD) {
			continue;
		}

		len = unesc_rx[SRIC_LEN];
		if (unesc_pos < (SRIC_OVERHEAD + len)) {
			continue;
		}

		crc = crc16(unesc_rx, SRIC_HEADER_SIZE + len);
		recv_crc = unesc_rx[SRIC_DATA + len]
		           | (unesc_rx[SRIC_DATA + len + 1] << 8);
		if (crc != recv_crc) {
			continue;
		}

		proc_rx_frame();

		unesc_pos = 0;
	}

	return TRUE;
}

void sric_if_init(const char* fname)
{
	fd = open(fname, O_RDWR | O_NONBLOCK);
	if (fd == -1) {
		wlog("Failed to open serial device \"%s\"", fname);
		exit(1);
	}

	serial_conf();
	if_gio = g_io_channel_unix_new(fd);

	g_io_add_watch(if_gio, G_IO_IN, rx, NULL);

	sric_enum_start();
}

/* Set up the next frame for transmission
   Returns true if there's a frame to transmit */
static gboolean next_tx(void)
{
	uint16_t crc;
	uint8_t  len;
	int16_t  esclen;

	tx_frame = txq_next( enumerated?1:0 );

	if (tx_frame == NULL) {
		/* Queue's empty */
		return FALSE;
	}

	/*** Build up the frame in the output buffer ***/
	frame_cancelled = false;

	/* Alias payload length for convenience */
	len = tx_frame->payload_length;

	txbuf[0] = tx_frame->type;
	txbuf[SRIC_DEST] = tx_frame->address;
	txbuf[SRIC_SRC] = 1;
	txbuf[SRIC_LEN] = len;
	memcpy(txbuf + SRIC_DATA, tx_frame->payload, len);

	crc                        = crc16(txbuf, SRIC_HEADER_SIZE + len);
	txbuf[SRIC_DATA + len]     = crc;
	txbuf[SRIC_DATA + len + 1] = crc >> 8;

	/* Don't escape the initial framing byte */
	esclen = escape_frame(txbuf + 1,
	                      SRIC_OVERHEAD - 1 + len,
	                      sizeof (txbuf) - 1);
	g_assert(esclen > 0);

	/* Remember the framing byte: */
	txlen = esclen + 1;
	txpos = 0;

	return TRUE;
}

static gboolean tx_timeout(void *dummy __attribute__((unused)))
{

	/* If this is reached, we haven't received an ack for the most recently
	 * sent frame, so it requires retransmission. */

	/* Unless it was cancelled */
	if (frame_cancelled) {
		free(tx_frame);
		tx_frame = NULL;
	}

	txpos = 0;
	sric_if_tx_ready();
	return FALSE;
}

static gboolean if_tx(GIOChannel* src, GIOCondition cond, gpointer data)
{
	int w;

	if (tx_frame != NULL && txpos == txlen)
		/* We've transmitted, not yet had an ack; so, we've arrived here
		 * because another frame got queued. Don't send it yet. */
		goto empty;

	if (tx_frame == NULL && !next_tx()) {
		/* Nothing to transmit at this time */
		goto empty;
	}

	{
		char *foo;
		int i, pos;

		foo = alloca(((txlen - txpos) * 3) + 16);
		strcpy(foo, "w:");
		pos = 2;
		for (i = txpos; i < txlen; i++) {
			sprintf(&foo[pos], " %2.2X", txbuf[i]);
			pos += strlen(&foo[pos]);
		}

		wlog_debug(foo);
	}

	w = write(fd, txbuf + txpos, txlen - txpos);
	if (w == -1) {
		wlog("Error writing to serial port: %s", strerror(errno));
		exit(1);
	}

	txpos += w;
	if (txpos != txlen)
		/* There's more to be transmitted */
		return TRUE;

	/* We've sent one frame. However, if it's an ack, or the sender
	 * indicates they don't want a response, don't wait for an ack to come
	 * back and just continue transmitting */
	if (tx_frame->tag == NULL || tx_frame->address & 0x80) {
		tx_frame = NULL;
		return TRUE;
	}

	retxmit_timeout_id = g_timeout_add(RETXMIT_TIMEOUT, tx_timeout, NULL);

empty:
	write_srcid = 0;
	return FALSE;
}

void sric_if_tx_ready(void)
{
	if (write_srcid == 0) {
		write_srcid = g_io_add_watch(if_gio, G_IO_OUT, if_tx, NULL);
	}
}

void sric_if_cancel(void *tag)
{

	if (tx_frame == NULL || tx_frame->tag != tag)
		return;

	frame_cancelled = true;
	return;
}
