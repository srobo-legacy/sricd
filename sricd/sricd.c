#include "init.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include "log.h"
#include "sched.h"
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

const char* DEFAULT_SOCKET_PATH = "/tmp/sricd.sock";
const char* DEFAULT_SERIAL_DEV  = "/dev/sric-a";

static void print_help_and_exit(const char* pname)
{
	printf("Usage: %s [-h] [-v] [-V] [-f] [-d] [-s socket] [-u device]\n",
	       pname);
	printf("\t-V\tprint version\n");
	printf("\t-d\trun as daemon (default)\n");
	printf("\t-f\tkeep in foreground\n");
	printf("\t-h\tprint this help\n");
	printf("\t-s\tspecify the location of the socket\n");
	printf("\t-v\tincrease verbosity (provide twice for even more)\n");
	printf("\t-u\tspecify the serial port device path\n");
	exit(0);
}

static void print_version_and_exit()
{
	printf("sricd version 0.1\n");
	exit(0);
}

static void background()
{
	daemon(1, 0);
}

static const char* restart_file;

static const char* socket_path;
static const char* sdev_path;

void restart()
{
	wlog("restarting sricd at path: %s", restart_file);
	execl(restart_file,
	      restart_file,
	      "-f",
	      "-s",
	      socket_path,
	      log_enable ? "-v" : NULL,
	      NULL);
	wlog("restart failed: %s", strerror(errno));
	exit(1);
}

int main(int argc, char** argv)
{
	int arg, rv;
	int fg = 0;
	unsigned long long time1, time2, startup_time;
	struct timeval tv1, tv2;
	GMainLoop* ml = NULL;

	socket_path = DEFAULT_SOCKET_PATH;
	sdev_path = DEFAULT_SERIAL_DEV;
	restart_file = argv[0];
	rv = gettimeofday(&tv1, 0);
	assert(rv == 0);
	while ((arg = getopt(argc, argv, "hvVfds:u:")) != -1) {
		switch (arg) {
		case 'h':
		case '?':
			print_help_and_exit(argv[0]);
			break;

		case 'V':
			print_version_and_exit();
			break;

		case 'f':
			fg = 1;
			break;

		case 'd':
			fg = 0;
			break;

		case 'v':
			log_enable = true;
			log_level++;
			break;

		case 's':
			socket_path = optarg;
			break;

		case 'u':
			sdev_path = optarg;
			break;
		}
	}
	signal(SIGUSR1, restart);
	init(socket_path, sdev_path);
	if (!fg) {
		wlog("backgrounding");
		background();
	}
	rv = gettimeofday(&tv2, 0);
	assert(rv == 0);
	time1 = tv1.tv_sec * 1000000 + tv1.tv_usec;
	time2 = tv2.tv_sec * 1000000 + tv2.tv_usec;
	startup_time = time2 - time1;
	wlog("startup took %lluµs", startup_time);
	wlog("entering main loop");

	ml = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ml);

	return 0;
}

