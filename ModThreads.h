#pragma once

#include "stdafx.h"

#include "API_functions.h"

// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).

typedef struct {
	HANDLE hThread;
	LPVOID pMyData;
	DWORD ID;
} Thread_1;

typedef struct {
	std::vector<Thread_1>::iterator threadElement;
	uint32_t databaseID;
	uint8_t  map_ID;
	uint8_t  eventID;
} MYDATA_1, *PMYDATA_1;

extern std::vector<Thread_1> threads_1;

uint8_t getThreadCount();