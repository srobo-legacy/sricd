#include "input.h"
#include <poll.h>
#include "pool.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

static pool* listener_pool;

typedef struct _input_listener input_listener;

struct _input_listener
{
	int fd;
	input_event callback;
	input_error_event err_callback;
	void* userdata;
	input_listener* next;
};

static int listener_count = 0;
static input_listener* listener_first = NULL;

static void invoke_and_drop(int fd)
{
	input_listener* prev = NULL;
	input_listener* cur = listener_first;
	while (cur) {
		if (cur->fd == fd) {
			if (prev) {
				prev->next = cur->next;
			} else {
				listener_first = cur->next;
			}
			--listener_count;
			cur->callback(cur->fd, cur->userdata);
			pool_free(listener_pool, cur);
			return;
		} else {
			prev = cur;
			cur = cur->next;
		}
	}
}

static void error_and_drop(int fd)
{
	input_listener* prev = NULL;
	input_listener* cur = listener_first;
	while (cur) {
		if (cur->fd == fd) {
			if (prev) {
				prev->next = cur->next;
			} else {
				listener_first = cur->next;
			}
			--listener_count;
			cur->err_callback(cur->fd, cur->userdata);
			pool_free(listener_pool, cur);
			return;
		} else {
			prev = cur;
			cur = cur->next;
		}
	}
}

void input_init(void)
{
	listener_pool = pool_create(sizeof(input_listener));
}

void input_listen(int fd, input_event callback, input_error_event err_callback, void* ud)
{
	input_listener* new = pool_alloc(listener_pool);
	assert(fd > 0);
	assert(callback || err_callback);
	new->fd = fd;
	new->callback = callback;
	new->err_callback = err_callback;
	new->userdata = ud;
	listener_count++;
	new->next = listener_first;
	listener_first = new;
}

void input_update(void)
{
	struct pollfd* descriptors;
	input_listener* listener = listener_first;
	int i = 0, rv;
	if (!listener)
		return;
	descriptors = alloca(sizeof(struct pollfd) * listener_count);
	while (listener) {
		descriptors[i].fd = listener->fd;
		descriptors[i].events = POLLIN;
		descriptors[i].revents = 0;
		listener = listener->next;
		++i;
	}
	rv = poll(descriptors, listener_count, 1000);
	if (rv == 0) return;
	if (rv < 0) {
		if (errno == EAGAIN || errno == EINTR) {
			// interruption, try again
			// oh the wonders of the humble tail call
			input_update();
			return;
		}
	}
	for (i = 0; i < rv; ++i) {
		if (descriptors[i].revents & POLLIN) {
			invoke_and_drop(descriptors[i].fd);
		} else if (descriptors[i].revents & (POLLHUP | POLLERR)) {
			error_and_drop(descriptors[i].fd);
		}
	}
}
