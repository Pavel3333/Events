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

extern HANDLE M_NETWORK_NOT_USING;
extern HANDLE M_MODELS_NOT_USING;

//---------------------

void closeEvent1(PEVENTDATA_1*);

void closeEvent2(PEVENTDATA_2*);

bool createEvent1(PEVENTDATA_1*, uint8_t);
bool createEvent2(PEVENTDATA_2*, LPCWSTR, BOOL isSignaling = FALSE);