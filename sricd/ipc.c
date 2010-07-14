#include "ipc.h"
#include "input.h"
#include "client.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "log.h"

static const char* sock_path;

static void ipc_new(int, void*);
static void ipc_death(int, void*);
static void ipc_client_incoming(int, void*);
static void ipc_client_death(int, void*);

static void ipc_boot(void)
{
	int s, rv;
	struct sockaddr_un addr;
	socklen_t len;
	s = socket(PF_UNIX, SOCK_STREAM, 0);
	assert(s);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sock_path);
	len = sizeof(addr.sun_len) + sizeof(addr.sun_family) + strlen(sock_path);
	addr.sun_len = len;
	rv = unlink(sock_path);
	assert(rv == 0 || errno == ENOENT);
	rv = bind(s, (struct sockaddr*)&addr, len);
	assert(rv == 0);
	rv = listen(s, 5);
	assert(rv == 0);
	wlog("created IPC socket as fd %d", s);
	input_listen(s, ipc_new, ipc_death, NULL, NULL);
}

static void ipc_new(int sock, void* x)
{
	client* c;
	struct sockaddr_un addr;
	socklen_t len = sizeof(addr);
	int new;
	(void)x;
	new = accept(sock, (struct sockaddr*)&addr, &len);
	wlog("accepted new connection");
	input_listen(sock, ipc_new, ipc_death, NULL, NULL);
	// do stuff with new here
	c = client_create(new);
	input_listen(new, ipc_client_incoming, ipc_client_death, NULL, c);
	wlog("generating some FIFOs");
	client_open_fifos(c);
}

static void ipc_client_incoming(int fd, void* client_object)
{
	client* c = (client*)client_object;
	assert(c);
	// do R/W stuff here
	input_listen(fd, ipc_client_incoming, ipc_client_death, NULL, client_object);
}

static void ipc_client_death(int fd, void* client_object)
{
	client* c = (client*)client_object;
	assert(c);
	close(fd);
	client_destroy(c);
	wlog("IPC client died");
}

static void ipc_death(int sock, void* x)
{
	wlog("IPC socket died, restarting");
	close(sock);
	ipc_boot();
}

void ipc_init(const char* socket_path)
{
	sock_path = socket_path;
	ipc_boot();
}
