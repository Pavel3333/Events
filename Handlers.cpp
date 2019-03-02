#include "Handlers.h"
#include "MyLogger.h"


INIT_LOCAL_MSG_BUFFER;


//обработка событий

uint8_t handleBattleEvent(PyThreadState *_save)
{
	traceLog
	
	if (!isInited || first_check || battleEnded || !EVENT_START_TIMER) { traceLog
		return 1;
	} traceLog

	extendedDebugLog("parsing...");

	g_models_mutex.lock();
	superExtendedDebugLog("MODELS_USING");

	EVENT_START_TIMER->request = parse_event_threadsafe(EVENT_START_TIMER->eventID);

	if (EVENT_START_TIMER->request) { traceLog
		extendedDebugLogEx(ERROR, "parsing FAILED! Error code: %d", EVENT_START_TIMER->request);

		//GUI_setError(parsing_result);

		return 4;
	} traceLog

	superExtendedDebugLog("MODELS_NOT_USING");
	g_models_mutex.unlock();

	extendedDebugLog("parsing OK!");

	if (current_map.time_preparing) {
		Py_BLOCK_THREADS;

		GUI_setTime(current_map.time_preparing); //выводим время

		Py_UNBLOCK_THREADS;
	}

	if (current_map.stageID >= STAGES_COUNT) { traceLog
		extendedDebugLogEx(ERROR, "StageID is incorrect!");

		return 3;
	}

	if (
		current_map.stageID != STAGE_ID::WAITING      &&
		current_map.stageID != STAGE_ID::START        &&
		current_map.stageID != STAGE_ID::COMPETITION  &&
		current_map.stageID != STAGE_ID::END_BY_TIME  &&
		current_map.stageID != STAGE_ID::END_BY_COUNT &&
		current_map.stageID != STAGE_ID::STREAMER_MODE
		) { traceLog
		extendedDebugLogEx(ERROR, "StageID is not right for this event!");
		
		return 4;
	}

	Py_BLOCK_THREADS;

	if (lastStageID != STAGE_ID::GET_SCORE && lastStageID != STAGE_ID::ITEMS_NOT_EXISTS) GUI_setMsg(current_map.stageID); //выводим нужное сообщение

	if (current_map.stageID == STAGE_ID::END_BY_TIME || current_map.stageID == STAGE_ID::END_BY_COUNT) { traceLog
		current_map.time_preparing = NULL;

		GUI_setTime(NULL);

		if (current_map.stageID == STAGE_ID::END_BY_COUNT) { traceLog
			GUI_setTimerVisible(false);

			isTimeVisible = false;
		} traceLog

		GUI_setMsg(current_map.stageID);

		EVENT_START_TIMER->request = handleBattleEndEvent(_save);

		if (EVENT_START_TIMER->request) { traceLog
			extendedDebugLog("Warning - handle_battle_event - event_fini - Error code %d", EVENT_START_TIMER->request);

			//GUI_setWarning(event_result);
		} traceLog
	}
	else {
		if (!isTimeVisible) { traceLog
			GUI_setTimerVisible(true);

			isTimeVisible = true;
		} traceLog
	} traceLog

	Py_UNBLOCK_THREADS;

	if (current_map.stageID == STAGE_ID::START ||
		current_map.stageID == STAGE_ID::COMPETITION ||
		current_map.stageID == STAGE_ID::STREAMER_MODE) { traceLog
		if (isModelsAlreadyCreated && !isModelsAlreadyInited && current_map.minimap_count && current_map.modelsSects.size()) { traceLog
			if (EVENT_START_TIMER->eventID == EVENT_ID::IN_BATTLE_GET_FULL) { traceLog
				superExtendedDebugLog("sect count: %u\npos count: %u", current_map.modelsSects.size(), current_map.minimap_count);

				extendedDebugLog("creating...");

				g_models_mutex.lock();
				superExtendedDebugLog("MODELS_USING");

				// Что это ???
				// Может models.clear() стоит ?
				models.~vector();
				//lights.~vector();

				superExtendedDebugLog("MODELS_NOT_USING");
				g_models_mutex.unlock();

				Py_BLOCK_THREADS;

				/*
				Первый способ - нативный вызов в main-потоке добавлением в очередь. Ненадёжно!

				int creating_result = Py_AddPendingCall(&create_models, nullptr); //create_models();

				if (creating_result == -1) { traceLog
					extendedDebugLogEx(ERROR, "IN_BATTLE_GET_FULL - create_models - failed to start PendingCall of creating models!");

					return 3;
				} traceLog
				*/

				/*
				Второй способ - вызов асинхронной функции BigWorld.fetchModel(path, onLoadedMethod)

				Более-менее надежно, выполняется на уровне движка
				*/

				EVENT_START_TIMER->request = create_models();

				Py_UNBLOCK_THREADS;

				if (EVENT_START_TIMER->request) { traceLog
					extendedDebugLogEx(ERROR, "IN_BATTLE_GET_FULL - handleBattleEvent - Error code %d", EVENT_START_TIMER->request);

					return 7;
				} traceLog

				//ожидаем события полного создания моделей

				superExtendedDebugLog("waiting EVENT_ALL_MODELS_CREATED");

				DWORD EVENT_ALL_MODELS_CREATED_WaitResult = WaitForSingleObject(
					EVENT_ALL_MODELS_CREATED->hEvent, // event handle
					INFINITE);                        // indefinite wait

				switch (EVENT_ALL_MODELS_CREATED_WaitResult) {
				case WAIT_OBJECT_0:  traceLog
					superExtendedDebugLog("EVENT_ALL_MODELS_CREATED signaled!");

					//место для рабочего кода

					Py_BLOCK_THREADS;

					extendedDebugLog("creating OK!");

					EVENT_START_TIMER->request = init_models();

					if (EVENT_START_TIMER->request) { traceLog
						extendedDebugLogEx(ERROR, "IN_BATTLE_GET_FULL - init_models: error %d", EVENT_START_TIMER->request);

						//GUI_setError(EVENT_START_TIMER->request);

						Py_UNBLOCK_THREADS;

						return 8;
					} traceLog

					EVENT_START_TIMER->request = set_visible(true);

					if (EVENT_START_TIMER->request) { traceLog
						extendedDebugLogEx(WARNING, "IN_BATTLE_GET_FULL - set_visible: error %d", EVENT_START_TIMER->request);

						//GUI_setWarning(EVENT_START_TIMER->request);
						
						Py_UNBLOCK_THREADS;

						return 11;
					} traceLog

					isModelsAlreadyInited = true;

					Py_UNBLOCK_THREADS;

					break;

					// An error occurred
				default: traceLog
					extendedDebugLogEx(ERROR, "IN_HANGAR - something wrong with WaitResult!");

					Py_BLOCK_THREADS;

					return 10;
				} traceLog
			} traceLog

			return NULL;
		} traceLog
	} traceLog

	if (isModelsAlreadyCreated && isModelsAlreadyInited && EVENT_START_TIMER->eventID == EVENT_ID::IN_BATTLE_GET_SYNC && sync_map.all_models_count && !sync_map.modelsSects_deleting.empty()) { traceLog
		std::vector<ModelsSection>::iterator it_sect_sync;
		std::vector<float*>::iterator        it_model;

		g_models_mutex.lock();
		superExtendedDebugLog("MODELS_USING");

		it_sect_sync = sync_map.modelsSects_deleting.begin();

		superExtendedDebugLog("SYNC - %d models sections to delete", sync_map.modelsSects_deleting.size());

		Py_BLOCK_THREADS;

		while (it_sect_sync != sync_map.modelsSects_deleting.end()) { traceLog
			if (it_sect_sync->isInitialised) {
				superExtendedDebugLog("SYNC - %d models to delete in section %d", it_sect_sync->models.size(), it_sect_sync->ID);

				it_model = it_sect_sync->models.begin();

				while (it_model != it_sect_sync->models.end()) { traceLog
					if (*it_model == nullptr) { traceLog
						extendedDebugLog("WARNING, handleBattleEvent - *it_model is NULL!%d");

						it_model = it_sect_sync->models.erase(it_model);
										
						continue;
					}

					EVENT_START_TIMER->request = delModelPy(*it_model);

					if (EVENT_START_TIMER->request) { traceLog
						extendedDebugLogEx(WARNING, "handleBattleEvent - delModelPy - Error code %d", EVENT_START_TIMER->request);

						//GUI_setError(EVENT_START_TIMER->request);

						it_model++;

						continue;
					}

					delete[] *it_model;
					*it_model = nullptr;

					it_model = it_sect_sync->models.erase(it_model);

					/*

					EVENT_START_TIMER->request = delModelCoords(it_sect_sync->ID, *it_model);

					if (EVENT_START_TIMER->request) { traceLog
						extendedDebugLogEx(ERROR, "handleBattleEvent - delModelCoords - Error code %d", EVENT_START_TIMER->request);

						//GUI_setError(res);

						it_model++;

						continue;
					}

					*/
				}
			}
							
			superExtendedDebugLog("SYNC - %d models after deleting", sync_map.modelsSects_deleting.size());

			if (it_sect_sync->path != nullptr) {
				delete[] it_sect_sync->path;

				it_sect_sync->path = nullptr;
			}

			it_sect_sync->models.~vector();

			it_sect_sync = sync_map.modelsSects_deleting.erase(it_sect_sync); //удаляем секцию из вектора секций синхронизации
		} traceLog

		Py_UNBLOCK_THREADS;

		sync_map.modelsSects_deleting.~vector();

		superExtendedDebugLog("MODELS_NOT_USING");
		g_models_mutex.unlock();
	}
	else { traceLog
		return NULL;
	} traceLog

	return NULL;
}

uint8_t handleStartTimerEvent(PyThreadState* _save)
{
	if (!isInited || !EVENT_START_TIMER) {
		extendedDebugLogEx(ERROR, "handleStartTimerEvent - isInited or EVENT_START_TIMER is NULL!");

		return 1;
	}

	BOOL            bSuccess;
	__int64         qwDueTime;
	LARGE_INTEGER   liDueTime;

	if (EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_FULL && EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog
		extendedDebugLogEx(ERROR, "handleStartTimerEvent - eventID not equal!");

		return 2;
	} traceLog

	if (first_check || battleEnded) { traceLog
		extendedDebugLogEx(ERROR, "handleStartTimerEvent - first_check or battleEnded!");

		return 3;
	} traceLog

	//инициализация таймера для получения полного списка моделей и синхронизации

	hBattleTimer = CreateWaitableTimer(
		NULL,                   // Default security attributes
		FALSE,                  // Create auto-reset timer
		TEXT("NY_Event_BattleTimer"));   // Name of waitable timer

	if (hBattleTimer) { traceLog
		qwDueTime = 0; // задержка перед созданием таймера - 0 секунд

		// Copy the relative time into a LARGE_INTEGER.
		liDueTime.LowPart = (DWORD)NULL;//(DWORD)(qwDueTime & 0xFFFFFFFF);
		liDueTime.HighPart = (LONG)NULL;//(qwDueTime >> 32);

		bSuccess = SetWaitableTimer(
			hBattleTimer,           // Handle to the timer object
			&liDueTime,       // When timer will become signaled
			1000,             // Periodic timer interval of 1 seconds
			TimerAPCProc,     // Completion routine
			NULL,             // Argument to the completion routine
			FALSE);           // Do not restore a suspended system

		if (bSuccess)
		{
			while (!first_check && !battleEnded && !battleTimerLastError) { traceLog
				if (!isTimerStarted) { traceLog
					isTimerStarted = true;
				} traceLog

				//рабочая часть

				EVENT_START_TIMER->eventID = EVENT_ID::IN_BATTLE_GET_FULL;

				if (isModelsAlreadyCreated && isModelsAlreadyInited) EVENT_START_TIMER->eventID = EVENT_ID::IN_BATTLE_GET_SYNC;

				EVENT_START_TIMER->request = send_token_threadsafe(databaseID, mapID, EVENT_START_TIMER->eventID, MODEL_ID::BALL);

				if (EVENT_START_TIMER->request) { traceLog
					if (EVENT_START_TIMER->request > 9) { traceLog
						extendedDebugLogEx(ERROR, "handleStartTimerEvent - send_token_threadsafe - Error code %d", EVENT_START_TIMER->request);
								
						//GUI_setError(EVENT_START_TIMER->request);
								
						battleTimerLastError = 1;

						break;
					} traceLog

					extendedDebugLogEx(WARNING, "handleStartTimerEvent - send_token_threadsafe - Warning code %d", EVENT_START_TIMER->request);
				} traceLog

				superExtendedDebugLog("generating token OK!");

				EVENT_START_TIMER->request = handleBattleEvent(_save);

				if (EVENT_START_TIMER->request) { traceLog
					extendedDebugLogEx(WARNING, "handleStartTimerEvent - create_models: error %d", EVENT_START_TIMER->request);

					//GUI_setError(EVENT_START_TIMER->request);
				} traceLog

				SleepEx(
					INFINITE,     // Wait forever
					TRUE);        // Put thread in an alertable state
			} traceLog

			if (battleTimerLastError) { traceLog
				extendedDebugLogEx(WARNING, "handleStartTimerEvent: error %d", battleTimerLastError);

				CancelWaitableTimer(hBattleTimer);
			} traceLog
		}
		else extendedDebugLogEx(ERROR, "handleStartTimerEvent - SetWaitableTimer: error %d", GetLastError());

		CloseHandle(hBattleTimer);
		
		hBattleTimer = NULL;
	}
	else extendedDebugLogEx(ERROR, "handleStartTimerEvent: CreateWaitableTimer: error %d", GetLastError());

	return battleTimerLastError;
}

uint8_t handleInHangarEvent(PyThreadState* _save) {
	if (!isInited || !EVENT_IN_HANGAR) {
		extendedDebugLogEx(ERROR, "handleInHangarEvent - isInited or EVENT_IN_HANGAR is NULL!");

		return 1;
	}

	if (EVENT_IN_HANGAR->eventID != EVENT_ID::IN_HANGAR) { traceLog
		extendedDebugLogEx(ERROR, "handleInHangarEvent - eventID not equal!");

		return 2;
	} traceLog

	BOOL            bSuccess;
	__int64         qwDueTime;
	LARGE_INTEGER   liDueTime;

	//рабочая часть

	hHangarTimer = CreateWaitableTimer(
		NULL,                   // Default security attributes
		FALSE,                  // Create auto-reset timer
		TEXT("NY_Event_HangarTimer"));   // Name of waitable timer

	if (hHangarTimer) { traceLog
		qwDueTime = 0; // задержка перед созданием таймера - 0 секунд

		// Copy the relative time into a LARGE_INTEGER.
		liDueTime.LowPart  = (DWORD)NULL;//(DWORD)(qwDueTime & 0xFFFFFFFF);
		liDueTime.HighPart = (LONG)NULL; //(qwDueTime >> 32);

		bSuccess = SetWaitableTimer(
			hHangarTimer,     // Handle to the timer object
			&liDueTime,       // When timer will become signaled
			15000,            // Periodic timer interval of 15 seconds
			TimerAPCProc,     // Completion routine
			NULL,             // Argument to the completion routine
			FALSE);           // Do not restore a suspended system

		if (bSuccess)
		{
			while (first_check && !hangarTimerLastError) {
				EVENT_IN_HANGAR->request = send_token_threadsafe(databaseID, mapID, EVENT_IN_HANGAR->eventID, MODEL_ID::BALL);

				if (EVENT_IN_HANGAR->request) {
					extendedDebugLogEx(ERROR, "handleInHangarEvent - send_token_threadsafe: error %d!", EVENT_IN_HANGAR->request);

					hangarTimerLastError = 5;

					break;
				}

				first_check = parse_event_threadsafe(EVENT_ID::IN_HANGAR);

				if (first_check) { traceLog
					if (first_check > 9) { traceLog
						extendedDebugLogEx(ERROR, "handleInHangarEvent - Error code %d", first_check);

						//GUI_setError(first_check);

						hangarTimerLastError = 2;

						break;
					}
					else {
						extendedDebugLogEx(WARNING, "handleInHangarEvent - Warning code %d", first_check);

						//GUI_setWarning(first_check);
					} traceLog

					Py_BLOCK_THREADS;

					if (auto err = HangarMessages::showMessage(PyConfig::g_self->i18n)) {
						extendedDebugLogEx(WARNING, "handleInHangarEvent - showMessage: error %d", err);
					}

					Py_UNBLOCK_THREADS;
				} traceLog

				SleepEx(
					INFINITE,     // Wait forever
					TRUE);        // Put thread in an alertable state
			} traceLog

			if (hangarTimerLastError) { traceLog
				extendedDebugLogEx(WARNING, "handleInHangarEvent: error %d", hangarTimerLastError);

				CancelWaitableTimer(hHangarTimer);
			} traceLog
		}
		else extendedDebugLogEx(ERROR, "handleInHangarEvent - SetWaitableTimer: error %d", GetLastError());

		CloseHandle(hHangarTimer);

		hHangarTimer = NULL;
	}
	else extendedDebugLogEx(ERROR, "handleInHangarEvent: CreateWaitableTimer: error %d", GetLastError());

	return hangarTimerLastError;
}

uint8_t handleBattleEndEvent(PyThreadState* _save)
{
	traceLog
	
	if (!isInited || first_check) { traceLog
		return 1;
	} traceLog

	Py_UNBLOCK_THREADS;

	std::vector<ModModel*>::iterator it_model;
	std::vector<ModLight*>::iterator it_light;

	g_models_mutex.lock();
	superExtendedDebugLog("MODELS_USING");

	Py_BLOCK_THREADS;

	if (!models.empty()) { traceLog
		EVENT_BATTLE_ENDED->request = del_models();

		if (EVENT_BATTLE_ENDED->request) { traceLog
			extendedDebugLogEx(WARNING, "handleBattleEndEvent - del_models: %d", EVENT_BATTLE_ENDED->request);
		} traceLog

		it_model = models.begin();

		while (it_model != models.end()) {
			if (*it_model == nullptr) { traceLog
				extendedDebugLogEx(WARNING, "handleBattleEndEvent - *it_model is NULL!%d");

				it_model = models.erase(it_model);

				continue;
			}

			Py_XDECREF((*it_model)->model);

			(*it_model)->model = NULL;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			it_model++;
		} traceLog

		models.~vector();
	} traceLog

	/*if (!lights.empty()) { traceLog
			it_light = lights.begin();

			while (it_light != lights.end()) {
				if (*it_light == nullptr) { traceLog
					extendedDebugLogEx(WARNING, "handleBattleEndEvent - *it_light is NULL!%d");

					it_light = lights.erase(it_light);

					continue;
				}

				Py_XDECREF((*it_light)->model);

				(*it_light)->model = NULL;
				(*it_light)->coords = nullptr;

				delete *it_light;
				*it_light = nullptr;

				it_light = lights.erase(it_light);
			} traceLog

			lights.~vector();
		} traceLog*/

	isModelsAlreadyInited = false;

	Py_UNBLOCK_THREADS;

	if (!current_map.modelsSects.empty() && current_map.minimap_count) { traceLog
		clearModelsSections();
	} traceLog

	isModelsAlreadyCreated = false;

	current_map.minimap_count = NULL;

	superExtendedDebugLog("MODELS_NOT_USING");
	g_models_mutex.unlock();

	Py_BLOCK_THREADS;

	if (isTimeVisible) { traceLog
		GUI_setTime(NULL);
		GUI_setTimerVisible(false);

		isTimeVisible = false;

		current_map.time_preparing = NULL;
	} traceLog

	extendedDebugLog("fini OK!");

	return NULL;
};

uint8_t handleDelModelEvent(PyThreadState* _save) { traceLog
	if (EVENT_DEL_MODEL->eventID != EVENT_ID::DEL_LAST_MODEL) { traceLog
		extendedDebugLogEx(ERROR, "DEL_LAST_MODEL - eventID not equal!");

		return 1;
	} traceLog

	//рабочая часть

	INIT_LOCAL_MSG_BUFFER;

	MODEL_ID modelID;
	float* coords = new float[3];

	g_models_mutex.lock();
	superExtendedDebugLog("MODELS_USING");

	Py_BLOCK_THREADS;

	auto err = BigWorldUtils::getLastModelCoords(5.0, &modelID, &coords);

	Py_UNBLOCK_THREADS;

	superExtendedDebugLog("MODELS_NOT_USING");
	g_models_mutex.unlock();

	if      (!err) { traceLog
		EVENT_DEL_MODEL->request = send_token_threadsafe(databaseID, mapID, EVENT_ID::DEL_LAST_MODEL, modelID, coords);
		
		if (EVENT_DEL_MODEL->request) { traceLog
			if (EVENT_DEL_MODEL->request > 9) { traceLog
				extendedDebugLogEx(ERROR, "DEL_LAST_MODEL - send_token_threadsafe - Error code %d", EVENT_DEL_MODEL->request);

				//GUI_setError(EVENT_DEL_MODEL->request);
							
				return 4;
			} traceLog

			extendedDebugLogEx(WARNING, "DEL_LAST_MODEL - send_token_threadsafe - Warning code %d", EVENT_DEL_MODEL->request);

			//GUI_setWarning(EVENT_DEL_MODEL->request);
		} traceLog

		EVENT_DEL_MODEL->request = parse_event_threadsafe(EVENT_ID::DEL_LAST_MODEL);

		if (EVENT_DEL_MODEL->request) { traceLog
			if (EVENT_DEL_MODEL->request > 9) { traceLog
				extendedDebugLogEx(ERROR, "DEL_LAST_MODEL - parse_event_threadsafe - Error code %d", EVENT_DEL_MODEL->request);

				//GUI_setError(EVENT_DEL_MODEL->request);

				return 5;
			}
			extendedDebugLogEx(WARNING, "DEL_LAST_MODEL - parse_event_threadsafe - Warning code %d", EVENT_DEL_MODEL->request);

			//GUI_setWarning(EVENT_DEL_MODEL->request);

			return 6;
		} traceLog

		g_models_mutex.lock();
		superExtendedDebugLog("MODELS_USING");

		Py_BLOCK_THREADS;

		EVENT_DEL_MODEL->request = delModelPy(coords);

		Py_UNBLOCK_THREADS;

		if (EVENT_DEL_MODEL->request) { traceLog
			extendedDebugLogEx(ERROR, "DEL_LAST_MODEL - delModelPy - Error code %d", EVENT_DEL_MODEL->request);

			//GUI_setError(EVENT_DEL_MODEL->request);

			return 8;
		} traceLog

		scoreID = (int8_t)modelID;
		current_map.stageID = STAGE_ID::GET_SCORE;

		delete[] coords;

		/*
		EVENT_DEL_MODEL->request = delModelCoords(modelID, coords);

		if (EVENT_DEL_MODEL->request) { traceLog
			extendedDebugLogEx(ERROR, "DEL_LAST_MODEL - delModelCoords - Error code %d", EVENT_DEL_MODEL->request);

			//GUI_setError(EVENT_DEL_MODEL->request);

			return 10;
		} traceLog

		*/

		superExtendedDebugLog("MODELS_NOT_USING");
		g_models_mutex.unlock();

	}
	else if (err > 0) { traceLog
		extendedDebugLog("DEL_LAST_MODEL - Model not found!");

		current_map.stageID = STAGE_ID::ITEMS_NOT_EXISTS;
	}
	else { traceLog
		extendedDebugLogEx(ERROR, "DEL_LAST_MODEL - getLastModelCoords - error %d!", err);

		return 13;
	}

	Py_BLOCK_THREADS;

	if (current_map.stageID == STAGE_ID::GET_SCORE && scoreID != -1) { traceLog
		GUI_setMsg(current_map.stageID, 5.0f, scoreID);

		scoreID = -1;
	}
	else if (current_map.stageID == STAGE_ID::ITEMS_NOT_EXISTS) { traceLog
		GUI_setMsg(current_map.stageID, 5.0f);
	} traceLog

	Py_UNBLOCK_THREADS;

	return NULL;
}

//заставить событие сигнализировать

uint8_t makeEventInThread(EVENT_ID eventID) { traceLog //переводим ивенты в сигнальные состояния
	if (!isInited || !databaseID || battleEnded) { traceLog
		return 1;
	} traceLog

	if (eventID == EVENT_ID::IN_HANGAR) { traceLog
		if (!EVENT_IN_HANGAR) { traceLog
			return 2;
		} traceLog

		EVENT_IN_HANGAR->eventID = eventID;

		if (!SetEvent(EVENT_IN_HANGAR->hEvent)) { traceLog
			debugLogEx(ERROR, "EVENT_IN_HANGAR not setted!");

			return 3;
		} traceLog
	}
	else if (eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog
		if (!EVENT_START_TIMER) { traceLog
			return 4;
		} traceLog

		EVENT_START_TIMER->eventID = eventID;

		if (!SetEvent(EVENT_START_TIMER->hEvent)) { traceLog
			debugLogEx(ERROR, "EVENT_START_TIMER not setted!");

			return 5;
		} traceLog
	}
	else if (eventID == EVENT_ID::DEL_LAST_MODEL) { traceLog
		if (!EVENT_DEL_MODEL) { traceLog
			return 6;
		} traceLog

		EVENT_DEL_MODEL->eventID = eventID;

		if (!SetEvent(EVENT_DEL_MODEL->hEvent)) { traceLog
			debugLogEx(ERROR, "EVENT_DEL_MODEL not setted!");

			return 7;
		} traceLog
	} 
	else { traceLog
		return 8;
	} traceLog

	return NULL;
}
