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
	void (*token_callback)(struct ostric_client *this);
	void *priv;
	bool has_token;		/* Self explanatory */
	bool keep_token;	/* Don't propagate token once we have it */

	/* message sending foo: */
	bool has_resp;		/* There's a response waiting for the token */
	bool is_ack;		/* Response should have ack bit set */
	uint8_t resp_dst;	/* Destination address of response */
	uint8_t resp_len;	/* Self explanatory */
	uint8_t resp[128];
};

extern int ostric_pty_fd;
extern GSList *ostric_client_list;

/* Bus / gateway state */
extern bool gw_token_mode;	/* False -> no tokens, True -> tokens on */
extern bool gw_has_token;	/* Self explanatory */
extern bool gw_keep_token;	/* "REQ_TOKEN" command issued, sit on token */

#endif /* _SRICD_OSTRIC_OSTRIC_H_ */
