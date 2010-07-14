#include "init.h"
#include "input.h"
#include "output-queue.h"

void init()
{
	txq_init();
	input_init();
}
