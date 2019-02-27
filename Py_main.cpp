#define CONSOLE_VER1

#include "Handlers.h"
#include "Py_config.h"
#include <cstdlib>
#include <direct.h>
#include "MyLogger.h"


PyObject* m_g_gui = NULL;

//threads functions

/*
ПОЧЕМУ НЕЛЬЗЯ ЗАКРЫВАТЬ ТАЙМЕРЫ В САМИХ СЕБЕ

-поток открыл ожидающий таймер
-таймер и говорит ему: поток, у меня тут ашыпка
-таймер сделал харакири
-поток: ТАЙМЕР!!!
-программа ушла в вечное ожидание
*/

DWORD timerThread() {
	if (!isInited ||!EVENT_START_TIMER) { traceLog
		return 1;
	} traceLog
		
	PyGILState_STATE gstate;
	PyThreadState* _save;

	//включаем GIL для этого потока

	gstate = PyGILState_Ensure();

	//-----------------------------

	Py_UNBLOCK_THREADS;

	INIT_LOCAL_MSG_BUFFER;

	superExtendedDebugLog("waiting EVENT_START_TIMER");

	DWORD EVENT_START_TIMER_WaitResult = WaitForSingleObject(
		EVENT_START_TIMER->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_START_TIMER_WaitResult) {
		case WAIT_OBJECT_0:  traceLog
			superExtendedDebugLog("EVENT_START_TIMER signaled!");

			//рабочая часть

			EVENT_START_TIMER->request = handleStartTimerEvent(_save);

			if (EVENT_START_TIMER->request) { traceLog
				extendedDebugLogEx(WARNING, "EVENT_START_TIMER - handleStartTimerEvent: error %d", EVENT_START_TIMER->request);
			}

			break;

			// An error occurred
		default: traceLog
			extendedDebugLogEx(ERROR, "START_TIMER - something wrong with WaitResult!");

			return 2;
	} traceLog

	Py_BLOCK_THREADS;

	//выключаем GIL для этого потока

	PyGILState_Release(gstate);

	//------------------------------

	return NULL;
}

DWORD handlerThread() {
	if (!isInited || !EVENT_IN_HANGAR || !EVENT_START_TIMER || !EVENT_DEL_MODEL || !EVENT_BATTLE_ENDED) { traceLog
		return 1;
	} traceLog

	//включаем GIL для этого потока

	PyGILState_STATE gstate = PyGILState_Ensure();

	//-----------------------------

	PyThreadState* _save;

	Py_UNBLOCK_THREADS;

	INIT_LOCAL_MSG_BUFFER;

	superExtendedDebugLog("waiting EVENT_IN_HANGAR");

	DWORD EVENT_IN_HANGAR_WaitResult = WaitForSingleObject(
		EVENT_IN_HANGAR->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_IN_HANGAR_WaitResult) {
	case WAIT_OBJECT_0: traceLog
		superExtendedDebugLog("EVENT_IN_HANGAR signaled!");

		//место для рабочего кода

		EVENT_IN_HANGAR->request = handleInHangarEvent(_save);

		if (EVENT_IN_HANGAR->request) { traceLog
			extendedDebugLogEx(WARNING, "EVENT_IN_HANGAR - handleInHangarEvent: error %d", EVENT_IN_HANGAR->request);
		}

		break;
		// An error occurred
	default: traceLog
		extendedDebugLogEx(ERROR, "IN_HANGAR - something wrong with WaitResult!");

		return 2;
	} traceLog
	if (first_check) { traceLog
		extendedDebugLogEx(ERROR, "IN_HANGAR - Error %d!", (uint32_t)first_check);

		return 3;
	}
	
	Py_BLOCK_THREADS;

	HangarMessages->showMessage(g_self->i18n);

	if (HangarMessages->lastError) {
		extendedDebugLogEx(WARNING, "showMessage: error %d", HangarMessages->lastError);
	}

	Py_UNBLOCK_THREADS;

	if (hHangarTimer) { traceLog //закрываем таймер, если он был создан
		CancelWaitableTimer(hHangarTimer);
		CloseHandle(hHangarTimer);

		hHangarTimer = NULL;
	} traceLog

	//вывод сообщения в центр уведомлений

	//создаем поток с таймером

	if (hBattleTimerThread) { traceLog
		WaitForSingleObject(hBattleTimerThread, INFINITE);
		
		hBattleTimerThread  = NULL;
		battleTimerThreadID = NULL;
	} traceLog

	hBattleTimerThread = CreateThread(
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		TimerThread,                            // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&battleTimerThreadID);                  // returns the thread identifier 

	if (!hBattleTimerThread) {
		extendedDebugLogEx(ERROR, "Creating timer thread: error %d", GetLastError());

		return 4;
	} traceLog

	HANDLE hEvents[HEVENTS_COUNT] = {
		EVENT_DEL_MODEL->hEvent,  //событие удаления модели
		EVENT_BATTLE_ENDED->hEvent //событие выхода из боя
	};

	PyObject* delLabelCBID_p;

	uint8_t lastEventError = NULL;

	while (!first_check && !battleEnded && !lastEventError) { traceLog
		superExtendedDebugLog("waiting EVENTS");

		DWORD EVENTS_WaitResult = WaitForMultipleObjects(
			HEVENTS_COUNT,
			hEvents,
			FALSE,
			INFINITE);

		switch (EVENTS_WaitResult) {
		case WAIT_OBJECT_0 + 0:  traceLog //сработало событие удаления модели
			superExtendedDebugLog("DEL_LAST_MODEL signaled!");

			//место для рабочего кода

			EVENT_DEL_MODEL->request = handleDelModelEvent(_save);

			if (EVENT_DEL_MODEL->request) {
				extendedDebugLogEx(WARNING, "DEL_LAST_MODEL - handleDelModelEvent: error %d!", EVENT_DEL_MODEL->request);

				break;
			}

			break;
		case WAIT_OBJECT_0 + 1: traceLog //сработало событие окончания боя
			isTimerStarted = false;

			Py_BLOCK_THREADS;

			EVENT_BATTLE_ENDED->request = handleBattleEndEvent(_save);

			if (EVENT_BATTLE_ENDED->request) { traceLog
				extendedDebugLogEx(WARNING, "EVENT_BATTLE_ENDED - handleBattleEndEvent: error %d!", EVENT_BATTLE_ENDED->request);

				goto end_EVENT_BATTLE_ENDED;
			}

			battleEnded = true;

			current_map.stageID = STAGE_ID::COMPETITION;

			delLabelCBID_p = GUI_getAttr("delLabelCBID");

			if (!delLabelCBID_p || delLabelCBID_p == Py_None) { traceLog
				delLabelCBID = 0;

				Py_XDECREF(delLabelCBID_p);
			}
			else { traceLog
				delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);

				Py_DECREF(delLabelCBID_p);
			} traceLog

			gBigWorldUtils->cancelCallback(delLabelCBID);
			delLabelCBID = 0;

			allModelsCreated = NULL;

			GUI_setVisible(false);
			GUI_clearText();

			mapID   = NULL;

end_EVENT_BATTLE_ENDED:
			Py_UNBLOCK_THREADS;

			break;
			// An error occurred
		default: traceLog
			extendedDebugLogEx(WARNING, "EVENTS - something wrong with WaitResult!");

			lastEventError = 1;

			break;
		} traceLog
	} traceLog

	if (lastEventError) extendedDebugLogEx(WARNING, "Error in event: %d", lastEventError);

	if (hBattleTimerThread) { traceLog
		WaitForSingleObject(hBattleTimerThread, INFINITE);
		
		hBattleTimerThread  = NULL;
		battleTimerThreadID = NULL;
	} traceLog

	//освобождаем ивенты

	closeEvent1(&EVENT_START_TIMER);
	closeEvent1(&EVENT_IN_HANGAR);
	closeEvent1(&EVENT_DEL_MODEL);

	closeEvent2(&EVENT_ALL_MODELS_CREATED);
	closeEvent2(&EVENT_BATTLE_ENDED);

	//освобождаем мутексы

	if (M_MODELS_NOT_USING) { traceLog
		CloseHandle(M_MODELS_NOT_USING);

		M_MODELS_NOT_USING = NULL;
	} traceLog

	Py_BLOCK_THREADS;

	//выключаем GIL для этого потока

	PyGILState_Release(gstate);

	return NULL;
}

//threads

DWORD WINAPI TimerThread(LPVOID lpParam) {
	UNREFERENCED_PARAMETER(lpParam);

	INIT_LOCAL_MSG_BUFFER;

	DWORD result = timerThread();

	hBattleTimerThread  = NULL;
	battleTimerThreadID = NULL;

	//закрываем поток

	extendedDebugLog("Closing timer thread %ld, result: %ld", battleTimerThreadID, result);

	return result;
}

DWORD WINAPI HandlerThread(LPVOID lpParam) {
	UNREFERENCED_PARAMETER(lpParam);

	INIT_LOCAL_MSG_BUFFER;

	DWORD result = NULL;

	while (!result) {
		if (!createEventsAndMutexes()) { traceLog
			extendedDebugLogEx(ERROR, "HandlerThread - createEventsAndMutexes: error!");

			break;
		} traceLog

		result = handlerThread();

		if (result) {
			extendedDebugLogEx(ERROR, "HandlerThread - handlerThread: error %ld", result);

			break;
		}
	}

	hHandlerThread  = NULL;
	handlerThreadID = NULL;

	//закрываем поток

	extendedDebugLog("Closing handler thread %ld", handlerThreadID);

	return result;
}

//loaders

uint8_t event_start() {
	if (!isInited || first_check) { traceLog
		return 1;
	} traceLog

	isModelsAlreadyCreated = false;

	INIT_LOCAL_MSG_BUFFER;

	gBigWorldUtils->getMapID(&mapID);

	if (gBigWorldUtils->lastError) {
		extendedDebugLogEx(ERROR, "getMapID: error %d!", gBigWorldUtils->lastError);

		return 2;
	}

	extendedDebugLog("mapID = %d", (uint32_t)mapID);

	if (mapID != MAIN_COMPETITION_MAP && mapID != FUN_COMPETITION_MAP) {
		extendedDebugLogEx(ERROR, "incorrect map! (%d)", (uint32_t)mapID);

		return 3;
	}

	battleEnded = false;

	GUI_setTimerVisible(true);
	GUI_setVisible(true);

	isTimeVisible = true;

	request = makeEventInThread(EVENT_ID::IN_BATTLE_GET_FULL);

	if (request) { traceLog
		debugLogEx(ERROR, "start - error %d", request);

		return 4;
	} traceLog

	return NULL;
}

uint8_t event_сheck() { traceLog
	if (!isInited) { traceLog
		return 1;
	} traceLog

	INIT_LOCAL_MSG_BUFFER;

	debugLog("checking...");

	gBigWorldUtils->getDBID(&databaseID);

	if (gBigWorldUtils->lastError) {
		debugLogEx(ERROR, "getDBID: error %d!", gBigWorldUtils->lastError);

		return 3;
	}

	debugLog("DBID created");

	mapID = NULL;

	battleEnded = false;

	request = makeEventInThread(EVENT_ID::IN_HANGAR);

	if (request) { traceLog
		return 4;
	}
	else {
		return NULL;
	} traceLog
}

uint8_t event_init(PyObject* template_, PyObject* apply, PyObject* byteify) { traceLog
	if (!template_ || !apply || !byteify) { traceLog
		return 1;
	} traceLog

	if (m_g_gui && PyCallable_Check(template_) && PyCallable_Check(apply)) { traceLog
		Py_INCREF(template_);
		Py_INCREF(apply);

		PyObject_CallMethod_increfed(result, m_g_gui, "register", "sOOO", g_self->ids, template_, g_self->data, apply);

		Py_XDECREF(result);
		Py_DECREF(apply);
		Py_DECREF(template_);
	} traceLog

	if (!m_g_gui && PyCallable_Check(byteify)) { traceLog
		Py_INCREF(byteify);

		PyObject* args1 = PyTuple_New(1);
		PyTuple_SET_ITEM(args1, NULL, g_self->i18n);

		PyObject* result = PyObject_CallObject(byteify, args1);

		if (result) { traceLog
			PyObject* old = g_self->i18n;

			g_self->i18n = result;

			PyDict_Clear(old);
			Py_DECREF(old);
		} traceLog

		Py_DECREF(byteify);
	} traceLog

	return NULL;
}

//Py loaders

static PyObject* event_start_py(PyObject *self, PyObject *args) { traceLog
	request = event_start();

	if (!request) {
		Py_RETURN_NONE;
	}
	else {
		return PyInt_FromSize_t(request);
	}
};

static PyObject* event_fini_py(PyObject *self, PyObject *args) { traceLog
	if (!EVENT_BATTLE_ENDED) { traceLog
		return PyInt_FromSize_t(1);
	} traceLog

	INIT_LOCAL_MSG_BUFFER;

	if (!SetEvent(EVENT_BATTLE_ENDED->hEvent)) { traceLog
		debugLogEx(ERROR, "EVENT_BATTLE_ENDED not setted!");

		return PyInt_FromSize_t(2);
	} traceLog

	Py_RETURN_NONE;
};

static PyObject* event_err_code(PyObject *self, PyObject *args) { traceLog
	return PyInt_FromSize_t(first_check);
};

static PyObject* event_сheck_py(PyObject *self, PyObject *args) { traceLog
	uint8_t res = event_сheck();

	if (res) { traceLog
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
};

static PyObject* event_init_py(PyObject *self, PyObject *args) { traceLog
	if (!isInited) { traceLog
		return PyInt_FromSize_t(1);
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
};

static PyObject* event_inject_handle_key_event(PyObject *self, PyObject *args) { //traceLog
	if (!isInited || first_check || !databaseID || !mapID || !spaceKey || isStreamer) { traceLog
		Py_RETURN_NONE;
	} //traceLog

	PyObject* event_ = PyTuple_GET_ITEM(args, NULL);
	PyObject* isKeyGetted_Space = NULL;

	if (m_g_gui) { //traceLog
		PyObject* __get_key = PyString_FromString("get_key");
		
		PyObject_CallMethodObjArgs_increfed(isKeyGetted_Space_tmp, m_g_gui, __get_key, spaceKey, NULL);

		Py_DECREF(__get_key);

		isKeyGetted_Space = isKeyGetted_Space_tmp;
	}
	else {
		PyObject* key = PyObject_GetAttrString(event_, "key");

		if (!key) { traceLog
			Py_RETURN_NONE;
		} //traceLog

		PyObject* ____contains__ = PyString_FromString("__contains__");

		PyObject_CallMethodObjArgs_increfed(isKeyGetted_Space_tmp, spaceKey, ____contains__, key, NULL);

		Py_DECREF(____contains__);

		isKeyGetted_Space = isKeyGetted_Space_tmp;
	} traceLog

	if (isKeyGetted_Space == Py_True) { traceLog
		request = makeEventInThread(EVENT_ID::DEL_LAST_MODEL);

		if (request) { traceLog
			INIT_LOCAL_MSG_BUFFER;

			debugLogEx(ERROR, "making DEL_LAST_MODEL: error %d", request);

			Py_RETURN_NONE;
		} traceLog
	} //traceLog

	Py_XDECREF(isKeyGetted_Space);

	Py_RETURN_NONE;
};

static struct PyMethodDef event_methods[] =
{
	{ "b",             event_сheck_py,                METH_VARARGS, ":P" }, //check
	{ "c",             event_start_py,                METH_NOARGS,  ":P" }, //start
	{ "d",             event_fini_py,                 METH_NOARGS,  ":P" }, //fini
	{ "e",             event_err_code,                METH_NOARGS,  ":P" }, //get_error_code
	{ "g",             event_init_py,                 METH_VARARGS, ":P" }, //init
	{ "event_handler", event_inject_handle_key_event, METH_VARARGS, ":P" }, //inject_handle_key_event
	{ "omc",           event_onModelCreated,          METH_VARARGS, ":P" }, //onModelCreated
	{ NULL, NULL, 0, NULL }
};

PyDoc_STRVAR(event_methods__doc__,
	"Trajectory Mod module"
);

//---------------------------INITIALIZATION--------------------------

PyMODINIT_FUNC initevent(void)
{
	INIT_LOCAL_MSG_BUFFER;

	InitializeCriticalSection(&CS_NETWORK_NOT_USING);
	InitializeCriticalSection(&CS_PARSING_NOT_USING);
	
	//BigWorldUtils creating

	gBigWorldUtils = new BigWorldUtils();
	if (!gBigWorldUtils->inited) {
		goto freeBigWorldUtils;
	}

	//HangarMessages creating

	HangarMessages = new HangarMessagesC();
	if (!HangarMessages->inited) {
		debugLogEx(ERROR, "initevent - initHangarMessages: error %d!", HangarMessages->lastError);
		goto freeHangarMessages;
	}

	debugLog("Config init...");

	if (PyType_Ready(&Config_p)) {
		goto freeHangarMessages;
	}

	Py_INCREF(&Config_p);

	debugLog("Config init OK");

	//загрузка конфига мода

	PyObject* g_config = PyObject_CallObject((PyObject*)&Config_p, NULL);

	Py_DECREF(&Config_p);

	if (!g_config || !g_self) {
		goto freeHangarMessages;
	}

	//инициализация модуля

	event_module = Py_InitModule3("event",
		event_methods,
		event_methods__doc__);

	if (!event_module) {
		goto freeHangarMessages;
	}

	if (PyModule_AddObject(event_module, "l", g_config)) {
		goto freeHangarMessages;
	}

	//получение указателя на метод модуля onModelCreated

	onModelCreatedPyMeth = PyObject_GetAttrString(event_module, "omc");

	if (!onModelCreatedPyMeth) {
		goto freeHangarMessages;
	}

	//Space key

	spaceKey = PyList_New(1);

	if (spaceKey) {
		PyList_SET_ITEM(spaceKey, 0, PyInt_FromSize_t(KEY_DEL_LAST_MODEL));
	}

	//загрузка modGUI

	debugLog("Mod_GUI module loading...");

	PyObject* mGUI_module = PyImport_ImportModule("NY_Event.native.mGUI");

	if (!mGUI_module) {
		goto freeHangarMessages;
	}

	debugLog("Mod_GUI class loading...");

	modGUI = PyObject_CallMethod(mGUI_module, "Mod_GUI", NULL);

	Py_DECREF(mGUI_module);

	if (!modGUI) {
		goto freeHangarMessages;
	}

	debugLog("Mod_GUI class loaded OK!");

	//загрузка g_gui

	debugLog("g_gui module loading...");

	PyObject* mod_mods_gui = PyImport_ImportModule("gui.mods.mod_mods_gui");

	if (!mod_mods_gui) { traceLog
		PyErr_Clear();

		delete gBigWorldUtils;
		gBigWorldUtils = nullptr;

		debugLog("mod_mods_gui is NULL!");
	}
	else {
		m_g_gui = PyObject_GetAttrString(mod_mods_gui, "g_gui");

		Py_DECREF(mod_mods_gui);

		if (!m_g_gui) { traceLog
			goto freeHangarMessages;
		} traceLog

		debugLog("mod_mods_gui loaded OK!");
	} traceLog

	if (!m_g_gui) { traceLog
		// переделать на WinAPI: CreateDirectory?
		_mkdir("mods/configs");
		_mkdir("mods/configs/pavel3333");
		_mkdir("mods/configs/pavel3333/NY_Event");
		_mkdir("mods/configs/pavel3333/NY_Event/i18n");

		if (!read_data(true) || !read_data(false)) { traceLog
			goto freeHangarMessages;
		} traceLog
	}
	else {
		PyObject_CallMethod_increfed(data_i18n, m_g_gui, "register_data", "sOOs", g_self->ids, g_self->data, g_self->i18n, "pavel3333");

		if (!data_i18n) { traceLog
			Py_DECREF(modGUI);
			goto freeHangarMessages;
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
	} traceLog
	
	//инициализация curl

	uint32_t curl_init_result = curl_init();

	if (curl_init_result) { traceLog
		debugLogEx(ERROR, "Initialising CURL handle: error %d", curl_init_result);

		goto freeHangarMessages;
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

		goto freeHangarMessages;
	} traceLog

	return;

freeHangarMessages:
	delete HangarMessages;
	HangarMessages = nullptr;;

freeBigWorldUtils:
	delete gBigWorldUtils;
	gBigWorldUtils = nullptr;
}
