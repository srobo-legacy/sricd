/**
 * @file sric.h
 * @brief libsric interface.
 */

#ifndef _INCLUDED_LIBSRIC
#define _INCLUDED_LIBSRIC

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Maximum frame payload size.
 */
#define SRIC_MAX_PAYLOAD_SIZE 64
/**
 * The high address under SRIC - all addresses are < SRIC_HIGH_ADDRESS
 */
#define SRIC_HIGH_ADDRESS 128

/**
 * libsric error codes.
 */
typedef enum _sric_error {
	SRIC_ERROR_NONE,           /**< no error                                  */
	SRIC_ERROR_NOSUCHADDRESS,  /**< no such remote address                    */
	SRIC_ERROR_NOSENDNOTE,     /**< cannot send notifications                 */
	SRIC_ERROR_BADPAYLOAD,     /**< payload length is > SRIC_MAX_PAYLOAD_SIZE */
	SRIC_ERROR_SRICD,          /**< could not connect to sricd                */
	SRIC_ERROR_LOOP,           /**< cannot send to self                       */
	SRIC_ERROR_TIMEOUT,        /**< request timed out                         */
	SRIC_ERROR_BROADCAST       /**< cannot listen on broadcast addr           */
} sric_error;

/**
 * A libsric context.
 */
typedef struct _sric_context* sric_context;

/**
 * A single frame.
 */
typedef struct _sric_frame {
	int address;                        /**< Remote address.               */
	int note;                           /**< Note ID, or -1 for non-notes. */
	int payload_length;                 /**< Length of the payload field.  */
	unsigned char payload[SRIC_MAX_PAYLOAD_SIZE]; /**< Frame payload. */
} sric_frame;

/**
 * Remote device.
 */
typedef struct _sric_device {
	int address; /**< Device address. */
	int type;    /**< Device type. */
} sric_device;

/**
 * Create a libsric context.
 *
 * \ref SRIC_ERROR_SRICD will be set if no connection could be made.
 *
 * @return The new libsric context.
 */
sric_context sric_init(void);

/**
 * Tear down a libsric context.
 *
 * @param ctx The context to tear down.
 */
void sric_quit(sric_context ctx);

/**
 * Get the last error on a libsric context.
 *
 * @param ctx The context to check.
 * @return The last error, or SRIC_ERROR_NONE if there was no error.
 */
sric_error sric_get_error(sric_context ctx);

/**
 * Clears the error in a libsric context.
 *
 * \ref SRIC_ERROR_SRICD will be set if the connection was invalid.
 *
 * @param ctx The context to check.
 */
void sric_clear_error(sric_context ctx);

/**
 * Send a TX frame.
 *
 * \ref SRIC_ERROR_SRICD will be set if the connection was invalid.
 * \ref SRIC_ERROR_NOSENDNOTE will be set if an attempt to send a note was made.
 * \ref SRIC_ERROR_BADPAYLOAD will be set if the payload size was negative.
 * \ref SRIC_ERROR_BADPAYLOAD will be set if the payload was too large.
 * \ref SRIC_ERROR_NOSUCHADDRESS will be set if the target was nonexistant.
 * \ref SRIC_ERROR_NOSUCHADDRESS will be set if the target address was invalid.
 * \ref SRIC_ERROR_LOOP will be set if the target was the source.
 *
 * @param ctx The libsric context.
 * @param frame A pointer to the frame to send.
 * @return Whether this was successful.
 */
int sric_tx(sric_context ctx, const sric_frame* frame);

/**
 * Poll for an RX frame.
 *
 * \ref SRIC_ERROR_SRICD will be set if the connection was invalid.
 * \ref SRIC_ERROR_TIMEOUT will be set if the polling timed out.
 *
 * @param ctx The libsric context.
 * @param frame A pointer to the frame in which to store the result.
 * @param timeout The timeout length, in ms. Pass -1 for indefinite.
 * @return Whether a frame was found.
 */
int sric_poll_rx(sric_context ctx, sric_frame* frame, int timeout);

/**
 * Send a TX frame, and wait for the corresponding RX.
 *
 * @note DO NOT use this in conjunction with \ref sric_tx and \ref sric_rx
 *  - either use one API or the other for a given context.
 *
 * \ref SRIC_ERROR_SRICD will be set if the connection was invalid.
 * \ref SRIC_ERROR_NOSENDNOTE will be set if an attempt to send a note was made.
 * \ref SRIC_ERROR_BADPAYLOAD will be set if the payload size was negative.
 * \ref SRIC_ERROR_BADPAYLOAD will be set if the payload was too large.
 * \ref SRIC_ERROR_NOSUCHADDRESS will be set if the target was nonexistant.
 * \ref SRIC_ERROR_NOSUCHADDRESS will be set if the target address was invalid.
 * \ref SRIC_ERROR_LOOP will be set if the target was the source.
 *
 * @param ctx The libsric context.
 * @param outframe The TX frame to send.
 * @param inframe A pointer to space to store the RX frame.
 * @return Whether the TXRX was successful.
 */
inline static int sric_txrx(sric_context ctx, const sric_frame* outframe,
                            sric_frame* inframe)
{
	if (sric_tx(ctx, outframe)) {
		return 1;
	}
	return sric_poll_rx(ctx, inframe, -1);
}

/**
 * Set the notification flags for a given device.
 *
 * \ref SRIC_ERROR_SRICD will be set if the connection was invalid.
 * \ref SRIC_ERROR_NOSUCHADDRESS will be set if the target was nonexistant.
 * \ref SRIC_ERROR_NOSUCHADDRESS will be set if the target address was invalid.
 * \ref SRIC_ERROR_LOOP will be set if the target was the source.
 * \ref SRIC_ERROR_BROADCAST will be set if the broadcast address was passed.
 *
 * @param ctx The libsric context.
 * @param device The device address.
 * @param flags The note flags.
 * @return Whether the call was successful.
 */
int sric_note_set_flags(sric_context ctx, int device, uint64_t flags);

/**
 * Return the notification flags for a given device.
 *
 * @note This call can never fail.
 *
 * @param ctx The libsric context.
 * @param device The device address.
 * @return The note flags on that device.
 */
uint64_t sric_note_get_flags(sric_context ctx, int device);

/**
 * Register for a given notification on a device.
 *
 * This is implemented using \ref sric_note_set_flags, and can return any of
 * the same errors.
 *
 * @param ctx The libsric context.
 * @param device The device address.
 * @param note The notification number.
 * @return Whether the call was successful.
 */
inline static int sric_note_register(sric_context ctx, int device, int note)
{
	return sric_note_set_flags(ctx, device, sric_note_get_flags(ctx, device)
	                           | ((uint64_t)1 << note));
}

/**
 * Unregister for a given notification on a device.
 *
 * This is implemented using \ref sric_note_set_flags, and can return any of
 * the same errors.
 *
 * @param ctx The libsric context.
 * @param device The device address.
 * @param note The notification number.
 * @return Whether the call was successful.
 */
inline static int sric_note_unregister(sric_context ctx, int device, int note)
{
	return sric_note_set_flags(ctx, device, sric_note_get_flags(ctx, device)
	                           & ~((uint64_t)1 << note));
}

/**
 * Unregister for all notifications on a device.
 *
 * This is implemented using \ref sric_note_set_flags, and can return any of
 * the same errors.
 *
 * @param ctx The libsric context.
 * @param device The device address.
 * @return Whether the call was successful.
 */
inline static int sric_note_unregister_device(sric_context ctx, int device)
{
	return sric_note_set_flags(ctx, device, 0);
}

/**
 * Unregister for all notifications for all devices.
 *
 * \ref SRIC_ERROR_SRICD will be set if the connection was invalid.
 *
 * @param ctx The libsric context.
 * @return Whether the call was successful.
 */
int sric_note_unregister_all(sric_context ctx);

/**
 * Poll for a notification frame.
 *
 * \ref SRIC_ERROR_SRICD will be set if the connection was invalid.
 * \ref SRIC_ERROR_TIMEOUT will be set if the polling timed out.
 *
 * @param ctx The libsric context.
 * @param frame A pointer to the frame in which to store the result.
 * @param timeout The timeout length, in ms. Pass -1 for indefinite.
 * @return Whether a frame was found.
 */
int sric_poll_note(sric_context ctx, sric_frame* frame, int timeout);

/**
 * Block indefinitely for a note.
 *
 * This is implemented using \ref sric_poll_note and may return any of the same
 * errors.
 *
 * @param ctx The libsric context.
 * @param frame A pointer to the frame in which to store the result.
 * @return Whether a frame was found.
 */
inline static int sric_note(sric_context ctx, sric_frame* frame)
{
	return sric_poll_note(ctx, frame, -1);
}

/**
 * Enumerate over known devices.
 *
 * \ref SRIC_ERROR_SRICD will be set if the connection was invalid.
 *
 * Pass the previously found device, or NULL to start enumerating.
 *
 * @param ctx The libsric context.
 * @param device The previously enumerated device, or NULL.
 * @return The next device, or NULL if all devices have been enumerated.
 */
const sric_device* sric_enumerate_devices(sric_context       ctx,
                                          const sric_device* device);

#ifdef __cplusplus
}
#endif

#endif
