#pragma once
#include "API_functions.h"
#include "python2.7/Python.h"
#include "python2.7/structmember.h"

typedef struct {
	PyObject_HEAD
	char* ids;
	char* patch;
	char* author;
	char* version;
	uint16_t version_id;
	PyObject* buttons;
	PyObject* data;
	PyObject* i18n;
} ConfigObject;

extern ConfigObject* g_self;

static PyObject* getMessagesList();

uint8_t event_fini();

void init_config();

static PyObject* init_data();
static PyObject* init_i18n();

static PyObject * Config_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static void Config_dealloc(ConfigObject* self);

static PyMemberDef config_members[9] = {
	{ "ids",        T_STRING, offsetof(ConfigObject, ids),        NULL },
	{ "author",     T_STRING, offsetof(ConfigObject, author),     NULL },
	{ "patch",      T_STRING, offsetof(ConfigObject, patch),      NULL },
	{ "version",    T_STRING, offsetof(ConfigObject, version),    NULL },
	{ "version_id", T_SHORT,  offsetof(ConfigObject, version_id), NULL },
	{ "buttons",    T_OBJECT, offsetof(ConfigObject, buttons),    NULL },
	{ "data",       T_OBJECT, offsetof(ConfigObject, data),       NULL },
	{ "i18n",       T_OBJECT, offsetof(ConfigObject, i18n),       NULL },
	{ NULL }
};

extern PyTypeObject Config_p;