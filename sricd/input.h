#ifndef _INCLUDED_INPUT
#define _INCLUDED_INPUT
#ifdef __cplusplus
extern "C"
{
#endif
typedef void (*input_event)(int, void*);
void input_init(void);
void input_listen(int fd, input_event callback,
                  input_event err_callback,
                  input_event out_callback, void* ud);
void input_update(int timeout);
#ifdef __cplusplus
}
#endif
#endif
