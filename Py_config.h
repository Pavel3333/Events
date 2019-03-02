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
	static bool inited;
	static Config config;
	static ConfigObject* g_self;
	static PyTypeObject* Config_p;
	static PyObject* m_g_gui;
	static PyObject* g_config;

	static int init();
	static void fini();

private:
	static PyObject* init_data();
	static PyObject* init_i18n();

	static PyObject* Config_new(PyTypeObject*, PyObject*, PyObject*);
	static void Config_dealloc(ConfigObject*);

	static PyObject* getMessagesList();

	static bool write_data(char*, PyObject*);
	static bool read_data(bool);
};
