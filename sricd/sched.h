#ifndef _INCLUDED_SCHED
#define _INCLUDED_SCHED

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*sched_callback)(int, void*);

void sched_init(void);
int sched_event(int ms, sched_callback callback, void* ud);
void sched_tick(void);
void sched_cancel(int event);
int sched_next_event(void);

#ifdef __cplusplus
}
#endif

#endif
