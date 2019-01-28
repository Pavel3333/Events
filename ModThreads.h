#pragma once
#include "API_functions.h"

// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).

typedef struct {
	DWORD ID;

	uint32_t databaseID;
	uint8_t  map_ID;
	uint8_t  eventID;
} MYDATA_1, *PMYDATA_1;

uint8_t getThreadCount();