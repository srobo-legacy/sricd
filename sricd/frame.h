#ifndef _INCLUDED_FRAME
#define _INCLUDED_FRAME

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FRAME_PAYLOAD_MAX 64

// one SRIC frame
typedef struct _frame frame;

struct _frame {
	uint8_t source_address;
	uint8_t dest_address;
	uint8_t note;
	uint8_t payload_length;
	uint8_t payload[FRAME_PAYLOAD_MAX];
};

#ifdef __cplusplus
}
#endif

#endif
