#pragma once

#include "ModThreads.h"

//Главные ивенты

extern PEVENTDATA_1 EVENT_IN_HANGAR;
extern PEVENTDATA_1 EVENT_START_TIMER;
extern PEVENTDATA_1 EVENT_DEL_MODEL;

//Второстепенные ивенты

extern PEVENTDATA_2 EVENT_ALL_MODELS_CREATED;

extern PEVENTDATA_2 EVENT_BATTLE_ENDED;

//Мутексы

extern HANDLE M_MODELS_NOT_USING;

//Критические секции

extern CRITICAL_SECTION CS_NETWORK_NOT_USING;
extern CRITICAL_SECTION CS_PARSING_NOT_USING;

//---------------------

void closeEvent1(PEVENTDATA_1*);

void closeEvent2(PEVENTDATA_2*);

bool createEvent1(PEVENTDATA_1*, uint8_t);
bool createEvent2(PEVENTDATA_2*, LPCWSTR, BOOL isSignaling = FALSE);

uint32_t parse_event_threadsafe(EVENT_ID);
uint32_t send_token_threadsafe(uint32_t, uint8_t, EVENT_ID, uint8_t modelID = NULL, float* coords_del = nullptr);