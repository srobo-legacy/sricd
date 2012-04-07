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
#include <stdbool.h>

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
	printf("\t-i\tignore the first SIGTERM received\n");
	printf("\t-p\tpath to PID file\n");
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
static const char* pid_path = NULL;
static bool ignore_sigterm = false;

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

static void handle_sigterm( int signum )
{
	if( ignore_sigterm ) {
		wlog( "SIGTERM received -- ignoring this time only." );
		ignore_sigterm = false;
		return;
	}

	exit(0);
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
	while ((arg = getopt(argc, argv, "hvVfdip:s:u:")) != -1) {
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

		case 'p':
			pid_path = optarg;
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

		case 'i':
			ignore_sigterm = true;
			break;
		}
	}
	signal(SIGUSR1, restart);
	signal(SIGTERM, handle_sigterm);
	init(socket_path, sdev_path);
	if (!fg) {
		wlog("backgrounding");
		background();
	}

	if( pid_path != NULL ) {
		int pid = getpid();
		FILE *pid_file = fopen( pid_path, "w" );
		if( pid_file == NULL ) {
			wlog( "Couldn't open PID file." );
			return 1;
		}
		fprintf( pid_file, "%i\n", pid );
		fclose( pid_file );
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

