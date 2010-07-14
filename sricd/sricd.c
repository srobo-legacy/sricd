#include "init.h"
#include "input.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

static void print_help_and_exit(const char* pname)
{
	printf("Usage: %s [-h] [-V] [-f] [-d]\n", pname);
	printf("\t-f\tkeep in foreground\n");
	printf("\t-d\trun as daemon (default)\n");
	printf("\t-h\tprint this help\n");
	printf("\t-V\tprint version\n");
	exit(0);
}

static void print_version_and_exit()
{
	printf("sricd version 0.1\n");
	exit(0);
}

int main(int argc, char** argv)
{
	int arg;
	int fg = 0;
	while ((arg = getopt(argc, argv, "hVfd")) != -1) {
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
		}
	}
	init();
	if (!fg)
		daemon(0, 0);
	while (1)
		input_update();
	return 0;
}
