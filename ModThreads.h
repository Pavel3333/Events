﻿#pragma once

#include "API_functions.h"

// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).

typedef struct {
	HANDLE hEvent = NULL;

	EVENT_ID eventID = EVENT_ID::IN_HANGAR;
} EVENTDATA_1, *PEVENTDATA_1;

typedef struct {
	HANDLE hEvent = NULL;
} EVENTDATA_2, *PEVENTDATA_2;

uint8_t getThreadCount();