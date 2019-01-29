#pragma once
#include "stdafx.h"
#include "API_functions.h"
#include "python2.7/Python.h"
#include "python2.7/structmember.h"

Config config = Config();

uint8_t makeEventInThread(uint8_t, uint8_t);

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

ConfigObject* g_self = NULL;

static PyObject* getMessagesList() {
	PyObject* messagesList = PyList_New(MESSAGES_COUNT);

	for (uint8_t i = NULL; i < MESSAGES_COUNT; i++) {
		PyList_SET_ITEM(messagesList, i, PyString_FromString(MESSAGES[i]));
	}

	return messagesList;
};

uint8_t event_fini();

void init_config() {
	config.i18n.version = config.version_id;
	config.data.version = config.version_id;
}

static PyObject* init_data() {
	PyObject* data = PyDict_New();
	if (data == NULL) {
		return NULL;
	}

	PyObject* data_version = PyInt_FromSize_t(config.data.version);

	if (PyDict_SetItem(data, PyString_FromStringAndSize("version", 7U), data_version)) {
		Py_DECREF(data_version);
		Py_DECREF(data);
		return NULL;
	}

	PyObject* data_enabled = PyBool_FromLong(config.data.enabled);

	if (PyDict_SetItem(data, PyString_FromStringAndSize("enabled", 7U), data_enabled)) {
		Py_DECREF(data_enabled);
		Py_DECREF(data);
		return NULL;
	}

	return data;
};

static PyObject* init_i18n() {
	PyObject* i18n = PyDict_New();
	if (i18n == NULL) {
		return NULL;
	}

	PyObject* i18n_version = PyInt_FromSize_t(config.i18n.version);

	if (PyDict_SetItem(i18n, PyString_FromStringAndSize("version", 7U), i18n_version)) {
		Py_DECREF(i18n_version);
		Py_DECREF(i18n);
		return NULL;
	}

	PyObject* UI_description                      = PyString_FromStringAndSize("NY_Event Mod", 12U);

	PyObject* empty_tooltip                       = PyString_FromStringAndSize("", NULL);

	PyObject* UI_message_thx                      = PyString_FromStringAndSize("NY_Event: Successfully loaded.", 30U);
	PyObject* UI_message_thx_2                    = PyString_FromStringAndSize("Official site", 13U);
	PyObject* UI_message_channel                  = PyString_FromStringAndSize("Official channel RAINN VOD of NY_Event", 38U);

	PyObject* UI_err_2                            = PyString_FromStringAndSize("Error. Redownload mod ", 22U);
	PyObject* UI_err_3                            = PyString_FromStringAndSize("Error. Maybe network don't works?", 33U);
	PyObject* UI_err_6                            = PyString_FromStringAndSize("Error. You were not allowed to event.", 37U);

	PyObject* UI_messages                         = getMessagesList();

	if (PyDict_SetItem(i18n, PyString_FromStringAndSize("UI_description",                         14U), UI_description)                      ||
		PyDict_SetItem(i18n, PyString_FromStringAndSize("UI_message_thx",                         14U), UI_message_thx)                      ||
		PyDict_SetItem(i18n, PyString_FromStringAndSize("UI_message_thx_2",                       16U), UI_message_thx_2)                    ||
		PyDict_SetItem(i18n, PyString_FromStringAndSize("UI_message_channel",                     18U), UI_message_channel)                  ||
		PyDict_SetItem(i18n, PyString_FromStringAndSize("UI_err_2",                               8U),  UI_err_2)                            ||
		PyDict_SetItem(i18n, PyString_FromStringAndSize("UI_err_3",                               8U),  UI_err_3)                            ||
		PyDict_SetItem(i18n, PyString_FromStringAndSize("UI_err_6",                               8U),  UI_err_6)                            ||
		PyDict_SetItem(i18n, PyString_FromStringAndSize("UI_messages",                            11U), UI_messages)
		) {
		Py_DECREF(i18n);
		return NULL;
	}

	return i18n;
}

static PyObject * Config_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	init_config();

	g_self = (ConfigObject*)type->tp_alloc(type, 0);

	if (g_self != NULL) {

		g_self->ids = config.ids;
		if (g_self->ids == NULL) {
			Py_DECREF(g_self);
			return NULL;
		}

		g_self->patch = config.patch;
		if (g_self->patch == NULL) {
			Py_DECREF(g_self);
			return NULL;
		}

		g_self->author = config.author;
		if (g_self->author == NULL) {
			Py_DECREF(g_self);
			return NULL;
		}

		g_self->version = config.version;
		if (g_self->version == NULL) {
			Py_DECREF(g_self);
			return NULL;
		}

		g_self->version_id = config.version_id;
		if (g_self->version_id == NULL) {
			Py_DECREF(g_self);
			return NULL;
		}

		g_self->data = init_data();

		if (!g_self->data) {
			Py_DECREF(g_self);
			return NULL;
		}

		g_self->i18n = init_i18n();

	}

	return (PyObject *)g_self;
}

static void Config_dealloc(ConfigObject* self)
{
	if (self->buttons) {
		PyDict_Clear(self->buttons);

		Py_DECREF(self->buttons);
	}

	if (self->data) {
		PyDict_Clear(self->data);

		Py_DECREF(self->data);
	}

	if (self->i18n) {
		PyDict_Clear(self->i18n);

		Py_DECREF(self->i18n);
	}

	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMemberDef config_members[] = {
	{ "ids", T_STRING, offsetof(ConfigObject, ids), NULL },
	{ "author", T_STRING, offsetof(ConfigObject, author), NULL },
	{ "patch", T_STRING, offsetof(ConfigObject, patch), NULL },
	{ "version", T_STRING, offsetof(ConfigObject, version), NULL },
	{ "version_id", T_SHORT, offsetof(ConfigObject, version_id), NULL },
	{ "buttons", T_OBJECT, offsetof(ConfigObject, buttons), NULL },
	{ "data", T_OBJECT, offsetof(ConfigObject, data), NULL },
	{ "i18n", T_OBJECT, offsetof(ConfigObject, i18n), NULL },
	{ NULL }
};

PyTypeObject Config_p{
	PyVarObject_HEAD_INIT(NULL, 0)
	"pos.Config",					  /*tp_name*/
	sizeof(ConfigObject),             /*tp_basicsize*/

									  /* Methods to implement standard operations */

	NULL,
	(destructor)Config_dealloc,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	/* Method suites for standard classes */

	NULL,
	NULL,
	NULL,

	/* More standard operations (here for binary compatibility) */

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	/* Functions to access object as input/output buffer */
	NULL,

	/* Flags to define presence of optional/expanded features */
	Py_TPFLAGS_DEFAULT,
	"Config for Free Positions Mod", /* Documentation string */

								/* Assigned meaning in release 2.0 */
								/* call function for all accessible objects */
	NULL,

	/* delete references to contained objects */
	NULL,

	/* Assigned meaning in release 2.1 */
	/* rich comparisons */
	NULL,

	/* weak reference enabler */
	NULL,

	/* Added in release 2.2 */
	/* Iterators */
	NULL,
	NULL,

	/* Attribute descriptor and subclassing stuff */
	NULL, //struct PyMethodDef *tp_methods;
	config_members, //struct PyMemberDef *tp_members;
	NULL, //struct PyGetSetDef *tp_getset;
	NULL, //struct _typeobject *tp_base;
	NULL, //PyObject *tp_dict;
	NULL,
	NULL,
	NULL,
	NULL,//(initproc)Config_init,
	NULL,
	Config_new
};