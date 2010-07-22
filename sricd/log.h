#ifndef _INCLUDED_LOG
#define _INCLUDED_LOG
#include <stdbool.h>
#include <stdarg.h>
#ifdef __cplusplus
extern "C"
{
#endif
extern bool log_enable;
void wlogv(const char* fmt, va_list va);
void wlog(const char* fmt, ...);
#ifdef __cplusplus
}
#endif
#endif
