#pragma once

#include "ModThreads.h"
#include "Network.h"

//WaitableTimer

extern HANDLE hHangarTimer;
extern HANDLE hBattleTimer;

//������ � �� ID

extern HANDLE hBattleTimerThread;
extern DWORD  battleTimerThreadID;

extern HANDLE hHandlerThread;
extern DWORD  handlerThreadID;

//��������� ���� ������ ��������

extern uint8_t hangarTimerLastError;
extern uint8_t battleTimerLastError;

//������� ������

extern PEVENTDATA_1 EVENT_IN_HANGAR;
extern PEVENTDATA_1 EVENT_START_TIMER;
extern PEVENTDATA_1 EVENT_DEL_MODEL;

//�������������� ������

extern PEVENTDATA_2 EVENT_ALL_MODELS_CREATED;

extern PEVENTDATA_2 EVENT_BATTLE_ENDED;

//�������

extern HANDLE M_MODELS_NOT_USING;

//����������� ������

extern CRITICAL_SECTION CS_NETWORK_NOT_USING;

//---------------------

void closeEvent1(PEVENTDATA_1*);

void closeEvent2(PEVENTDATA_2*);

bool createEvent1(PEVENTDATA_1*, uint8_t);
bool createEvent2(PEVENTDATA_2*, LPCWSTR, BOOL isSignaling = FALSE);

bool createEventsAndMutexes();


uint32_t parse_event_threadsafe(EVENT_ID);
uint32_t send_token_threadsafe(uint32_t, uint8_t, EVENT_ID, MODEL_ID modelID, float* coords_del = nullptr);
