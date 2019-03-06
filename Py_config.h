#pragma once
#include "API_functions.h"
#include "BW_native.h"
#include <filesystem>

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

	static PyObject* m_g_gui;
	static ConfigObject* g_self;
	static PyObject* g_config;

	static Config config;

	static MyErr init();
	static void fini();

	static PyObject* Config_new(PyTypeObject*, PyObject*, PyObject*);
	static void Config_dealloc(ConfigObject*);
private:
	static PyObject* getMessagesList();
	static void init_config();
	static PyObject* init_data();
	static PyObject* init_i18n();
	static bool write_data(std::filesystem::path, PyObject*);
	static bool read_data(bool);
};

extern PyTypeObject Config_p;
