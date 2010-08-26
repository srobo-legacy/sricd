#include "input.h"
#include <poll.h>
#include "pool.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "log.h"
#include <unistd.h>
#include <string.h>

static pool* listener_pool;

typedef struct _input_listener input_listener;

struct _input_listener {
	int             fd;
	input_event     callback;
	input_event     err_callback;
	input_event     out_callback;
	void*           userdata;
	input_listener* next;
};

static int             listener_count = 0;
static input_listener* listener_first = NULL;

static void invoke_and_drop(int fd)
{
	input_listener* prev = NULL;
	input_listener* cur  = listener_first;
	while (cur) {
		if (cur->fd == fd) {
			if (prev) {
				prev->next = cur->next;
			} else {
				listener_first = cur->next;
			}
			--listener_count;
			if (cur->callback) {
				cur->callback(cur->fd, cur->userdata);
			}
			pool_free(listener_pool, cur);
			return;
		} else {
			prev = cur;
			cur  = cur->next;
		}
	}
}

static void out_and_drop(int fd)
{
	input_listener* prev = NULL;
	input_listener* cur  = listener_first;
	while (cur) {
		if (cur->fd == fd) {
			if (prev) {
				prev->next = cur->next;
			} else {
				listener_first = cur->next;
			}
			--listener_count;
			if (cur->out_callback) {
				cur->out_callback(cur->fd, cur->userdata);
			}
			pool_free(listener_pool, cur);
			return;
		} else {
			prev = cur;
			cur  = cur->next;
		}
	}
}

static void error_and_drop(int fd)
{
	input_listener* prev = NULL;
	input_listener* cur  = listener_first;
	while (cur) {
		if (cur->fd == fd) {
			if (prev) {
				prev->next = cur->next;
			} else {
				listener_first = cur->next;
			}
			--listener_count;
			if (cur->err_callback) {
				cur->err_callback(cur->fd, cur->userdata);
			}
			pool_free(listener_pool, cur);
			return;
		} else {
			prev = cur;
			cur  = cur->next;
		}
	}
}

void input_init(void)
{
	listener_pool = pool_create(sizeof (input_listener));
}

void input_listen(int fd, input_event callback,
                  input_event err_callback,
                  input_event out_callback, void* ud)
{
	input_listener* new = pool_alloc(listener_pool);
	assert(fd > 0);
	assert(callback || err_callback || out_callback);
	new->fd           = fd;
	new->callback     = callback;
	new->err_callback = err_callback;
	new->userdata     = ud;
	listener_count++;
	new->next         = listener_first;
	listener_first    = new;
}

void input_update(int timeout)
{
	struct pollfd*  descriptors;
	input_listener* listener = listener_first;
	int             i        = 0, rv;
	wlog("input with timeout %d", timeout);
	if (!listener) {
		wlog("no listeners to poll - calling usleep instead");
		usleep(timeout * 1000);
		return;
	}
	descriptors = alloca(sizeof (struct pollfd) * listener_count);
	while (listener) {
		descriptors[i].fd     = listener->fd;
		descriptors[i].events = POLLIN;
		if (listener->out_callback) {
			descriptors[i].events |= POLLOUT;
		}
		descriptors[i].revents = 0;
		listener               = listener->next;
		++i;
	}
	rv = poll(descriptors, listener_count, timeout);
	if (rv == 0) {
		return;
	}
	if (rv < 0) {
		if (errno == EAGAIN || errno == EINTR) {
			wlog("input poll was interrupted");
		} else {
			wlog("another input error - %s", strerror(errno));
		}
		return;
	}
	for (i = 0; i < listener_count; ++i) {
		if (descriptors[i].revents & POLLIN) {
			invoke_and_drop(descriptors[i].fd);
		} else if (descriptors[i].revents & POLLOUT) {
			out_and_drop(descriptors[i].fd);
		} else if (descriptors[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
			error_and_drop(descriptors[i].fd);
		}
	}
}

