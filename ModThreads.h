#pragma once

#include "API_functions.h"

typedef struct {
	HANDLE hEvent = NULL;

	EVENT_ID eventID = EVENT_ID::IN_HANGAR;
} EVENTDATA_1, *PEVENTDATA_1;

typedef struct {
	HANDLE hEvent = NULL;
} EVENTDATA_2, *PEVENTDATA_2;

uint8_t getThreadCount();