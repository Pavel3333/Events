﻿#include "BW_native.h"

//constructor

BW_NativeC::BW_NativeC() {
	this->inited = false;

	this->m_BigWorld = NULL;
	this->m_Model = NULL;
	this->m_fetchModel = NULL;
	this->m_callback = NULL;
	this->m_cancelCallback = NULL;
	this->m_g_appLoader = NULL;
	this->m_partial = NULL;
	this->m_json = NULL;

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

	this->m_Model = NULL;
	this->m_fetchModel = NULL;
	this->m_callback = NULL;
	this->m_cancelCallback = NULL;
	this->m_g_appLoader = NULL;
	this->m_partial = NULL;
	this->m_json = NULL;
}


//private methods

uint8_t BW_NativeC::init_p() {
	//загрузка BigWorld

	m_BigWorld = PyImport_AddModule("BigWorld");

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

PyObject* BW_NativeC::getPlayer_p() {
	PyObject* __player = PyString_FromString("player");

	PyObject_CallMethodObjArgs_increfed(player, BW_Native->m_BigWorld, __player, NULL);

	Py_DECREF(__player);

	return player;
}

uint8_t BW_NativeC::callback_p(long* CBID, PyObject* func, float time_f) {
	if (!func) { traceLog
		return 1;
	} traceLog

	PyObject* time_p = NULL;

	if (!time_f) time_p = PyFloat_FromDouble(0.0);
	else         time_p = PyFloat_FromDouble(time_f);

	if (!time_p) { traceLog
		return 2;
	} traceLog

	PyObject_CallMethodObjArgs_increfed(res, m_callback, time_p, func, NULL);

	Py_DECREF(time_p);

	if (!res) { traceLog
		return 3;
	} traceLog

	*CBID = PyInt_AS_LONG(res);

	superExtendedDebugLog("[NY_Event]: Callback created!\n");

	Py_DECREF(res);

	return NULL;
}

uint8_t BW_NativeC::cancelCallback_p(long* CBID) {
	if (!*CBID) { traceLog
		return 1;
	} traceLog

	PyObject_CallFunctionObjArgs_increfed(res, m_cancelCallback, PyInt_FromLong(*CBID), NULL);

	Py_XDECREF(res);

	*CBID = NULL;

	return NULL;
}

uint8_t BW_NativeC::getMapID_p(uint8_t* mapID) {
	PyObject* player = getPlayer_p();

	if (!player) { traceLog
		return 1;
	} traceLog

	PyObject* arena = PyObject_GetAttrString(player, "arena");

	Py_DECREF(player);

	if (!arena) { traceLog
		return 2;
	} traceLog

	PyObject* arenaType = PyObject_GetAttrString(arena, "arenaType");

	Py_DECREF(arena);

	if (!arenaType) { traceLog
		return 3;
	} traceLog

	PyObject* map_PS = PyObject_GetAttrString(arenaType, "geometryName");

	Py_DECREF(arenaType);

	if (!map_PS) { traceLog
		return 4;
	} traceLog

	char* map_s = PyString_AS_STRING(map_PS);

	Py_DECREF(map_PS);

	char map_ID_s[4];
	memcpy(map_ID_s, map_s, 3);
	if (map_ID_s[2] == '_') map_ID_s[2] = NULL;
	map_ID_s[3] = NULL;

	*mapID = atoi(map_ID_s);

	return NULL;
}

uint8_t BW_NativeC::getDBID_p(uint32_t* DBID) {
	PyObject* player = getPlayer_p();

	if (!player) { traceLog
		return 1;
	} traceLog

	PyObject* DBID_string = PyObject_GetAttrString(player, "databaseID");

	Py_DECREF(player);

	if (!DBID_string) { traceLog
		return 2;
	} traceLog

	PyObject* DBID_int = PyNumber_Int(DBID_string);

	Py_DECREF(DBID_string);

	if (!DBID_int) { traceLog
		return 3;
	} traceLog

	*DBID = PyInt_AS_LONG(DBID_int);

	Py_DECREF(DBID_int);

	return NULL;
}


//public methods

void BW_NativeC::callback(long* CBID, PyObject* func, float time_f) {
	if (!inited) return;

	lastError = callback_p(CBID, func, time_f);
}

void BW_NativeC::cancelCallback(long* CBID) {
	if (!inited) return;

	lastError = cancelCallback_p(CBID);
}

void BW_NativeC::getMapID(uint8_t* mapID) {
	if (!inited) return;

	lastError = getMapID_p(mapID);
}

void BW_NativeC::getDBID(uint32_t* DBID) {
	if (!inited) return;

	lastError = getDBID_p(DBID);
}

BW_NativeC* BW_Native = nullptr;