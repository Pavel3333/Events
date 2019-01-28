#pragma once
#include "python2.7/Python.h"

namespace Py {
	extern PyObject* BigWorld;
	extern PyObject* g_gui;
	extern PyObject* g_appLoader;
	extern PyObject* json;
	extern PyObject* modGUI;

	void init_imports();
}
