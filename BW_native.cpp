﻿#include "BW_native.h"

PyObject* BigWorld    = NULL;
PyObject* g_gui       = NULL;
PyObject* g_appLoader = NULL;
PyObject* functools   = NULL;
PyObject* json        = NULL;

//native functions

void callback(long* CBID, PyObject* func, float time_f) { traceLog();
	if (!func) { traceLog();
		return;
	}

	PyObject* time_p;

	if (!time_f) time_p = PyFloat_FromDouble(0.0);
	else         time_p = PyFloat_FromDouble(time_f);

	if (!time_p) { traceLog();
		return;
	}

	PyObject* __callback_text = PyString_FromStringAndSize("callback", 8U);

	PyObject* res = PyObject_CallMethodObjArgs(BigWorld, __callback_text, time_p, func, NULL);

	Py_DECREF(__callback_text);

	if (!res) { traceLog();
		return;
	}

	*CBID = PyInt_AS_LONG(res);

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("[NY_Event]: Callback created!\n"));
#endif

	Py_DECREF(res);
}

void cancelCallback(long* CBID) { traceLog();
	if (!*CBID) { traceLog();
		return;
	}

	PyObject* __cancelCallback = PyString_FromStringAndSize("cancelCallback", 14U);

	PyObject* res = PyObject_CallMethodObjArgs(BigWorld, __cancelCallback, PyInt_FromLong(*CBID), NULL);

	Py_DECREF(__cancelCallback);
	Py_XDECREF(res);

	*CBID = NULL;
}
