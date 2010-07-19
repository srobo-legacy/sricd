#ifndef _INCLUDED_DEVICE
#define _INCLUDED_DEVICE

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _device
{
	int address;
	int type;
} device;

void device_init(void);
void device_add(const device* dev);
void device_del(int address);
void device_reset(void);

#ifdef __cplusplus
}
#endif

#endif
