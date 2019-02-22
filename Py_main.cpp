#define CONSOLE_VER1

#include "Handlers.h"
#include "Py_config.h"
#include <cstdlib>
#include <direct.h>

#pragma warning disable C5033

std::ofstream dbg_log("NY_Event_debug_log.txt", std::ios::app);

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

uint32_t timerThread() {
	if (!isInited ||!EVENT_START_TIMER) { traceLog
		return 1;
	} traceLog

	PyGILState_STATE gstate;

	PyThreadState* _save;

	INIT_LOCAL_MSG_BUFFER;

	superExtendedDebugLog("[NY_Event]: waiting EVENT_START_TIMER\n");

	DWORD EVENT_START_TIMER_WaitResult = WaitForSingleObject(
		EVENT_START_TIMER->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_START_TIMER_WaitResult) {
		case WAIT_OBJECT_0:  traceLog
			superExtendedDebugLog("[NY_Event]: EVENT_START_TIMER signaled!\n");

			//включаем GIL для этого потока

			gstate = PyGILState_Ensure();

			//-----------------------------

			Py_UNBLOCK_THREADS;

			//рабочая часть

			request = handleStartTimerEvent(_save);

			if (request) { traceLog
				extendedDebugLogFmt("[NY_Event][WARNING]: EVENT_START_TIMER - handleStartTimerEvent: error %d\n", request);
			}

			Py_BLOCK_THREADS;

			//выключаем GIL для этого потока

			PyGILState_Release(gstate);

			//------------------------------

			break;

			// An error occurred
		default: traceLog
			extendedDebugLog("[NY_Event][ERROR]: START_TIMER - something wrong with WaitResult!\n");

			return 3;
	} traceLog

	//закрываем поток

	extendedDebugLogFmt("[NY_Event]: Closing timer thread %d\n", handlerThreadID);

	return NULL;
}

DWORD WINAPI TimerThread(LPVOID lpParam) {
	UNREFERENCED_PARAMETER(lpParam);

	uint32_t result = timerThread();

	hBattleTimerThread  = NULL;
	battleTimerThreadID = NULL;

	return result;
}

uint32_t handlerThread() {
	if (!isInited || !EVENT_IN_HANGAR || !EVENT_START_TIMER || !EVENT_DEL_MODEL || !EVENT_BATTLE_ENDED) { traceLog
		return 1;
	} traceLog

	//включаем GIL для этого потока

	PyGILState_STATE gstate = PyGILState_Ensure();

	//-----------------------------

	PyThreadState* _save;

	Py_UNBLOCK_THREADS;

	INIT_LOCAL_MSG_BUFFER;

	superExtendedDebugLog("[NY_Event]: waiting EVENT_IN_HANGAR\n");

	DWORD EVENT_IN_HANGAR_WaitResult = WaitForSingleObject(
		EVENT_IN_HANGAR->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_IN_HANGAR_WaitResult) {
	case WAIT_OBJECT_0: traceLog
		superExtendedDebugLog("[NY_Event]: EVENT_IN_HANGAR signaled!\n");

		//место для рабочего кода

		EVENT_IN_HANGAR->request = handleInHangarEvent(_save);

		if (EVENT_IN_HANGAR->request) { traceLog
			extendedDebugLogFmt("[NY_Event][WARNING]: EVENT_IN_HANGAR - handleInHangarEvent: error %d\n", EVENT_IN_HANGAR->request);
		}

		break;
		// An error occurred
	default: traceLog
		extendedDebugLog("[NY_Event][ERROR]: IN_HANGAR - something wrong with WaitResult!\n");

		return 3;
	} traceLog
	if (first_check) { traceLog
		extendedDebugLogFmt("[NY_Event][ERROR]: IN_HANGAR - Error %d!\n", (uint32_t)first_check);

		return 4;
	}
	
	Py_BLOCK_THREADS;

	EVENT_IN_HANGAR->request = showMessage(g_self->i18n);

	if (EVENT_IN_HANGAR->request) {
		extendedDebugLogFmt("[NY_Event][WARNING]: handleInHangarEvent - showMessage: error %d\n", EVENT_IN_HANGAR->request);
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
		extendedDebugLogFmt("[NY_Event][ERROR]: Creating timer thread: error %d\n", GetLastError());

		return 5;
	} traceLog

	HANDLE hEvents[HEVENTS_COUNT] = {
		EVENT_DEL_MODEL->hEvent,  //событие удаления модели
		EVENT_BATTLE_ENDED->hEvent //событие выхода из боя
	};

	uint8_t lastEventError = NULL;

	while (!first_check && !battleEnded && !lastEventError) { traceLog
	superExtendedDebugLog("[NY_Event]: waiting EVENTS\n");

		DWORD EVENTS_WaitResult = WaitForMultipleObjects(
			HEVENTS_COUNT,
			hEvents,
			FALSE,
			INFINITE);

		switch (EVENTS_WaitResult) {
		case WAIT_OBJECT_0 + 0:  traceLog //сработало событие удаления модели
			superExtendedDebugLog("[NY_Event]: DEL_LAST_MODEL signaled!\n");

			//место для рабочего кода

			EVENT_DEL_MODEL->request = handleDelModelEvent(_save);

			if (EVENT_DEL_MODEL->request) {
				extendedDebugLogFmt("[NY_Event][WARNING]: DEL_LAST_MODEL - handleDelModelEvent: error %d!\n", EVENT_DEL_MODEL->request);

				break;
			}

			break;
		case WAIT_OBJECT_0 + 1: traceLog //сработало событие окончания боя
			isTimerStarted = false;

			Py_BLOCK_THREADS;

			EVENT_BATTLE_ENDED->request = handleBattleEndEvent(_save);

			if (!EVENT_BATTLE_ENDED->request) { traceLog
				battleEnded = true;

				current_map.stageID = STAGE_ID::COMPETITION;

				PyObject* delLabelCBID_p = GUI_getAttr("delLabelCBID");

				if (!delLabelCBID_p || delLabelCBID_p == Py_None) { traceLog
					delLabelCBID = NULL;

					Py_XDECREF(delLabelCBID_p);
				}
				else { traceLog
					delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);

					Py_DECREF(delLabelCBID_p);
				} traceLog

				BW_Native->cancelCallback(&delLabelCBID);

				allModelsCreated = NULL;

				GUI_setVisible(false);
				GUI_clearText();

				mapID   = NULL;
			}

			Py_UNBLOCK_THREADS;

			break;
			// An error occurred
		default: traceLog
			extendedDebugLog("[NY_Event][WARNING]: EVENTS - something wrong with WaitResult!\n");

			lastEventError = 1;

			break;
		} traceLog
	} traceLog

	if (lastEventError) extendedDebugLogFmt("[NY_Event][WARNING]: Error in event: %d\n", lastEventError);

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

	//закрываем поток

	extendedDebugLogFmt("Closing handler thread %d\n", handlerThreadID);

	Py_BLOCK_THREADS;

	//выключаем GIL для этого потока

	PyGILState_Release(gstate);

	//------------------------------

	return NULL;
}

DWORD WINAPI HandlerThread(LPVOID lpParam) {
	UNREFERENCED_PARAMETER(lpParam);

	uint32_t result = handlerThread();

	hHandlerThread  = NULL;
	handlerThreadID = NULL;

	return result;
}

//-----------------

static PyObject* event_start(PyObject *self, PyObject *args) { traceLog
	if (!isInited || first_check) { traceLog
		return PyInt_FromSize_t(1);
	} traceLog

	INIT_LOCAL_MSG_BUFFER;

	PyObject* __player = PyString_FromString("player");

	PyObject_CallMethodObjArgs_increfed(player, BW_Native->m_BigWorld, __player, NULL);

	Py_DECREF(__player);

	isModelsAlreadyCreated = false;

	if (!player) { traceLog
		return PyInt_FromSize_t(2);
	} traceLog

	PyObject* arena = PyObject_GetAttrString(player, "arena");

	Py_DECREF(player);

	if (!arena) { traceLog
		return PyInt_FromSize_t(3);
	} traceLog

	PyObject* arenaType = PyObject_GetAttrString(arena, "arenaType");

	Py_DECREF(arena);

	if (!arenaType) { traceLog
		return PyInt_FromSize_t(4);
	} traceLog

	PyObject* map_PS = PyObject_GetAttrString(arenaType, "geometryName");

	Py_DECREF(arenaType);

	if (!map_PS) { traceLog
		return PyInt_FromSize_t(5);
	} traceLog

	char* map_s = PyString_AS_STRING(map_PS);

	Py_DECREF(map_PS);

	char map_ID_s[4];
	memcpy(map_ID_s, map_s, 3);
	if (map_ID_s[2] == '_') map_ID_s[2] = NULL;
	map_ID_s[3] = NULL;

	mapID = atoi(map_ID_s);

	extendedDebugLogFmt("[NY_Event]: mapID = %d\n", (uint32_t)mapID);

	if (mapID != MAIN_COMPETITION_MAP && mapID != FUN_COMPETITION_MAP) {
		extendedDebugLogFmt("[NY_Event][ERROR]: incorrect map! (%d)\n", (uint32_t)mapID);

		return PyInt_FromSize_t(7);
	}

	battleEnded = false;

	GUI_setTimerVisible(true);
	GUI_setVisible(true);

	isTimeVisible = true;

	request = makeEventInThread(EVENT_ID::IN_BATTLE_GET_FULL);

	if (request) { traceLog
		debugLogFmt("[NY_Event][ERROR]: start - error %d\n", request);

		return PyInt_FromSize_t(6);
	} traceLog

	Py_RETURN_NONE;
};

static PyObject* event_fini_py(PyObject *self, PyObject *args) { traceLog
	if (!EVENT_BATTLE_ENDED) { traceLog
		return PyInt_FromSize_t(1);
	} traceLog

	INIT_LOCAL_MSG_BUFFER;

	if (!SetEvent(EVENT_BATTLE_ENDED->hEvent)) { traceLog
		debugLogFmt("[NY_Event][ERROR]: EVENT_BATTLE_ENDED not setted!\n");

		return PyInt_FromSize_t(2);
	} traceLog

	Py_RETURN_NONE;
};

static PyObject* event_err_code(PyObject *self, PyObject *args) { traceLog
	return PyInt_FromSize_t(first_check);
};

bool createEventsAndSecondThread() { traceLog
	INIT_LOCAL_MSG_BUFFER;
	
	if (!createEvent1(&EVENT_IN_HANGAR,   EVENT_ID::IN_HANGAR)) { traceLog
		return false;
	} traceLog
	if (!createEvent1(&EVENT_START_TIMER, EVENT_ID::IN_BATTLE_GET_FULL)) { traceLog
		return false;
	} traceLog
	if (!createEvent1(&EVENT_DEL_MODEL,   EVENT_ID::DEL_LAST_MODEL)) { traceLog
		return false;
	} traceLog

	M_MODELS_NOT_USING  = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	if (!M_MODELS_NOT_USING) { traceLog
		debugLogFmt("[NY_Event][ERROR]: MODELS_NOT_USING creating: error %d\n", GetLastError());

		return false;
	}

	if (!createEvent2(&EVENT_ALL_MODELS_CREATED, L"NY_Event_AllModelsCreatedEvent")) { traceLog
		return false;
	} traceLog
	if (!createEvent2(&EVENT_BATTLE_ENDED,       L"NY_Event_BattleEndedEvent")) { traceLog
		return false;
	} traceLog

	//Handler thread creating

	if (hHandlerThread) { traceLog
		CloseHandle(hHandlerThread);

		hHandlerThread = NULL;
	} traceLog

	hHandlerThread = CreateThread( //создаем второй поток
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		HandlerThread,                          // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&handlerThreadID);                      // returns the thread identifier 

	if (!hHandlerThread) { traceLog
		debugLogFmt("[NY_Event][ERROR]: Handler thread creating: error %d\n", GetLastError());

		return false;
	} traceLog

	return true;
}

uint8_t event_сheck() { traceLog
	if (!isInited) { traceLog
		return 1;
	} traceLog

	// инициализация второго потока, если не существует, иначе - завершить второй поток и начать новый

	if (!createEventsAndSecondThread()) { traceLog
		return 2;
	} traceLog

	//------------------------------------------------------------------------------------------------

	INIT_LOCAL_MSG_BUFFER;

	debugLogFmt("[NY_Event]: checking...\n");

	PyObject* __player = PyString_FromString("player");

	PyObject_CallMethodObjArgs_increfed(player, BW_Native->m_BigWorld, __player, NULL);

	Py_DECREF(__player);

	if (!player) { traceLog
		return 3;
	} traceLog

	PyObject* DBID_string = PyObject_GetAttrString(player, "databaseID");

	Py_DECREF(player);

	if (!DBID_string) { traceLog
		return 4;
	} traceLog

	PyObject* DBID_int = PyNumber_Int(DBID_string);

	Py_DECREF(DBID_string);

	if (!DBID_int) { traceLog
		return 5;
	} traceLog

	databaseID = PyInt_AS_LONG(DBID_int);

	Py_DECREF(DBID_int);

	debugLogFmt("[NY_Event]: DBID created\n");

	mapID = NULL;

	battleEnded = false;

	request = makeEventInThread(EVENT_ID::IN_HANGAR);

	if (request) { traceLog
		return 6;
	}
	else {
		return NULL;
	} traceLog
}

static PyObject* event_сheck_py(PyObject *self, PyObject *args) { traceLog
	uint8_t res = event_сheck();

	if (res) { traceLog
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
};

uint8_t event_init(PyObject* template_, PyObject* apply, PyObject* byteify) { traceLog
	if (!template_ || !apply || !byteify) { traceLog
		return 1;
	} traceLog

	if (BW_Native->m_g_gui && PyCallable_Check(template_) && PyCallable_Check(apply)) { traceLog
		Py_INCREF(template_);
		Py_INCREF(apply);

		PyObject* __register = PyString_FromString("register");

		PyObject_CallMethodObjArgs_increfed(result, BW_Native->m_g_gui, __register, PyString_FromString(g_self->ids), template_, g_self->data, apply, NULL);

		Py_XDECREF(result);
		Py_DECREF(__register);
		Py_DECREF(apply);
		Py_DECREF(template_);
	} traceLog

	if (!BW_Native->m_g_gui && PyCallable_Check(byteify)) { traceLog
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

	if (BW_Native->m_g_gui) { //traceLog
		PyObject* __get_key = PyString_FromString("get_key");
		
		PyObject_CallMethodObjArgs_increfed(isKeyGetted_Space_tmp, BW_Native->m_g_gui, __get_key, spaceKey, NULL);

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

			debugLogFmt("[NY_Event][ERROR]: making DEL_LAST_MODEL: error %d\n", request);

			Py_RETURN_NONE;
		} traceLog
	} //traceLog

	Py_XDECREF(isKeyGetted_Space);

	Py_RETURN_NONE;
};

static struct PyMethodDef event_methods[] =
{
	{ "b",             event_сheck_py,                METH_VARARGS, ":P" }, //check
	{ "c",             event_start,                   METH_NOARGS,  ":P" }, //start
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

	BW_Native = new BW_NativeC();

	if (!BW_Native->inited) { traceLog
		debugLogFmt("[NY_Event][ERROR]: initevent - initNative: error %d!\n", BW_Native->lastError);

		return;
	}

	if (!initHangarMessages()) { traceLog
		debugLog("[NY_Event][ERROR]: initevent - initHangarMessages: error!\n");

		delete BW_Native;
		BW_Native = nullptr;

		return;
	}

	debugLog("[NY_Event]: Config init...\n");

	if (PyType_Ready(&Config_p)) { traceLog
		finiHangarMessages();
		
		delete BW_Native;
		BW_Native = nullptr;

		return;
	} traceLog

	Py_INCREF(&Config_p);

	debugLog("[NY_Event]: Config init OK\n");

	//загрузка конфига мода

	PyObject* g_config = PyObject_CallObject((PyObject*)&Config_p, NULL);

	Py_DECREF(&Config_p);

	if (!g_config || !g_self) { traceLog
		finiHangarMessages();

		delete BW_Native;
		BW_Native = nullptr;

		return;
	} traceLog

	//инициализация модуля

	event_module = Py_InitModule3("event",
		event_methods,
		event_methods__doc__);

	if (!event_module) { traceLog
		finiHangarMessages();

		delete BW_Native;
		BW_Native = nullptr;
		
		return;
	} traceLog

	if (PyModule_AddObject(event_module, "l", g_config)) { traceLog
		finiHangarMessages();

		delete BW_Native;
		BW_Native = nullptr;
		
		return;
	} traceLog

	//получение указателя на метод модуля onModelCreated

	onModelCreatedPyMeth = PyObject_GetAttrString(event_module, "omc");

	if (!onModelCreatedPyMeth) { traceLog
		finiHangarMessages();

		delete BW_Native;
		BW_Native = nullptr;

		return;
	} traceLog

	//Space key

	spaceKey = PyList_New(1);

	if (spaceKey) { traceLog
		PyList_SET_ITEM(spaceKey, 0, PyInt_FromSize_t(57));
	} traceLog

	//загрузка modGUI

	debugLog("Mod_GUI module loading...\n");

	PyObject* mGUI_module = PyImport_ImportModule("NY_Event.native.mGUI");

	if (!mGUI_module) { traceLog
		finiHangarMessages();

		delete BW_Native;
		BW_Native = nullptr;

		return;
	} traceLog

	debugLog("[NY_Event]: Mod_GUI class loading...\n");

	PyObject* __Mod_GUI = PyString_FromString("Mod_GUI");

	modGUI = PyObject_CallMethodObjArgs(mGUI_module, __Mod_GUI, NULL);
	
	Py_DECREF(__Mod_GUI);
	Py_DECREF(mGUI_module);

	if (!modGUI) { traceLog
		finiHangarMessages();

		delete BW_Native;
		BW_Native = nullptr;

		return;
	} traceLog

	debugLog("[NY_Event]: Mod_GUI class loaded OK!\n");

	//загрузка g_gui

	debugLog("[NY_Event]: g_gui module loading...\n");

	PyObject* mod_mods_gui = PyImport_ImportModule("gui.mods.mod_mods_gui");

	if (!mod_mods_gui) { traceLog
		PyErr_Clear();

		delete BW_Native;
		BW_Native = nullptr;

		debugLog("[NY_Event]: mod_mods_gui is NULL!\n");
	}
	else {
		m_g_gui = PyObject_GetAttrString(mod_mods_gui, "g_gui");
		Py_DECREF(mod_mods_gui);

		if (!m_g_gui) { traceLog
			finiHangarMessages();

			delete BW_Native;
			BW_Native = nullptr;

			Py_DECREF(modGUI);
			return;
		} traceLog

		debugLog("[NY_Event]: mod_mods_gui loaded OK!\n");
	} traceLog

	if (!m_g_gui) { traceLog
		_mkdir("mods/configs");
		_mkdir("mods/configs/pavel3333");
		_mkdir("mods/configs/pavel3333/NY_Event");
		_mkdir("mods/configs/pavel3333/NY_Event/i18n");

		if (!read_data(true) || !read_data(false)) { traceLog
			finiHangarMessages();

			delete BW_Native;
			BW_Native = nullptr;

			Py_DECREF(modGUI);
			return;
		} traceLog
	}
	else {
		PyObject* ids = PyString_FromString(g_self->ids);

		if (!ids) { traceLog
			finiHangarMessages();

			delete BW_Native;
			BW_Native = nullptr;

			Py_DECREF(modGUI);
			return;
		} traceLog

		PyObject* __register_data = PyString_FromString("register_data");
		PyObject* __pavel3333 = PyString_FromString("pavel3333");
		PyObject* data_i18n = PyObject_CallMethodObjArgs(m_g_gui, __register_data, ids, g_self->data, g_self->i18n, __pavel3333, NULL);

		Py_DECREF(__pavel3333);
		Py_DECREF(__register_data);
		Py_DECREF(ids);

		if (!data_i18n) { traceLog
			finiHangarMessages();

			delete BW_Native;
			BW_Native = nullptr;

			Py_DECREF(modGUI);
			return;
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
		finiHangarMessages();

		delete BW_Native;
		BW_Native = nullptr;

		debugLogFmt("[NY_Event][ERROR]: Initialising CURL handle: error %d\n", curl_init_result);

		return;
	} traceLog

	isInited = true;
};
