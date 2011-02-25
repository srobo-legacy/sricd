#include <assert.h>
#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

bool log_enable = false;
int log_level = 0;
static uint64_t start_time;

/* Return time since epoch in µs */
static uint64_t get_time64( void )
{
	struct timeval t;
	assert( gettimeofday( &t, NULL ) == 0 );

	return (((uint64_t)t.tv_sec) * 1000000) + (uint64_t)t.tv_usec;
}

void wlog_init( void )
{
	start_time = get_time64();
}

void wlogv(const char* fmt, va_list va)
{
	uint64_t t;

	if (!log_enable) {
		return;
	}

	t = get_time64() - start_time;
	fprintf( stderr, "%Lu.%4.4Lu: ",
		 t/1000000,
		 (t % 1000000)/100 );

	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
}

void wlog(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	wlogv(fmt, va);
	va_end(va);
}

void wlog_debug(const char* fmt, ...)
{
	va_list va;

	if (log_level < 2)
		return;

	va_start(va, fmt);
	wlogv(fmt, va);
	va_end(va);
}

