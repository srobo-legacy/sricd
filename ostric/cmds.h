#include <stdint.h>

#define GW_CMD_USE_TOKEN	0
#define GW_CMD_REQ_TOKEN	1
#define GW_CMD_HAVE_TOKEN	2
#define GW_CMD_GEN_TOKEN	3

int bus_command(uint8_t *buffer, int len);
int gateway_command(uint8_t *buffer, int len);
