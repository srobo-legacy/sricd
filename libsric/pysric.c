#include <Python.h>

static PyMethodDef pysric_methods[] = {
{	NULL,	NULL,	0,	NULL	}
};

PyMODINIT_FUNC
initpysric(void)
{
	PyObject *m;

	m = Py_InitModule("pysric", pysric_methods);
	return;
}
