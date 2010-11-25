#ifndef _SRICD_OSTRIC_CMDS_H_
#define _SRICD_OSTRIC_CMDS_H_
#include <stdint.h>
#include "../sricd/sric-cmds.h"

void bus_command(uint8_t *buffer, int len);
void gateway_command(uint8_t *buffer, int len);
#endif /* _SRICD_OSTRIC_CMDS_H_ */
