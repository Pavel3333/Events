#include "Py_config.h"
#include "BW_native.h"
#include "MyLogger.h"
#include "python2.7/structmember.h"
#include <fstream>


INIT_LOCAL_MSG_BUFFER;


// Инициализация
bool          PyConfig::inited   = false;
Config        PyConfig::config   = Config();
ConfigObject* PyConfig::g_self   = nullptr;
PyTypeObject* PyConfig::Config_p = nullptr;
PyObject*     PyConfig::m_g_gui  = nullptr;
PyObject*     PyConfig::g_config = nullptr;


// что это и зачем?
// почему METH_OLDARGS ?
static PyMemberDef config_members[9] = {
	{ "ids",        T_STRING, offsetof(ConfigObject, ids),        METH_OLDARGS },
	{ "author",     T_STRING, offsetof(ConfigObject, author),     METH_OLDARGS },
	{ "patch",      T_STRING, offsetof(ConfigObject, patch),      METH_OLDARGS },
	{ "version",    T_STRING, offsetof(ConfigObject, version),    METH_OLDARGS },
	{ "version_id", T_SHORT,  offsetof(ConfigObject, version_id), METH_OLDARGS },
	{ "buttons",    T_OBJECT, offsetof(ConfigObject, buttons),    METH_OLDARGS },
	{ "data",       T_OBJECT, offsetof(ConfigObject, data),       METH_OLDARGS },
	{ "i18n",       T_OBJECT, offsetof(ConfigObject, i18n),       METH_OLDARGS },
	{ NULL }
};


MyErr PyConfig::init()
{
	debugLog("Config init...");

	config.i18n.version = config.version_id;
	config.data.version = config.version_id;


	// нужна ли эта штука вообще?
	Config_p = new PyTypeObject { //TODO: fix me
		PyVarObject_HEAD_INIT(NULL, 0)
		"event.Config",					  /*tp_name*/
		sizeof(ConfigObject),             /*tp_basicsize*/

										  /* Methods to implement standard operations */

		NULL,
		(destructor)(Config_dealloc),
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
		Config_new
	};

	if (PyType_Ready(Config_p)) {
		return_err 1;
	}

	Py_INCREF(Config_p);

	//загрузка конфига мода

	g_config = PyObject_CallObject((PyObject*)Config_p, NULL);

	Py_DECREF(Config_p);

	if (!g_config || !g_self) {
		return_err 2;
	}

	//загрузка g_gui

	INIT_LOCAL_MSG_BUFFER;

	debugLog("g_gui module loading...");

	PyObject* mod_mods_gui = PyImport_ImportModule("gui.mods.mod_mods_gui");

	if (!mod_mods_gui) { traceLog
		PyErr_Clear();

		debugLog("mod_mods_gui is NULL!");
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

	return MyErr::OK;
}


void PyConfig::fini()
{
	if (!inited)
		return;

	Py_XDECREF(g_config);

	delete Config_p;
	Config_p = nullptr;
}


PyObject* PyConfig::init_data()
{
	PyObject* data = PyDict_New();
	if (data == NULL) {
		return NULL;
	}

	PyObject* data_version = PyInt_FromSize_t(config.data.version);

	if (PyDict_SetItem(data, PyString_FromString("version"), data_version)) {
		Py_DECREF(data_version);
		Py_DECREF(data);
		return NULL;
	}

	PyObject* data_enabled = PyBool_FromLong(config.data.enabled);

	if (PyDict_SetItem(data, PyString_FromString("enabled"), data_enabled)) {
		Py_DECREF(data_enabled);
		Py_DECREF(data);
		return NULL;
	}

	return data;
}

PyObject* PyConfig::init_i18n()
{
	PyObject* i18n = PyDict_New();
	if (i18n == NULL) {
		return NULL;
	}

	PyObject* i18n_version = PyInt_FromSize_t(config.i18n.version);

	if (PyDict_SetItem(i18n, PyString_FromString("version"), i18n_version)) {
		Py_DECREF(i18n_version);
		Py_DECREF(i18n);
		return NULL;
	}

	PyObject* UI_description = PyString_FromString("NY_Event Mod");

	// зачем это?
	//PyObject* empty_tooltip = PyString_FromString("");

	PyObject* UI_message_thx     = PyString_FromString("NY_Event: Successfully loaded.");
	PyObject* UI_message_thx_2   = PyString_FromString("Official site");
	PyObject* UI_message_channel = PyString_FromString("Official channel RAINN VOD of NY_Event");

	PyObject* UI_err_2 = PyString_FromString("Error. Redownload mod");
	PyObject* UI_err_3 = PyString_FromString("Error. Maybe network don't works?");
	PyObject* UI_err_6 = PyString_FromString("Error. You were not allowed to event. To participate in the event, please write to ArrrTes");

	PyObject* UI_messages = getMessagesList();

	if (PyDict_SetItem(i18n, PyString_FromString("UI_description"), UI_description)         ||
		PyDict_SetItem(i18n, PyString_FromString("UI_message_thx"), UI_message_thx)         ||
		PyDict_SetItem(i18n, PyString_FromString("UI_message_thx_2"), UI_message_thx_2)     ||
		PyDict_SetItem(i18n, PyString_FromString("UI_message_channel"), UI_message_channel) ||
		PyDict_SetItem(i18n, PyString_FromString("UI_err_2"), UI_err_2)                     ||
		PyDict_SetItem(i18n, PyString_FromString("UI_err_3"), UI_err_3)                     ||
		PyDict_SetItem(i18n, PyString_FromString("UI_err_6"), UI_err_6)                     ||
		PyDict_SetItem(i18n, PyString_FromString("UI_messages"), UI_messages)
		) {
		Py_DECREF(i18n);
		return NULL;
	}

	return i18n;
}

PyObject* PyConfig::getMessagesList()
{
	PyObject* messagesList = PyList_New(MESSAGES_COUNT);

	for (uint8_t i = 0; i < MESSAGES_COUNT; i++) {
		PyList_SET_ITEM(messagesList, i, PyString_FromString(MESSAGES[i]));
	}

	return messagesList;
}

bool PyConfig::write_data(std::filesystem::path data_path, PyObject* data_p)
{
	traceLog

	// Это нужно потестировать
	PyObject* dumpsFunc = PyObject_GetAttrString(gBigWorldUtils->m_json, "dumps");
	PyObject* args = PyTuple_Pack(1, data_p);
	PyObject* kwargs = PyDict_New();
	PyDict_SetItemString(kwargs, "indent", PyInt_FromSize_t(4));

	PyObject* data_json_s = PyObject_Call(dumpsFunc, args, kwargs);

	Py_CLEAR(args);
	Py_CLEAR(kwargs);
	Py_XDECREF(dumpsFunc);

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
	PyObject* data_src;
	
	if (isData) { traceLog
		data_path = "mods/configs/pavel3333/NY_Event/NY_Event.json";
		data_src = g_self->data;
	}
	else {
		data_src = g_self->i18n;
		data_path = "mods/configs/pavel3333/NY_Event/i18n/ru.json";
	} traceLog

	std::ifstream data(data_path, std::ios::binary);

	if (!data.is_open()) { traceLog
		data.close();
		if (!write_data(data_path, data_src)) { traceLog
			return false;
		} traceLog
	}
	else {
		data.seekg(0, std::ios::end);
		size_t size = (size_t)data.tellg(); //getting file size
		data.seekg(0);

		char* data_s = new char[size + 1];

		while (!data.eof()) {
			data.read(data_s, size);
		} traceLog

		data.close();

		auto data_json_s = PyObject_CallMethod(gBigWorldUtils->m_json, "loads", "s", data_s);

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

PyObject* PyConfig::Config_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	(void)args;
	(void)kwds;

	g_self = (ConfigObject*)type->tp_alloc(type, 0);

	if (g_self) {
		g_self->ids = config.ids;

		if (!g_self->ids) {
			Py_DECREF(g_self);

			return NULL;
		}

		g_self->patch = config.patch;

		if (!g_self->patch) {
			Py_DECREF(g_self);

			return NULL;
		}

		g_self->author = config.author;

		if (!g_self->author) {
			Py_DECREF(g_self);

			return NULL;
		}

		g_self->version = config.version;

		if (!g_self->version) {
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

		if (!g_self->i18n) {
			Py_DECREF(g_self);

			return NULL;
		}
	}

	return (PyObject *)g_self;
}

void PyConfig::Config_dealloc(ConfigObject* self)
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
