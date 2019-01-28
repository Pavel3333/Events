#include"py_imports.h"
#include "common.h"


PyObject* Py::BigWorld = NULL;
PyObject* Py::g_gui = NULL;
PyObject* Py::g_appLoader = NULL;
PyObject* Py::json = NULL;
PyObject* Py::modGUI = NULL;



void Py::init_imports()
{
	json = PyImport_ImportModule("json");

	BigWorld = PyImport_AddModule("BigWorld");

	PyObject* appLoader = PyImport_ImportModule("gui.app_loader");
	g_appLoader = PyObject_GetAttrString(appLoader, "g_appLoader");
	Py_DECREF(appLoader);


	LOG_debug("Mod_GUI module loading...\n");

	PyObject* mGUI_module = PyImport_ImportModule("NY_Event.native.mGUI");

	if (!mGUI_module) {
		Py_DECREF(g_appLoader);
		return;
	}

	LOG_debug("Mod_GUI class loading...\n");

	PyObject* Mod_GUIMethodName = PyString_FromString("Mod_GUI");
	modGUI = PyObject_CallMethodObjArgs(mGUI_module, Mod_GUIMethodName, NULL);
	Py_DECREF(Mod_GUIMethodName);

	Py_DECREF(mGUI_module);

	if (!modGUI) {
		Py_DECREF(g_appLoader);
		return;
	}

	LOG_debug("Mod_GUI class loaded OK!\n");

	LOG_debug("g_gui module loading...\n");

	PyObject* mod_mods_gui = PyImport_ImportModule("gui.mods.mod_mods_gui");

	if (!mod_mods_gui) {
		PyErr_Clear();
		g_gui = NULL;

		LOG_debug("mod_mods_gui is NULL!\n");
	}
	else {
		g_gui = PyObject_GetAttrString(mod_mods_gui, "g_gui");
		Py_DECREF(mod_mods_gui);

		if (!g_gui) {
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			return;
		}

		LOG_debug("mod_mods_gui loaded OK!\n");
	}
}
