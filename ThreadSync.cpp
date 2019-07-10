#include "pch.h"
#include "ThreadSync.h"
#include "MyLogger.h"


//WaitableTimer

HANDLE hHangarTimer = NULL;
HANDLE hBattleTimer = NULL;

//Потоки и их ID

HANDLE hBattleTimerThread  = NULL;
DWORD  battleTimerThreadID = NULL;

HANDLE hHandlerThread  = NULL;
DWORD  handlerThreadID = NULL;

//последние коды ошибок таймеров

uint8_t hangarTimerLastError = NULL;
uint8_t battleTimerLastError = NULL;

//Главные ивенты

PEVENTDATA_1 EVENT_IN_HANGAR   = NULL;
PEVENTDATA_1 EVENT_START_TIMER = NULL;
PEVENTDATA_1 EVENT_DEL_MODEL   = NULL;

//Второстепенные ивенты

PEVENTDATA_2 EVENT_ALL_MODELS_CREATED = NULL;

PEVENTDATA_2 EVENT_BATTLE_ENDED = NULL;

//Мутексы

std::mutex g_models_mutex;

//Критические секции

CRITICAL_SECTION CS_NETWORK_NOT_USING;

//---------------------

void closeEvent(PEVENTDATA_1* pEvent) {
	traceLog
	if (*pEvent) {
		traceLog //если уже была инициализирована структура - удаляем
		if ((*pEvent)->hEvent) {
			traceLog
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		} traceLog

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	} traceLog
}

void closeEvent(PEVENTDATA_2* pEvent) {
	traceLog
	if (*pEvent) {
		traceLog //если уже была инициализирована структура - удаляем
		if ((*pEvent)->hEvent) {
			traceLog
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		} traceLog

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	} traceLog
}

bool createEvent(PEVENTDATA_1* pEvent, uint8_t eventID)
{
	INIT_LOCAL_MSG_BUFFER;

	traceLog
	closeEvent(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_1)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выделяем память в куче для ивента
		sizeof(EVENTDATA_1));

	if (!(*pEvent)) {
		traceLog //нехватка памяти, завершаем работу

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

		extendedDebugLogEx(ERROR, "Primary event creating: error %d", GetLastError());

		return false;
	} traceLog

	return true;
}

bool createEvent(PEVENTDATA_2* pEvent, LPCWSTR eventName, BOOL isSignaling) {
	traceLog
	closeEvent(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_2)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выделяем память в куче для ивента
		sizeof(EVENTDATA_2));

	if (!(*pEvent)) {
		traceLog //нехватка памяти, завершаем работу

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
		INIT_LOCAL_MSG_BUFFER;

		extendedDebugLogEx(ERROR, "Secondary event creating: error %d", GetLastError());

		return false;
	} traceLog

	return true;
}

bool createEventsAndMutexes() { traceLog
	INIT_LOCAL_MSG_BUFFER;
	
	if (!createEvent(&EVENT_IN_HANGAR,   EVENT_ID::IN_HANGAR)) { traceLog
		return false;
	} traceLog
	if (!createEvent(&EVENT_START_TIMER, EVENT_ID::IN_BATTLE_GET_FULL)) { traceLog
		return false;
	} traceLog
	if (!createEvent(&EVENT_DEL_MODEL,   EVENT_ID::DEL_LAST_MODEL)) { traceLog
		return false;
	} traceLog

	if (!createEvent(&EVENT_ALL_MODELS_CREATED, L"NY_Event_AllModelsCreatedEvent")) { traceLog
		return false;
	} traceLog
	if (!createEvent(&EVENT_BATTLE_ENDED,       L"NY_Event_BattleEndedEvent")) { traceLog
		return false;
	} traceLog

	return true;
}


uint32_t parse_event_threadsafe(EVENT_ID eventID)
{
	return parse_event_safe(eventID);
}


uint32_t send_token_threadsafe(uint32_t id, uint8_t map_id, EVENT_ID eventID, MODEL_ID modelID, float* coords_del) {
	uint32_t result = NULL;

	EnterCriticalSection(&CS_NETWORK_NOT_USING);

	result = send_token(id, map_id, eventID, modelID, coords_del);

	LeaveCriticalSection(&CS_NETWORK_NOT_USING);

	return result;
}
