#include <Python.h>

#include "sric.h"

static sric_context sric_ctx;

static PyMethodDef pysric_methods[] = {
{	NULL,	NULL,	0,	NULL	}
};

PyMODINIT_FUNC
initpysric(void)
{
	PyObject *m;

	sric_ctx = sric_init();
	if (sric_ctx == NULL) {
		fprintf(stderr, "Unable to initialize SRIC library\n");
		abort();
	}

	m = Py_InitModule("pysric", pysric_methods);
	return;
}
