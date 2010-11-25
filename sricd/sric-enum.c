#include "sric-enum.h"
#include "output-queue.h"
#include "sric-cmds.h"

#define ENUM_PRI 0

static uint8_t reset_count = 0;

/* Append a reset command */
static void send_reset( void )
{
	frame_t f;

	f.type = FRAME_SRIC;
	/* Broadcast */
	f.address = 0;
	f.tag = NULL;
	f.payload_length = 1;
	f.payload[0] = 0x80 | SRIC_SYSCMD_RESET;

	txq_push(&f, ENUM_PRI);
}

/* Send ADDRESS_ASSIGN command on the SRIC bus,
   assigning address addr to the device that currently has
   the token */
static void send_address( uint8_t addr )
{
	frame_t f;

	f.type = FRAME_SRIC;
	/* Broadcast */
	f.address = 0;
	f.tag = NULL;
	f.payload_length = 2;
	f.payload[0] = 0x80 | SRIC_SYSCMD_ADDR_ASSIGN;
	f.payload[1] = addr;

	txq_push(&f, ENUM_PRI);
}

/* Send ADVANCE_TOKEN command to address addr */
static void send_tok_advance( uint8_t addr )
{
	frame_t f;

	f.type = FRAME_SRIC;
	f.address = addr;
	f.tag = NULL;
	f.payload_length = 1;
	f.payload[0] = 0x80 | SRIC_SYSCMD_TOK_ADVANCE;

	txq_push(&f, ENUM_PRI);
}

static gboolean send_reset_timeout( void* _rs )
{
	uint8_t *rs = _rs;

	send_reset();

	(*rs)++;
	if( *rs > 5 )
		return FALSE;
	return TRUE;
}

void sric_enum_start( void )
{
	g_timeout_add( 50, send_reset_timeout, &reset_count );
}

bool sric_enum_rx( void )
{
	/* Return false when done */

	/* Push items onto the output queue as we go */
	return FALSE;
}
