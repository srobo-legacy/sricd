#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "sric-enum.h"
#include "output-queue.h"
#include "sric-cmds.h"
#include "device.h"

#define ENUM_PRI 0

/* We don't actually want a response from sric-if through the tag, but we also
 * want to indicate that an ack is required. Therefore, feed a non-null tag
 * when we want to elicit an ack, but which will never be used because we're
 * in enumeration mode */
#define BAD_ENUM_TAG	((void*)0xDEADDEAD)

static uint8_t reset_count = 0;
static uint8_t current_next_address = 2;
static uint8_t current_addr_info_addr = 2;
static uint8_t device_classes[DEVICE_HIGH_ADDRESS];
static bool addr_info_replies[DEVICE_HIGH_ADDRESS];

typedef enum {
	S_IDLE,
	S_KILLING_TOKEN,
	S_RESETTING,
	S_ENUMERATING,
	S_SETTING_ADDR,
	S_SETTING_GW_ADDR,
	S_ADVANCING_TOKEN,
	S_FETCHING_CLASSES,
	S_ENUMERATED,
} enum_state_t;

static enum_state_t state = S_IDLE;

typedef enum {
	/* Start enumeration */
	EV_START_ENUM,
	/* Finished flushing the token */
	EV_FLUSH_DONE,
	/* Have captured  token */
	EV_TOKEN_CATCH_TIMEOUT,
	/* The bus has been reset */
	EV_BUS_RESET,
	/* An unenumerated device is on the bus */
	EV_DEV_PRESENT,
	/* Received ack from newest device on bus */
	EV_NEW_DEV_ACK,
	/* Token has reached gateway; bus enumerated */
	EV_TOK_AT_GW,
	/* Received an addr info response */
	EV_ADDR_INFO_ACK
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
	f.tag = BAD_ENUM_TAG;
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
	f.tag = BAD_ENUM_TAG;
	f.payload_length = 1;
	f.payload[0] = 0x80 | SRIC_SYSCMD_TOK_ADVANCE;

	txq_push(&f, ENUM_PRI);
}

static void send_addr_info_req( uint8_t addr )
{
	frame_t f;

	f.type = FRAME_SRIC;
	f.address = addr;
	f.tag = BAD_ENUM_TAG;
	f.payload_length = 1;
	f.payload[0] = 0x80 | SRIC_SYSCMD_ADDR_INFO;

	txq_push(&f, ENUM_PRI);
}

static void gw_cmd( uint8_t cmd, uint8_t len, uint8_t *d )
{
	frame_t f;
	assert( len+1 <= PAYLOAD_MAX );

	f.type = FRAME_GW_CONF;
	f.tag = BAD_ENUM_TAG;
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

static gboolean kill_token_timeout( void* _rs )
{

	sric_enum_fsm( EV_TOKEN_CATCH_TIMEOUT );
	return FALSE;
}

static gboolean send_reset_timeout( void* _rs )
{
	uint8_t *rs = _rs;

	(*rs)++;
	if( *rs > 5 ) {
		sric_enum_fsm( EV_BUS_RESET );
		return FALSE;
	} else {
		send_reset();
		return TRUE;
	}
}

void sric_enum_fsm( enum_event_t ev )
{
	int i;

	switch( state ) {

	case S_IDLE:
		if( ev == EV_START_ENUM ) {
			send_tok_advance( 0 );
			send_tok_advance( 0 );
			gw_cmd_req_token();
			g_timeout_add( 100, kill_token_timeout, NULL );
			state = S_KILLING_TOKEN;
		}
		break;

	case S_KILLING_TOKEN:
		if ( ev == EV_TOKEN_CATCH_TIMEOUT ) {
			gw_cmd_use_token( false );
			g_timeout_add( 50, send_reset_timeout, &reset_count );
			state = S_RESETTING;
		}
		break;

	case S_RESETTING:
		if( ev == EV_BUS_RESET ) {
			gw_cmd_gen_token(); /* Put token on bus */
			gw_cmd_req_token(); /* Hold onto it when it comes back*/
			/* We next want to know whether the token went all the
			 * way around the bus, or if it's blocked by a device */
			gw_cmd_have_token();
			state = S_ENUMERATING;
		}
		break;

	case S_ENUMERATING:

		if ( ev == EV_TOK_AT_GW || ev == EV_DEV_PRESENT) {

			if (current_next_address >= DEVICE_HIGH_ADDRESS) {
				wlog( "Too many devices on sric "
				      "bus, you must be in some kind "
				      "of alternate reality (or an "
				      "error occured\n" );
				exit(1);
			}

			/* Give it an address */
			send_address( current_next_address );

			if ( ev == EV_DEV_PRESENT ) {
				state = S_SETTING_ADDR;
			} else {
				state = S_SETTING_GW_ADDR;
			}
		}

		break;

	case S_SETTING_GW_ADDR:

		if ( ev == EV_NEW_DEV_ACK ) {
			current_next_address++;

			/* Now we want to fetch bus devices classes */
			state = S_FETCHING_CLASSES;

			memset(addr_info_replies, 0, sizeof(addr_info_replies));
			current_addr_info_addr = 2;

			/* Start using token for all transmissions */
			gw_cmd_use_token(true);
			/* Despatch token around bus */
			gw_cmd_gen_token();

			/* Issue first addr info req */
			send_addr_info_req(current_addr_info_addr++);
		}

		break;

	case S_SETTING_ADDR:
		if (ev == EV_NEW_DEV_ACK) {
			/* Device has acked address; advance token */
			send_tok_advance(current_next_address);
			state = S_ADVANCING_TOKEN;
		}
		break;

	case S_ADVANCING_TOKEN:
		if (ev == EV_NEW_DEV_ACK) {
			/* Excellent; new device enumerated and token passed
			 * onwards. Increase address, check where token is. */
			current_next_address++;
			gw_cmd_have_token();
			state = S_ENUMERATING;
		}
		break;

	case S_FETCHING_CLASSES:
		if (ev == EV_ADDR_INFO_ACK) {
			if (current_addr_info_addr == current_next_address) {
				/* We caught all the acks */
				for (i = 2; i < current_next_address; i++) {
					device_add(i, device_classes[i]);
				}

				/* Move into enumerated state */
				state = S_ENUMERATED;
				break;
			}

			/* Otherwise, issue next addr_info req */
			send_addr_info_req(current_addr_info_addr++);
		}
		break;
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

	/* Catch HAVE_TOKEN response in ENUMERATING state */
	if (state == S_ENUMERATING && f->payload_len == 1 &&
				(f->dest_address & 0x80)) {

		if (f->payload[0] == 0)
			sric_enum_fsm(EV_DEV_PRESENT);
		else
			sric_enum_fsm(EV_TOK_AT_GW);

	/* If this is an ack from the newest address on the bus, then it's just
	 * received an ASSIGN_ADDR. */
	} else if( state != S_IDLE ) {
		if ((f->source_address & 0x7F) == current_next_address &&
		    (f->dest_address & 0x80)) {
			sric_enum_fsm(EV_NEW_DEV_ACK);

			/* Otherwise it can only be a response to addr info. */
		} else if ((f->dest_address & 0x80) && state == S_FETCHING_CLASSES &&
			   f->payload_len == 1) {
			addr_info_replies[f->source_address & 0x7F] = true;
			device_classes[f->source_address & 0x7F] = f->payload[0];
			sric_enum_fsm(EV_ADDR_INFO_ACK);
		}
	}

	/* Return false when done */
	if (state == S_ENUMERATED)
		return FALSE;
	else
		return TRUE;
}
