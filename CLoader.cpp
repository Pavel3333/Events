#include "pch.h"
#include "CLoader.h"
#include "Handlers.h"
#include "Py_config.h"
#include "Py_common.h"
#include "MyLogger.h"


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

	return 0;
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

	if (auto err = HangarMessages::showMessage()) {
		extendedDebugLogEx(WARNING, "showMessage: error %d", err);
	}

	Py_UNBLOCK_THREADS;

	if (hHangarTimer) { traceLog //закрываем таймер, если он был создан
		CancelWaitableTimer(hHangarTimer);
		CloseHandle(hHangarTimer);

		hHangarTimer = nullptr;
	} traceLog

	//вывод сообщения в центр уведомлений

	//создаем поток с таймером

	if (hBattleTimerThread) { traceLog
		WaitForSingleObject(hBattleTimerThread, INFINITE);
		
		hBattleTimerThread  = nullptr;
		battleTimerThreadID = 0;
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

	uint8_t lastEventError = 0;

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

			delLabelCBID_p = GUI::getAttr("delLabelCBID");

			if (!delLabelCBID_p || delLabelCBID_p == Py_None) { traceLog
				GUI::delLabelCBID = 0;

				Py_XDECREF(delLabelCBID_p);
			}
			else { traceLog
				GUI::delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);
				
				Py_DECREF(delLabelCBID_p);

				BigWorldUtils::cancelCallback(GUI::delLabelCBID);
			} traceLog

				GUI::delLabelCBID = 0;

			allModelsCreated = 0;

			GUI::setVisible(false);
			GUI::clearText();

			mapID   = 0;

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
		
		hBattleTimerThread  = nullptr;
		battleTimerThreadID = 0;
	}

	//освобождаем ивенты

	closeEvent(&EVENT_START_TIMER);
	closeEvent(&EVENT_IN_HANGAR);
	closeEvent(&EVENT_DEL_MODEL);

	closeEvent(&EVENT_ALL_MODELS_CREATED);
	closeEvent(&EVENT_BATTLE_ENDED);

	Py_BLOCK_THREADS;

	//выключаем GIL для этого потока

	PyGILState_Release(gstate);

	return 0;
}

//threads

DWORD WINAPI TimerThread(LPVOID lpParam) {
	UNREFERENCED_PARAMETER(lpParam);

	INIT_LOCAL_MSG_BUFFER;

	DWORD result = timerThread();

	hBattleTimerThread  = nullptr;
	battleTimerThreadID = 0;

	//закрываем поток

	extendedDebugLog("Closing timer thread %ld, result: %ld", battleTimerThreadID, result);

	return result;
}

DWORD WINAPI HandlerThread(LPVOID lpParam) {
	UNREFERENCED_PARAMETER(lpParam);

	INIT_LOCAL_MSG_BUFFER;

	DWORD result = 0;

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

	hHandlerThread  = nullptr;
	handlerThreadID = 0;

	//закрываем поток

	extendedDebugLog("Closing handler thread %ld", handlerThreadID);

	return result;
}

//C loaders

uint8_t event_start() {
	if (!isInited || first_check) { traceLog
		return 1;
	} traceLog

	isModelsAlreadyCreated = false;

	INIT_LOCAL_MSG_BUFFER;

	if (auto err = BigWorldUtils::getMapID(mapID)) {
		extendedDebugLogEx(ERROR, "getMapID: error %d!", err);
		return 2;
	}

	extendedDebugLog("mapID = %d", (uint32_t)mapID);

	if (mapID != MAIN_COMPETITION_MAP && mapID != FUN_COMPETITION_MAP) {
		extendedDebugLogEx(ERROR, "incorrect map! (%d)", (uint32_t)mapID);

		return 3;
	}

	battleEnded = false;

	GUI::setTimerVisible(true);
	GUI::setVisible(true);

	isTimeVisible = true;

	request = makeEventInThread(EVENT_ID::IN_BATTLE_GET_FULL);

	if (request) { traceLog
		debugLogEx(ERROR, "start - error %d", request);

		return 4;
	} traceLog

	return 0;
}

uint8_t event_check() { traceLog
	if (!isInited) { traceLog
		return 1;
	} traceLog

	INIT_LOCAL_MSG_BUFFER;

	debugLog("checking...");

	if (auto err = BigWorldUtils::getDBID(databaseID)) {
		debugLogEx(ERROR, "getDBID: error %d!", err);

		return 2;
	}

	debugLog("DBID created");

	mapID = 0;

	battleEnded = false;

	request = makeEventInThread(EVENT_ID::IN_HANGAR);

	if (request) { traceLog
		return 3;
	} traceLog

	return 0;
}

uint8_t event_init(PyObject* template_, PyObject* apply, PyObject* byteify) { traceLog
	if (!template_ || !apply || !byteify) { traceLog
		return 1;
	} traceLog

	if (PyConfig::m_g_gui && PyCallable_Check(template_) && PyCallable_Check(apply)) { traceLog
		Py_INCREF(template_);
		Py_INCREF(apply);

		PyObject* result = PyObject_CallMethod(PyConfig::m_g_gui, "register", "sOOO", PyConfig::g_self->ids, template_, PyConfig::g_self->data, apply);

		Py_XDECREF(result);
		Py_DECREF(apply);
		Py_DECREF(template_);
	} traceLog

	if (!PyConfig::m_g_gui && PyCallable_Check(byteify)) { traceLog
		PyObject* args = PyTuple_Pack(1, PyConfig::g_self->i18n);
		
		PyObject_CallObject_increfed(result, byteify, args);

		if (result) { traceLog
			PyDict_Clear(PyConfig::g_self->i18n);
			Py_DECREF(PyConfig::g_self->i18n);

			PyConfig::g_self->i18n = result;
		} traceLog
	} traceLog

	return 0;
}
