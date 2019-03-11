#include "pch.h"
#include "Py_config.h"
#include "MyLogger.h"
#include "CConfig.h"
#include "python2.7/structmember.h"
#include <filesystem>


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

PyTypeObject Config_p {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pos.Config",					  /*tp_name*/
	sizeof(ConfigObject),             /*tp_basicsize*/

										/* Methods to implement standard operations */

	NULL,
	(destructor)(PyConfig::Config_dealloc),
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
	"Config for NY_Event Mod", /* Documentation string */

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
	PyConfig::Config_new
};

bool PyConfig::inited = false;

PyObject* PyConfig::m_g_gui    = nullptr;
ConfigObject* PyConfig::g_self = nullptr;
PyObject* PyConfig::g_config   = nullptr;

INIT_LOCAL_MSG_BUFFER;

PyObject* PyConfig::getMessagesList() {
	PyObject* messagesList = PyList_New(MESSAGES_COUNT);

	for (uint8_t i = NULL; i < MESSAGES_COUNT; i++) {
		PyList_SET_ITEM(messagesList, i, PyString_FromString(MESSAGES[i]));
	}

	return messagesList;
};

MyErr PyConfig::init()
{
	debugLog("Config init...");

	Config::i18n.version = Config::version_id;
	Config::data.version = Config::version_id;

	if (PyType_Ready(&Config_p)) {
		return_err 1;
	}

	Py_INCREF(&Config_p);

	//загрузка конфига мода

	g_config = PyObject_CallObject((PyObject*)(&Config_p), NULL);

	////Py_DECREF(&Config_p);

	if (!g_config || !g_self) {
		return_err 2;
	}

	//загрузка g_gui

	INIT_LOCAL_MSG_BUFFER;

	debugLog("g_gui module loading...");

	PyObject* mod_mods_gui = PyImport_ImportModule("gui.mods.mod_mods_gui");

	if (!mod_mods_gui) { traceLog
		PyErr_Clear();

		debugLog("mod_mods_gui not initialized");
	}
	else {
		m_g_gui = PyObject_GetAttrString(mod_mods_gui, "g_gui");

		Py_DECREF(mod_mods_gui);

		if (!m_g_gui) { traceLog
			return_err 3;
		}

		debugLog("mod_mods_gui loaded OK!");
	}

	if (!m_g_gui) {
		CreateDirectoryA("mods/configs", NULL);
		CreateDirectoryA("mods/configs/pavel3333", NULL);
		CreateDirectoryA("mods/configs/pavel3333/NY_Event", NULL);
		CreateDirectoryA("mods/configs/pavel3333/NY_Event/i18n", NULL);

		if (!read_data(true) || !read_data(false)) { traceLog
			return_err 4;
		} traceLog
	}
	else {
		PyObject* data_i18n = PyObject_CallMethod(m_g_gui, "register_data", "sOOs", g_self->ids, g_self->data, g_self->i18n, "pavel3333");

		if (!data_i18n) { traceLog
			return_err 5;
		} traceLog

		PyObject* old = g_self->data;

		g_self->data = PyTuple_GET_ITEM(data_i18n, NULL);

		PyDict_Clear(old);

		Py_DECREF(old);

		old = g_self->i18n;

		g_self->i18n = PyTuple_GET_ITEM(data_i18n, 1);

		PyDict_Clear(old);

		Py_DECREF(old);
		Py_DECREF(data_i18n);
	}

	inited = true;

	debugLog("Config init OK");

	return_ok;
}

void PyConfig::fini()
{
	if (!inited)
		return;

	Py_XDECREF(g_config);
	g_config = nullptr;
}

PyObject* PyConfig::init_data() {
	PyObject* data_py = PyDict_New();
	if (!data_py) goto end_init_data_1;

	PyObject* data_version = PyInt_FromSize_t(Config::data.version);

	if (PyDict_SetItemString(data_py, "version", data_version)) goto end_init_data_2;

	PyObject* data_enabled = PyBool_FromLong(Config::data.enabled);

	if (PyDict_SetItemString(data_py, "enabled", data_enabled)) {
		Py_DECREF(data_enabled);
	end_init_data_2:
		Py_DECREF(data_version);
		Py_DECREF(data_py);
	end_init_data_1:
		return NULL;
	}

	return data_py;
}

PyObject* PyConfig::init_i18n() {
	PyObject* i18n_py = PyDict_New();
	if (!i18n_py) goto end_init_i18n_1;

	PyObject* i18n_version = PyInt_FromSize_t(Config::i18n.version);

	if (PyDict_SetItemString(i18n_py, "version", i18n_version)) goto end_init_i18n_2;

	PyObject* UI_description = PyString_FromString("NY_Event Mod");

	PyObject* empty_tooltip  = PyString_FromStringAndSize("", NULL);

	PyObject* UI_message_thx     = PyString_FromString("NY_Event: Successfully loaded.");
	PyObject* UI_message_thx_2   = PyString_FromString("Official site");
	PyObject* UI_message_channel = PyString_FromString("Official channel RAINN VOD of NY_Event");

	PyObject* UI_err_2 = PyString_FromString("Error. Redownload mod");
	PyObject* UI_err_3 = PyString_FromString("Error. Maybe network don't works?");
	PyObject* UI_err_6 = PyString_FromString("Error. You were not allowed to event. To participate in the event, please write to ArrrTes");

	PyObject* UI_messages = getMessagesList();

	if (PyDict_SetItemString(i18n_py, "UI_description", UI_description)         ||
		PyDict_SetItemString(i18n_py, "UI_message_thx", UI_message_thx)         ||
		PyDict_SetItemString(i18n_py, "UI_message_thx_2", UI_message_thx_2)     ||
		PyDict_SetItemString(i18n_py, "UI_message_channel", UI_message_channel) ||
		PyDict_SetItemString(i18n_py, "UI_err_2", UI_err_2)                     ||
		PyDict_SetItemString(i18n_py, "UI_err_3", UI_err_3)                     ||
		PyDict_SetItemString(i18n_py, "UI_err_6", UI_err_6)                     ||
		PyDict_SetItemString(i18n_py, "UI_messages", UI_messages)
		) { traceLog
		Py_DECREF(UI_messages);
		Py_DECREF(UI_err_6);
		Py_DECREF(UI_err_3);
		Py_DECREF(UI_err_2);
		Py_DECREF(UI_message_channel);
		Py_DECREF(UI_message_thx_2);
		Py_DECREF(UI_message_thx);
		Py_DECREF(empty_tooltip);
		Py_DECREF(UI_description);
	end_init_i18n_2: traceLog
		Py_DECREF(i18n_version);
		Py_DECREF(i18n_py);
	end_init_i18n_1: traceLog
		return NULL;
	} traceLog

	return i18n_py;
}

bool PyConfig::write_data(std::filesystem::path data_path, PyObject* data_p)
{
	traceLog

	PyObject* dumpsFunc = PyObject_GetAttrString(BigWorldUtils::m_json, "dumps");

	if (!dumpsFunc) goto end_write_data_1;

	PyObject* args = PyTuple_Pack(1, data_p);

	if (!args) goto end_write_data_2;

	PyObject* kwargs = PyDict_New();

	if (!kwargs) goto end_write_data_3;

	PyObject* indent    = PyInt_FromSize_t(4);
	PyObject* sort_keys = PyBool_FromLong(1);

	if(
		PyDict_SetItemString(kwargs, "indent",    indent)    ||
		PyDict_SetItemString(kwargs, "sort_keys", sort_keys)
		) { traceLog
		Py_DECREF(sort_keys);
		Py_DECREF(indent);

		Py_DECREF(kwargs);
	end_write_data_3: traceLog
		Py_DECREF(args);
	end_write_data_2: traceLog
		Py_DECREF(dumpsFunc);
	end_write_data_1: traceLog
		return false;
	} traceLog

	PyObject* data_json_s = PyObject_Call(dumpsFunc, args, kwargs);

	Py_DECREF(kwargs);
	Py_DECREF(args);
	Py_DECREF(dumpsFunc);

	if (!data_json_s) { traceLog
		return false;
	} traceLog

	size_t data_size = PyObject_Length(data_json_s);

	// странно открываешь файл на запись
	std::ofstream data_w(data_path);

	data_w.write(PyString_AS_STRING(data_json_s), data_size);

	data_w.close();

	Py_DECREF(data_json_s);

	return true;
}

bool PyConfig::read_data(bool isData)
{
	traceLog
	
	std::filesystem::path data_path;

	PyObject* data_src = nullptr;
	
	if (isData) { traceLog
		data_path = "mods/configs/pavel3333/NY_Event/NY_Event.json";
		data_src = g_self->data;
	}
	else { traceLog
		data_src = g_self->i18n;
		data_path = "mods/configs/pavel3333/NY_Event/i18n/ru.json";
	} traceLog

	if (!data_src) { traceLog
		return false;
	} traceLog

	std::ifstream data(data_path, std::ios::binary);

	if (!data.is_open()) { traceLog
		data.close();
		if (!write_data(data_path, data_src)) { traceLog
			return false;
		} traceLog
	}
	else { traceLog
		data.seekg(0, std::ios::end);
		size_t size = (size_t)data.tellg(); //getting file size
		data.seekg(0);

		char* data_s = new char[size + 1];

		while (!data.eof()) {
			data.read(data_s, size);
		} traceLog

		data.close();

		auto data_json_s = PyObject_CallMethod(BigWorldUtils::m_json, "loads", "s", data_s);

		delete[] data_s;

		if (!data_json_s) { traceLog
			PyErr_Clear();

			if (!write_data(data_path, data_src)) { traceLog
				return false;
			} traceLog

			return true;
		} traceLog

		PyObject* old = data_src;
		if (isData) g_self->data = data_json_s;
		else g_self->i18n = data_json_s;

		PyDict_Clear(old);
		Py_DECREF(old);
	} traceLog

	return true;
}


PyObject* PyConfig::Config_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(args);
	UNREFERENCED_PARAMETER(kwds);

	g_self = (ConfigObject*)type->tp_alloc(type, 0);

	if (g_self) {
		g_self->ids        = Config::ids;
		g_self->patch      = Config::patch;
		g_self->author     = Config::author;
		g_self->version    = Config::version;
		g_self->version_id = Config::version_id;

		g_self->data = init_data();

		if (!g_self->data) goto end_Config_new_1;

		g_self->i18n = init_i18n();

		if (!g_self->i18n) {
		end_Config_new_1: traceLog
			Py_DECREF(g_self);

			return NULL;
		}
	}

	return (PyObject *)g_self;
}

void PyConfig::Config_dealloc(ConfigObject* self) {
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
