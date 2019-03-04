#pragma once
#include "API_functions.h"

#define KEY_DEL_LAST_MODEL 256 //Keys.KEY_LEFTMOUSE

struct ConfigObject {
	PyObject_HEAD
	char* ids;
	char* patch;
	char* author;
	char* version;
	uint16_t version_id;
	PyObject* buttons;
	PyObject* data;
	PyObject* i18n;
};

class PyConfig {
public:
	static PyObject* m_g_gui;
	static ConfigObject* g_self;

	static Config config;

	static PyObject* Config_new(PyTypeObject*, PyObject*, PyObject*);
	static void Config_dealloc(ConfigObject*);
private:
	static PyObject* getMessagesList();
	static void init_config();
	static PyObject* init_data();
	static PyObject* init_i18n();
};

extern PyTypeObject Config_p;
