#include "ThreadSync.h"

//WaitableTimer

HANDLE hHangarTimer = NULL;
HANDLE hBattleTimer = NULL;

//ѕотоки и их ID

HANDLE hBattleTimerThread = NULL;
DWORD  battleTimerThreadID = NULL;

HANDLE hHandlerThread = NULL;
DWORD  handlerThreadID = NULL;

//последние коды ошибок таймеров

uint8_t hangarTimerLastError = NULL;
uint8_t battleTimerLastError = NULL;

//√лавные ивенты

PEVENTDATA_1 EVENT_IN_HANGAR   = NULL;
PEVENTDATA_1 EVENT_START_TIMER = NULL;
PEVENTDATA_1 EVENT_DEL_MODEL   = NULL;

//¬торостепенные ивенты

PEVENTDATA_2 EVENT_ALL_MODELS_CREATED = NULL;

PEVENTDATA_2 EVENT_BATTLE_ENDED = NULL;

//ћутексы

HANDLE M_MODELS_NOT_USING  = NULL;

// ритические секции

CRITICAL_SECTION CS_NETWORK_NOT_USING;
CRITICAL_SECTION CS_PARSING_NOT_USING;

//---------------------

void closeEvent1(PEVENTDATA_1* pEvent) {
	traceLog
	if (*pEvent) {
		traceLog //если уже была инициализирована структура - удал€ем
		if ((*pEvent)->hEvent) {
			traceLog
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		} traceLog

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	} traceLog
}

void closeEvent2(PEVENTDATA_2* pEvent) {
	traceLog
	if (*pEvent) {
		traceLog //если уже была инициализирована структура - удал€ем
		if ((*pEvent)->hEvent) {
			traceLog
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		} traceLog

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	} traceLog
}

bool createEvent1(PEVENTDATA_1* pEvent, uint8_t eventID) {
	traceLog
	closeEvent1(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_1)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выдел€ем пам€ть в куче дл€ ивента
		sizeof(EVENTDATA_1));

	if (!(*pEvent)) {
		traceLog //нехватка пам€ти, завершаем работу

		return false;
	} traceLog

	(*pEvent)->hEvent = CreateEvent(
		NULL,                      // default security attributes
		FALSE,                     // auto-reset event
		FALSE,                     // initial state is nonsignaled
		EVENT_NAMES[eventID]       // object name
	);

	if (!((*pEvent)->hEvent)) {
		traceLog
		

		extendedDebugLogFmt("Primary event creating: error %d\n", GetLastError());

		return false;
	} traceLog

	return true;
}

bool createEvent2(PEVENTDATA_2* pEvent, LPCWSTR eventName, BOOL isSignaling) {
	traceLog
	closeEvent2(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_2)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выдел€ем пам€ть в куче дл€ ивента
		sizeof(EVENTDATA_2));

	if (!(*pEvent)) {
		traceLog //нехватка пам€ти, завершаем работу

		return false;
	} traceLog

	(*pEvent)->hEvent = CreateEvent(
		NULL,                      // default security attributes
		FALSE,                     // auto-reset event
		isSignaling,
		eventName                  // object name
	);

	if (!((*pEvent)->hEvent)) {
		traceLog
		

		extendedDebugLogFmt("Secondary event creating: error %d\n", GetLastError());

		return false;
	} traceLog

	return true;
}

bool createEventsAndSecondThread() { traceLog
	
	
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
		debugLogFmt("MODELS_NOT_USING creating: error %d\n", GetLastError());

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
		WaitForSingleObject(hHandlerThread, INFINITE);
		
		hHandlerThread  = NULL;
		handlerThreadID = NULL;
	} traceLog

	hHandlerThread = CreateThread( //создаем второй поток
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		HandlerThread,                          // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&handlerThreadID);                      // returns the thread identifier 

	if (!hHandlerThread) { traceLog
		debugLogFmt("Handler thread creating: error %d\n", GetLastError());

		return false;
	} traceLog

	return true;
}


uint32_t parse_event_threadsafe(EVENT_ID eventID) {
	uint32_t result = NULL;

	EnterCriticalSection(&CS_PARSING_NOT_USING);

	result = parse_event(eventID);

	LeaveCriticalSection(&CS_PARSING_NOT_USING);

	return result;
}

uint32_t send_token_threadsafe(uint32_t id, uint8_t map_id, EVENT_ID eventID, uint8_t modelID, float* coords_del) {
	uint32_t result = NULL;

	EnterCriticalSection(&CS_NETWORK_NOT_USING);

	result = send_token(id, map_id, eventID, modelID, coords_del);

	LeaveCriticalSection(&CS_NETWORK_NOT_USING);

	return result;
}
