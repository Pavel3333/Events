#include "BW_native.h"

PyObject* BigWorld    = NULL;
PyObject* g_gui       = NULL;
PyObject* g_appLoader = NULL;
PyObject* functools   = NULL;
PyObject* json        = NULL;

//native functions

void callback(long* CBID, PyObject* func, float time_f) {
	if (!func) {
		return;
	}

	PyObject* time_p;

	if (!time_f) time_p = PyFloat_FromDouble(0.0);
	else         time_p = PyFloat_FromDouble(time_f);

	if (!time_p) {
		return;
	}

	PyObject* __callback_text = PyString_FromStringAndSize("callback", 8U);

	PyObject* res = PyObject_CallMethodObjArgs(BigWorld, __callback_text, time_p, func, NULL);

	Py_DECREF(__callback_text);

	if (!res) {
		return;
	}

	*CBID = PyInt_AS_LONG(res);

	superExtendedDebugLog("[NY_Event]: Callback created!");

	Py_DECREF(res);
}

void cancelCallback(long* CBID) {
	if (!*CBID) {
		return;
	}

	PyObject* __cancelCallback = PyString_FromStringAndSize("cancelCallback", 14U);

	PyObject* res = PyObject_CallMethodObjArgs(BigWorld, __cancelCallback, PyInt_FromLong(*CBID), NULL);

	Py_DECREF(__cancelCallback);
	Py_XDECREF(res);

	*CBID = NULL;
}
