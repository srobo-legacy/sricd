#ifndef _INCLUDED_IPC
#define _INCLUDED_IPC

#ifdef __cplusplus
extern "C"
{
#endif

void ipc_init(const char* socket_path);

void ipc_listen( void );

#ifdef __cplusplus
}
#endif

#endif
