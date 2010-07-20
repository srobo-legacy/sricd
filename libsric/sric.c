#include "sric.h"
#include <assert.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/un.h>

static const char* SRICD_PATH = "/tmp/sricd.sock";

static const unsigned char SRICD_TX = 0;
static const unsigned char SRICD_NOTE_FLAGS = 1;
static const unsigned char SRICD_POLL_RX = 2;
static const unsigned char SRICD_POLL_NOTE = 3;
static const unsigned char SRICD_NOTE_CLEAR = 4;
static const unsigned char SRICD_LIST_DEVICES = 5;

static const unsigned char SRIC_E_BADADDR = 1;
static const unsigned char SRIC_E_TIMEOUT = 2;

struct _sric_context {
	int fd;
	sric_error error;
	uint64_t noteflags[SRIC_HIGH_ADDRESS];
	sric_device* devices;
};

sric_context sric_init(void)
{
	struct sockaddr_un address;
	socklen_t len;
	int fd;
	sric_context ctx;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, SRICD_PATH);
	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	len = sizeof(address);
	if (!fd) return 0;
	if (connect(fd, (const struct sockaddr*)&address, len)) {
		close(fd);
		return 0;
	}
	ctx = malloc(sizeof(struct _sric_context));
	assert(ctx);
	memset(ctx, 0, sizeof(*ctx));
	ctx->fd = fd;
	return ctx;
}

void sric_quit(sric_context ctx)
{
	close(ctx->fd);
	free(ctx);
}

sric_error sric_get_error(sric_context ctx)
{
	return ctx ? ctx->error : SRIC_ERROR_SRICD;
}

void sric_clear_error(sric_context ctx)
{
	if (ctx)
		ctx->error = SRIC_ERROR_NONE;
}

static int check_address(sric_context ctx, int address, int allow_broadcast)
{
	if (address < 0) {
		ctx->error = SRIC_ERROR_NOSUCHADDRESS;
		return 0;
	} else if (address == 0 && !allow_broadcast) {
		ctx->error = SRIC_ERROR_BROADCAST;
		return 0;
	} else if (address == 1) {
		ctx->error = SRIC_ERROR_LOOP;
		return 0;
	} else if (address >= SRIC_HIGH_ADDRESS) {
		ctx->error = SRIC_ERROR_NOSUCHADDRESS;
		return 0;
	}
	return 1;
}

static int check_frame(sric_context ctx, const sric_frame* frame)
{
	assert(ctx);
	assert(frame);
	if (!check_address(ctx, frame->address, 1)) {
		return 0;
	} else if (frame->note != -1) {
		ctx->error = SRIC_ERROR_NOSENDNOTE;
		return 0;
	} else if (frame->payload_length < 0) {
		ctx->error = SRIC_ERROR_BADPAYLOAD;
		return 0;
	} else if (frame->payload_length > SRIC_MAX_PAYLOAD_SIZE) {
		ctx->error = SRIC_ERROR_BADPAYLOAD;
		return 0;
	}
	return 1;
}

static int send_command(sric_context ctx, const unsigned char* data, int length)
{
	unsigned char result;
	ssize_t rv;
	assert(ctx);
	assert(data);
	assert(length >= 1);
	rv = write(ctx->fd, data, length);
	if (rv != length) {
		ctx->error = SRIC_ERROR_SRICD;
		return 1;
	}
	rv = read(ctx->fd, &result, 1);
	if (rv != 1) {
		ctx->error = SRIC_ERROR_SRICD;
		return 1;
	} else if (result == SRIC_E_BADADDR) {
		ctx->error = SRIC_ERROR_BADPAYLOAD;
		return 1;
	} else if (result == SRIC_E_TIMEOUT) {
		ctx->error = SRIC_ERROR_TIMEOUT;
		return 1;
	} else if (result != 0) {
		ctx->error = SRIC_ERROR_SRICD;
		return 1;
	}
	return 0;
}

static int read_data(sric_context ctx, void* ptr, int length)
{
	ssize_t rv;
	assert(ptr);
	assert(length);
	assert(ctx);
	rv = read(ctx->fd, ptr, length);
	if (rv != length) {
		ctx->error = SRIC_ERROR_SRICD;
		return 1;
	}
	return 0;
}

int sric_tx(sric_context ctx, const sric_frame* frame)
{
	unsigned char command[SRIC_MAX_PAYLOAD_SIZE + 5];
	assert(frame);
	if (!ctx) return 1;
	sric_clear_error(ctx);
	if (!check_frame(ctx, frame)) return 1;
	// send some stuff
	command[0] = SRICD_TX;
	command[1] = (frame->address & 0xFF);
	command[2] = ((frame->address >> 8) & 0xFF);
	command[3] = (frame->payload_length & 0xFF);
	command[4] = ((frame->payload_length >> 8) & 0xFF);
	memcpy(command + 5, frame->payload, frame->payload_length);
	return send_command(ctx, command, 5 + frame->payload_length);
}

static int sric_poll(sric_context ctx, sric_frame* frame, int timeout, unsigned char type)
{
	uint16_t sdata;
	unsigned char command[5];
	assert(frame);
	if (!ctx) return 1;
	sric_clear_error(ctx);
	command[0] = type;
	command[1] = ((timeout >>  0) & 0xFF);
	command[2] = ((timeout >>  8) & 0xFF);
	command[3] = ((timeout >> 16) & 0xFF);
	command[4] = ((timeout >> 24) & 0xFF);
	if (send_command(ctx, command, sizeof(command)))
		return 1;
	// read the response
	if (read_data(ctx, &sdata, 2))
		return 1;
	frame->address = ntohs(sdata);
	if (read_data(ctx, &sdata, 2))
		return 1;
	frame->note = ntohs(sdata);
	if (read_data(ctx, &sdata, 2))
		return 1;
	frame->payload_length = ntohs(sdata);
	return read_data(ctx, frame->payload, frame->payload_length);
}

int sric_poll_rx(sric_context ctx, sric_frame* frame, int timeout)
{
	return sric_poll(ctx, frame, timeout, SRICD_POLL_RX);
}

int sric_note_set_flags(sric_context ctx, int device, uint64_t flags)
{
	unsigned char command[11];
	if (!ctx) return 1;
	sric_clear_error(ctx);
	if (!check_address(ctx, device, 0)) return 1;
	if (ctx->noteflags[device] != flags) {
		// do stuff
		ctx->noteflags[device] = flags;
		// send some stuff
		command[ 0] = SRICD_NOTE_FLAGS;
		command[ 1] = (device & 0xFF);
		command[ 2] = ((device >> 8) & 0xFF);
		command[ 3] = ((flags >>  0) & 0xFF);
		command[ 4] = ((flags >>  8) & 0xFF);
		command[ 5] = ((flags >> 16) & 0xFF);
		command[ 6] = ((flags >> 24) & 0xFF);
		command[ 7] = ((flags >> 32) & 0xFF);
		command[ 8] = ((flags >> 40) & 0xFF);
		command[ 9] = ((flags >> 48) & 0xFF);
		command[10] = ((flags >> 56) & 0xFF);
		return send_command(ctx, command, sizeof(command));
	} else {
		// trivial: flags have not changed
		return 0;
	}
}

uint64_t sric_note_get_flags(sric_context ctx, int device)
{
	if (!ctx) return 0;
	sric_clear_error(ctx);
	if (!check_address(ctx, device, 0)) return 0;
	return ctx->noteflags[device];
}

int sric_note_unregister_all(sric_context ctx)
{
	if (!ctx) return 1;
	memset(ctx->noteflags, 0, sizeof(ctx->noteflags));
	sric_clear_error(ctx);
	return send_command(ctx, &SRICD_NOTE_CLEAR, 1);
}

int sric_poll_note(sric_context ctx, sric_frame* frame, int timeout)
{
	return sric_poll(ctx, frame, timeout, SRICD_POLL_NOTE);
}

static void sric_refresh_devices(sric_context ctx)
{
	unsigned short device_count = 0, i;
	short data;
	assert(ctx);
	if (ctx->devices) {
		free(ctx->devices);
		ctx->devices = 0;
	}
	if (send_command(ctx, &SRICD_LIST_DEVICES, 1)) {
		return;
	}
	if (read_data(ctx, &device_count, 2)) {
		return;
	}
	device_count = ntohs(device_count);
	ctx->devices = malloc(sizeof(sric_device) * (device_count + 1));
	for (i = 0; i < device_count; ++i) {
		if (read_data(ctx, &data, 2)) {
			free(ctx->devices);
			ctx->devices = 0;
			return;
		}
		data = ntohs(data);
		ctx->devices[i].address = data;
		if (read_data(ctx, &data, 2)) {
			free(ctx->devices);
			ctx->devices = 0;
			return;
		}
		data = ntohs(data);
		ctx->devices[i].type = data;
	}
	// add the sentinel
	ctx->devices[i].address = 0;
	ctx->devices[i].type = -1;
}

const sric_device* sric_enumerate_devices(sric_context ctx, const sric_device* device)
{
	if (!ctx) return NULL;
	if (!device) {
		sric_refresh_devices(ctx);
		// if there are no devices,
		// ctx->devices will be NULL
		// so this will return NULL
		return &ctx->devices[0];
	} else {
		// the array is contiguous, get the next one
		device = device + 1;
		return device->type == -1 ? NULL : device;
	}
}
