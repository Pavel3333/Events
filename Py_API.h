#pragma once

#include "ThreadSync.h"
#include "BW_native.h"
#include "GUI.h"
#include "HangarMessages.h"


struct ModModel {
	bool processed = false;
	PyObject* model = NULL;
	float* coords = nullptr;
};


struct ModLight {
	PyObject* model = NULL;
	float* coords = nullptr;
};

extern PyObject* event_module;

extern PyObject* keyDelLastModel;

extern uint16_t allModelsCreated;

extern PyObject* onModelCreatedPyMeth;

extern uint32_t request;

extern uint8_t  mapID;
extern uint32_t databaseID;

extern bool isModelsAlreadyInited;

extern bool isTimerStarted;
extern bool isTimeVisible;

extern std::vector<ModModel*> models;
//extern std::vector<ModLight*> lights;

void clearModelsSections();

MyErr delModelPy(float*);

uint8_t delModelCoords(MODEL_ID, float*);

PyObject* event_light(float coords[3]);

bool setModelPosition(PyObject*, const float coords[3]);

PyObject* event_model(char*, float coords[3], bool isAsync = false);

PyObject* event_onModelCreated(PyObject*, PyObject*);

uint8_t create_models();

uint8_t init_models();

uint8_t set_visible(bool);

uint8_t del_models();

VOID CALLBACK TimerAPCProc(LPVOID, DWORD, DWORD);
