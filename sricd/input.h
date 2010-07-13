#ifndef _INCLUDED_INPUT
#define _INCLUDED_INPUT

typedef void (*input_event)(int, void*);
typedef void (*input_error_event)(int, void*);

void input_init(void);
void input_listen(int fd, input_event callback, input_error_event err_callback, void* ud);
void input_update(void);

#endif
