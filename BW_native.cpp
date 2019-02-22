#include "BW_native.h"

//private methods

uint8_t BW_NativeC::init_p() { //initialization
	m_BigWorld = PyImport_AddModule("BigWorld"); //загрузка BigWorld

	if (!m_BigWorld) { traceLog
		return 1;
	} traceLog

	//загрузка Model

	m_Model = PyObject_GetAttrString(m_BigWorld, "Model");

	if (!m_Model) { traceLog
		return 2;
	} traceLog

	//загрузка fetchModel

	m_fetchModel = PyObject_GetAttrString(m_BigWorld, "fetchModel");

	if (!m_fetchModel) { traceLog
		Py_DECREF(m_Model);

		return 3;
	} traceLog

	//загрузка callback

	m_callback = PyObject_GetAttrString(m_BigWorld, "callback");

	if (!m_callback) { traceLog
		Py_DECREF(m_fetchModel);
		Py_DECREF(m_Model);
		
		return 4;
	} traceLog

	//загрузка cancelCallback

	m_cancelCallback = PyObject_GetAttrString(m_BigWorld, "cancelCallback");

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

	m_g_appLoader = PyObject_GetAttrString(appLoader, "g_appLoader");

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

	m_partial = PyObject_GetAttrString(functools, "partial");

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

	inited = true;

	return NULL;
} 

uint8_t BW_NativeC::callback_p(long* CBID, PyObject* func, float time_f) {
	if (!inited) return 1;

	if (!func) { traceLog
		return 2;
	} traceLog

	PyObject* time_p = NULL;

	if (!time_f) time_p = PyFloat_FromDouble(0.0);
	else         time_p = PyFloat_FromDouble(time_f);

	if (!time_p) { traceLog
		return 3;
	} traceLog

	PyObject_CallMethodObjArgs_increfed(res, m_callback, time_p, func, NULL);

	Py_DECREF(time_p);

	if (!res) { traceLog
		return 4;
	} traceLog

	*CBID = PyInt_AS_LONG(res);

	superExtendedDebugLog("[NY_Event]: Callback created!\n");

	Py_DECREF(res);
}

uint8_t BW_NativeC::cancelCallback_p(long* CBID) {
	if (!this->inited) return 1;

	if (!*CBID) { traceLog
		return 2;
	} traceLog

	PyObject_CallFunctionObjArgs_increfed(res, m_cancelCallback, PyInt_FromLong(*CBID), NULL);

	Py_XDECREF(res);

	*CBID = NULL;
}


//constructor

BW_NativeC::BW_NativeC() {
	this->inited = false;

	this->m_BigWorld       = NULL;
	this->m_Model          = NULL;
	this->m_fetchModel     = NULL;
	this->m_callback       = NULL;
	this->m_cancelCallback = NULL;
	this->m_g_appLoader    = NULL;
	this->m_partial        = NULL;
	this->m_json           = NULL;

	this->lastError = init_p();
}

//destructor

BW_NativeC::~BW_NativeC() {
	if (!this->inited) return;

	Py_XDECREF(this->m_json);
	Py_XDECREF(this->m_partial);
	Py_XDECREF(this->m_g_appLoader);
	Py_XDECREF(this->m_cancelCallback);
	Py_XDECREF(this->m_callback);
	Py_XDECREF(this->m_fetchModel);
	Py_XDECREF(this->m_Model);

	this->m_Model          = NULL;
	this->m_fetchModel     = NULL;
	this->m_callback       = NULL;
	this->m_cancelCallback = NULL;
	this->m_g_appLoader    = NULL;
	this->m_partial        = NULL;
	this->m_json           = NULL;
}


//public methods

void BW_NativeC::callback(long* CBID, PyObject* func, float time_f) {
	lastError = callback_p(CBID, func, time_f);
}

void BW_NativeC::cancelCallback(long* CBID) {
	lastError = cancelCallback_p(CBID);
}
