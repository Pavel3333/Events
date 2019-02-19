#include "BW_native.h"

PyObject* m_BigWorld       = NULL;
PyObject* m_Model          = NULL;
PyObject* m_fetchModel     = NULL;
PyObject* m_callback       = NULL;
PyObject* m_cancelCallback = NULL;
PyObject* m_g_gui          = NULL;
PyObject* m_g_appLoader    = NULL;
PyObject* m_partial        = NULL;
PyObject* m_json           = NULL;

//initialization

uint8_t initNative() { traceLog
	//загрузка BigWorld

	m_BigWorld = PyImport_AddModule("BigWorld");

	if (!m_BigWorld) { traceLog
		return 1;
	} traceLog

	//загрузка Model

	PyObject* __Model = PyString_FromString("Model");

	m_Model = PyObject_GetAttr(m_BigWorld, __Model);

	Py_DECREF(__Model);

	if (!m_Model) { traceLog
		return 2;
	} traceLog

	//загрузка fetchModel

	PyObject* __fetchModel = PyString_FromString("fetchModel");

	m_fetchModel = PyObject_GetAttr(m_BigWorld, __fetchModel);

	Py_DECREF(__fetchModel);

	if (!m_fetchModel) { traceLog
		Py_DECREF(m_Model);

		return 3;
	} traceLog

	//загрузка callback

	PyObject* __callback_s = PyString_FromString("callback");

	m_callback = PyObject_GetAttr(m_BigWorld, __callback_s);

	Py_DECREF(__callback_s);

	if (!m_callback) { traceLog
		Py_DECREF(m_fetchModel);
		Py_DECREF(m_Model);
		
		return 4;
	} traceLog

	//загрузка cancelCallback

	PyObject* __cancelCallback = PyString_FromString("cancelCallback");

	m_cancelCallback = PyObject_GetAttr(m_BigWorld, __cancelCallback);

	Py_DECREF(__cancelCallback);

	if (!m_cancelCallback) { traceLog
		Py_DECREF(m_callback);
		Py_DECREF(m_fetchModel);
		Py_DECREF(m_Model);

		return 5;
	} traceLog

	//загрузка g_appLoader

	PyObject* appLoader = PyImport_ImportModule("gui.app_loader");

	if (!appLoader) { traceLog
		Py_DECREF(m_cancelCallback);
		Py_DECREF(m_callback);
		Py_DECREF(m_fetchModel);
		Py_DECREF(m_Model);
		
		return 6;
	} traceLog

	PyObject* __g_appLoader = PyString_FromString("g_appLoader");

	m_g_appLoader = PyObject_GetAttr(appLoader, __g_appLoader);

	Py_DECREF(__g_appLoader);
	Py_DECREF(appLoader);

	if (!m_g_appLoader) { traceLog
		Py_DECREF(m_cancelCallback);
		Py_DECREF(m_callback);
		Py_DECREF(m_fetchModel);
		Py_DECREF(m_Model);

		return 7;
	} traceLog

	//загрузка partial

	PyObject* functools = PyImport_ImportModule("functools");

	if (!functools) {
		Py_DECREF(m_g_appLoader);
		Py_DECREF(m_cancelCallback);
		Py_DECREF(m_callback);
		Py_DECREF(m_fetchModel);
		Py_DECREF(m_Model);

		return 8;
	}

	PyObject* __g_partial = PyString_FromString("partial");

	m_partial = PyObject_GetAttr(functools, __g_partial);

	Py_DECREF(__g_partial);
	Py_DECREF(functools);

	if (!m_partial) { traceLog
		Py_DECREF(m_g_appLoader);
		Py_DECREF(m_cancelCallback);
		Py_DECREF(m_callback);
		Py_DECREF(m_fetchModel);
		Py_DECREF(m_Model);

		return 9;
	} traceLog

	//загрузка json

	m_json = PyImport_ImportModule("json");

	if (!m_json) { traceLog
		Py_DECREF(m_partial);
		Py_DECREF(m_g_appLoader);
		Py_DECREF(m_cancelCallback);
		Py_DECREF(m_callback);
		Py_DECREF(m_fetchModel);
		Py_DECREF(m_Model);

		return 10;
	} traceLog

	return NULL;
}

//finalization

void finiNative() {
	Py_XDECREF(m_json);
	Py_XDECREF(m_partial);
	Py_XDECREF(m_g_appLoader);
	Py_XDECREF(m_g_gui);
	Py_XDECREF(m_cancelCallback);
	Py_XDECREF(m_callback);
	Py_XDECREF(m_fetchModel);
	Py_XDECREF(m_Model);

	m_Model          = NULL;
	m_fetchModel     = NULL;
	m_callback       = NULL;
	m_cancelCallback = NULL;
	m_g_gui          = NULL;
	m_g_appLoader    = NULL;
	m_partial        = NULL;
	m_json           = NULL;
}

//native functions

void callback(long* CBID, PyObject* func, float time_f) {
	if (!func) { traceLog
		return;
	} traceLog

	PyObject* time_p;

	if (!time_f) time_p = PyFloat_FromDouble(0.0);
	else         time_p = PyFloat_FromDouble(time_f);

	if (!time_p) { traceLog
		return;
	} traceLog

	PyObject* res = PyObject_CallMethodObjArgs(m_callback, time_p, func, NULL);

	if (!res) { traceLog
		return;
	} traceLog

	*CBID = PyInt_AS_LONG(res);

	superExtendedDebugLog("[NY_Event]: Callback created!\n");

	Py_DECREF(res);
}

void cancelCallback(long* CBID) {
	if (!*CBID) { traceLog
		return;
	} traceLog

	PyObject* res = PyObject_CallMethodObjArgs(m_cancelCallback, PyInt_FromLong(*CBID), NULL);

	Py_XDECREF(res);

	*CBID = NULL;
}
