#include "Handlers.h"

//обработка событий

uint8_t handleBattleEvent(EVENT_ID eventID) { traceLog
	if (!isInited || first_check || battleEnded || eventID == EVENT_ID::IN_HANGAR || !M_MODELS_NOT_USING || !EVENT_START_TIMER) { traceLog
		return 1;
	} traceLog

	INIT_LOCAL_MSG_BUFFER;

	extendedDebugLog("[NY_Event]: parsing...\n");

	PyThreadState *_save;

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog
			superExtendedDebugLog("[NY_Event]: MODELS_USING\n");

			Py_UNBLOCK_THREADS;

			if      (eventID == EVENT_ID::IN_BATTLE_GET_FULL) EVENT_START_TIMER->request = parse_config();
			else if (eventID == EVENT_ID::IN_BATTLE_GET_SYNC) EVENT_START_TIMER->request = parse_sync();
			else if (eventID == EVENT_ID::DEL_LAST_MODEL)     EVENT_START_TIMER->request = parse_del_model();

			if (EVENT_START_TIMER->request) { traceLog
				extendedDebugLogFmt("[NY_Event]: parsing FAILED! Error code: %d\n", EVENT_START_TIMER->request);

				//GUI_setError(parsing_result);

				//освобождаем мутекс для этого потока

				if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog
					extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

					return 3;
				}

				return 4;
			} traceLog

			Py_BLOCK_THREADS;

			//освобождаем мутекс для этого потока

			if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog
				extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

				return 3;
			}

			superExtendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog
			extendedDebugLog("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			return 2;
	END_USING_MODELS;

	extendedDebugLog("[NY_Event]: parsing OK!\n");

	if (current_map.time_preparing)  //выводим время
		GUI_setTime(current_map.time_preparing);

	if (current_map.stageID >= 0 && current_map.stageID < STAGES_COUNT) { traceLog
		if (    //выводим сообщение
			current_map.stageID == STAGE_ID::WAITING ||
			current_map.stageID == STAGE_ID::START ||
			current_map.stageID == STAGE_ID::COMPETITION ||
			current_map.stageID == STAGE_ID::END_BY_TIME ||
			current_map.stageID == STAGE_ID::END_BY_COUNT ||
			current_map.stageID == STAGE_ID::STREAMER_MODE
			) { traceLog
			if (lastStageID != STAGE_ID::GET_SCORE && lastStageID != STAGE_ID::ITEMS_NOT_EXISTS) GUI_setMsg(current_map.stageID);

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
					extendedDebugLogFmt("[NY_Event]: Warning - handle_battle_event - event_fini - Error code %d\n", EVENT_START_TIMER->request);

					//GUI_setWarning(event_result);
				} traceLog
			}
			else {
				if (!isTimeVisible) { traceLog
					GUI_setTimerVisible(true);

					isTimeVisible = true;
				} traceLog
			} traceLog

			if (current_map.stageID == STAGE_ID::START ||
				current_map.stageID == STAGE_ID::COMPETITION ||
				current_map.stageID == STAGE_ID::STREAMER_MODE) { traceLog
				if (isModelsAlreadyCreated && !isModelsAlreadyInited && current_map.minimap_count && current_map.modelsSects.size()) { traceLog
					if (eventID == EVENT_ID::IN_BATTLE_GET_FULL) { traceLog
						superExtendedDebugLogFmt("sect count: %u\npos count: %u\n", current_map.modelsSects.size(), current_map.minimap_count);

						extendedDebugLog("[NY_Event]: creating...\n");

						BEGIN_USING_MODELS;
							case WAIT_OBJECT_0: traceLog
								superExtendedDebugLog("[NY_Event]: MODELS_USING\n");

								models.~vector();
								//lights.~vector();

								//освобождаем мутекс для этого потока

								if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog
									extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

									return 6;
								}

								superExtendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

								break;
							case WAIT_ABANDONED: traceLog
								extendedDebugLog("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

								return 5;
						END_USING_MODELS;

						/*
						Первый способ - нативный вызов в main-потоке добавлением в очередь. Ненадёжно!

						int creating_result = Py_AddPendingCall(&create_models, nullptr); //create_models();

						if (creating_result == -1) { traceLog
							extendedDebugLog("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - create_models - failed to start PendingCall of creating models!\n");

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
							extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - handleBattleEvent - Error code %d\n", EVENT_START_TIMER->request);

							return 7;
						} traceLog

						//ожидаем события полного создания моделей

						superExtendedDebugLog("[NY_Event]: waiting EVENT_ALL_MODELS_CREATED\n");

						DWORD EVENT_ALL_MODELS_CREATED_WaitResult = WaitForSingleObject(
							EVENT_ALL_MODELS_CREATED->hEvent, // event handle
							INFINITE);                        // indefinite wait

						switch (EVENT_ALL_MODELS_CREATED_WaitResult) {
						case WAIT_OBJECT_0:  traceLog
							superExtendedDebugLog("[NY_Event]: EVENT_ALL_MODELS_CREATED signaled!\n");

							//-------------

							//место для рабочего кода

							Py_BLOCK_THREADS;

							extendedDebugLog("[NY_Event]: creating OK!\n");

							EVENT_START_TIMER->request = init_models();

							if (EVENT_START_TIMER->request) { traceLog
								if (EVENT_START_TIMER->request > 9) { traceLog
									extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - init_models - Error code %d\n", EVENT_START_TIMER->request);

									//GUI_setError(EVENT_START_TIMER->request);

									return 9;
								} traceLog

								extendedDebugLogFmt("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - init_models - Warning code %d\n", EVENT_START_TIMER->request);

								//GUI_setWarning(EVENT_START_TIMER->request);

								return 8;
							} traceLog

							EVENT_START_TIMER->request = set_visible(true);

							if (EVENT_START_TIMER->request) { traceLog
								if (EVENT_START_TIMER->request > 9) { traceLog
									extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - set_visible - Error code %d\n", EVENT_START_TIMER->request);

									//GUI_setError(EVENT_START_TIMER->request);

									return 12;
								} traceLog

								extendedDebugLogFmt("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - set_visible - Warning code %d\n", EVENT_START_TIMER->request);

								//GUI_setWarning(EVENT_START_TIMER->request);

								return 11;
							} traceLog

							isModelsAlreadyInited = true;

							break;

							// An error occurred
						default: traceLog
							extendedDebugLog("[NY_Event][ERROR]: IN_HANGAR - something wrong with WaitResult!\n");

							Py_BLOCK_THREADS;

							return 10;
						} traceLog
					} traceLog

					return NULL;
				} traceLog
			} traceLog

			if (isModelsAlreadyCreated && isModelsAlreadyInited && eventID == EVENT_ID::IN_BATTLE_GET_SYNC && sync_map.all_models_count && !sync_map.modelsSects_deleting.empty()) { traceLog
				std::vector<ModelsSection>::iterator it_sect_sync;
				std::vector<float*>::iterator        it_model;

				BEGIN_USING_MODELS;
					case WAIT_OBJECT_0: traceLog
						superExtendedDebugLog("[NY_Event]: MODELS_USING\n");

						it_sect_sync = sync_map.modelsSects_deleting.begin();

						while (it_sect_sync != sync_map.modelsSects_deleting.end()) {
							if (it_sect_sync->isInitialised) {
								it_model = it_sect_sync->models.begin();

								while (it_model != it_sect_sync->models.end()) {
									if (*it_model == nullptr) { traceLog
										it_model = it_sect_sync->models.erase(it_model);

										continue;
									}

									EVENT_START_TIMER->request = delModelPy(*it_model);

									if (EVENT_START_TIMER->request) { traceLog
										extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEvent - delModelPy - Error code %d\n", EVENT_START_TIMER->request);

										//GUI_setError(EVENT_START_TIMER->request);

										it_model++;

										continue;
									}

									EVENT_START_TIMER->request = delModelCoords(it_sect_sync->ID, *it_model);

									if (EVENT_START_TIMER->request) { traceLog
										extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEvent - delModelCoords - Error code %d\n", EVENT_START_TIMER->request);

										//GUI_setError(res);

										it_model++;

										continue;
									}

									delete[] * it_model;
									*it_model = nullptr;

									it_model = it_sect_sync->models.erase(it_model);
								}
							}

							if (it_sect_sync->path != nullptr) {
								delete[] it_sect_sync->path;

								it_sect_sync->path = nullptr;
							}

							it_sect_sync->models.~vector();

							it_sect_sync = sync_map.modelsSects_deleting.erase(it_sect_sync); //удаляем секцию из вектора секций синхронизации
						} traceLog

						sync_map.modelsSects_deleting.~vector();

						//освобождаем мутекс для этого потока

						if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog
							extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

							return 14;
						}

						superExtendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

						break;
					case WAIT_ABANDONED: traceLog
						extendedDebugLog("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

						return 13;
					END_USING_MODELS;
			}
			else { traceLog
				return NULL;
			} traceLog
		}
		else extendedDebugLog("[NY_Event]: Warning - StageID is not right for this event\n");
	}
	else extendedDebugLog("[NY_Event]: Warning - StageID is not correct\n");

	return NULL;
}

uint8_t handleStartTimerEvent(PyThreadState* _save) {
	if (!isInited || !EVENT_START_TIMER) {
		extendedDebugLog("[NY_Event][ERROR]: handleStartTimerEvent - isInited or EVENT_START_TIMER is NULL!\n");

		return 1;
	}

	INIT_LOCAL_MSG_BUFFER;

	BOOL            bSuccess;
	__int64         qwDueTime;
	LARGE_INTEGER   liDueTime;

	EVENT_ID eventID;

	if (EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_FULL && EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog
		extendedDebugLog("[NY_Event][ERROR]: handleStartTimerEvent - eventID not equal!\n");

		return 2;
	} traceLog

	if (first_check || battleEnded) { traceLog
		extendedDebugLog("[NY_Event][ERROR]: handleStartTimerEvent - first_check or battleEnded!\n");

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

				eventID = EVENT_ID::IN_BATTLE_GET_FULL;

				if (isModelsAlreadyCreated && isModelsAlreadyInited) eventID = EVENT_ID::IN_BATTLE_GET_SYNC;

				BEGIN_USING_NETWORK;
					case WAIT_OBJECT_0: traceLog
						superExtendedDebugLog("[NY_Event]: NETWORK_USING\n");

						EVENT_START_TIMER->request = send_token(databaseID, mapID, eventID);

						if (EVENT_START_TIMER->request) { traceLog
							if (EVENT_START_TIMER->request > 9) { traceLog
								extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - send_token - Error code %d\n", EVENT_START_TIMER->request);

								//GUI_setError(EVENT_START_TIMER->request);

								battleTimerLastError = 1;

								//освобождаем мутекс для этого потока

								if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog
									extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

									battleTimerLastError = 8;

									break;
								}

								superExtendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

								break;
							} traceLog

							extendedDebugLogFmt("[NY_Event][WARNING]: handleStartTimerEvent - send_token - Warning code %d\n", EVENT_START_TIMER->request);
						} traceLog

						superExtendedDebugLog("[NY_Event]: generating token OK!\n");

						Py_BLOCK_THREADS;

						EVENT_START_TIMER->request = handleBattleEvent(eventID);

						Py_UNBLOCK_THREADS;

						if (EVENT_START_TIMER->request) { traceLog
							extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - create_models - Error code %d\n", EVENT_START_TIMER->request);

							//GUI_setError(EVENT_START_TIMER->request);

							//освобождаем мутекс для этого потока

							if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog
								extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

								battleTimerLastError = 6;

								break;
							}

							superExtendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

							battleTimerLastError = 5;

							break;
						} traceLog

						//освобождаем мутекс для этого потока

						if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog
							extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

							battleTimerLastError = 4;

							break;
						}

						superExtendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

						break;
					case WAIT_ABANDONED: traceLog
						extendedDebugLog("[NY_Event][ERROR]: handleStartTimerEvent - NETWORK_NOT_USING: WAIT_ABANDONED!\n");

						battleTimerLastError = 3;

						break;
					END_USING_NETWORK;

					SleepEx(
						INFINITE,     // Wait forever
						TRUE);        // Put thread in an alertable state
			} traceLog

			if (battleTimerLastError) { traceLog
				extendedDebugLogFmt("[NY_Event][WARNING]: handleStartTimerEvent: error %d\n", battleTimerLastError);

				CancelWaitableTimer(hBattleTimer);
			} traceLog
		}
		else extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - SetWaitableTimer: error %d\n", GetLastError());

		CloseHandle(hBattleTimer);
		
		hBattleTimer = NULL;
	}
	else extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent: CreateWaitableTimer: error %d\n", GetLastError());

	return battleTimerLastError;
}

uint8_t handleInHangarEvent(PyThreadState* _save) {
	if (!isInited || !EVENT_IN_HANGAR) {
		extendedDebugLog("[NY_Event][ERROR]: handleInHangarEvent - isInited or EVENT_IN_HANGAR is NULL!\n");

		return 1;
	}

	INIT_LOCAL_MSG_BUFFER;

	EVENT_ID eventID = EVENT_IN_HANGAR->eventID;

	if (eventID != EVENT_ID::IN_HANGAR) { traceLog
		extendedDebugLog("[NY_Event][ERROR]: handleInHangarEvent - eventID not equal!\n");

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
				BEGIN_USING_NETWORK;
					case WAIT_OBJECT_0: traceLog
						superExtendedDebugLog("[NY_Event]: NETWORK_USING\n");

						first_check = send_token(databaseID, mapID, eventID);

						//освобождаем мутекс для этого потока

						if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog
							extendedDebugLogFmt("[NY_Event][ERROR]: handleInHangarEvent - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

							hangarTimerLastError = 4;
						}

						superExtendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

						break;
					case WAIT_ABANDONED: traceLog
						extendedDebugLog("[NY_Event][ERROR]: handleInHangarEvent - NETWORK_NOT_USING: WAIT_ABANDONED!\n");

						hangarTimerLastError = 3;
				END_USING_NETWORK;

				if (first_check) { traceLog
					if (first_check > 9) { traceLog
						extendedDebugLogFmt("[NY_Event][ERROR]: handleInHangarEvent - Error code %d\n", first_check);

						//GUI_setError(first_check);

						hangarTimerLastError = 2;
					}
					else {
						extendedDebugLogFmt("[NY_Event][WARNING]: handleInHangarEvent - Warning code %d\n", first_check);

						//GUI_setWarning(first_check);
					} traceLog

					Py_BLOCK_THREADS;

					EVENT_IN_HANGAR->request = showMessage(g_self->i18n);

					if (EVENT_IN_HANGAR->request) {
						extendedDebugLogFmt("[NY_Event][WARNING]: handleInHangarEvent - showMessage: error %d\n", EVENT_IN_HANGAR->request);
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
		else extendedDebugLogFmt("[NY_Event][ERROR]: handleInHangarEvent - SetWaitableTimer: error %d\n", GetLastError());

		CloseHandle(hHangarTimer);

		hHangarTimer = NULL;
	}
	else extendedDebugLogFmt("[NY_Event][ERROR]: handleInHangarEvent: CreateWaitableTimer: error %d\n", GetLastError());

	return hangarTimerLastError;
}

uint8_t handleBattleEndEvent(PyThreadState* _save) { traceLog
	if (!isInited || first_check || !M_MODELS_NOT_USING) { traceLog
		return 1;
	} traceLog

	INIT_LOCAL_MSG_BUFFER;

	std::vector<ModModel*>::iterator it_model;
	std::vector<ModLight*>::iterator it_light;

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog
			superExtendedDebugLog("[NY_Event]: MODELS_USING\n");

			if (!models.empty()) { traceLog
				EVENT_BATTLE_ENDED->request = del_models();

				if (EVENT_BATTLE_ENDED->request) { traceLog
					extendedDebugLogFmt("[NY_Event][WARNING]: handleBattleEndEvent - del_models: %d\n", EVENT_BATTLE_ENDED->request);
				} traceLog

				it_model = models.begin();

				while (it_model != models.end()) {
					if (*it_model == nullptr) { traceLog
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

			isModelsAlreadyInited = false;

			/*if (!lights.empty()) { traceLog
				it_light = lights.begin();

				while (it_light != lights.end()) {
					if (*it_light == nullptr) { traceLog
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

			//освобождаем мутекс для этого потока

			if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog
				extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEndEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

				return 3;
			}

			superExtendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

			Py_BLOCK_THREADS;

			break;
		case WAIT_ABANDONED: traceLog
			extendedDebugLog("[NY_Event][ERROR]: handleBattleEndEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			return 2;
		END_USING_MODELS;

		if (isTimeVisible) { traceLog
			GUI_setTime(NULL);
			GUI_setTimerVisible(false);

			isTimeVisible = false;

			current_map.time_preparing = NULL;
		} traceLog

		extendedDebugLog("[NY_Event]: fini OK!\n");

		return NULL;
};

uint8_t handleDelModelEvent(PyThreadState* _save) { traceLog
	EVENT_ID eventID = EVENT_DEL_MODEL->eventID;

	if (eventID != EVENT_ID::DEL_LAST_MODEL) { traceLog
		extendedDebugLog("[NY_Event][ERROR]: DEL_LAST_MODEL - eventID not equal!\n");

		return 1;
	} traceLog

	//рабочая часть

	INIT_LOCAL_MSG_BUFFER;

	uint8_t modelID;
	float* coords = new float[3];

	Py_BLOCK_THREADS;

	EVENT_DEL_MODEL->request = findLastModelCoords(5.0, &modelID, &coords);

	Py_UNBLOCK_THREADS;

	if (!EVENT_DEL_MODEL->request) { traceLog
		BEGIN_USING_NETWORK;
		case WAIT_OBJECT_0: traceLog
			superExtendedDebugLog("[NY_Event]: NETWORK_USING\n");

			EVENT_DEL_MODEL->request = send_token(databaseID, mapID, EVENT_ID::DEL_LAST_MODEL, modelID, coords);

			//освобождаем мутекс для этого потока

			if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog
				extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

				return 11;
			}

			superExtendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog
			extendedDebugLog("[NY_Event][ERROR]: DEL_LAST_MODEL - NETWORK_NOT_USING: WAIT_ABANDONED!\n");

			return 10;
		END_USING_NETWORK;

		if (EVENT_DEL_MODEL->request) { traceLog
			if (EVENT_DEL_MODEL->request > 9) { traceLog
				extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - send_token - Error code %d\n", EVENT_DEL_MODEL->request);

				//GUI_setError(EVENT_DEL_MODEL->request);

				return 9;
			} traceLog

			extendedDebugLogFmt("[NY_Event][WARNING]: DEL_LAST_MODEL - send_token - Warning code %d\n", EVENT_DEL_MODEL->request);

			//GUI_setWarning(EVENT_DEL_MODEL->request);

			return 8;
		} traceLog

		BEGIN_USING_MODELS;
			case WAIT_OBJECT_0: traceLog
				superExtendedDebugLog("[NY_Event]: MODELS_USING\n");

				EVENT_DEL_MODEL->request = delModelPy(coords);

				if (EVENT_DEL_MODEL->request) { traceLog
					extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - delModelPy - Error code %d\n", EVENT_DEL_MODEL->request);

					//GUI_setError(EVENT_DEL_MODEL->request);

					//освобождаем мутекс для этого потока

					if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog
						extendedDebugLogFmt("[NY_Event][ERROR]: handleDelModelEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

						return 7;
					}

					return 6;
				} traceLog

				scoreID = modelID;
				current_map.stageID = STAGE_ID::GET_SCORE;

				delete[] coords;

				/*
				EVENT_DEL_MODEL->request = delModelCoords(modelID, coords);

				if (EVENT_DEL_MODEL->request) { traceLog
					extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - delModelCoords - Error code %d\n", EVENT_DEL_MODEL->request);

					//GUI_setError(EVENT_DEL_MODEL->request);

					//освобождаем мутекс для этого потока

					if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog
						extendedDebugLogFmt("[NY_Event][ERROR]: handleDelModelEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

						return 7;
					}

					return 6;
				} traceLog

				*/

				//освобождаем мутекс для этого потока

				if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog
					extendedDebugLogFmt("[NY_Event][ERROR]: handleDelModelEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

					return 5;
				}

				superExtendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

				break;
			case WAIT_ABANDONED: traceLog
				extendedDebugLog("[NY_Event][ERROR]: handleDelModelEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

				return 4;
		END_USING_MODELS;
	}
	else if (EVENT_DEL_MODEL->request == 7) { traceLog
		current_map.stageID = STAGE_ID::ITEMS_NOT_EXISTS;
	} traceLog

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

	INIT_LOCAL_MSG_BUFFER;

	if (eventID == EVENT_ID::IN_HANGAR || eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC || eventID == EVENT_ID::DEL_LAST_MODEL) { traceLog //посылаем ивент и обрабатываем в треде
		if (eventID == EVENT_ID::IN_HANGAR) { traceLog
			if (!EVENT_IN_HANGAR) { traceLog
				return 4;
			} traceLog

			EVENT_IN_HANGAR->eventID = eventID;

			if (!SetEvent(EVENT_IN_HANGAR->hEvent)) { traceLog
				debugLogFmt("[NY_Event][ERROR]: EVENT_IN_HANGAR not setted!\n");

				return 5;
			} traceLog
		}
		else if (eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog
			if (!EVENT_START_TIMER) { traceLog
				return 4;
			} traceLog

			EVENT_START_TIMER->eventID = eventID;

			if (!SetEvent(EVENT_START_TIMER->hEvent))
			{
				debugLogFmt("[NY_Event][ERROR]: EVENT_START_TIMER not setted!\n");

				return 5;
			} traceLog
		}
		else if (eventID == EVENT_ID::DEL_LAST_MODEL) { traceLog
			if (!EVENT_DEL_MODEL) { traceLog
				return 4;
			} traceLog

			EVENT_DEL_MODEL->eventID = eventID;

			if (!SetEvent(EVENT_DEL_MODEL->hEvent))
			{
				debugLogFmt("[NY_Event][ERROR]: EVENT_DEL_MODEL not setted!\n");

				return 5;
			} traceLog
		} traceLog

		return NULL;
	} traceLog

	return 2;
};