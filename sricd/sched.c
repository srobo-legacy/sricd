#include "sched.h"
#include "pool.h"
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>

static pool* sched_pool;
static int current_id = 1;

typedef struct _sched_evt sched_evt;

static sched_evt* head = 0;

struct _sched_evt
{
	sched_evt* next;
	sched_callback callback;
	void* ud;
	int nr;
	int time;
};

static int get_time(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

void sched_init(void)
{
	sched_pool = pool_create(sizeof(sched_evt));
}

int sched_event(int ms, sched_callback callback, void* ud)
{
	int t;
	int nr;
	sched_evt* new;
	sched_evt* evt = head;
	sched_evt* last = NULL;
	assert(ms >= 0);
	assert(callback);
	nr = current_id++;
	if (ms == 0) {
		callback(nr, ud);
	} else {
		t = get_time() + ms;
		new = pool_alloc(sched_pool);
		new->next = NULL;
		new->callback = callback;
		new->ud = ud;
		new->nr = nr;
		new->time = t;
		// insert in the queue
		while (evt) {
			if (evt->time > t) {
				new->next = evt;
				if (last) {
					last->next = new;
				} else {
					head = new;
				}
				break;
			} else {
				last = evt;
				evt = evt->next;
			}
		}
		if (!evt) {
			new->next = NULL;
			if (last) {
				last->next = new;
			} else {
				head = new;
			}
		}
	}
	return nr;
}

void sched_tick(void)
{
	int t = get_time();
	sched_evt* evt;
	while ((evt = head) && evt->time <= t) {
		evt->callback(evt->nr, evt->ud);
		head = evt->next;
		pool_free(sched_pool, head);
	}
}

void sched_cancel(int event)
{
	sched_evt* last = NULL;
	sched_evt* cur = head;
	assert(event);
	while (cur) {
		if (cur->nr == event) {
			if (last) {
				last->next = cur->next;
			} else {
				head = cur->next;
			}
			pool_free(sched_pool, cur);
			return;
		}
		last = cur;
		cur = cur->next;
	}
}

int sched_next_event(void)
{
	if (head) {
		return head->time - get_time();
	} else {
		return -1;
	}
}
