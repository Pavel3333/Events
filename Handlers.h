#pragma once

#include "Py_API.h"

uint8_t handleBattleEvent(PyThreadState*);
uint8_t handleStartTimerEvent(PyThreadState*);
uint8_t handleInHangarEvent(PyThreadState*);
uint8_t handleBattleEndEvent(PyThreadState*);
uint8_t handleDelModelEvent(PyThreadState*);

uint8_t makeEventInThread(EVENT_ID);