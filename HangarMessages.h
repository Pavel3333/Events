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
	
	static MyErr showMessage();

private:
	static MyErr init_p();
	static MyErr showCheckMessage_p(PyObject*);
	static MyErr showYoutubeMessage_p(PyObject*);
	static MyErr showMessage_p();
};
