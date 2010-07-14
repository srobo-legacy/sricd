#include "init.h"
#include "input.h"
#include "output-queue.h"
#include "ipc.h"
#include "sched.h"
#include "log.h"
#include <assert.h>

void init(const char* socket_path)
{
	assert(socket_path);
	wlog("starting up sched");
	sched_init();
	wlog("starting up tx queue");
	txq_init();
	wlog("starting up input listener");
	input_init();
	wlog("starting up IPC socket %s", socket_path);
	ipc_init(socket_path);
	wlog("startup complete");
}
