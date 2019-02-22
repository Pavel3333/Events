#pragma once

#include "ThreadSync.h"
#include "BW_native.h"
#include "GUI.h"
#include "HangarMessages.h"

typedef struct {
	bool processed = false;
	PyObject* model = NULL;
	float* coords = nullptr;
} ModModel;

typedef struct {
	PyObject* model = NULL;
	float* coords = nullptr;
} ModLight;

extern PyObject* event_module;

extern PyObject* spaceKey;

extern uint16_t allModelsCreated;

extern PyObject* onModelCreatedPyMeth;

extern uint32_t request;

extern uint8_t  mapID;
extern uint32_t databaseID;

extern BW_NativeC*      BW_Native;
extern HangarMessagesC* HangarMessages;

extern bool isModelsAlreadyInited;

extern bool isTimerStarted;
extern bool isTimeVisible;

extern HANDLE hHangarTimer;
extern HANDLE hBattleTimer;

extern HANDLE hBattleTimerThread;
extern DWORD  battleTimerThreadID;

extern HANDLE hHandlerThread;
extern DWORD  handlerThreadID;

extern uint8_t hangarTimerLastError;
extern uint8_t battleTimerLastError;

extern std::vector<ModModel*> models;
//extern std::vector<ModLight*> lights;

bool write_data(char*, PyObject*);

bool read_data(bool);

void clearModelsSections();

uint8_t findLastModelCoords(float, uint8_t*, float**);

uint8_t delModelPy(float*);

uint8_t delModelCoords(uint8_t, float*);

PyObject* event_light(float coords[3]);

bool setModelPosition(PyObject*, float*);

PyObject* event_model(char*, float coords[3], bool isAsync = false);

PyObject* event_onModelCreated(PyObject*, PyObject*);

uint8_t create_models();

uint8_t init_models();

uint8_t set_visible(bool);

uint8_t del_models();

VOID CALLBACK TimerAPCProc(LPVOID, DWORD, DWORD);