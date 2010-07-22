#ifndef _INCLUDED_OUTPUT_QUEUE
#define _INCLUDED_OUTPUT_QUEUE
#include "queue.h"
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
// prio varies from 0 to 3 inclusive, 3 is highest
void txq_init(void);
void txq_push(const tx* tx, int prio);
const tx* txq_next(void);
bool txq_empty(void);
#ifdef __cplusplus
}
#endif
#endif
