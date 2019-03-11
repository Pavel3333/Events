#include "pch.h"
#include "PyLoader.h"
#include "CLoader.h"
#include "Handlers.h"
#include "Py_config.h"
#include "MyLogger.h"


INIT_LOCAL_MSG_BUFFER;

// Инициализация

bool PyLoader::inited = false;

PyObject* PyLoader::py_module            = nullptr;
PyObject* PyLoader::keyDelLastModel      = nullptr;
PyObject* PyLoader::onModelCreatedPyMeth = nullptr;

//public methods

MyErr PyLoader::init()
{
	//инициализация модуля

	py_module = Py_InitModule("event", event_methods);

	if (!py_module)
		return_err 1;

	if (PyModule_AddObject(py_module, "l", PyConfig::g_config)) {
		return_err 2;
	}

	//получение указателя на метод onModelCreated

	onModelCreatedPyMeth = PyObject_GetAttrString(py_module, "omc");

	if (!onModelCreatedPyMeth) {
		return_err 3;
	}

	//DelLastModel key

	keyDelLastModel = PyList_New(1);

	if (keyDelLastModel) {
		PyList_SET_ITEM(keyDelLastModel, 0, PyInt_FromSize_t(KEY_DEL_LAST_MODEL));
	}

	inited = true;

	return_ok;
}

void  PyLoader::fini()
{
	if (!inited)
		return;

	Py_XDECREF(onModelCreatedPyMeth);
	Py_XDECREF(keyDelLastModel);

	onModelCreatedPyMeth = nullptr;
	keyDelLastModel      = nullptr;
}


PyObject* PyLoader::start(PyObject *self, PyObject *args) { traceLog
	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	if (!inited)
		return PyInt_FromSize_t(5);
	
	request = event_start();

	if (!request) {
		Py_RETURN_NONE;
	}
	else {
		return PyInt_FromSize_t(request);
	}
}

PyObject* PyLoader::fini(PyObject *self, PyObject *args) { traceLog
	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	if (!inited)
		return PyInt_FromSize_t(1);

	if (!EVENT_BATTLE_ENDED) { traceLog
		return PyInt_FromSize_t(2);
	} traceLog

	if (!SetEvent(EVENT_BATTLE_ENDED->hEvent)) { traceLog
		debugLogEx(ERROR, "EVENT_BATTLE_ENDED not setted!");

		return PyInt_FromSize_t(3);
	} traceLog

	Py_RETURN_NONE;
}

PyObject* PyLoader::check(PyObject *self, PyObject *args) { traceLog
	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	if (!inited)
		return PyInt_FromSize_t(5);

	uint8_t res = event_check();

	if (res) { traceLog
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
}

PyObject* PyLoader::initCfg(PyObject *self, PyObject *args) { traceLog
	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	if (!inited)
		return PyInt_FromSize_t(1);

	if (!isInited) { traceLog
		return PyInt_FromSize_t(2);
	} traceLog

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

PyObject* PyLoader::keyHandler(PyObject *self, PyObject *args) { //traceLog
	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	if (!inited)
		Py_RETURN_NONE;

	if (!isInited || first_check || !databaseID || !mapID || !keyDelLastModel || isStreamer) { traceLog
		Py_RETURN_NONE;
	} //traceLog

	PyObject* event_ = PyTuple_GET_ITEM(args, NULL);
	PyObject* isKeyGetted = NULL;

	if (PyConfig::m_g_gui) { //traceLog
		PyObject* __get_key = PyString_FromString("get_key");
		
		PyObject* isKeyGetted_tmp = PyObject_CallMethodObjArgs(PyConfig::m_g_gui, __get_key, keyDelLastModel, NULL);

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

struct PyMethodDef PyLoader::event_methods[] =
{
	{ "b",             PyLoader::check,                METH_VARARGS, ":P" }, //check
	{ "c",             PyLoader::start,                METH_NOARGS,  ":P" }, //start
	{ "d",             PyLoader::fini,                 METH_NOARGS,  ":P" }, //fini
	{ "g",             PyLoader::initCfg,              METH_VARARGS, ":P" },
	{ "event_handler", PyLoader::keyHandler,           METH_VARARGS, ":P" }, //keyHandler
	{ "omc",           event_onModelCreated,           METH_VARARGS, ":P" }, //onModelCreated
	{ NULL, NULL, 0, NULL }
};

//точка входа в библиотеку

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

	if (auto err = GUI::init()) {
		debugLogEx(ERROR, "initevent - init GUI: error %d!", err);
		goto freeGUI;
	}

	if (auto err = PyLoader::init()) {
		debugLogEx(ERROR, "initevent - init PyLoader: error %d!", err);
		goto freePyLoader;
	}

	if (auto err = curl_init()) { traceLog
		debugLogEx(ERROR, "initevent - curl_init: error %d", err);

		goto freePyLoader;
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

		goto freePyLoader;
	} traceLog

	return;

freePyLoader:
	PyLoader::fini();

freeGUI:
	GUI::fini();

freePyConfig:
	PyConfig::fini();

freeHangarMessages:
	HangarMessages::fini();

freeBigWorldUtils:
	BigWorldUtils::fini();
}