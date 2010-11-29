#ifndef _SRICD_OSTRIC_OSTRIC_H_
#define _SRICD_OSTRIC_OSTRIC_H_
#include <stdbool.h>
#include <stdint.h>

#include <glib.h>

struct ostric_client;
struct ostric_client {
	int address;
	void (*msg_callback)(struct ostric_client *this, uint8_t *buffer,
				int len, uint8_t **resp, int *rlen);
	void *priv;
	bool has_token;
};

extern int ostric_pty_fd;
extern GSList *ostric_client_list;

/* Bus / gateway state */
extern bool gw_token_mode;	/* False -> no tokens, True -> tokens on */
extern GSList *next_token_recp;	/* Next recepient of token in client list, or
				 * null if it should be the gateway */

#endif /* _SRICD_OSTRIC_OSTRIC_H_ */
