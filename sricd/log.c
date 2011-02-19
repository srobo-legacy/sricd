#include "log.h"
#include <stdarg.h>
#include <stdio.h>

bool log_enable = false;
int log_level = 0;

void wlogv(const char* fmt, va_list va)
{
	if (!log_enable) {
		return;
	}
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

