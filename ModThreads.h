#pragma once

#include "stdafx.h"

#include "API_functions.h"

// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).
typedef struct {
	uint8_t threadNum;
	uint32_t databaseID;
	uint8_t  map_ID;
	uint8_t  eventID;
} MYDATA_1, *PMYDATA_1;

typedef struct {
	uint8_t threadNum;
} MYDATA_2, *PMYDATA_2;

typedef struct {
	HANDLE hThread;
	PMYDATA_1 pMyData;
	DWORD ID;
} Thread_1;

typedef struct {
	HANDLE hThread;
	PMYDATA_2 pMyData;
	DWORD ID;
} Thread_2;

extern std::vector<Thread_1> threads_1;
extern std::vector<Thread_2> threads_2;

uint8_t getThreadCount(bool);