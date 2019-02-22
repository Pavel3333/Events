#pragma once

#include "Py_common.h"

class HangarMessagesC {
public:
	bool inited;

	bool showed;

	PyObject* m_SM_TYPE;
	PyObject* m_pushMessage;

	uint8_t lastError;
public:
	HangarMessagesC();
	~HangarMessagesC();

	void showMessage(PyObject*);
private:
	uint8_t init_p();
	uint8_t showMessage_p(PyObject*);
};
