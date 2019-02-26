#pragma once
#include "API_functions.h"


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

extern ConfigObject* g_self;

extern PyObject* m_g_gui;

static PyObject* getMessagesList();

void init_config();

static PyObject* init_data();
static PyObject* init_i18n();

static PyObject * Config_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static void Config_dealloc(ConfigObject* self);

extern PyTypeObject Config_p;
