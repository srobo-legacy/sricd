#include "init.h"
#include "queue.h"
#include "input.h"
#include "output-queue.h"

void init()
{
	queue_init();
	txq_init();
	input_init();
}
