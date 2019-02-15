#pragma once

#include "Py_API.h"

uint8_t handleBattleEvent(EVENT_ID eventID);
uint8_t handleStartTimerEvent(PyThreadState* _save);
uint8_t handleInHangarEvent(PyThreadState* _save);
uint8_t handleBattleEndEvent(PyThreadState* _save);
uint8_t handleDelModelEvent(PyThreadState* _save);