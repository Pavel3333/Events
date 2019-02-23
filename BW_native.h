#pragma once

#include <atomic>
#include "Py_common.h"


class BigWorldUtils {
public:
	std::atomic_bool inited = false;

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

	uint8_t lastError;

	BigWorldUtils();
	~BigWorldUtils();

	void callback(long*, PyObject*, float time_f = 1.0);
	void cancelCallback(long*);

	void getMapID(uint8_t*);
	void getDBID(uint32_t*);
	void getLastModelCoords(float, uint8_t*, float**);
private:
	void init();

	PyObject* getPlayer_p() const;

	uint8_t callback_p(long*, PyObject*, float time_f = 1.0);
	uint8_t cancelCallback_p(long*);
	uint8_t getMapID_p(uint8_t*);
	uint8_t getDBID_p(uint32_t*);
	uint8_t getLastModelCoords_p(float, uint8_t*, float**);
};

extern BigWorldUtils* gBigWorldUtils;
