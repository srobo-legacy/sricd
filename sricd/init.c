#include "init.h"
#include "output-queue.h"
#include "ipc.h"
#include "device.h"
#include "sched.h"
#include "log.h"
#include "sric-if.h"
#include <assert.h>

void init(const char* socket_path, const char* sdev_path)
{
	assert(socket_path);
	wlog("starting up sched");
	sched_init();
	wlog("starting up tx queue");
	txq_init();
	wlog("starting up device list");
	device_init();
	wlog("starting up IPC socket %s", socket_path);
	ipc_init(socket_path);
	wlog("starting up SRIC interface");
	sric_if_init(sdev_path);
	wlog("startup complete");
}

