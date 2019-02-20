#include "ThreadSync.h"

//Главные ивенты

PEVENTDATA_1 EVENT_IN_HANGAR   = NULL;
PEVENTDATA_1 EVENT_START_TIMER = NULL;
PEVENTDATA_1 EVENT_DEL_MODEL   = NULL;

//Второстепенные ивенты

PEVENTDATA_2 EVENT_ALL_MODELS_CREATED = NULL;

PEVENTDATA_2 EVENT_BATTLE_ENDED = NULL;

//Мутексы

HANDLE M_MODELS_NOT_USING  = NULL;

//Критические секции

CRITICAL_SECTION CS_NETWORK_NOT_USING;
CRITICAL_SECTION CS_PARSING_NOT_USING;

//---------------------

void closeEvent1(PEVENTDATA_1* pEvent) {
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

void closeEvent2(PEVENTDATA_2* pEvent) {
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

bool createEvent1(PEVENTDATA_1* pEvent, uint8_t eventID) {
	traceLog
	closeEvent1(pEvent); //закрываем ивент, если существует

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
		INIT_LOCAL_MSG_BUFFER;

		extendedDebugLogFmt("[NY_Event][ERROR]: Primary event creating: error %d\n", GetLastError());

		return false;
	} traceLog

	return true;
}

bool createEvent2(PEVENTDATA_2* pEvent, LPCWSTR eventName, BOOL isSignaling) {
	traceLog
	closeEvent2(pEvent); //закрываем ивент, если существует

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

		extendedDebugLogFmt("[NY_Event][ERROR]: Secondary event creating: error %d\n", GetLastError());

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