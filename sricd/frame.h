#ifndef _INCLUDED_FRAME
#define _INCLUDED_FRAME

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FRAME_PAYLOAD_MAX 64

/* Offsets of fields in frames */
enum {
	SRIC_DEST = 1,
	SRIC_SRC,
	SRIC_LEN,
	SRIC_DATA
	/* CRC is last two bytes */
};

// one SRIC frame
typedef struct _frame frame;

struct _frame {
	uint8_t source_address;
	uint8_t dest_address;
	uint8_t note;
	uint8_t payload_length;
	uint8_t payload[FRAME_PAYLOAD_MAX];
};

// A packed SRIC frame
typedef struct {
	uint8_t dest_address;
	uint8_t source_address;
	uint8_t payload_len;
	uint8_t payload[FRAME_PAYLOAD_MAX];
} __attribute__ ((__packed__)) packed_frame_t;

#ifdef __cplusplus
}
#endif

#endif
