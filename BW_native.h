#pragma once

#include "Py_common.h"

class BW_NativeC {
public:
	bool inited;

	PyObject* m_BigWorld;
	PyObject* m_Model;
	PyObject* m_fetchModel;
	PyObject* m_callback;
	PyObject* m_cancelCallback;
	PyObject* m_g_appLoader;
	PyObject* m_partial;
	PyObject* m_json;

	uint8_t lastError;
public:
	BW_NativeC();
	~BW_NativeC();

	void callback(long*, PyObject*, float time_f = 1.0);
	void cancelCallback(long*);

	void getMapID(uint8_t*);
	void getDBID(uint32_t*);
	void getLastModelCoords(float, uint8_t*, float**);
private:
	uint8_t init_p();

	PyObject* getPlayer_p();

	uint8_t callback_p(long*, PyObject*, float time_f = 1.0);
	uint8_t cancelCallback_p(long*);
	uint8_t getMapID_p(uint8_t*);
	uint8_t getDBID_p(uint32_t*);
	uint8_t getLastModelCoords_p(float, uint8_t*, float**);
};

extern BW_NativeC* BW_Native;