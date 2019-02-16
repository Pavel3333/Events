#include "ThreadSync.h"

//Главные ивенты

PEVENTDATA_1 EVENT_IN_HANGAR = NULL;
PEVENTDATA_1 EVENT_START_TIMER = NULL;
PEVENTDATA_1 EVENT_DEL_MODEL = NULL;

//Второстепенные ивенты

PEVENTDATA_2 EVENT_ALL_MODELS_CREATED = NULL;

PEVENTDATA_2 EVENT_BATTLE_ENDED = NULL;

//Мутексы

HANDLE M_NETWORK_NOT_USING = NULL;
HANDLE M_MODELS_NOT_USING = NULL;

//---------------------

void closeEvent1(PEVENTDATA_1* pEvent) {
	traceLog();
	if (*pEvent) {
		traceLog(); //если уже была инициализирована структура - удаляем
		if ((*pEvent)->hEvent) {
			traceLog();
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		} traceLog();

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	} traceLog();
}

void closeEvent2(PEVENTDATA_2* pEvent) {
	traceLog();
	if (*pEvent) {
		traceLog(); //если уже была инициализирована структура - удаляем
		if ((*pEvent)->hEvent) {
			traceLog();
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		} traceLog();

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	} traceLog();
}

bool createEvent1(PEVENTDATA_1* pEvent, uint8_t eventID) {
	traceLog();
	closeEvent1(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_1)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выделяем память в куче для ивента
		sizeof(EVENTDATA_1));

	if (!(*pEvent)) {
		traceLog(); //нехватка памяти, завершаем работу

		ExitProcess(1);
	} traceLog();

	(*pEvent)->hEvent = CreateEvent(
		NULL,                      // default security attributes
		FALSE,                     // auto-reset event
		FALSE,                     // initial state is nonsignaled
		EVENT_NAMES[eventID]       // object name
	);

	if (!((*pEvent)->hEvent)) {
		traceLog();

		extendedDebugLog("[NY_Event][ERROR]: Primary event creating: error %v", GetLastError());

		return false;
	} traceLog();

	return true;
}

bool createEvent2(PEVENTDATA_2* pEvent, LPCWSTR eventName, BOOL isSignaling) {
	traceLog();
	closeEvent2(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_2)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выделяем память в куче для ивента
		sizeof(EVENTDATA_2));

	if (!(*pEvent)) {
		traceLog(); //нехватка памяти, завершаем работу

		ExitProcess(1);
	} traceLog();

	(*pEvent)->hEvent = CreateEvent(
		NULL,                      // default security attributes
		FALSE,                     // auto-reset event
		isSignaling,
		eventName                  // object name
	);

	if (!((*pEvent)->hEvent)) {
		traceLog();

		extendedDebugLog("[NY_Event][ERROR]: Secondary event creating: error %v", GetLastError());

		return false;
	} traceLog();

	return true;
}
