#pragma once
#include "API_functions.h"
#include "Py_common.h"


class BigWorldUtils {
public:
	bool inited = false;

	PyObject* m_BigWorld       = nullptr;
	PyObject* m_Model          = nullptr;
	PyObject* m_fetchModel     = nullptr;
	PyObject* m_addModel       = nullptr;
	PyObject* m_delModel       = nullptr;
	PyObject* m_callback       = nullptr;
	PyObject* m_cancelCallback = nullptr;
	PyObject* m_g_appLoader    = nullptr;
	PyObject* m_partial        = nullptr;
	PyObject* m_json           = nullptr;

	int   lastError    = 0;
	char* lastErrorStr = nullptr;

	BigWorldUtils();
	~BigWorldUtils();

	// BigWorld.callback()
	// Registers a callback function to be called after a certain time, but not before the next tick
	void callback(long*, PyObject*, float delay = 1.0);

	// BigWorld.cancelCallback()
	// Cancels a previously registered callback
	void cancelCallback(long);

	void getMapID(uint8_t*);
	void getDBID(uint32_t*);
	void getLastModelCoords(float, MODEL_ID*, float**);

private:
	void init();

	PyObject* getPlayer_p() const;

	int callback_p(long*, PyObject*, float);
	int cancelCallback_p(long);
	int getMapID_p(uint8_t*);
	int getDBID_p(uint32_t*);
	int getLastModelCoords_p(float, MODEL_ID*, float**);
};

extern BigWorldUtils* gBigWorldUtils;
