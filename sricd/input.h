#ifndef _INCLUDED_INPUT
#define _INCLUDED_INPUT

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*input_event)(int, void*);
typedef void (*input_error_event)(int, void*);

void input_init(void);
void input_listen(int fd, input_event callback, input_error_event err_callback, void* ud);
void input_update(void);

#ifdef __cplusplus
}
#endif

#endif
