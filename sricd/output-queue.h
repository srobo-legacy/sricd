#ifndef _INCLUDED_OUTPUT_QUEUE
#define _INCLUDED_OUTPUT_QUEUE

#include "client.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _tx {
	int   address;
	void* tag; // if tag == NULL, this is a response
	int   payload_length;
	char  payload[PAYLOAD_MAX];
} tx;

void txq_init(void);
void txq_push(const tx* tx);
const tx* txq_next(void);
bool txq_empty(void);

#ifdef __cplusplus
}
#endif

#endif
