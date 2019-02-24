#include "Handlers.h"

//обработка событий

uint8_t handleBattleEvent(PyThreadState *_save) { traceLog
	if (!isInited || first_check || battleEnded || !M_MODELS_NOT_USING || !EVENT_START_TIMER) { traceLog
		return 1;
	} traceLog

	

	extendedDebugLog("parsing...\n");

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog
			superExtendedDebugLog("MODELS_USING\n");

			EVENT_START_TIMER->request = parse_event_threadsafe(EVENT_START_TIMER->eventID);

			if (EVENT_START_TIMER->request) { traceLog
				extendedDebugLogFmt("parsing FAILED! Error code: %d\n", EVENT_START_TIMER->request);

				//GUI_setError(parsing_result);

				RELEASE_MODELS("handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 5);

				return 4;
			} traceLog

			RELEASE_MODELS("handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 3);

			superExtendedDebugLog("MODELS_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog
			extendedDebugLog("handleBattleEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			return 2;
	END_USING_MODELS;

	extendedDebugLog("parsing OK!\n");

	if (current_map.time_preparing) {
		Py_BLOCK_THREADS;

		GUI_setTime(current_map.time_preparing); //выводим время

		Py_UNBLOCK_THREADS;
	}

	if (current_map.stageID >= STAGES_COUNT) { traceLog
		extendedDebugLog("StageID is incorrect!\n");

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
		extendedDebugLog("StageID is not right for this event!\n");
		
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
			extendedDebugLogFmt("Warning - handle_battle_event - event_fini - Error code %d\n", EVENT_START_TIMER->request);

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
				superExtendedDebugLogFmt("sect count: %u\npos count: %u\n", current_map.modelsSects.size(), current_map.minimap_count);

				extendedDebugLog("creating...\n");

				BEGIN_USING_MODELS;
					case WAIT_OBJECT_0: traceLog
						superExtendedDebugLog("MODELS_USING\n");

						models.~vector();
						//lights.~vector();

						RELEASE_MODELS("handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 6);

						superExtendedDebugLog("MODELS_NOT_USING\n");

						break;
					case WAIT_ABANDONED: traceLog
						extendedDebugLog("handleBattleEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

						return 5;
				END_USING_MODELS;
						
				Py_BLOCK_THREADS;

				/*
				Первый способ - нативный вызов в main-потоке добавлением в очередь. Ненадёжно!

				int creating_result = Py_AddPendingCall(&create_models, nullptr); //create_models();

				if (creating_result == -1) { traceLog
					extendedDebugLog("IN_BATTLE_GET_FULL - create_models - failed to start PendingCall of creating models!\n");

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
					extendedDebugLogFmt("IN_BATTLE_GET_FULL - handleBattleEvent - Error code %d\n", EVENT_START_TIMER->request);

					return 7;
				} traceLog

				//ожидаем события полного создания моделей

				superExtendedDebugLog("waiting EVENT_ALL_MODELS_CREATED\n");

				DWORD EVENT_ALL_MODELS_CREATED_WaitResult = WaitForSingleObject(
					EVENT_ALL_MODELS_CREATED->hEvent, // event handle
					INFINITE);                        // indefinite wait

				switch (EVENT_ALL_MODELS_CREATED_WaitResult) {
				case WAIT_OBJECT_0:  traceLog
					superExtendedDebugLog("EVENT_ALL_MODELS_CREATED signaled!\n");

					//место для рабочего кода

					Py_BLOCK_THREADS;

					extendedDebugLog("creating OK!\n");

					EVENT_START_TIMER->request = init_models();

					if (EVENT_START_TIMER->request) { traceLog
						LOG(ERROR) << "IN_BATTLE_GET_FULL - init_models: error " << EVENT_START_TIMER->request;
						//GUI_setError(EVENT_START_TIMER->request);

						Py_UNBLOCK_THREADS;

						return 8;
					} traceLog

					EVENT_START_TIMER->request = set_visible(true);

					if (EVENT_START_TIMER->request) { traceLog
						extendedDebugLogFmt("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - set_visible: error %d\n", EVENT_START_TIMER->request);

						//GUI_setWarning(EVENT_START_TIMER->request);
						
						Py_UNBLOCK_THREADS;

						return 11;
					} traceLog

					isModelsAlreadyInited = true;

					Py_UNBLOCK_THREADS;

					break;

					// An error occurred
				default: traceLog
					extendedDebugLog("IN_HANGAR - something wrong with WaitResult!\n");

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

		BEGIN_USING_MODELS;
			case WAIT_OBJECT_0: traceLog
				superExtendedDebugLog("MODELS_USING\n");

				it_sect_sync = sync_map.modelsSects_deleting.begin();

				superExtendedDebugLogFmt("SYNC - %d models sections to delete\n", sync_map.modelsSects_deleting.size());

				Py_BLOCK_THREADS;

				while (it_sect_sync != sync_map.modelsSects_deleting.end()) { traceLog
					if (it_sect_sync->isInitialised) {
						superExtendedDebugLogFmt("SYNC - %d models to delete in section %d\n", it_sect_sync->models.size(), it_sect_sync->ID);

						it_model = it_sect_sync->models.begin();

						while (it_model != it_sect_sync->models.end()) { traceLog
							if (*it_model == nullptr) { traceLog
								extendedDebugLog("[NY_Event][WARNING]: handleBattleEvent - *it_model is NULL!%d\n");

								it_model = it_sect_sync->models.erase(it_model);
										
								continue;
							}

							EVENT_START_TIMER->request = delModelPy(*it_model);

							if (EVENT_START_TIMER->request) { traceLog
								extendedDebugLogFmt("[NY_Event][WARNING]: handleBattleEvent - delModelPy - Error code %d\n", EVENT_START_TIMER->request);

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
								extendedDebugLogFmt("handleBattleEvent - delModelCoords - Error code %d\n", EVENT_START_TIMER->request);

								//GUI_setError(res);

								it_model++;

								continue;
							}

							*/
						}
					}
							
					superExtendedDebugLogFmt("SYNC - %d models after deleting\n", sync_map.modelsSects_deleting.size());

					if (it_sect_sync->path != nullptr) {
						delete[] it_sect_sync->path;

						it_sect_sync->path = nullptr;
					}

					it_sect_sync->models.~vector();

					it_sect_sync = sync_map.modelsSects_deleting.erase(it_sect_sync); //удаляем секцию из вектора секций синхронизации
				} traceLog

				Py_UNBLOCK_THREADS;

				sync_map.modelsSects_deleting.~vector();

				RELEASE_MODELS("handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 14);

				superExtendedDebugLog("MODELS_NOT_USING\n");

				break;
			case WAIT_ABANDONED: traceLog
				extendedDebugLog("handleBattleEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

				return 13;
			END_USING_MODELS;
	}
	else { traceLog
		return NULL;
	} traceLog

	return NULL;
}

uint8_t handleStartTimerEvent(PyThreadState* _save) {
	if (!isInited || !EVENT_START_TIMER) {
		extendedDebugLog("handleStartTimerEvent - isInited or EVENT_START_TIMER is NULL!\n");

		return 1;
	}

	

	BOOL            bSuccess;
	__int64         qwDueTime;
	LARGE_INTEGER   liDueTime;

	if (EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_FULL && EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog
		extendedDebugLog("handleStartTimerEvent - eventID not equal!\n");

		return 2;
	} traceLog

	if (first_check || battleEnded) { traceLog
		extendedDebugLog("handleStartTimerEvent - first_check or battleEnded!\n");

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

				EVENT_START_TIMER->request = send_token_threadsafe(databaseID, mapID, EVENT_START_TIMER->eventID);

				if (EVENT_START_TIMER->request) { traceLog
					if (EVENT_START_TIMER->request > 9) { traceLog
						extendedDebugLogFmt("handleStartTimerEvent - send_token_threadsafe - Error code %d\n", EVENT_START_TIMER->request);
								
						//GUI_setError(EVENT_START_TIMER->request);
								
						battleTimerLastError = 1;

						break;
					} traceLog

					extendedDebugLogFmt("[NY_Event][WARNING]: handleStartTimerEvent - send_token_threadsafe - Warning code %d\n", EVENT_START_TIMER->request);
				} traceLog

				superExtendedDebugLog("generating token OK!\n");

				EVENT_START_TIMER->request = handleBattleEvent(_save);

				if (EVENT_START_TIMER->request) { traceLog
					extendedDebugLogFmt("[NY_Event][WARNING]: handleStartTimerEvent - create_models: error %d\n", EVENT_START_TIMER->request);

					//GUI_setError(EVENT_START_TIMER->request);
				} traceLog

				SleepEx(
					INFINITE,     // Wait forever
					TRUE);        // Put thread in an alertable state
			} traceLog

			if (battleTimerLastError) { traceLog
				extendedDebugLogFmt("[NY_Event][WARNING]: handleStartTimerEvent: error %d\n", battleTimerLastError);

				CancelWaitableTimer(hBattleTimer);
			} traceLog
		}
		else extendedDebugLogFmt("handleStartTimerEvent - SetWaitableTimer: error %d\n", GetLastError());

		CloseHandle(hBattleTimer);
		
		hBattleTimer = NULL;
	}
	else extendedDebugLogFmt("handleStartTimerEvent: CreateWaitableTimer: error %d\n", GetLastError());

	return battleTimerLastError;
}

uint8_t handleInHangarEvent(PyThreadState* _save) {
	if (!isInited || !EVENT_IN_HANGAR) {
		extendedDebugLog("handleInHangarEvent - isInited or EVENT_IN_HANGAR is NULL!\n");

		return 1;
	}

	

	if (EVENT_IN_HANGAR->eventID != EVENT_ID::IN_HANGAR) { traceLog
		extendedDebugLog("handleInHangarEvent - eventID not equal!\n");

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
				EVENT_IN_HANGAR->request = send_token_threadsafe(databaseID, mapID, EVENT_IN_HANGAR->eventID);

				if (EVENT_IN_HANGAR->request) {
					extendedDebugLogFmt("handleInHangarEvent - send_token_threadsafe: error %d!\n", EVENT_IN_HANGAR->request);

					hangarTimerLastError = 5;

					break;
				}

				first_check = parse_event_threadsafe(EVENT_ID::IN_HANGAR);

				if (first_check) { traceLog
					if (first_check > 9) { traceLog
						extendedDebugLogFmt("handleInHangarEvent - Error code %d\n", first_check);

						//GUI_setError(first_check);

						hangarTimerLastError = 2;

						break;
					}
					else {
						extendedDebugLogFmt("[NY_Event][WARNING]: handleInHangarEvent - Warning code %d\n", first_check);

						//GUI_setWarning(first_check);
					} traceLog

					Py_BLOCK_THREADS;

					HangarMessages->showMessage(g_self->i18n);

					if (HangarMessages->lastError) {
						extendedDebugLogFmt("[NY_Event][WARNING]: handleInHangarEvent - showMessage: error %d\n", HangarMessages->lastError);
					}

					Py_UNBLOCK_THREADS;
				} traceLog

				SleepEx(
					INFINITE,     // Wait forever
					TRUE);        // Put thread in an alertable state
			} traceLog

			if (hangarTimerLastError) { traceLog
				extendedDebugLogFmt("[NY_Event][WARNING]: handleInHangarEvent: error %d\n", hangarTimerLastError);

				CancelWaitableTimer(hHangarTimer);
			} traceLog
		}
		else extendedDebugLogFmt("handleInHangarEvent - SetWaitableTimer: error %d\n", GetLastError());

		CloseHandle(hHangarTimer);

		hHangarTimer = NULL;
	}
	else extendedDebugLogFmt("handleInHangarEvent: CreateWaitableTimer: error %d\n", GetLastError());

	return hangarTimerLastError;
}

uint8_t handleBattleEndEvent(PyThreadState* _save) { traceLog
	if (!isInited || first_check || !M_MODELS_NOT_USING) { traceLog
		return 1;
	} traceLog

	

	Py_UNBLOCK_THREADS;

	std::vector<ModModel*>::iterator it_model;
	std::vector<ModLight*>::iterator it_light;

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog
			superExtendedDebugLog("MODELS_USING\n");

			Py_BLOCK_THREADS;

			if (!models.empty()) { traceLog
				EVENT_BATTLE_ENDED->request = del_models();

				if (EVENT_BATTLE_ENDED->request) { traceLog
					extendedDebugLogFmt("[NY_Event][WARNING]: handleBattleEndEvent - del_models: %d\n", EVENT_BATTLE_ENDED->request);
				} traceLog

				it_model = models.begin();

				while (it_model != models.end()) {
					if (*it_model == nullptr) { traceLog
						extendedDebugLog("[NY_Event][WARNING]: handleBattleEndEvent - *it_model is NULL!%d\n");

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
							extendedDebugLog("[NY_Event][WARNING]: handleBattleEndEvent - *it_light is NULL!%d\n");

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

			Py_UNBLOCK_THREADS;

			if (!current_map.modelsSects.empty() && current_map.minimap_count) { traceLog
				clearModelsSections();
			} traceLog

			isModelsAlreadyCreated = false;

			current_map.minimap_count = NULL;

			RELEASE_MODELS("handleBattleEndEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 3);

			superExtendedDebugLog("MODELS_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog
			extendedDebugLog("handleBattleEndEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			Py_BLOCK_THREADS;

			return 2;
	END_USING_MODELS;

	Py_BLOCK_THREADS;

	if (isTimeVisible) { traceLog
		GUI_setTime(NULL);
		GUI_setTimerVisible(false);

		isTimeVisible = false;

		current_map.time_preparing = NULL;
	} traceLog

	extendedDebugLog("fini OK!\n");

	return NULL;
};

uint8_t handleDelModelEvent(PyThreadState* _save) { traceLog
	if (EVENT_DEL_MODEL->eventID != EVENT_ID::DEL_LAST_MODEL) { traceLog
		extendedDebugLog("DEL_LAST_MODEL - eventID not equal!\n");

		return 1;
	} traceLog

	//рабочая часть

	

	uint8_t modelID;
	float* coords = new float[3];

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog
			superExtendedDebugLog("MODELS_USING\n");

			Py_BLOCK_THREADS;

			gBigWorldUtils->getLastModelCoords(5.0, &modelID, &coords);

			Py_UNBLOCK_THREADS;

			RELEASE_MODELS("handleDelModelEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 2);

			superExtendedDebugLog("MODELS_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog
			extendedDebugLog("handleDelModelEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			return 3;
	END_USING_MODELS;

	if      (!gBigWorldUtils->lastError) { traceLog
		EVENT_DEL_MODEL->request = send_token_threadsafe(databaseID, mapID, EVENT_ID::DEL_LAST_MODEL, modelID, coords);
		
		if (EVENT_DEL_MODEL->request) { traceLog
			if (EVENT_DEL_MODEL->request > 9) { traceLog
				extendedDebugLogFmt("DEL_LAST_MODEL - send_token_threadsafe - Error code %d\n", EVENT_DEL_MODEL->request);

				//GUI_setError(EVENT_DEL_MODEL->request);
							
				return 4;
			} traceLog

			extendedDebugLogFmt("[NY_Event][WARNING]: DEL_LAST_MODEL - send_token_threadsafe - Warning code %d\n", EVENT_DEL_MODEL->request);

			//GUI_setWarning(EVENT_DEL_MODEL->request);
		} traceLog

		EVENT_DEL_MODEL->request = parse_event_threadsafe(EVENT_ID::DEL_LAST_MODEL);

		if (EVENT_DEL_MODEL->request) { traceLog
			if (EVENT_DEL_MODEL->request > 9) { traceLog
				extendedDebugLogFmt("DEL_LAST_MODEL - parse_event_threadsafe - Error code %d\n", EVENT_DEL_MODEL->request);

				//GUI_setError(EVENT_DEL_MODEL->request);

				return 5;
			}
			extendedDebugLogFmt("[NY_Event][WARNING]: DEL_LAST_MODEL - parse_event_threadsafe - Warning code %d\n", EVENT_DEL_MODEL->request);

			//GUI_setWarning(EVENT_DEL_MODEL->request);

			return 6;
		} traceLog

		BEGIN_USING_MODELS;
			case WAIT_OBJECT_0: traceLog
				superExtendedDebugLog("MODELS_USING\n");

				Py_BLOCK_THREADS;

				EVENT_DEL_MODEL->request = delModelPy(coords);

				Py_UNBLOCK_THREADS;

				if (EVENT_DEL_MODEL->request) { traceLog
					extendedDebugLogFmt("DEL_LAST_MODEL - delModelPy - Error code %d\n", EVENT_DEL_MODEL->request);

					//GUI_setError(EVENT_DEL_MODEL->request);

					RELEASE_MODELS("handleDelModelEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 7);

					return 8;
				} traceLog

				scoreID = modelID;
				current_map.stageID = STAGE_ID::GET_SCORE;

				delete[] coords;

				/*
				EVENT_DEL_MODEL->request = delModelCoords(modelID, coords);

				if (EVENT_DEL_MODEL->request) { traceLog
					extendedDebugLogFmt("DEL_LAST_MODEL - delModelCoords - Error code %d\n", EVENT_DEL_MODEL->request);

					//GUI_setError(EVENT_DEL_MODEL->request);

					RELEASE_MODELS("handleDelModelEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 9);

					return 10;
				} traceLog

				*/

				RELEASE_MODELS("handleDelModelEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", 11);

				superExtendedDebugLog("MODELS_NOT_USING\n");

				break;
			case WAIT_ABANDONED: traceLog
				extendedDebugLog("handleDelModelEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

				return 12;
		END_USING_MODELS;
	}
	else if (gBigWorldUtils->lastError == 7 || gBigWorldUtils->lastError == 8) { traceLog
		extendedDebugLogFmt("DEL_LAST_MODEL - Model not found!\n");

		current_map.stageID = STAGE_ID::ITEMS_NOT_EXISTS;
	}
	else { traceLog
		extendedDebugLogFmt("DEL_LAST_MODEL - getLastModelCoords - error %d!\n", gBigWorldUtils->lastError);

		return 13;
	}

	Py_BLOCK_THREADS;

	if (current_map.stageID == STAGE_ID::GET_SCORE && scoreID != -1) { traceLog
		GUI_setMsg(current_map.stageID, scoreID, 5.0f);

		scoreID = -1;
	}
	else if (current_map.stageID == STAGE_ID::ITEMS_NOT_EXISTS) { traceLog
		GUI_setMsg(current_map.stageID);
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
			debugLogFmt("EVENT_IN_HANGAR not setted!\n");

			return 3;
		} traceLog
	}
	else if (eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog
		if (!EVENT_START_TIMER) { traceLog
			return 4;
		} traceLog

		EVENT_START_TIMER->eventID = eventID;

		if (!SetEvent(EVENT_START_TIMER->hEvent)) { traceLog
			debugLogFmt("EVENT_START_TIMER not setted!\n");

			return 5;
		} traceLog
	}
	else if (eventID == EVENT_ID::DEL_LAST_MODEL) { traceLog
		if (!EVENT_DEL_MODEL) { traceLog
			return 6;
		} traceLog

		EVENT_DEL_MODEL->eventID = eventID;

		if (!SetEvent(EVENT_DEL_MODEL->hEvent)) { traceLog
			debugLogFmt("EVENT_DEL_MODEL not setted!\n");

			return 7;
		} traceLog
	} 
	else { traceLog
		return 8;
	} traceLog

	return NULL;
};