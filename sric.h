#ifndef _INCLUDED_LIBSRIC
#define _INCLUDED_LIBSRIC

#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SRIC_MAX_PAYLOAD_SIZE 64
#define SRIC_HIGH_ADDRESS 256

typedef enum _sric_error {
	SRIC_ERROR_NONE,           // no error
	SRIC_ERROR_NOSUCHADDRESS,  // no such remote address
	SRIC_ERROR_NOSENDNOTE,     // cannot send notifications
	SRIC_ERROR_BADPAYLOAD,     // payload length is > SRIC_MAX_PAYLOAD_SIZE
	SRIC_ERROR_SRICD,          // could not connect to sricd
	SRIC_ERROR_LOOP,           // cannot send to self
	SRIC_ERROR_TIMEOUT,        // request timed out
	SRIC_ERROR_BROADCAST       // cannot listen for notifications on broadcast addr
} sric_error;

typedef struct _sric_context* sric_context;

typedef struct _sric_frame {
	int address;
	int note; // -1 for non-note frames
	int payload_length;
	unsigned char payload[SRIC_MAX_PAYLOAD_SIZE];
} sric_frame;

typedef struct _sric_device {
	int address;
	int type;
} sric_device;

sric_context sric_init(void);
void sric_quit(sric_context ctx);

sric_error sric_get_error(sric_context ctx);
void sric_clear_error(sric_context ctx);

bool sric_tx(sric_context ctx, const sric_frame* frame);
bool sric_poll_rx(sric_context ctx, sric_frame* frame, int timeout);
inline static bool sric_txrx(sric_context ctx, const sric_frame* outframe, sric_frame* inframe, int timeout) {
	return sric_tx(ctx, outframe) && sric_poll_rx(ctx, inframe, timeout);
}

bool sric_note_set_flags(sric_context ctx, int device, uint64_t flags);
uint64_t sric_note_get_flags(sric_context ctx, int device);

inline static bool sric_note_register(sric_context ctx, int device, int note)
	{ return sric_note_set_flags(ctx, device, sric_note_get_flags(ctx, device) | ((uint64_t)1 << note)); }
inline static bool sric_note_unregister(sric_context ctx, int device, int note)
	{ return sric_note_set_flags(ctx, device, sric_note_get_flags(ctx, device) & ~((uint64_t)1 << note)); }
inline static bool sric_note_unregister_device(sric_context ctx, int device)
	{ return sric_note_set_flags(ctx, device, 0); }
bool sric_note_unregister_all(sric_context ctx);

bool sric_poll_note(sric_context ctx, sric_frame* frame, int timeout);
inline static bool sric_note(sric_context ctx, sric_frame* frame)
	{ return sric_poll_note(ctx, frame, -1); }

// pass NULL as the first device
const sric_device* sric_enumerate_devices(sric_context ctx, const sric_device* device);

#ifdef __cplusplus
}
#endif

#endif
