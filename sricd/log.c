#include "log.h"
#include <stdarg.h>
#include <stdio.h>

bool log_enable = false;

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

