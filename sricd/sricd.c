#include "init.h"
#include "input.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include "log.h"

const char* DEFAULT_SOCKET_PATH = "/tmp/sricd.sock";

static void print_help_and_exit(const char* pname)
{
	printf("Usage: %s [-h] [-v] [-V] [-f] [-d] [-s socket]\n", pname);
	printf("\t-V\tprint version\n");
	printf("\t-d\trun as daemon (default)\n");
	printf("\t-f\tkeep in foreground\n");
	printf("\t-h\tprint this help\n");
	printf("\t-s\tspecify the location of the socket\n");
	printf("\t-v\tenable verbose mode\n");
	exit(0);
}

static void print_version_and_exit()
{
	printf("sricd version 0.1\n");
	exit(0);
}

int main(int argc, char** argv)
{
	int arg, rv;
	int fg = 0;
	unsigned long long time1, time2, startup_time;
	struct timeval tv1, tv2;
	const char* socket_path = DEFAULT_SOCKET_PATH;
	rv = gettimeofday(&tv1, 0);
	assert(rv == 0);
	while ((arg = getopt(argc, argv, "hvVfds:")) != -1) {
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
			break;
		case 's':
			socket_path = optarg;
			break;
		}
	}
	init(socket_path);
	if (!fg) {
		wlog("calling daemon(0, 0)");
		daemon(0, 0);
	}
	rv = gettimeofday(&tv2, 0);
	assert(rv == 0);
	time1 = tv1.tv_sec*1000000 + tv1.tv_usec;
	time2 = tv2.tv_sec*1000000 + tv2.tv_usec;
	startup_time = time2 - time1;
	wlog("startup took %lluµs", startup_time);
	wlog("entering main loop");
	while (1)
		input_update();
	return 0;
}
