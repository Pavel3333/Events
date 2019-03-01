#pragma once
#include "API_functions.h"

DWORD timerThread();
DWORD handlerThread();

DWORD WINAPI TimerThread(LPVOID);
DWORD WINAPI HandlerThread(LPVOID);

uint8_t event_start();
uint8_t event_check();
uint8_t event_init(PyObject*, PyObject*, PyObject*);