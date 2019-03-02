#include "PyLoader.h"

#include "CLoader.h"

#include "Handlers.h"
#include "Py_config.h"
#include <cstdlib>
#include "MyLogger.h"


INIT_LOCAL_MSG_BUFFER;


static PyObject* event_start_py(PyObject *self, PyObject *args) { traceLog
	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);
	
	request = event_start();

	if (!request) {
		Py_RETURN_NONE;
	}
	else {
		return PyInt_FromSize_t(request);
	}
}

static PyObject* event_fini_py(PyObject *self, PyObject *args) { traceLog
	if (!EVENT_BATTLE_ENDED) { traceLog
		return PyInt_FromSize_t(1);
	} traceLog

	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	if (!SetEvent(EVENT_BATTLE_ENDED->hEvent)) { traceLog
		debugLogEx(ERROR, "EVENT_BATTLE_ENDED not setted!");

		return PyInt_FromSize_t(2);
	} traceLog

	Py_RETURN_NONE;
}

static PyObject* event_check_py(PyObject *self, PyObject *args) { traceLog
	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	uint8_t res = event_check();

	if (res) { traceLog
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
}

static PyObject* event_init_py(PyObject *self, PyObject *args) { traceLog
	if (!isInited) { traceLog
		return PyInt_FromSize_t(1);
	} traceLog

	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	PyObject* template_ = NULL;
	PyObject* apply     = NULL;
	PyObject* byteify   = NULL;

	if (!PyArg_ParseTuple(args, "OOO", &template_, &apply, &byteify)) { traceLog
		return PyInt_FromSize_t(2);
	} traceLog

	uint8_t res = event_init(template_, apply, byteify);

	if (res) { traceLog
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
}

static PyObject* event_keyHandler_py(PyObject *self, PyObject *args) { //traceLog
	if (!isInited || first_check || !databaseID || !mapID || !keyDelLastModel || isStreamer) { traceLog
		Py_RETURN_NONE;
	} //traceLog

	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	PyObject* event_ = PyTuple_GET_ITEM(args, NULL);
	PyObject* isKeyGetted = NULL;

	if (PyConfig::m_g_gui) { //traceLog
		PyObject* __get_key = PyString_FromString("get_key");
		
		auto isKeyGetted_tmp = PyObject_CallMethodObjArgs(PyConfig::m_g_gui, __get_key, keyDelLastModel, NULL);

		Py_DECREF(__get_key);

		isKeyGetted = isKeyGetted_tmp;
	}
	else {
		PyObject* key = PyObject_GetAttrString(event_, "key");

		if (!key) { traceLog
			Py_RETURN_NONE;
		} //traceLog

		PyObject* ____contains__ = PyString_FromString("__contains__");

		auto isKeyGetted_tmp = PyObject_CallMethodObjArgs(keyDelLastModel, ____contains__, key, NULL);

		Py_DECREF(____contains__);

		isKeyGetted = isKeyGetted_tmp;
	} traceLog

	if (isKeyGetted == Py_True) { traceLog
		request = makeEventInThread(EVENT_ID::DEL_LAST_MODEL);

		if (request) { traceLog

			debugLogEx(ERROR, "making DEL_LAST_MODEL: error %d", request);

			Py_RETURN_NONE;
		} traceLog
	} //traceLog

	Py_XDECREF(isKeyGetted);

	Py_RETURN_NONE;
}

static struct PyMethodDef event_methods[] =
{
	{ "b",             event_check_py,                METH_VARARGS, ":P" }, //check
	{ "c",             event_start_py,                METH_NOARGS,  ":P" }, //start
	{ "d",             event_fini_py,                 METH_NOARGS,  ":P" }, //fini
	{ "g",             event_init_py,                 METH_VARARGS, ":P" }, //init
	{ "event_handler", event_keyHandler_py,           METH_VARARGS, ":P" }, //keyHandler
	{ "omc",           event_onModelCreated,          METH_VARARGS, ":P" }, //onModelCreated
	{ NULL, NULL, 0, NULL }
};

//---------------------------INITIALIZATION--------------------------

PyMODINIT_FUNC initevent(void)
{
	// почему это здесь?
	InitializeCriticalSection(&CS_NETWORK_NOT_USING);
	
	if (auto err = BigWorldUtils::init()) {
		debugLogEx(ERROR, "initevent - init BigWorldUtils: error %d!", err);
		goto freeBigWorldUtils;
	}

	if (auto err = HangarMessages::init()) {
		debugLogEx(ERROR, "initevent - init HangarMessages: error %d!", err);
		goto freeHangarMessages;
	}

	if (auto err = PyConfig::init()) {
		debugLogEx(ERROR, "initevent - init PyConfig: error %d!", err);
		goto freePyConfig;
	}

	//инициализация модуля

	event_module = Py_InitModule("event", event_methods);

	if (!event_module) {
		goto freePyConfig;
	}

	if (PyModule_AddObject(event_module, "l", PyConfig::g_config)) {
		goto freePyConfig;
	}

	//получение указателя на метод модуля onModelCreated

	onModelCreatedPyMeth = PyObject_GetAttrString(event_module, "omc");

	if (!onModelCreatedPyMeth) {
		goto freePyConfig;
	}

	//Space key

	keyDelLastModel = PyList_New(1);

	if (keyDelLastModel) {
		PyList_SET_ITEM(keyDelLastModel, 0, PyInt_FromSize_t(KEY_DEL_LAST_MODEL));
	}

	//загрузка modGUI

	debugLog("Mod_GUI module loading...");

	PyObject* mGUI_module = PyImport_ImportModule("NY_Event.native.mGUI");

	if (!mGUI_module) {
		goto freePyConfig;
	}

	debugLog("Mod_GUI class loading...");

	modGUI = PyObject_CallMethod(mGUI_module, "Mod_GUI", NULL);

	Py_DECREF(mGUI_module);

	if (!modGUI) {
		goto freePyConfig;
	}

	debugLog("Mod_GUI class loaded OK!");
	
	//инициализация curl

	uint32_t curl_init_result = curl_init();

	if (curl_init_result) { traceLog
		debugLogEx(ERROR, "Initialising CURL handle: error %d", curl_init_result);

		goto freePyConfig;
	} traceLog

	isInited = true;

	//Handler thread creating

	if (hHandlerThread) { traceLog
		WaitForSingleObject(hHandlerThread, INFINITE);

		hHandlerThread = NULL;
		handlerThreadID = NULL;
	} traceLog

	//создаем второй поток

	hHandlerThread = CreateThread( 
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		HandlerThread,                          // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&handlerThreadID);                      // returns the thread identifier 

	if (!hHandlerThread) { traceLog
		debugLogEx(ERROR, "Handler thread creating: error %d", GetLastError());

		goto freePyConfig;
	} traceLog

	return;

freePyConfig:
	PyConfig::fini();

freeHangarMessages:
	HangarMessages::fini();

freeBigWorldUtils:
	BigWorldUtils::fini();
}
