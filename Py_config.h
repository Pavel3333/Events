#pragma once
#include "API_functions.h"
#include <atomic>

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
	std::atomic_bool inited = false;

	Config config = Config();

	ConfigObject* g_self = nullptr;

	PyTypeObject* Config_p = nullptr;

	PyObject* m_g_gui = nullptr;

	PyObject* g_config = nullptr;

	int   lastError = 0;
	char* lastErrorStr = nullptr;

	PyConfig();
	~PyConfig();

private:
	uint8_t init();

	PyObject* init_data();
	PyObject* init_i18n();

	PyObject* Config_new(PyTypeObject*, PyObject*, PyObject*);
	void Config_dealloc(ConfigObject*);

	PyObject* getMessagesList();

	bool write_data(char*, PyObject*);
	bool read_data(bool);
};

extern PyConfig* gPyConfig;
