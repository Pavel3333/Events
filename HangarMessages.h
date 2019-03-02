#pragma once
#include "API_functions.h"


class HangarMessages {
public:
	static bool inited;
	static bool showed;

	static PyObject* m_SM_TYPE;
	static PyObject* m_pushMessage;

	static MyErr init();
	static void fini();
	
	static MyErr showMessage(PyObject*);

private:
	static int init_p();
	static int showMessage_p(PyObject*);
};
