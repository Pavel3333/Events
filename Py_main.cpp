#define CONSOLE_VER1

#include "ModThreads.h"
#include "Py_config.h"
#include "BW_native.h"
#include "GUI.h"
#include <cstdlib>
#include <direct.h>


typedef struct {
	bool processed = false;
	PyObject* model = NULL;
	float* coords = nullptr;
} ModModel;

typedef struct {
	PyObject* model = NULL;
	float* coords = nullptr;
} ModLight;

std::vector<ModModel*> models;
std::vector<ModLight*> lights;

std::ofstream dbg_log("NY_Event_debug_log.txt", std::ios::app);

PyObject* event_module = NULL;

PyObject* modGUI = NULL;

PyObject* spaceKey = NULL;

uint16_t allModelsCreated      = NULL;

PyObject* onModelCreatedPyMeth = NULL;

uint8_t  first_check = 100;
uint32_t request     = 100;

uint8_t  mapID      = NULL;
uint32_t databaseID = NULL;

bool isInited = false;

bool battleEnded = true;

bool isModelsAlreadyCreated = false;
bool isModelsAlreadyInited  = false;

bool isTimerStarted = false;
bool isTimeVisible  = false;

bool isStreamer     = false;

HANDLE hTimer         = NULL;

HANDLE hTimerThread   = NULL;
DWORD  timerThreadID  = NULL;

HANDLE hHandlerThread  = NULL;
DWORD  handlerThreadID = NULL;

uint8_t timerLastError = NULL;

//Главные ивенты

PEVENTDATA_1 EVENT_IN_HANGAR   = NULL;
PEVENTDATA_1 EVENT_START_TIMER = NULL;
PEVENTDATA_1 EVENT_DEL_MODEL   = NULL;

//Второстепенные ивенты

PEVENTDATA_2 EVENT_ALL_MODELS_CREATED = NULL;

PEVENTDATA_2 EVENT_BATTLE_ENDED = NULL;

//Мутексы

HANDLE M_NETWORK_NOT_USING = NULL;
HANDLE M_MODELS_NOT_USING  = NULL;

//---------------------

STAGE_ID lastStageID = STAGE_ID::COMPETITION;
EVENT_ID lastEventID = EVENT_ID::IN_HANGAR;

uint8_t handleBattleEndEvent(PyThreadState* _save);
uint8_t makeEventInThread(uint8_t);

void closeEvent1(PEVENTDATA_1* pEvent) {
	traceLog();
	if (*pEvent) {
		traceLog(); //если уже была инициализирована структура - удаляем
		if ((*pEvent)->hEvent) {
			traceLog();
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		} traceLog();

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	} traceLog();
}

void closeEvent2(PEVENTDATA_2* pEvent) {
	traceLog();
	if (*pEvent) {
		traceLog(); //если уже была инициализирована структура - удаляем
		if ((*pEvent)->hEvent) {
			traceLog();
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		} traceLog();

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	} traceLog();
}

bool createEvent1(PEVENTDATA_1* pEvent, uint8_t eventID) {
	traceLog();
	closeEvent1(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_1)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выделяем память в куче для ивента
		sizeof(EVENTDATA_1));

	if (!(*pEvent)) {
		traceLog(); //нехватка памяти, завершаем работу

		ExitProcess(1);
	} traceLog();

	(*pEvent)->hEvent = CreateEvent(
		NULL,                      // default security attributes
		FALSE,                     // auto-reset event
		FALSE,                     // initial state is nonsignaled
		EVENT_NAMES[eventID]       // object name
	);

	if (!((*pEvent)->hEvent)) {
		traceLog();
		INIT_LOCAL_MSG_BUFFER;

		extendedDebugLogFmt("[NY_Event][ERROR]: Primary event creating: error %d\n", GetLastError());

		return false;
	} traceLog();

	return true;
}

bool createEvent2(PEVENTDATA_2* pEvent, LPCWSTR eventName, BOOL isSignaling = FALSE) {
	traceLog();
	closeEvent2(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_2)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выделяем память в куче для ивента
		sizeof(EVENTDATA_2));

	if (!(*pEvent)) {
		traceLog(); //нехватка памяти, завершаем работу

		ExitProcess(1);
	} traceLog();

	(*pEvent)->hEvent = CreateEvent(
		NULL,                      // default security attributes
		FALSE,                     // auto-reset event
		isSignaling,
		eventName                  // object name
	);

	if (!((*pEvent)->hEvent)) {
		traceLog();
		INIT_LOCAL_MSG_BUFFER;

		extendedDebugLogFmt("[NY_Event][ERROR]: Secondary event creating: error %d\n", GetLastError());

		return false;
	} traceLog();

	return true;
}

VOID CALLBACK TimerAPCProc(
	LPVOID lpArg,               // Data value
	DWORD dwTimerLowValue,      // Timer low value
	DWORD dwTimerHighValue)     // Timer high value
{
	// Formal parameters not used in this example.
	UNREFERENCED_PARAMETER(lpArg);

	UNREFERENCED_PARAMETER(dwTimerLowValue);
	UNREFERENCED_PARAMETER(dwTimerHighValue);
}

bool write_data(char* data_path, PyObject* data_p) { traceLog();

	PyObject* arg2 = Py_False;
	Py_INCREF(arg2);

	PyObject* arg3 = Py_True;
	Py_INCREF(arg3);

	PyObject* arg4 = Py_True;
	Py_INCREF(arg4);

	PyObject* arg5 = Py_True;
	Py_INCREF(arg5);

	PyObject* arg6 = Py_None;
	Py_INCREF(arg6);

	PyObject* indent = PyInt_FromSize_t(4);
	Py_INCREF(indent);

	PyObject* __dumps = PyString_FromString("dumps");

	PyObject* data_json_s = PyObject_CallMethodObjArgs(json, __dumps, data_p, arg2, arg3, arg4, arg5, arg6, indent, NULL);

	Py_DECREF(__dumps);
	Py_DECREF(arg2);
	Py_DECREF(arg3);
	Py_DECREF(arg4);
	Py_DECREF(arg5);
	Py_DECREF(arg6);
	Py_DECREF(indent);

	if (!data_json_s) { traceLog();
		return false;
	} traceLog();

	size_t data_size = PyObject_Length(data_json_s);

	std::ofstream data_w(data_path);

	data_w.write(PyString_AS_STRING(data_json_s), data_size);

	data_w.close();

	Py_DECREF(data_json_s);

	return true;
}

bool read_data(bool isData) { traceLog();
	char* data_path;
	PyObject* data_src;
	if (isData) { traceLog(); 
		data_path = "mods/configs/pavel3333/NY_Event/NY_Event.json";
		data_src = g_self->data;
	}
	else {
		data_src = g_self->i18n;
		data_path = "mods/configs/pavel3333/NY_Event/i18n/ru.json";
	} traceLog();

	std::ifstream data(data_path, std::ios::binary);

	if (!data.is_open()) { traceLog();
		data.close();
		if (!write_data(data_path, data_src)) { traceLog();
			return false;
		} traceLog();
	}
	else {
		data.seekg(0, std::ios::end);
		size_t size = (size_t)data.tellg(); //getting file size
		data.seekg(0);

		char* data_s = new char[size + 1];

		while (!data.eof()) {
			data.read(data_s, size);
		} traceLog();

		data.close();

		PyObject* data_p = PyString_FromString(data_s);

		delete[] data_s;

		PyObject* __loads = PyString_FromString("loads");

		PyObject* data_json_s = PyObject_CallMethodObjArgs(json, __loads, data_p, NULL);

		Py_DECREF(__loads);
		Py_DECREF(data_p);

		if (!data_json_s) { traceLog();
			PyErr_Clear();

			if (!write_data(data_path, data_src)) { traceLog();
				return false;
			} traceLog();

			return true;
		} traceLog();

		PyObject* old = data_src;
		if (isData) g_self->data = data_json_s;
		else g_self->i18n = data_json_s;

		PyDict_Clear(old);
		Py_DECREF(old);
	} traceLog();

	return true;
}

void clearModelsSections() { traceLog();
	std::vector<ModelsSection>::iterator it_sect = current_map.modelsSects.begin();

	while (it_sect != current_map.modelsSects.end()) {
		if (!it_sect->models.empty() && it_sect->isInitialised) {
			std::vector<float*>::iterator it_model = it_sect->models.begin();

			while (it_model != it_sect->models.end()) {
				if (*it_model == nullptr) { traceLog();
					it_model = it_sect->models.erase(it_model);

					continue;
				}

				for (uint8_t counter = NULL; counter < 3; counter++) {
					memset(&(*it_model)[counter], NULL, 4);
				}

				delete[] *it_model;
				*it_model = nullptr;

				it_model = it_sect->models.erase(it_model);
			}

			it_sect->models.~vector();
		}

		if (it_sect->path != nullptr) {
			delete[] it_sect->path;

			it_sect->path = nullptr;
		}

		it_sect = current_map.modelsSects.erase(it_sect);
	} traceLog();

	current_map.modelsSects.~vector();

	std::vector<ModelsSection>::iterator it_sect_sync = sync_map.modelsSects_deleting.begin();

	while (it_sect_sync != sync_map.modelsSects_deleting.end()) {
		if (it_sect_sync->isInitialised) {
			std::vector<float*>::iterator it_model = it_sect_sync->models.begin();

			while (it_model != it_sect_sync->models.end()) {
				if (*it_model != nullptr) { traceLog();
					delete[] * it_model;
					*it_model = nullptr;
				}

				it_model = it_sect_sync->models.erase(it_model);
			}
		}

		if (it_sect_sync->path != nullptr) {
			delete[] it_sect_sync->path;

			it_sect_sync->path = nullptr;
		}

		it_sect_sync->models.~vector();

		it_sect_sync = sync_map.modelsSects_deleting.erase(it_sect_sync); //удаляем секцию из вектора секций синхронизации
	} traceLog();

	sync_map.modelsSects_deleting.~vector();
}

uint8_t findLastModelCoords(float dist_equal, uint8_t* modelID, float** coords) { traceLog();
	PyObject* __player = PyString_FromString("player");

	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	isModelsAlreadyCreated = false;

	if (!player) { traceLog();
		return 1;
	} traceLog();

	PyObject* __vehicle = PyString_FromString("vehicle");
	PyObject* vehicle = PyObject_GetAttr(player, __vehicle);

	Py_DECREF(__vehicle);

	Py_DECREF(player);

	if (!vehicle) { traceLog();
		return 2;
	} traceLog();

	PyObject* __model = PyString_FromString("model");
	PyObject* model_p = PyObject_GetAttr(vehicle, __model);

	Py_DECREF(__model);
	Py_DECREF(vehicle);

	if (!model_p) { traceLog();
		return 3;
	} traceLog();

	PyObject* __position = PyString_FromString("position");
	PyObject* position_Vec3 = PyObject_GetAttr(model_p, __position);

	Py_DECREF(__position);
	Py_DECREF(model_p);

	if (!position_Vec3) { traceLog();
		return 4;
	} traceLog();

	double* coords_pos = new double[3];

	for (uint8_t i = NULL; i < 3; i++) {
		PyObject* __tuple = PyString_FromString("tuple");

		PyObject* position = PyObject_CallMethodObjArgs(position_Vec3, __tuple, NULL);

		Py_DECREF(__tuple);

		if (!position) { traceLog();
			return 5;
		} traceLog();

		PyObject* coord_p = PyTuple_GetItem(position, i);

		if (!coord_p) { traceLog();
			return 6;
		} traceLog();

		coords_pos[i] = PyFloat_AS_DOUBLE(coord_p);
	} traceLog();

	double distTemp;
	double dist = -1.0;
	int8_t modelTypeLast = -1;
	float* coords_res = nullptr;

	for (auto it = current_map.modelsSects.cbegin();
		it != current_map.modelsSects.cend();
		it++) {
		if (it->isInitialised) {
			for (auto it2 = it->models.cbegin();
				it2 != it->models.cend();
				it2++) { traceLog();
				if (*it2 == nullptr) { traceLog();
					continue;
				}

				distTemp = getDist2Points(coords_pos, *it2);

				if (dist == -1.0 || distTemp < dist) {
					dist = distTemp;
					modelTypeLast = it->ID;

					coords_res = *it2;
				}
			}
		}
	} traceLog();

	delete[] coords_pos;

	if (dist == -1.0 || modelTypeLast == -1 || coords_res == nullptr) { traceLog();
		return 8;
	} traceLog();

	if (dist > dist_equal) { traceLog();
		return 7;
	} traceLog();

	*modelID = modelTypeLast;
	*coords = coords_res;

	return NULL;
}

uint8_t delModelPy(float* coords) { traceLog();
	if (coords == nullptr) { traceLog();
		return 1;
	} traceLog();

	std::vector<ModModel*>::iterator it_model = models.begin();

	while (it_model != models.end()) {
		if (*it_model == nullptr) { traceLog();
			it_model = models.erase(it_model);

			continue;
		}

		if (!(*it_model)->model || (*it_model)->model == Py_None || !(*it_model)->processed) {
			superExtendedDebugLog("NULL\n");

			Py_XDECREF((*it_model)->model);

			(*it_model)->model = NULL;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			continue;
		}

		superExtendedDebugLog("[NY_Event]: del debug 1.1\n");

		if ((*it_model)->coords[0] == coords[0] &&
			(*it_model)->coords[1] == coords[1] &&
			(*it_model)->coords[2] == coords[2]) {

			PyObject* py_visible = PyBool_FromLong(false);

			PyObject* __visible = PyString_FromString("visible");

			if (!PyObject_SetAttr((*it_model)->model, __visible, py_visible)) { traceLog();
				Py_DECREF(py_visible);
				
				return NULL;
			}

			Py_DECREF(py_visible);

			return 3;

			/*PyObject* __delModel = PyString_FromString("delModel");

			PyObject* result = PyObject_CallMethodObjArgs(BigWorld, __delModel, (*it_model)->model, NULL);

			Py_DECREF(__delModel);

			if (result) {
				Py_DECREF(result);

				Py_XDECREF((*it_model)->model);

				(*it_model)->model = NULL;
				(*it_model)->coords = nullptr;
				(*it_model)->processed = false;

				delete *it_model;
				*it_model = nullptr;

				it_model = models.erase(it_model);

				return NULL;
			}*/
		}

		it_model++;
	} traceLog();

	superExtendedDebugLog("[NY_Event]: del debug 1.2\n");

	return 2;
}

uint8_t delModelCoords(uint16_t ID, float* coords) { traceLog();
	if (coords == nullptr) { traceLog();
		return 1;
	} traceLog();

	std::vector<ModelsSection>::iterator it_sect = current_map.modelsSects.begin();

	float* model;

	while (it_sect != current_map.modelsSects.end()) {
		if (!it_sect->models.empty() && it_sect->isInitialised) {
			if (it_sect->ID == ID) {
				std::vector<float*>::iterator it_model = it_sect->models.begin();

				while (it_model != it_sect->models.end()) {
					if (*it_model == nullptr) { traceLog();
						it_model = it_sect->models.erase(it_model);

						continue;
					}

					model = *it_model;

					if (model[0] == coords[0] &&
						model[1] == coords[1] &&
						model[2] == coords[2]
						) {
						for (uint8_t counter = NULL; counter < 3; counter++) {
							memset(&model[counter], NULL, 4);
						}

						delete[] * it_model;
						*it_model = nullptr;

						it_sect->models.erase(it_model);

						return NULL;
					}

					it_model++;
				}
			}
		}

		it_sect++;
	} traceLog();

	return 2;
}

static PyObject* event_light(float coords[3]) { traceLog();
	if (!isInited || battleEnded) { traceLog();
		return NULL;
	} traceLog();

	superExtendedDebugLog("light creating...\n");

	PyObject* __PyOmniLight = PyString_FromString("PyOmniLight");

	PyObject* Light = PyObject_CallMethodObjArgs(BigWorld, __PyOmniLight, NULL);

	Py_DECREF(__PyOmniLight);

	if (!Light) { traceLog();
		superExtendedDebugLog("PyOmniLight creating FAILED\n");

		return NULL;
	} traceLog();

	//---------inner radius---------

	PyObject* __innerRadius = PyString_FromString("innerRadius");
	PyObject* innerRadius = PyFloat_FromDouble(0.75);

	if (PyObject_SetAttr(Light, __innerRadius, innerRadius)) { traceLog();
		superExtendedDebugLog("PyOmniLight innerRadius setting FAILED\n");

		Py_DECREF(__innerRadius);
		Py_DECREF(innerRadius);
		Py_DECREF(Light);

		return NULL;
	} traceLog();

	Py_DECREF(__innerRadius);

	//---------outer radius---------

	PyObject* __outerRadius = PyString_FromString("outerRadius");
	PyObject* outerRadius = PyFloat_FromDouble(1.5);

	if (PyObject_SetAttr(Light, __outerRadius, outerRadius)) { traceLog();
		superExtendedDebugLog("PyOmniLight outerRadius setting FAILED\n");

		Py_DECREF(outerRadius);
		Py_DECREF(__outerRadius);
		Py_DECREF(Light);

		return NULL;
	} traceLog();

	Py_DECREF(__outerRadius);

	//----------multiplier----------

	PyObject* __multiplier = PyString_FromString("multiplier");
	PyObject* multiplier = PyFloat_FromDouble(500.0);

	if (PyObject_SetAttr(Light, __multiplier, multiplier)) { traceLog();
		superExtendedDebugLog("PyOmniLight multiplier setting FAILED\n");

		Py_DECREF(multiplier);
		Py_DECREF(__multiplier);
		Py_DECREF(Light);

		return NULL;
	} traceLog();

	Py_DECREF(__multiplier);

	//-----------position-----------

	PyObject* coords_p = PyTuple_New(3);

	if (!coords_p) { traceLog();
		superExtendedDebugLog("PyOmniLight coords creating FAILED\n");

		Py_DECREF(Light);

		return NULL;
	} traceLog();

	for (uint8_t i = NULL; i < 3; i++) {
		if (i == 1) {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i] + 0.5));
		}
		else {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i]));
		}
	} traceLog();

	PyObject* __position = PyString_FromString("position");

	if (PyObject_SetAttr(Light, __position, coords_p)) { traceLog();
		superExtendedDebugLog("PyOmniLight coords setting FAILED\n");

		Py_DECREF(__position);
		Py_DECREF(coords_p);
		Py_DECREF(Light);

		return NULL;
	} traceLog();

	Py_DECREF(__position);

	//------------colour------------

	PyObject* colour_p = PyTuple_New(4);

	if (!colour_p) { traceLog();
		superExtendedDebugLog("PyOmniLight colour creating FAILED\n");

		Py_DECREF(Light);

		return NULL;
	} traceLog();

	double* colour = new double[5];

	colour[0] = 255.0;
	colour[1] = 255.0;
	colour[2] = 255.0;
	colour[3] = 0.0;

	for (uint8_t i = NULL; i < 4; i++) PyTuple_SET_ITEM(colour_p, i, PyFloat_FromDouble(colour[i]));

	delete[] colour;

	//------------------------------

	PyObject* __colour = PyString_FromString("colour");

	if (PyObject_SetAttr(Light, __colour, colour_p)) { traceLog();
		superExtendedDebugLog("PyOmniLight colour setting FAILED\n");
		
		Py_DECREF(__colour);
		Py_DECREF(colour_p);
		Py_DECREF(Light);

		return NULL;
	} traceLog();

	Py_DECREF(__colour);

	superExtendedDebugLog("light creating OK!\n");

	return Light;
}

bool setModelPosition(PyObject* Model, float* coords_f) { traceLog();
	PyObject* coords_p = PyTuple_New(3);

	for (uint8_t i = NULL; i < 3; i++) PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords_f[i]));

	PyObject* __position = PyString_FromString("position");

	if (PyObject_SetAttr(Model, __position, coords_p)) { traceLog();
		Py_DECREF(__position);
		Py_DECREF(coords_p);
		Py_DECREF(Model);

		return false;
	} traceLog();

	Py_DECREF(__position);

	return true;
}

static PyObject* event_model(char* path, float coords[3], bool isAsync=false) { traceLog();
	if (!isInited || battleEnded) { traceLog();
		if (isAsync && allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

		return NULL;
	} traceLog();

	superExtendedDebugLog("model creating...\n");

	PyObject* Model = NULL;

	if (isAsync) { traceLog();
		if (coords == nullptr) { traceLog(); 
			if (allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return NULL;
		} traceLog();

		PyObject* __partial = PyString_FromString("partial");

		PyObject* coords_p = PyLong_FromVoidPtr((void*)coords); //передаем указатель на 3 координаты

		PyObject* partialized = PyObject_CallMethodObjArgs(functools, __partial, onModelCreatedPyMeth, coords_p, NULL);

		Py_DECREF(__partial);

		if (!partialized) { traceLog();
			if (allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return NULL;
		} traceLog();

		PyObject* __fetchModel = PyString_FromString("fetchModel");

		Model = PyObject_CallMethodObjArgs(BigWorld, __fetchModel, PyString_FromString(path), partialized, NULL); //запускаем асинхронное добавление модели

		Py_XDECREF(Model);
		Py_DECREF(__fetchModel);

		return NULL;
	} traceLog();

	PyObject* __Model = PyString_FromString("Model");

	Model = PyObject_CallMethodObjArgs(BigWorld, __Model, PyString_FromString(path), NULL);

	Py_DECREF(__Model);

	if (!Model) { traceLog();
		return NULL;
	} traceLog();
	
	if (coords != nullptr) { traceLog();
		if (!setModelPosition(Model, coords)) { traceLog(); //ставим на нужную позицию
			return NULL;
		} traceLog();
	} traceLog();

	superExtendedDebugLog("model creating OK!\n");

	return Model;
};

static PyObject* event_onModelCreated(PyObject *self, PyObject *args) { traceLog(); //принимает аргументы: указатель на координаты и саму модель
	if (!isInited || battleEnded || models.size() >= allModelsCreated) { traceLog();
		Py_RETURN_NONE;
	} traceLog();

	if (!EVENT_ALL_MODELS_CREATED->hEvent) { traceLog();
		extendedDebugLog("[NY_Event][ERROR]: AMCEvent or createModelsPyMeth event is NULL!\n");

		Py_RETURN_NONE;
	} traceLog();

	//рабочая часть

	PyObject* coords_pointer = NULL;
	PyObject* Model    = NULL;

	if (!PyArg_ParseTuple(args, "OO", &coords_pointer, &Model)) { traceLog();
		Py_RETURN_NONE;
	} traceLog();

	if (!Model || Model == Py_None) { traceLog();
		Py_XDECREF(Model);

		Py_RETURN_NONE;
	} traceLog();

	if(!coords_pointer) { traceLog();
		Py_DECREF(Model);

		Py_RETURN_NONE;
	} traceLog();

	void* coords_vptr = PyLong_AsVoidPtr(coords_pointer);

	if (!coords_vptr) { traceLog();
		Py_DECREF(coords_pointer);
		Py_DECREF(Model);

		Py_RETURN_NONE;
	} traceLog();

	float* coords_f = (float*)(coords_vptr);

	if (!setModelPosition(Model, coords_f)) { traceLog(); //ставим на нужную позицию
		Py_RETURN_NONE;
	} traceLog();

	Py_INCREF(Model);

	ModModel* newModel = new ModModel {
		false,
		Model,
		coords_f
	};

	ModLight* newLight = new ModLight {
		event_light(coords_f),
		coords_f
	};

	models.push_back(newModel);
	lights.push_back(newLight);

	if (models.size() >= allModelsCreated) { traceLog(); //если число созданных моделей - столько же или больше, чем надо
		//сигналим о том, что все модели были успешно созданы

		if (!SetEvent(EVENT_ALL_MODELS_CREATED->hEvent))
		{
			extendedDebugLog("[NY_Event][ERROR]: AMCEvent event not setted!\n");

			Py_RETURN_NONE;
		} traceLog();
	} traceLog();

	Py_RETURN_NONE;
}

uint8_t create_models() { traceLog();
	if (!isInited || battleEnded || !onModelCreatedPyMeth || !M_MODELS_NOT_USING) { traceLog();
		return 1;
	} traceLog();

	INIT_LOCAL_MSG_BUFFER;

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog();
			extendedDebugLog("[NY_Event]: MODELS_USING\n");

			for (auto it = current_map.modelsSects.begin(); //первый проход - получаем число всех созданных моделей
				it != current_map.modelsSects.end();
				it++) {

				if (!it->isInitialised || it->models.empty()) {
					traceLog();
					continue;
				}

				auto it2 = it->models.begin();

				while (it2 != it->models.end()) {
					if (*it2 == nullptr) {
						traceLog();
						it2 = it->models.erase(it2);

						continue;
					}

					allModelsCreated++;

					it2++;
				}
			} traceLog();

			//освобождаем мутекс для этого потока

			if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog();
				extendedDebugLogFmt("[NY_Event][ERROR]: create_models - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());
			}

			extendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog();
			extendedDebugLog("[NY_Event][ERROR]: create_models - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			return 3;
	END_USING_MODELS;

	for (auto it = current_map.modelsSects.cbegin(); //второй проход - создаем модели
		it != current_map.modelsSects.cend();
		it++) {
		if (!it->isInitialised || it->models.empty()) { traceLog();
			continue;
		}
		
		for (auto it2 = it->models.cbegin(); it2 != it->models.cend(); it2++) {
			superExtendedDebugLog("[");

			event_model(it->path, *it2, true);

			superExtendedDebugLog("], ");
		}
	} traceLog();

	superExtendedDebugLog("], \n");

	return NULL;
}

uint8_t init_models() { traceLog();
	if (!isInited || first_check || battleEnded || models.empty()) { traceLog();
		return 1;
	} traceLog();

	extendedDebugLog("[NY_Event]: models adding...\n");

	for (uint16_t i = NULL; i < models.size(); i++) {
		if (models[i] == nullptr) { traceLog();
			continue;
		}

		if (models[i]->model == Py_None || !models[i]->model || models[i]->processed) { traceLog();
			Py_XDECREF(models[i]->model);

			models[i]->model = NULL;
			models[i]->coords = nullptr;
			models[i]->processed = false;

			delete models[i];
			models[i] = nullptr;

			continue;
		}

		superExtendedDebugLog("[NY_Event]: addModel debug 2.1\n");

		PyObject* __addModel = PyString_FromString("addModel");

		PyObject* result = PyObject_CallMethodObjArgs(BigWorld, __addModel, models[i]->model, NULL);

		Py_DECREF(__addModel);

		superExtendedDebugLog("[NY_Event]: addModel debug 2.2\n");

		if (result) {
			Py_DECREF(result);

			models[i]->processed = true;

			superExtendedDebugLog("True\n");
		}
		else superExtendedDebugLog("False\n");
		superExtendedDebugLog("[NY_Event]: addModel debug 2.3\n");
	} traceLog();

	extendedDebugLog("[NY_Event]: models adding OK!\n");

	return NULL;
}

uint8_t set_visible(bool isVisible) { traceLog();
	if (!isInited || first_check || battleEnded || models.empty()) { traceLog();
		return 1;
	} traceLog();

	PyObject* py_visible = PyBool_FromLong(isVisible);

	extendedDebugLog("[NY_Event]: Models visiblity changing...\n");

	for (uint16_t i = NULL; i < models.size(); i++) {
		if (models[i] == nullptr) { traceLog();
			continue;
		}

		if (!models[i]->model || models[i]->model == Py_None || !models[i]->processed) { traceLog();
			Py_XDECREF(models[i]->model);

			models[i]->model = NULL;
			models[i]->coords = nullptr;
			models[i]->processed = false;

			delete models[i];
			models[i] = nullptr;

			continue;
		}

		PyObject* __visible = PyString_FromString("visible");

		PyObject_SetAttr(models[i]->model, __visible, py_visible);

		Py_DECREF(__visible);
	} traceLog();

	Py_DECREF(py_visible);

	extendedDebugLog("[NY_Event]: Models visiblity changing OK!\n");

	return NULL;
};

uint8_t del_models() {
	traceLog();
	if (!isInited || first_check || battleEnded) {
		traceLog();
		return 1;
	} traceLog();

	extendedDebugLog("[NY_Event]: models deleting...\n");

	std::vector<ModModel*>::iterator it_model = models.begin();

	while (it_model != models.end()) {
		if (*it_model == nullptr) {
			traceLog();
			it_model = models.erase(it_model);

			continue;
		}

		if (!(*it_model)->model || (*it_model)->model == Py_None || !(*it_model)->processed) {
			traceLog();
			superExtendedDebugLog("NULL\n");

			Py_XDECREF((*it_model)->model);

			(*it_model)->model = NULL;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			continue;
		}

		superExtendedDebugLog("[NY_Event]: del debug 1.1\n");

		PyObject* __delModel = PyString_FromString("delModel");

		PyObject* result = PyObject_CallMethodObjArgs(BigWorld, __delModel, (*it_model)->model, NULL);

		Py_DECREF(__delModel);

		if (result) {
			Py_DECREF(result);

			Py_XDECREF((*it_model)->model);

			(*it_model)->model = NULL;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			continue;
		}

		it_model++;

		superExtendedDebugLog("[NY_Event]: del debug 1.2\n");
	} traceLog();

	std::vector<ModLight*>::iterator it_light = lights.begin();

	while (it_light != lights.end()) {
		if (*it_light == nullptr) {
			traceLog();
			it_light = lights.erase(it_light);

			continue;
		}

		superExtendedDebugLog("[NY_Event]: del debug 1.1\n");

		if (!(*it_light)->model || (*it_light)->model == Py_None) {
			traceLog();
			superExtendedDebugLog("NULL\n");

			Py_XDECREF((*it_light)->model);

			(*it_light)->model = NULL;
			(*it_light)->coords = nullptr;

			delete *it_light;
			*it_light = nullptr;

			it_light = lights.erase(it_light);

			continue;
		}

		it_light++;

		superExtendedDebugLog("[NY_Event]: del debug 1.2\n");
	} traceLog();

	extendedDebugLog("[NY_Event]: models deleting OK!\n");

	return NULL;
};

uint8_t handleBattleEvent(EVENT_ID eventID) { traceLog();
	if (!isInited || first_check || battleEnded || !g_self || eventID == EVENT_ID::IN_HANGAR || !M_MODELS_NOT_USING) { traceLog();
		return 1;
	} traceLog();

	INIT_LOCAL_MSG_BUFFER;

	extendedDebugLog("[NY_Event]: parsing...\n");

	uint8_t parsing_result = NULL;

	PyThreadState *_save;

	Py_UNBLOCK_THREADS;

	if      (eventID == EVENT_ID::IN_BATTLE_GET_FULL) parsing_result = parse_config();
	else if (eventID == EVENT_ID::IN_BATTLE_GET_SYNC) parsing_result = parse_sync();
	else if (eventID == EVENT_ID::DEL_LAST_MODEL)     parsing_result = parse_del_model();

	if (parsing_result) { traceLog();
		extendedDebugLogFmt("[NY_Event]: parsing FAILED! Error code: %d\n", (uint32_t)parsing_result);

		//GUI_setError(parsing_result);

		return 2;
	} traceLog();

	Py_BLOCK_THREADS;

	extendedDebugLog("[NY_Event]: parsing OK!\n");
	
	if (current_map.time_preparing)  //выводим время
		GUI_setTime(current_map.time_preparing);
		
	if (current_map.stageID >= 0 && current_map.stageID < STAGES_COUNT) { traceLog();    //выводим сообщение
		if (
			current_map.stageID == STAGE_ID::WAITING      ||
			current_map.stageID == STAGE_ID::START        ||
			current_map.stageID == STAGE_ID::COMPETITION  ||
			current_map.stageID == STAGE_ID::END_BY_TIME  ||
			current_map.stageID == STAGE_ID::END_BY_COUNT ||
			current_map.stageID == STAGE_ID::STREAMER_MODE
			) { traceLog();
			if (lastStageID != STAGE_ID::GET_SCORE && lastStageID != STAGE_ID::ITEMS_NOT_EXISTS) GUI_setMsg(current_map.stageID);

			if(current_map.stageID == STAGE_ID::END_BY_TIME || current_map.stageID == STAGE_ID::END_BY_COUNT) { traceLog();
				current_map.time_preparing = NULL;

				GUI_setTime(NULL);

				if (current_map.stageID == STAGE_ID::END_BY_COUNT) { traceLog();
					GUI_setTimerVisible(false);

					isTimeVisible = false;
				} traceLog();

				GUI_setMsg(current_map.stageID);

				uint8_t event_result = handleBattleEndEvent(_save);

				if (event_result) { traceLog();
					extendedDebugLogFmt("[NY_Event]: Warning - handle_battle_event - event_fini - Error code %d\n", (uint32_t)event_result);

					//GUI_setWarning(event_result);
				} traceLog();
			}
			else {
				if (!isTimeVisible) { traceLog();
					GUI_setTimerVisible(true);

					isTimeVisible = true;
				} traceLog();
			} traceLog();

			if (current_map.stageID == STAGE_ID::START          ||
				current_map.stageID == STAGE_ID::COMPETITION    || 
				current_map.stageID == STAGE_ID::STREAMER_MODE) { traceLog();
				if (isModelsAlreadyCreated && !isModelsAlreadyInited && current_map.minimap_count && current_map.modelsSects.size()) { traceLog();
					if (eventID == EVENT_ID::IN_BATTLE_GET_FULL) { traceLog();
						superExtendedDebugLogFmt("sect count: %u\npos count: %u\n", current_map.modelsSects.size(), current_map.minimap_count);

						extendedDebugLog("[NY_Event]: creating...\n");

						BEGIN_USING_MODELS;
							case WAIT_OBJECT_0: traceLog();
								extendedDebugLog("[NY_Event]: MODELS_USING\n");

								models.~vector();
								lights.~vector();

								//освобождаем мутекс для этого потока

								if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog();
									extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());
								}

								extendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

								break;
							case WAIT_ABANDONED: traceLog();
								extendedDebugLog("[NY_Event][ERROR]: create_models - MODELS_NOT_USING: WAIT_ABANDONED!\n");

								return 3;
						END_USING_MODELS;

						/*
						Первый способ - нативный вызов в main-потоке добавлением в очередь. Ненадёжно!

						int creating_result = Py_AddPendingCall(&create_models, nullptr); //create_models();

						if (creating_result == -1) { traceLog();
							extendedDebugLog("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - create_models - failed to start PendingCall of creating models!\n");

							return 3;
						} traceLog();
						*/

						/*
						Второй способ - вызов асинхронной функции BigWorld.fetchModel(path, onLoadedMethod)

						Более-менее надежно, выполняется на уровне движка
						*/

						request = create_models();

						Py_UNBLOCK_THREADS;

						if (request) { traceLog();
							extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - create_models - Error code %d\n", request);

							return 3;
						} traceLog();

						//ожидаем события полного создания моделей

						extendedDebugLog("[NY_Event]: waiting EVENT_ALL_MODELS_CREATED\n");

						DWORD EVENT_ALL_MODELS_CREATED_WaitResult = WaitForSingleObject(
							EVENT_ALL_MODELS_CREATED->hEvent, // event handle
							INFINITE);                        // indefinite wait

						switch (EVENT_ALL_MODELS_CREATED_WaitResult) {
						case WAIT_OBJECT_0:  traceLog();
							extendedDebugLog("[NY_Event]: EVENT_ALL_MODELS_CREATED signaled!\n");

							//-------------

							//место для рабочего кода

							Py_BLOCK_THREADS;

							extendedDebugLog("[NY_Event]: creating OK!\n");

							request = init_models();

							if (request) { traceLog();
								if (request > 9) { traceLog();
									extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - init_models - Error code %d\n", request);

									//GUI_setError(request);

									return 5;
								} traceLog();

								extendedDebugLogFmt("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - init_models - Warning code %d\n", request);

								//GUI_setWarning(request);

								return 4;
							} traceLog();

							request = set_visible(true);

							if (request) { traceLog();
								if (request > 9) { traceLog();
									extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - set_visible - Error code %d\n", request);

									//GUI_setError(request);

									return 5;
								} traceLog();

								extendedDebugLogFmt("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - set_visible - Warning code %d\n", request);

								//GUI_setWarning(request);

								return 4;
							} traceLog();

							isModelsAlreadyInited = true;

							break;

							// An error occurred
						default: traceLog();
							extendedDebugLog("[NY_Event][ERROR]: IN_HANGAR - something wrong with WaitResult!\n");

							Py_BLOCK_THREADS;

							return 3;
						} traceLog();
					} traceLog();

					return NULL;
				} traceLog();
			} traceLog();

			if (isModelsAlreadyCreated && isModelsAlreadyInited && eventID == EVENT_ID::IN_BATTLE_GET_SYNC && sync_map.all_models_count && !sync_map.modelsSects_deleting.empty()) { traceLog();
				std::vector<ModelsSection>::iterator it_sect_sync;
				std::vector<float*>::iterator        it_model;

				BEGIN_USING_MODELS;
					case WAIT_OBJECT_0: traceLog();
						extendedDebugLog("[NY_Event]: MODELS_USING\n");

						it_sect_sync = sync_map.modelsSects_deleting.begin();

						while (it_sect_sync != sync_map.modelsSects_deleting.end()) {
							if (it_sect_sync->isInitialised) {
								it_model = it_sect_sync->models.begin();

								while (it_model != it_sect_sync->models.end()) {
									if (*it_model == nullptr) { traceLog();
										it_model = it_sect_sync->models.erase(it_model);

										continue;
									}

									request = delModelPy(*it_model);

									if (request) {
										extendedDebugLogFmt("[NY_Event][ERROR]: create_models - delModelPy - Error code %d\n", (uint32_t)request);

										//GUI_setError(request);

										it_model++;

										continue;
									}

									/*

									request = delModelCoords(it_sect_sync->ID, *it_model);

									if (request) {
										extendedDebugLogFmt("[NY_Event]: Error - create_models - delModelCoords - Error code %d\n", request);
								
										//GUI_setError(res);

										it_model++;

										continue;
									}

									*/

									delete[] * it_model;
									*it_model = nullptr;

									it_model = it_sect_sync->models.erase(it_model);
								}
							}

							if (it_sect_sync->path != nullptr) {
								delete[] it_sect_sync->path;

								it_sect_sync->path = nullptr;
							}

							it_sect_sync->models.~vector();

							it_sect_sync = sync_map.modelsSects_deleting.erase(it_sect_sync); //удаляем секцию из вектора секций синхронизации
						} traceLog();

						sync_map.modelsSects_deleting.~vector();

						//освобождаем мутекс для этого потока

						if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog();
							extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());
						}

						extendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

						break;
					case WAIT_ABANDONED: traceLog();
						extendedDebugLog("[NY_Event][ERROR]: handleBattleEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

						return 3;
				END_USING_MODELS;
			}
			else {
				return NULL;
			} traceLog();
		}
		else extendedDebugLog("[NY_Event]: Warning - StageID is not right for this event\n");
	}
	else extendedDebugLog("[NY_Event]: Warning - StageID is not correct\n");

	return NULL;
}

uint8_t handleStartTimerEvent(PyThreadState* _save) {
	INIT_LOCAL_MSG_BUFFER;

	BOOL            bSuccess;
	__int64         qwDueTime;
	LARGE_INTEGER   liDueTime;

	EVENT_ID eventID;

	if (EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_FULL && EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog();
		extendedDebugLog("[NY_Event][ERROR]: START_TIMER - eventID not equal!\n");

		return 2;
	} traceLog();

	if (first_check || battleEnded) { traceLog();
		extendedDebugLog("[NY_Event][ERROR]: START_TIMER - first_check or battleEnded!\n");

		return 3;
	} traceLog();

	//инициализация таймера для получения полного списка моделей и синхронизации

	hTimer = CreateWaitableTimer(
		NULL,                   // Default security attributes
		FALSE,                  // Create auto-reset timer
		TEXT("BattleTimer"));   // Name of waitable timer

	if (hTimer) { traceLog();
		qwDueTime = 0; // задержка перед созданием таймера - 0 секунд

		// Copy the relative time into a LARGE_INTEGER.
		liDueTime.LowPart = (DWORD)NULL;//(DWORD)(qwDueTime & 0xFFFFFFFF);
		liDueTime.HighPart = (LONG)NULL;//(qwDueTime >> 32);

		bSuccess = SetWaitableTimer(
			hTimer,           // Handle to the timer object
			&liDueTime,       // When timer will become signaled
			1000,             // Periodic timer interval of 1 seconds
			TimerAPCProc,     // Completion routine
			NULL,             // Argument to the completion routine
			FALSE);           // Do not restore a suspended system

		if (bSuccess)
		{
			while (!first_check && !battleEnded && !timerLastError) { traceLog();
				if (!isTimerStarted) { traceLog();
					isTimerStarted = true;
				} traceLog();

				//рабочая часть

				eventID = EVENT_ID::IN_BATTLE_GET_FULL;

				if (isModelsAlreadyCreated && isModelsAlreadyInited) eventID = EVENT_ID::IN_BATTLE_GET_SYNC;

				BEGIN_USING_NETWORK;
					case WAIT_OBJECT_0: traceLog();
						extendedDebugLog("[NY_Event]: NETWORK_USING\n");

						request = send_token(databaseID, mapID, eventID);

						if (request) { traceLog();
							if (request > 9) { traceLog();
								extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - send_token - Error code %d\n", request);

								//GUI_setError(request);

								timerLastError = 1;

								//освобождаем мутекс для этого потока

								if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog();
									extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

									return 8;
								}

								extendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

								return 7;
							} traceLog();

							extendedDebugLogFmt("[NY_Event][WARNING]: handleStartTimerEvent - send_token - Warning code %d\n", request);
						} traceLog();

						superExtendedDebugLog("[NY_Event]: generating token OK!\n");

						Py_BLOCK_THREADS;

						request = handleBattleEvent(eventID);

						Py_UNBLOCK_THREADS;

						if (request) {
							traceLog();
							extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - create_models - Error code %d\n", request);

							//GUI_setError(request);

							timerLastError = 2;

							//освобождаем мутекс для этого потока

							if (!ReleaseMutex(M_NETWORK_NOT_USING)) {
								traceLog();
								extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

								return 6;
							}

							extendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

							return 4;
						} traceLog();

						//освобождаем мутекс для этого потока

						if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog();
							extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

							return 6;
						}

						extendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

						break;
					case WAIT_ABANDONED: traceLog();
						extendedDebugLog("[NY_Event][ERROR]: handleStartTimerEvent - NETWORK_NOT_USING: WAIT_ABANDONED!\n");

						return 5;
				END_USING_NETWORK;

				SleepEx(
					INFINITE,     // Wait forever
					TRUE);        // Put thread in an alertable state
			} traceLog();

			if (timerLastError) { traceLog();
				extendedDebugLogFmt("[NY_Event][WARNING]: handleStartTimerEvent: error %d\n", timerLastError);

				CancelWaitableTimer(hTimer);
			} traceLog();
		}
		else extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent - SetWaitableTimer: error %d\n", GetLastError());

		CloseHandle(hTimer);
	}
	else extendedDebugLogFmt("[NY_Event][ERROR]: handleStartTimerEvent: CreateWaitableTimer: error %d\n", GetLastError());

	return NULL;
}

uint8_t handleInHangarEvent(PyThreadState* _save) {
	INIT_LOCAL_MSG_BUFFER;

	EVENT_ID eventID = EVENT_IN_HANGAR->eventID;

	if (eventID != EVENT_ID::IN_HANGAR) { traceLog();
		extendedDebugLog("[NY_Event][ERROR]: handleInHangarEvent - eventID not equal!\n");

		return 2;
	} traceLog();

	//рабочая часть

	BEGIN_USING_NETWORK;
		case WAIT_OBJECT_0: traceLog();
			extendedDebugLog("[NY_Event]: NETWORK_USING\n");

			first_check = send_token(databaseID, mapID, eventID);

			//освобождаем мутекс для этого потока

			if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog();
				extendedDebugLogFmt("[NY_Event][ERROR]: handleInHangarEvent - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

				return 4;
			}

			extendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog();
			extendedDebugLog("[NY_Event][ERROR]: handleInHangarEvent - NETWORK_NOT_USING: WAIT_ABANDONED!\n");

			return 3;
	END_USING_NETWORK;

	if (first_check) { traceLog();
		if (first_check > 9) { traceLog();
			extendedDebugLogFmt("[NY_Event][ERROR]: handleInHangarEvent - Error code %d\n", first_check);

			//GUI_setError(first_check);

			return 6;
		} traceLog();

		extendedDebugLogFmt("[NY_Event][WARNING]: handleInHangarEvent - Warning code %d\n", first_check);

		//GUI_setWarning(first_check);

		return 5;
	} traceLog();

	return NULL;
}

uint8_t handleBattleEndEvent(PyThreadState* _save) { traceLog();
	if (!isInited || first_check || !M_MODELS_NOT_USING) { traceLog();
		return 1;
	} traceLog();

	Py_UNBLOCK_THREADS;

	INIT_LOCAL_MSG_BUFFER;

	std::vector<ModModel*>::iterator it_model;
	std::vector<ModLight*>::iterator it_light;

	Py_BLOCK_THREADS;

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog();
			extendedDebugLog("[NY_Event]: MODELS_USING\n");

			if (!models.empty()) { traceLog();
				request = del_models();

				if (request) { traceLog();
					extendedDebugLogFmt("[NY_Event][WARNING]: handleBattleEndEvent - del_models: %d\n", request);
				} traceLog();

				it_model = models.begin();

				while (it_model != models.end()) {
					if (*it_model == nullptr) { traceLog();
						it_model = models.erase(it_model);

						continue;
					}

					Py_XDECREF((*it_model)->model);

					(*it_model)->model = NULL;
					(*it_model)->coords = nullptr;
					(*it_model)->processed = false;

					delete *it_model;
					*it_model = nullptr;

					it_model = models.erase(it_model);

					it_model++;
				} traceLog();

				models.~vector();
			} traceLog();

			isModelsAlreadyInited = false;

			if (!lights.empty()) { traceLog();
				it_light = lights.begin();

				while (it_light != lights.end()) {
					if (*it_light == nullptr) { traceLog();
						it_light = lights.erase(it_light);

						continue;
					}

					Py_XDECREF((*it_light)->model);

					(*it_light)->model = NULL;
					(*it_light)->coords = nullptr;

					delete *it_light;
					*it_light = nullptr;

					it_light = lights.erase(it_light);
				} traceLog();

				lights.~vector();
			} traceLog();

			Py_UNBLOCK_THREADS;

			if (!current_map.modelsSects.empty() && current_map.minimap_count) { traceLog();
				clearModelsSections();
			} traceLog();

			isModelsAlreadyCreated = false;

			current_map.minimap_count = NULL;

			//освобождаем мутекс для этого потока

			if (!ReleaseMutex(M_MODELS_NOT_USING)) { traceLog();
				extendedDebugLogFmt("[NY_Event][ERROR]: handleBattleEndEvent - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());
			}

			extendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

			Py_BLOCK_THREADS;

			break;
		case WAIT_ABANDONED: traceLog();
			extendedDebugLog("[NY_Event][ERROR]: handleBattleEndEvent - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			return 2;
	END_USING_MODELS;

	if (isTimeVisible) { traceLog();
		GUI_setTime(NULL);
		GUI_setTimerVisible(false);

		isTimeVisible = false;

		current_map.time_preparing = NULL;
	} traceLog();

	extendedDebugLog("[NY_Event]: fini OK!\n");

	return NULL;
};

//threads functions

/*
ПОЧЕМУ НЕЛЬЗЯ ЗАКРЫВАТЬ ТАЙМЕРЫ В САМИХ СЕБЕ

-поток открыл ожидающий таймер
-таймер и говорит ему: поток, у меня тут ашыпка
-таймер сделал харакири
-поток: ТАЙМЕР!!!
-программа ушла в вечное ожидание
*/

DWORD WINAPI TimerThread(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	if (!isInited || !M_NETWORK_NOT_USING ||!EVENT_START_TIMER) { traceLog();
		return 1;
	} traceLog();

	PyGILState_STATE gstate;

	PyThreadState* _save;

	INIT_LOCAL_MSG_BUFFER;

	extendedDebugLog("[NY_Event]: waiting EVENT_START_TIMER\n");

	DWORD EVENT_START_TIMER_WaitResult = WaitForSingleObject(
		EVENT_START_TIMER->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_START_TIMER_WaitResult) {
	case WAIT_OBJECT_0:  traceLog();
		extendedDebugLog("[NY_Event]: EVENT_START_TIMER signaled!\n");

		//включаем GIL для этого потока

		gstate = PyGILState_Ensure();

		//-----------------------------

		Py_UNBLOCK_THREADS;

		//рабочая часть

		request = handleStartTimerEvent(_save);

		if (request) { traceLog();
			extendedDebugLogFmt("[NY_Event][WARNING]: EVENT_START_TIMER - handleStartTimerEvent: error %d\n", request);
		}

		Py_BLOCK_THREADS;

		//выключаем GIL для этого потока

		PyGILState_Release(gstate);

		//------------------------------

		break;

		// An error occurred
	default: traceLog();
		extendedDebugLog("[NY_Event][ERROR]: START_TIMER - something wrong with WaitResult!\n");

		return 3;
	} traceLog();

	//закрываем поток

	extendedDebugLogFmt("[NY_Event]: Closing timer thread %d\n", handlerThreadID);

	ExitThread(NULL); //завершаем поток

	return NULL;
}

DWORD WINAPI HandlerThread(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	if (!isInited || !M_NETWORK_NOT_USING) { traceLog();
		return 1;
	} traceLog();

	EVENT_ID eventID;

	//включаем GIL для этого потока

	PyGILState_STATE gstate = PyGILState_Ensure();

	//-----------------------------

	PyThreadState* _save;

	Py_UNBLOCK_THREADS;

	INIT_LOCAL_MSG_BUFFER;

	extendedDebugLog("[NY_Event]: waiting EVENT_IN_HANGAR\n");

	DWORD EVENT_IN_HANGAR_WaitResult = WaitForSingleObject(
		EVENT_IN_HANGAR->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_IN_HANGAR_WaitResult) {
	case WAIT_OBJECT_0: traceLog();
		extendedDebugLog("[NY_Event]: EVENT_IN_HANGAR signaled!\n");

		//место для рабочего кода

		request = handleInHangarEvent(_save);

		if (request) { traceLog();
			extendedDebugLogFmt("[NY_Event][WARNING]: EVENT_IN_HANGAR - handleInHangarEvent: error %d\n", request);
		}

		break;
		// An error occurred
	default: traceLog();
		extendedDebugLog("[NY_Event][ERROR]: IN_HANGAR - something wrong with WaitResult!\n");

		return 3;
	} traceLog();
	if (first_check) { traceLog();
		extendedDebugLogFmt("[NY_Event][ERROR]: IN_HANGAR - Error %d!\n", (uint32_t)first_check);

		return 4;
	} traceLog();

	if (hTimerThread) { traceLog();
		CloseHandle(hTimerThread);

		hTimerThread = NULL;
	} traceLog();

	hTimerThread = CreateThread( //создаем поток с таймером
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		TimerThread,                            // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&timerThreadID);                        // returns the thread identifier 

	if (!hTimerThread)
	{
		extendedDebugLogFmt("[NY_Event][ERROR]: Creating timer thread: error %d\n", GetLastError());

		return 5;
	} traceLog();

	HANDLE hEvents[HEVENTS_COUNT] = {
		EVENT_DEL_MODEL->hEvent,  //событие удаления модели
		EVENT_BATTLE_ENDED->hEvent //событие выхода из боя
	};

	uint8_t find_result;
	uint8_t server_req;
	uint8_t deleting_py_models;

	uint8_t modelID;
	float* coords;

	uint8_t lastEventError = NULL;

	while (!first_check && !battleEnded && !lastEventError) { traceLog();
		extendedDebugLog("[NY_Event]: waiting EVENTS\n");

		DWORD EVENTS_WaitResult = WaitForMultipleObjects(
			HEVENTS_COUNT,
			hEvents,
			FALSE,
			INFINITE);

		switch (EVENTS_WaitResult) {
		case WAIT_OBJECT_0 + 0:  traceLog(); //сработало событие удаления модели
			extendedDebugLog("[NY_Event]: DEL_LAST_MODEL signaled!\n");

			//место для рабочего кода

			eventID    = EVENT_DEL_MODEL->eventID;

			if (eventID != EVENT_ID::DEL_LAST_MODEL) { traceLog();
				extendedDebugLog("[NY_Event][ERROR]: DEL_LAST_MODEL - eventID not equal!\n");

				lastEventError = 6;

				break;
			} traceLog();

			//рабочая часть

			coords = new float[3];

			Py_BLOCK_THREADS;

			find_result = findLastModelCoords(5.0, &modelID, &coords);

			Py_UNBLOCK_THREADS;

			if (!find_result) { traceLog();
				BEGIN_USING_NETWORK;
					case WAIT_OBJECT_0: traceLog();
						extendedDebugLog("[NY_Event]: NETWORK_USING\n");

						server_req = send_token(databaseID, mapID, EVENT_ID::DEL_LAST_MODEL, modelID, coords);

						//освобождаем мутекс для этого потока

						if (!ReleaseMutex(M_NETWORK_NOT_USING)) { traceLog();
							extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - NETWORK_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

							return 4;
						}

						extendedDebugLog("[NY_Event]: NETWORK_NOT_USING\n");

						break;
					case WAIT_ABANDONED: traceLog();
						extendedDebugLog("[NY_Event][ERROR]: DEL_LAST_MODEL - NETWORK_NOT_USING: WAIT_ABANDONED!\n");

						return 3;
				END_USING_NETWORK;

				if (server_req) { traceLog();
					if (server_req > 9) { traceLog();
						extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - send_token - Error code %d\n", (uint32_t)server_req);

						//GUI_setError(server_req);

						lastEventError = 5;

						break;
					} traceLog();

					extendedDebugLogFmt("[NY_Event][WARNING]: DEL_LAST_MODEL - send_token - Warning code %d\n", (uint32_t)server_req);

					//GUI_setWarning(server_req);

					lastEventError = 4;

					break;
				} traceLog();

				deleting_py_models = delModelPy(coords);

				if (deleting_py_models) { traceLog();
					extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - delModelPy - Error code %d\n", (uint32_t)deleting_py_models);

					//GUI_setError(deleting_py_models);

					lastEventError = 3;

					break;
				} traceLog();

				scoreID = modelID;
				current_map.stageID = STAGE_ID::GET_SCORE;

				/*
				uint8_t deleting_coords = delModelCoords(modelID, coords);

				if (deleting_coords) { traceLog();
						extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - delModelCoords - Error code %d\n", deleting_coords);
						
						//GUI_setError(deleting_coords);

						return 6;
				} traceLog();
				*/
			}
			else if (find_result == 7) { traceLog();
				current_map.stageID = STAGE_ID::ITEMS_NOT_EXISTS;
			} traceLog();

			Py_BLOCK_THREADS;

			if (current_map.stageID == STAGE_ID::GET_SCORE && scoreID != -1) { traceLog();
				GUI_setMsg(current_map.stageID, scoreID, 5.0f);

				scoreID = -1;
			}
			else if (current_map.stageID == STAGE_ID::ITEMS_NOT_EXISTS) { traceLog();
				GUI_setMsg(current_map.stageID);
			} traceLog();

			Py_UNBLOCK_THREADS;

			break;
		case WAIT_OBJECT_0 + 1: traceLog(); //сработало событие окончания боя
			if (hTimer) { traceLog(); //закрываем таймер, если он был создан
				CloseHandle(hTimer);

				hTimer = NULL;
			} traceLog();

			isTimerStarted = false;

			closeEvent1(&EVENT_START_TIMER);
			closeEvent1(&EVENT_IN_HANGAR);
			closeEvent1(&EVENT_DEL_MODEL);

			closeEvent2(&EVENT_ALL_MODELS_CREATED);
			closeEvent2(&EVENT_BATTLE_ENDED);

			if (M_NETWORK_NOT_USING) { traceLog();
				CloseHandle(M_NETWORK_NOT_USING);

				M_NETWORK_NOT_USING = NULL;
			} traceLog();

			if (M_MODELS_NOT_USING) { traceLog();
				CloseHandle(M_MODELS_NOT_USING);

				M_MODELS_NOT_USING = NULL;
			} traceLog();

			request = handleBattleEndEvent(_save);

			if (!request) { traceLog();
				battleEnded = true;

				current_map.stageID = STAGE_ID::COMPETITION;

				PyObject* delLabelCBID_p = GUI_getAttr("delLabelCBID");

				if (!delLabelCBID_p || delLabelCBID_p == Py_None) { traceLog();
					delLabelCBID = NULL;

					Py_XDECREF(delLabelCBID_p);
				}
				else { traceLog();
					delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);

					Py_DECREF(delLabelCBID_p);
				} traceLog();

				cancelCallback(&delLabelCBID);

				allModelsCreated = NULL;

				GUI_setVisible(false);
				GUI_clearText();

				request = NULL;
				mapID = NULL;
			}

			break;
			// An error occurred
		default: traceLog();
			extendedDebugLog("[NY_Event][WARNING]: EVENTS - something wrong with WaitResult!\n");

			lastEventError = 1;

			break;
		} traceLog();
	} traceLog();

	if (lastEventError) extendedDebugLogFmt("Error in event: %d\n", lastEventError);

	//закрываем поток

	extendedDebugLogFmt("Closing handler thread %d\n", handlerThreadID);

	Py_BLOCK_THREADS;

	//выключаем GIL для этого потока

	PyGILState_Release(gstate);

	//------------------------------

	ExitThread(NULL); //завершаем поток

	return NULL;
}

//-----------------

uint8_t makeEventInThread(EVENT_ID eventID) { traceLog(); //переводим ивенты в сигнальные состояния
	if (!isInited || !databaseID || battleEnded) { traceLog();
		return 1;
	} traceLog();

	INIT_LOCAL_MSG_BUFFER;

	if (eventID == EVENT_ID::IN_HANGAR || eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC || eventID == EVENT_ID::DEL_LAST_MODEL) { traceLog(); //посылаем ивент и обрабатываем в треде
		if      (eventID == EVENT_ID::IN_HANGAR) { traceLog();
			if (!EVENT_IN_HANGAR) { traceLog();
				return 4;
			} traceLog();

			EVENT_IN_HANGAR->eventID = eventID;

			if (!SetEvent(EVENT_IN_HANGAR->hEvent))
			{
				debugLogFmt("[NY_Event][ERROR]: EVENT_IN_HANGAR not setted!\n");

				return 5;
			} traceLog();
		}
		else if (eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC) { traceLog();
			if (!EVENT_START_TIMER) { traceLog();
				return 4;
			} traceLog();

			EVENT_START_TIMER->eventID    = eventID;

			if (!SetEvent(EVENT_START_TIMER->hEvent))
			{
				debugLogFmt("[NY_Event][ERROR]: EVENT_START_TIMER not setted!\n");

				return 5;
			} traceLog();
		}
		else if (eventID == EVENT_ID::DEL_LAST_MODEL) { traceLog();
			if (!EVENT_DEL_MODEL) { traceLog();
				return 4;
			} traceLog();

			EVENT_DEL_MODEL->eventID = eventID;

			if (!SetEvent(EVENT_DEL_MODEL->hEvent))
			{
				debugLogFmt("[NY_Event][ERROR]: EVENT_DEL_MODEL not setted!\n");

				return 5;
			} traceLog();
		} traceLog();

		return NULL;
	} traceLog();

	return 2;
};

static PyObject* event_start(PyObject *self, PyObject *args) { traceLog();
	if (!isInited || first_check) { traceLog();
		return PyInt_FromSize_t(1);
	} traceLog();

	INIT_LOCAL_MSG_BUFFER;

	PyObject* __player = PyString_FromString("player");

	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	isModelsAlreadyCreated = false;

	if (!player) { traceLog();
		return PyInt_FromSize_t(2);
	} traceLog();

	PyObject* __arena = PyString_FromString("arena");
	PyObject* arena = PyObject_GetAttr(player, __arena);

	Py_DECREF(__arena);

	Py_DECREF(player);

	if (!arena) { traceLog();
		return PyInt_FromSize_t(3);
	} traceLog();

	PyObject* __arenaType = PyString_FromString("arenaType");
	PyObject* arenaType = PyObject_GetAttr(arena, __arenaType);

	Py_DECREF(__arenaType);
	Py_DECREF(arena);

	if (!arenaType) { traceLog();
		return PyInt_FromSize_t(4);
	} traceLog();

	PyObject* __geometryName = PyString_FromString("geometryName");
	PyObject* map_PS = PyObject_GetAttr(arenaType, __geometryName);

	Py_DECREF(__geometryName);
	Py_DECREF(arenaType);

	if (!map_PS) { traceLog();
		return PyInt_FromSize_t(5);
	} traceLog();

	char* map_s = PyString_AS_STRING(map_PS);

	Py_DECREF(map_PS);

	char map_ID_s[4];
	memcpy(map_ID_s, map_s, 3);
	if (map_ID_s[2] == '_') map_ID_s[2] = NULL;
	map_ID_s[3] = NULL;

	mapID = atoi(map_ID_s);

	extendedDebugLogFmt("[NY_Event]: mapID = %d\n", (uint32_t)mapID);

	battleEnded = false;

	GUI_setTimerVisible(true);
	GUI_setVisible(true);

	isTimeVisible = true;

	request = makeEventInThread(EVENT_ID::IN_BATTLE_GET_FULL);

	if (request) { traceLog();
		debugLogFmt("[NY_Event][ERROR]: start - error %d\n", request);

		return PyInt_FromSize_t(6);
	} traceLog();

	Py_RETURN_NONE;
};

static PyObject* event_fini_py(PyObject *self, PyObject *args) { traceLog();
	if (!EVENT_BATTLE_ENDED) { traceLog();
		return PyInt_FromSize_t(1);
	} traceLog();

	INIT_LOCAL_MSG_BUFFER;

	if (!SetEvent(EVENT_BATTLE_ENDED->hEvent)) { traceLog();
		debugLogFmt("[NY_Event][ERROR]: EVENT_BATTLE_ENDED not setted!\n");

		return PyInt_FromSize_t(2);
	} traceLog();

	Py_RETURN_NONE;
};

static PyObject* event_err_code(PyObject *self, PyObject *args) { traceLog();
	return PyInt_FromSize_t(first_check);
};

bool createEventsAndSecondThread() { traceLog();
	INIT_LOCAL_MSG_BUFFER;
	
	if (!createEvent1(&EVENT_IN_HANGAR,   EVENT_ID::IN_HANGAR)) { traceLog();
		return false;
	} traceLog();
	if (!createEvent1(&EVENT_START_TIMER, EVENT_ID::IN_BATTLE_GET_FULL)) { traceLog();
		return false;
	} traceLog();
	if (!createEvent1(&EVENT_DEL_MODEL,   EVENT_ID::DEL_LAST_MODEL)) { traceLog();
		return false;
	} traceLog();

	M_NETWORK_NOT_USING = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	if (!M_NETWORK_NOT_USING) { traceLog();
		debugLogFmt("[NY_Event][ERROR]: NETWORK_NOT_USING creating: error %d\n", GetLastError());

		return false;
	}

	M_MODELS_NOT_USING  = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	if (!M_MODELS_NOT_USING) { traceLog();
		debugLogFmt("[NY_Event][ERROR]: MODELS_NOT_USING creating: error %d\n", GetLastError());

		return false;
	}

	if (!createEvent2(&EVENT_ALL_MODELS_CREATED, L"NY_Event_AllModelsCreatedEvent")) { traceLog();
		return false;
	} traceLog();
	if (!createEvent2(&EVENT_BATTLE_ENDED,       L"NY_Event_BattleEndedEvent")) { traceLog();
		return false;
	} traceLog();

	//Handler thread creating

	if (hHandlerThread) { traceLog();
		CloseHandle(hHandlerThread);

		hHandlerThread = NULL;
	} traceLog();

	hHandlerThread = CreateThread( //создаем второй поток
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		HandlerThread,                          // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&handlerThreadID);                      // returns the thread identifier 

	if (!hHandlerThread) { traceLog();
		debugLogFmt("[NY_Event][ERROR]: Handler thread creating: error %d\n", GetLastError());

		return false;
	} traceLog();

	return true;
}

uint8_t event_сheck() { traceLog();
	if (!isInited) { traceLog();
		return 1;
	} traceLog();

	// инициализация второго потока, если не существует, иначе - завершить второй поток и начать новый

	if (!createEventsAndSecondThread()) { traceLog();
		return 2;
	} traceLog();

	//------------------------------------------------------------------------------------------------

	INIT_LOCAL_MSG_BUFFER;

	debugLogFmt("[NY_Event]: checking...\n");

	PyObject* __player = PyString_FromString("player");
	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	if (!player) { traceLog();
		return 3;
	} traceLog();

	PyObject* __databaseID = PyString_FromString("databaseID");
	PyObject* DBID_string = PyObject_GetAttr(player, __databaseID);

	Py_DECREF(__databaseID);
	Py_DECREF(player);

	if (!DBID_string) { traceLog();
		return 4;
	} traceLog();

	PyObject* DBID_int = PyNumber_Int(DBID_string);

	Py_DECREF(DBID_string);

	if (!DBID_int) { traceLog();
		return 5;
	} traceLog();

	databaseID = PyInt_AS_LONG(DBID_int);

	Py_DECREF(DBID_int);

	debugLogFmt("[NY_Event]: DBID created\n");

	mapID = NULL;

	battleEnded = false;

	first_check = makeEventInThread(EVENT_ID::IN_HANGAR);

	if (first_check) { traceLog();
		return 6;
	}
	else {
		return NULL;
	} traceLog();
}

static PyObject* event_сheck_py(PyObject *self, PyObject *args) { traceLog();
	uint8_t res = event_сheck();

	if (res) { traceLog();
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
};

uint8_t event_init(PyObject* template_, PyObject* apply, PyObject* byteify) { traceLog();
	if (!template_ || !apply || !byteify) { traceLog();
		return 1;
	} traceLog();

	if (g_gui && PyCallable_Check(template_) && PyCallable_Check(apply)) { traceLog();
		Py_INCREF(template_);
		Py_INCREF(apply);

		PyObject* __register = PyString_FromString("register");
		PyObject* result = PyObject_CallMethodObjArgs(g_gui, __register, PyString_FromString(g_self->ids), template_, g_self->data, apply, NULL);

		Py_XDECREF(result);
		Py_DECREF(__register);
		Py_DECREF(apply);
		Py_DECREF(template_);
	} traceLog();

	if (!g_gui && PyCallable_Check(byteify)) { traceLog();
		Py_INCREF(byteify);

		PyObject* args1 = PyTuple_New(1);
		PyTuple_SET_ITEM(args1, NULL, g_self->i18n);

		PyObject* result = PyObject_CallObject(byteify, args1);

		if (result) { traceLog();
			PyObject* old = g_self->i18n;

			g_self->i18n = result;

			PyDict_Clear(old);
			Py_DECREF(old);
		} traceLog();

		Py_DECREF(byteify);
	} traceLog();

	return NULL;
}

static PyObject* event_init_py(PyObject *self, PyObject *args) { traceLog();
	if (!isInited) { traceLog();
		return PyInt_FromSize_t(1);
	} traceLog();

	PyObject* template_ = NULL;
	PyObject* apply     = NULL;
	PyObject* byteify   = NULL;

	if (!PyArg_ParseTuple(args, "OOO", &template_, &apply, &byteify)) { traceLog();
		return PyInt_FromSize_t(2);
	} traceLog();

	uint8_t res = event_init(template_, apply, byteify);

	if (res) { traceLog();
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
};

static PyObject* event_inject_handle_key_event(PyObject *self, PyObject *args) { traceLog();
	if (!isInited || first_check || !databaseID || !mapID || !spaceKey || isStreamer) { traceLog();
		Py_RETURN_NONE;
	} traceLog();

	PyObject* event_ = PyTuple_GET_ITEM(args, NULL);
	PyObject* isKeyGetted_Space = NULL;

	if (g_gui) { traceLog();
		PyObject* __get_key = PyString_FromString("get_key");
		
		isKeyGetted_Space = PyObject_CallMethodObjArgs(g_gui, __get_key, spaceKey, NULL);

		Py_DECREF(__get_key);
	}
	else {
		PyObject* __key = PyString_FromString("key");
		PyObject* key = PyObject_GetAttr(event_, __key);

		Py_DECREF(__key);

		if (!key) { traceLog();
			Py_RETURN_NONE;
		} traceLog();

		PyObject* ____contains__ = PyString_FromString("__contains__");

		isKeyGetted_Space = PyObject_CallMethodObjArgs(spaceKey, ____contains__, key, NULL);

		Py_DECREF(____contains__);
	} traceLog();

	if (isKeyGetted_Space == Py_True) { traceLog();
		request = makeEventInThread(EVENT_ID::DEL_LAST_MODEL);

		if (request) { traceLog();
			INIT_LOCAL_MSG_BUFFER;

			debugLogFmt("[NY_Event][ERROR]: making DEL_LAST_MODEL: error %d\n", request);

			Py_RETURN_NONE;
		} traceLog();
	} traceLog();

	Py_XDECREF(isKeyGetted_Space);

	Py_RETURN_NONE;
};

static struct PyMethodDef event_methods[] =
{
	{ "b",             event_сheck_py,                METH_VARARGS, ":P" }, //check
	{ "c",             event_start,                   METH_NOARGS,  ":P" }, //start
	{ "d",             event_fini_py,                 METH_NOARGS,  ":P" }, //fini
	{ "e",             event_err_code,                METH_NOARGS,  ":P" }, //get_error_code
	{ "g",             event_init_py,                 METH_VARARGS, ":P" }, //init
	{ "event_handler", event_inject_handle_key_event, METH_VARARGS, ":P" }, //inject_handle_key_event
	{ "omc",           event_onModelCreated,          METH_VARARGS, ":P" }, //onModelCreated
	{ NULL, NULL, 0, NULL }
};

PyDoc_STRVAR(event_methods__doc__,
	"Trajectory Mod module"
);

//---------------------------INITIALIZATION--------------------------

PyMODINIT_FUNC initevent(void)
{
	BigWorld = PyImport_AddModule("BigWorld");

	if (!BigWorld) { traceLog();
		return;
	} traceLog();

	PyObject* appLoader = PyImport_ImportModule("gui.app_loader");

	if (!appLoader) { traceLog();
		return;
	} traceLog();

	PyObject* __g_appLoader = PyString_FromString("g_appLoader");

	g_appLoader = PyObject_GetAttr(appLoader, __g_appLoader);

	Py_DECREF(__g_appLoader);
	Py_DECREF(appLoader);

	if (!g_appLoader) { traceLog();
		return;
	} traceLog();

	functools = PyImport_ImportModule("functools");

	if (!functools) { traceLog();
		return;
	} traceLog();

	json = PyImport_ImportModule("json");

	if (!json) { traceLog();
		Py_DECREF(g_appLoader);
		return;
	} traceLog();

	debugLog("[NY_Event]: Config init...\n");

	if (PyType_Ready(&Config_p)) { traceLog();
		Py_DECREF(g_appLoader);
		return;
	} traceLog();

	Py_INCREF(&Config_p);

	debugLog("[NY_Event]: Config init OK\n");

	PyObject* g_config = PyObject_CallObject((PyObject*)&Config_p, NULL);

	Py_DECREF(&Config_p);

	if (!g_config) { traceLog();
		Py_DECREF(g_appLoader);
		return;
	} traceLog();

	event_module = Py_InitModule3("event",
		event_methods,
		event_methods__doc__);

	if (!event_module) { traceLog();
		Py_DECREF(g_appLoader);
		return;
	} traceLog();

	if (PyModule_AddObject(event_module, "l", g_config)) { traceLog();
		Py_DECREF(g_appLoader);
		return;
	} traceLog();

	PyObject* __omc = PyString_FromString("omc");

	onModelCreatedPyMeth = PyObject_GetAttr(event_module, __omc);

	Py_DECREF(__omc);

	if (!onModelCreatedPyMeth) { traceLog();
		Py_DECREF(g_appLoader);
		return;
	} traceLog();

	//Space key

	spaceKey = PyList_New(1);

	if (spaceKey) { traceLog();
		PyList_SET_ITEM(spaceKey, 0U, PyInt_FromSize_t(57));
	} traceLog();

	//---------

#if debug_log
	OutputDebugString(_T("Mod_GUI module loading...\n"));
#endif

	PyObject* mGUI_module = PyImport_ImportModule("NY_Event.native.mGUI");

	if (!mGUI_module) { traceLog();
		Py_DECREF(g_appLoader);
		return;
	} traceLog();

	debugLog("[NY_Event]: Mod_GUI class loading...\n");

	PyObject* __Mod_GUI = PyString_FromString("Mod_GUI");

	modGUI = PyObject_CallMethodObjArgs(mGUI_module, __Mod_GUI, NULL);
	
	Py_DECREF(__Mod_GUI);
	Py_DECREF(mGUI_module);

	if (!modGUI) { traceLog();
		Py_DECREF(g_appLoader);
		return;
	} traceLog();

	debugLog("[NY_Event]: Mod_GUI class loaded OK!\n");

	debugLog("[NY_Event]: g_gui module loading...\n");

	PyObject* mod_mods_gui = PyImport_ImportModule("gui.mods.mod_mods_gui");

	if (!mod_mods_gui) { traceLog();
		PyErr_Clear();
		g_gui = NULL;

		debugLog("[NY_Event]: mod_mods_gui is NULL!\n");
	}
	else {
		PyObject* __g_gui = PyString_FromString("g_gui");

		g_gui = PyObject_GetAttr(mod_mods_gui, __g_gui);

		Py_DECREF(__g_gui);
		Py_DECREF(mod_mods_gui);

		if (!g_gui) { traceLog();
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			return;
		} traceLog();

		debugLog("[NY_Event]: mod_mods_gui loaded OK!\n");
	} traceLog();

	if (!g_gui) { traceLog();
		_mkdir("mods/configs");
		_mkdir("mods/configs/pavel3333");
		_mkdir("mods/configs/pavel3333/NY_Event");
		_mkdir("mods/configs/pavel3333/NY_Event/i18n");

		if (!read_data(true) || !read_data(false)) { traceLog();
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			return;
		} traceLog();
	}
	else {
		PyObject* ids = PyString_FromString(g_self->ids);

		if (!ids) { traceLog();
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			Py_DECREF(g_gui);
			return;
		} traceLog();

		PyObject* __register_data = PyString_FromString("register_data");
		PyObject* __pavel3333 = PyString_FromString("pavel3333");
		PyObject* data_i18n = PyObject_CallMethodObjArgs(g_gui, __register_data, ids, g_self->data, g_self->i18n, __pavel3333, NULL);

		Py_DECREF(__pavel3333);
		Py_DECREF(__register_data);
		Py_DECREF(ids);

		if (!data_i18n) { traceLog();
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			Py_DECREF(g_gui);
			Py_DECREF(ids);
			return;
		} traceLog();

		PyObject* old = g_self->data;

		g_self->data = PyTuple_GET_ITEM(data_i18n, NULL);

		PyDict_Clear(old);

		Py_DECREF(old);

		old = g_self->i18n;

		g_self->i18n = PyTuple_GET_ITEM(data_i18n, 1);

		PyDict_Clear(old);

		Py_DECREF(old);
		Py_DECREF(data_i18n);
	} traceLog();
	
	uint32_t curl_init_result = curl_init();

	if (curl_init_result) { traceLog();
		INIT_LOCAL_MSG_BUFFER;

		debugLogFmt("[NY_Event][ERROR]: Initialising CURL handle: error %d\n", curl_init_result);

		return;
	} traceLog();

	isInited = true;
};
