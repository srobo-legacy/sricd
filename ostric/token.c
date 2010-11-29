#include <stdbool.h>

#include "ostric.h"
#include "token.h"

static bool token_on_wire;

static void
juggle_token(void *self, void *null __attribute__((unused)))
{
	struct ostric_client *client;
	bool tmp;

	client = self;

	/* If we're keeping the token, don't move it onwards */
	if (client->has_token && client->keep_token) {
		client->token_callback(client);
		token_on_wire = false;
		return;
	}

	/* Otherwise, shunt token onwards */
	tmp = client->has_token;
	client->has_token = token_on_wire;
	token_on_wire = tmp;

	return;
}

void
advance_token()
{

	/* If we're sitting on the token, don't send it back round the bus */
	if (gw_keep_token && gw_has_token)
		return;

	/* Rotate token around list of clients */
	token_on_wire = gw_has_token;
	gw_has_token = false;

	g_slist_foreach(ostric_client_list, juggle_token, NULL);

	gw_has_token = token_on_wire;
	return;
}
