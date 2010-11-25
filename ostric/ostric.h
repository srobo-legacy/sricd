#ifndef _SRICD_OSTRIC_OSTRIC_H_
#define _SRICD_OSTRIC_OSTRIC_H_
#include <stdint.h>

#include <glib.h>

struct ostric_client;
struct ostric_client {
	int address;
	void (*msg_callback)(struct ostric_client *this, uint8_t *buffer,
				int len, uint8_t **resp, int *rlen);
	void *priv;
};

extern int ostric_pty_fd;

extern GSList *ostric_client_list;

#endif /* _SRICD_OSTRIC_OSTRIC_H_ */
