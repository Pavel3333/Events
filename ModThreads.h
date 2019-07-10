#pragma once
#include "API_functions.h"


typedef struct {
	HANDLE hEvent = nullptr;

	EVENT_ID eventID = EVENT_ID::IN_HANGAR;

	uint32_t request = 0;
} EVENTDATA_1, *PEVENTDATA_1;

typedef struct {
	HANDLE hEvent = nullptr;

	uint32_t request = 0;
} EVENTDATA_2, *PEVENTDATA_2;

uint8_t getThreadCount();
