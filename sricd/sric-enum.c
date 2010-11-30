#include "sric-enum.h"
#include "output-queue.h"
#include "sric-cmds.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define ENUM_PRI 0

static uint8_t reset_count = 0;
static bool enum_token_at_gw = false;	/* Has token circumnavigated bus yet? */

typedef enum {
	S_IDLE,
	S_RESETTING,
	S_ENUMERATING,
	S_ENUMERATED,
} enum_state_t;

typedef enum {
	/* Start enumeration */
	EV_START_ENUM,
	/* The bus has been reset */
	EV_BUS_RESET,
} enum_event_t;

/* State machine for enumeration */
void sric_enum_fsm( enum_event_t ev );

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

static void send_addr_info_req()
{
	frame_t f;

	f.type = FRAME_SRIC;
	f.address = 0;
	f.tag = NULL;
	f.payload_length = 1;
	f.payload[0] = 0x80 | SRIC_SYSCMD_ADDR_INFO;

	txq_push(&f, ENUM_PRI);
}

static void gw_cmd( uint8_t cmd, uint8_t len, uint8_t *d )
{
	frame_t f;
	assert( len+1 <= PAYLOAD_MAX );

	f.type = FRAME_GW_CONF;
	f.address = 1;
	f.payload_length = len + 1;
	f.payload[0] = cmd;
	if( d != NULL )
		memcpy( f.payload+1, d, len );

	txq_push(&f, ENUM_PRI);
}

static void gw_cmd_use_token( bool use )
{
	uint8_t b = use;
	gw_cmd( GW_CMD_USE_TOKEN, 1, &b );
}

static void gw_cmd_req_token( void )
{
	gw_cmd( GW_CMD_REQ_TOKEN, 0, NULL );
}

static void gw_cmd_have_token( void )
{
	gw_cmd( GW_CMD_HAVE_TOKEN, 0, NULL );
}

static void gw_cmd_gen_token( void )
{
	gw_cmd( GW_CMD_GEN_TOKEN, 0, NULL );
}

static gboolean send_reset_timeout( void* _rs )
{
	uint8_t *rs = _rs;

	send_reset();

	(*rs)++;
	if( *rs > 5 ) {
		sric_enum_fsm( EV_BUS_RESET );
		return FALSE;
	}
	return TRUE;
}

static gboolean check_new_device(void *_done)
{
	bool *done;

	done = _done;

	/* Has the token gone around the bus yet? */
	if (*done)
		return FALSE;

	/* If not, query gateway about it; if it's not there yet, this begins
	 * another portion of the enum FSM */
	gw_cmd_have_token();
	return TRUE;
}

void sric_enum_fsm( enum_event_t ev )
{
	static enum_state_t state = S_IDLE;

	switch( state ) {

	case S_IDLE:
		if( ev == EV_START_ENUM ) {
			gw_cmd_use_token( false );
			g_timeout_add( 50, send_reset_timeout, &reset_count );
			state = S_RESETTING;
		}
		break;

	case S_RESETTING:
		if( ev == EV_BUS_RESET ) {
			gw_cmd_gen_token(); /* Put token on bus */
			gw_cmd_req_token(); /* Hold onto it when it comes back*/
			g_timeout_add(50, check_new_device, &enum_token_at_gw);
			state = S_ENUMERATING;
		}
		break;

	case S_ENUMERATING:
	case S_ENUMERATED:
		break;
	}
}

void sric_enum_start( void )
{
	sric_enum_fsm( EV_START_ENUM );
}

bool sric_enum_rx( packed_frame_t *f )
{
	/* Return false when done */

	/* Push items onto the output queue as we go */
	return FALSE;
}
