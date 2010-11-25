#include <glib.h>

struct ostric_client;
struct ostric_client {
	int address;
	void (*msg_callback)(struct ostric_client *this, uint8_t *buffer,
				int len, uint8_t **resp, int *rlen);
	void (*enum_callback)(struct ostric_client *this, uint8_t *buffer,
				int len, uint8_t **resp, int *rlen);
	void *priv;
};

extern GSList *ostric_client_list;
