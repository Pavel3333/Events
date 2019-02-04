#define CONSOLE_VER1

#include "ModThreads.h"
#include "Py_config.h"
#include "BW_native.h"
#include "GUI.h"
#include <stdlib.h>
#include <direct.h>

#define debug_log true

#define extended_debug_log true

#define super_extended_debug_log false

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

PyObject* event_module = NULL;

PyObject* modGUI = NULL;

PyObject* spaceKey = NULL;

uint16_t allModelsCreated      = NULL;

PyObject* onModelCreatedPyMeth = NULL;

uint8_t  first_check = 100U;
uint32_t request     = 100U;

uint8_t  mapID      = NULL;
uint32_t databaseID = NULL;

extern EVENT_ID EventsID;
extern STAGE_ID StagesID;

bool isInited = false;

bool battleEnded = true;

bool isModelsAlreadyCreated = false;
bool isModelsAlreadyInited  = false;

bool isTimerStarted = false;
bool isTimeVisible  = false;

bool isStreamer = false;

HANDLE hTimer         = NULL;
HANDLE hSecondThread  = NULL;
DWORD  secondThreadID = NULL;

uint8_t timerLastError = NULL;

//Главные ивенты

PEVENTDATA_1 EVENT_IN_HANGAR   = NULL;
PEVENTDATA_1 EVENT_START_TIMER = NULL;

//Второстепенные ивенты

PEVENTDATA_2 EVENT_ALL_MODELS_CREATED = NULL;

uint8_t lastStageID = StagesID.COMPETITION;
uint8_t lastEventID = EventsID.IN_HANGAR;

uint8_t makeEventInThread(uint8_t, uint8_t);

bool write_data(char* data_path, PyObject* data_p) {
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

	PyObject* indent = PyInt_FromSize_t(4U);
	Py_INCREF(indent);

	PyObject* __dumps = PyString_FromStringAndSize("dumps", 5U);

	PyObject* data_json_s = PyObject_CallMethodObjArgs(json, __dumps, data_p, arg2, arg3, arg4, arg5, arg6, indent, NULL);

	Py_DECREF(__dumps);
	Py_DECREF(arg2);
	Py_DECREF(arg3);
	Py_DECREF(arg4);
	Py_DECREF(arg5);
	Py_DECREF(arg6);
	Py_DECREF(indent);

	if (!data_json_s) {
		return false;
	}

	size_t data_size = PyObject_Length(data_json_s);

	std::ofstream data_w(data_path);

	data_w.write(PyString_AS_STRING(data_json_s), data_size);

	data_w.close();

	Py_DECREF(data_json_s);
	return true;
}

bool read_data(bool isData) {
	char* data_path;
	PyObject* data_src;
	if (isData) { 
		data_path = "mods/configs/pavel3333/NY_Event/NY_Event.json";
		data_src = g_self->data;
	}
	else {
		data_src = g_self->i18n;
		data_path = "mods/configs/pavel3333/NY_Event/i18n/ru.json";
	}

	std::ifstream data(data_path, std::ios::binary);

	if (!data.is_open()) {
		data.close();
		if (!write_data(data_path, data_src)) {
			return false;
		}
	}
	else {
		data.seekg(0, std::ios::end);
		size_t size = (size_t)data.tellg(); //getting file size
		data.seekg(0);

		char* data_s = new char[size + 1];

		while (!data.eof()) {
			data.read(data_s, size);
		}
		data.close();

		PyObject* data_p = PyString_FromStringAndSize(data_s, size);

		delete[] data_s;

		PyObject* __loads = PyString_FromStringAndSize("loads", 5U);

		PyObject* data_json_s = PyObject_CallMethodObjArgs(json, __loads, data_p, NULL);

		Py_DECREF(__loads);
		Py_DECREF(data_p);

		if (!data_json_s) {
			PyErr_Clear();

			if (!write_data(data_path, data_src)) {
				return false;
			}

			return true;
		}

		PyObject* old = data_src;
		if (isData) g_self->data = data_json_s;
		else g_self->i18n = data_json_s;

		PyDict_Clear(old);
		Py_DECREF(old);
	}

	return true;
}

void clearModelsSections() {
	std::vector<ModelsSection>::iterator it_sect = current_map.modelsSects.begin();

	while (it_sect != current_map.modelsSects.end()) {
		if (!it_sect->models.empty() && it_sect->isInitialised) {
			std::vector<float*>::iterator it_model = it_sect->models.begin();

			while (it_model != it_sect->models.end()) {
				if (*it_model == nullptr) {
					it_model = it_sect->models.erase(it_model);

					continue;
				}

				for (uint8_t counter = NULL; counter < 3U; counter++) {
					memset(&(*it_model)[counter], NULL, 4U);
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
	}

	current_map.modelsSects.~vector();

	std::vector<ModelsSection>::iterator it_sect_sync = sync_map.modelsSects_deleting.begin();

	while (it_sect_sync != sync_map.modelsSects_deleting.end()) {
		if (it_sect_sync->isInitialised) {
			std::vector<float*>::iterator it_model = it_sect_sync->models.begin();

			while (it_model != it_sect_sync->models.end()) {
				if (*it_model != nullptr) {
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
	}

	sync_map.modelsSects_deleting.~vector();
}

uint8_t findLastModelCoords(float dist_equal, uint8_t* modelID, float** coords) {
	PyObject* __player = PyString_FromStringAndSize("player", 6U);

	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	isModelsAlreadyCreated = false;

	if (!player) {
		return 1U;
	}

	PyObject* __vehicle = PyString_FromStringAndSize("vehicle", 7U);
	PyObject* vehicle = PyObject_GetAttr(player, __vehicle);

	Py_DECREF(__vehicle);

	Py_DECREF(player);

	if (!vehicle) {
		return 2U;
	}

	PyObject* __model = PyString_FromStringAndSize("model", 5U);
	PyObject* model_p = PyObject_GetAttr(vehicle, __model);

	Py_DECREF(__model);
	Py_DECREF(vehicle);

	if (!model_p) {
		return 3U;
	}

	PyObject* __position = PyString_FromStringAndSize("position", 8U);
	PyObject* position_Vec3 = PyObject_GetAttr(model_p, __position);

	Py_DECREF(__position);
	Py_DECREF(model_p);

	if (!position_Vec3) {
		return 4U;
	}

	double* coords_pos = new double[3];

	for (uint8_t i = NULL; i < 3U; i++) {
		PyObject* __tuple = PyString_FromStringAndSize("tuple", 5U);

		PyObject* position = PyObject_CallMethodObjArgs(position_Vec3, __tuple, NULL);

		Py_DECREF(__tuple);

		if (!position) {
			return 5U;
		}

		PyObject* coord_p = PyTuple_GetItem(position, i);

		if (!coord_p) {
			return 6U;
		}

		coords_pos[i] = PyFloat_AS_DOUBLE(coord_p);
	}

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
				it2++) {
				if (*it2 == nullptr) continue;

				distTemp = getDist2Points(coords_pos, *it2);

				if (dist == -1.0 || distTemp < dist) {
					dist = distTemp;
					modelTypeLast = it->ID;

					coords_res = *it2;
				}
			}
		}
	}

	delete[] coords_pos;

	if (dist == -1.0 || modelTypeLast == -1 || coords_res == nullptr) {
		return 8U;
	}

	if (dist > dist_equal) {
		return 7U;
	}

	*modelID = modelTypeLast;
	*coords = coords_res;

	return NULL;
}

uint8_t delModelPy(float* coords) {
	if (coords == nullptr) {
		return 1U;
	}

	std::vector<ModModel*>::iterator it_model = models.begin();

	while (it_model != models.end()) {
		if (*it_model == nullptr) {
			it_model = models.erase(it_model);

			continue;
		}

		if (!(*it_model)->model || (*it_model)->model == Py_None || !(*it_model)->processed) {
#if debug_log && extended_debug_log && super_extended_debug_log
			OutputDebugString(_T("NULL\n"));
#endif
			Py_XDECREF((*it_model)->model);

			(*it_model)->model = NULL;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			continue;
		}
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("[NY_Event]: del debug 1.1\n"));
#endif

		if ((*it_model)->coords[0] == coords[0] &&
			(*it_model)->coords[1] == coords[1] &&
			(*it_model)->coords[2] == coords[2]) {

			PyObject* py_visible = PyBool_FromLong(false);

			PyObject* __visible = PyString_FromStringAndSize("visible", 7U);

			if (!PyObject_SetAttr((*it_model)->model, __visible, py_visible)) {
				Py_DECREF(py_visible);
				
				return NULL;
			}

			Py_DECREF(py_visible);

			return 3U;

			/*PyObject* __delModel = PyString_FromStringAndSize("delModel", 8U);

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
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("[NY_Event]: del debug 1.2\n"));
#endif

	return 2U;
}

uint8_t delModelCoords(uint16_t ID, float* coords) {
	if (coords == nullptr) {
		return 1U;
	}

	std::vector<ModelsSection>::iterator it_sect = current_map.modelsSects.begin();

	float* model;

	while (it_sect != current_map.modelsSects.end()) {
		if (!it_sect->models.empty() && it_sect->isInitialised) {
			if (it_sect->ID == ID) {
				std::vector<float*>::iterator it_model = it_sect->models.begin();

				while (it_model != it_sect->models.end()) {
					if (*it_model == nullptr) {
						it_model = it_sect->models.erase(it_model);

						continue;
					}

					model = *it_model;

					if (model[0] == coords[0] &&
						model[1] == coords[1] &&
						model[2] == coords[2]
						) {
						for (uint8_t counter = NULL; counter < 3U; counter++) {
							memset(&model[counter], NULL, 4U);
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
	}

	return 2U;
}

static PyObject* event_light(float coords[3]) {
	if (!isInited || battleEnded) {
		return NULL;
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("light creating...\n"));
#endif

	PyObject* __PyOmniLight = PyString_FromStringAndSize("PyOmniLight", 11U);

	PyObject* Light = PyObject_CallMethodObjArgs(BigWorld, __PyOmniLight, NULL);

	Py_DECREF(__PyOmniLight);

	if (!Light) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight creating FAILED\n"));
#endif
		return NULL;
	}

	//---------inner radius---------

	PyObject* __innerRadius = PyString_FromStringAndSize("innerRadius", 11U);
	PyObject* innerRadius = PyFloat_FromDouble(0.75);

	if (PyObject_SetAttr(Light, __innerRadius, innerRadius)) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight innerRadius setting FAILED\n"));
#endif
		Py_DECREF(__innerRadius);
		Py_DECREF(innerRadius);
		Py_DECREF(Light);

		return NULL;
	}

	Py_DECREF(__innerRadius);

	//---------outer radius---------

	PyObject* __outerRadius = PyString_FromStringAndSize("outerRadius", 11U);
	PyObject* outerRadius = PyFloat_FromDouble(1.5);

	if (PyObject_SetAttr(Light, __outerRadius, outerRadius)) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight outerRadius setting FAILED\n"));
#endif
		Py_DECREF(outerRadius);
		Py_DECREF(__outerRadius);
		Py_DECREF(Light);

		return NULL;
	}

	Py_DECREF(__outerRadius);

	//----------multiplier----------

	PyObject* __multiplier = PyString_FromStringAndSize("multiplier", 10U);
	PyObject* multiplier = PyFloat_FromDouble(500.0);

	if (PyObject_SetAttr(Light, __multiplier, multiplier)) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight multiplier setting FAILED\n"));
#endif
		Py_DECREF(multiplier);
		Py_DECREF(__multiplier);
		Py_DECREF(Light);

		return NULL;
	}

	Py_DECREF(__multiplier);

	//-----------position-----------

	PyObject* coords_p = PyTuple_New(3U);

	if (!coords_p) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight coords creating FAILED\n"));
#endif
		Py_DECREF(Light);

		return NULL;
	}

	for (uint8_t i = NULL; i < 3U; i++) {
		if (i == 1U) {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i] + 0.5));
		}
		else {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i]));
		}
	}

	PyObject* __position = PyString_FromStringAndSize("position", 8U);

	if (PyObject_SetAttr(Light, __position, coords_p)) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight coords setting FAILED\n"));
#endif
		Py_DECREF(__position);
		Py_DECREF(coords_p);
		Py_DECREF(Light);

		return NULL;
	}

	Py_DECREF(__position);

	//------------colour------------

	PyObject* colour_p = PyTuple_New(4U);

	if (!colour_p) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight colour creating FAILED\n"));
#endif
		Py_DECREF(Light);

		return NULL;
	}

	double* colour = new double[5];

	colour[0] = 255.0;
	colour[1] = 255.0;
	colour[2] = 255.0;
	colour[3] = 0.0;

	for (uint8_t i = NULL; i < 4U; i++) PyTuple_SET_ITEM(colour_p, i, PyFloat_FromDouble(colour[i]));

	delete[] colour;

	//------------------------------

	PyObject* __colour = PyString_FromStringAndSize("colour", 6U);

	if (PyObject_SetAttr(Light, __colour, colour_p)) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight colour setting FAILED\n"));
#endif
		Py_DECREF(__colour);
		Py_DECREF(colour_p);
		Py_DECREF(Light);

		return NULL;
	}

	Py_DECREF(__colour);

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("light creating OK!\n"));
#endif

	return Light;
}

bool setModelPosition(PyObject* Model, float* coords_f) {
	PyObject* coords_p = PyTuple_New(3);

	for (uint8_t i = NULL; i < 3; i++) PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords_f[i]));

	PyObject* __position = PyString_FromStringAndSize("position", 8U);

	if (PyObject_SetAttr(Model, __position, coords_p)) {
		Py_DECREF(__position);
		Py_DECREF(coords_p);
		Py_DECREF(Model);

		return false;
	}

	Py_DECREF(__position);

	return true;
}

static PyObject* event_model(char* path, float coords[3], bool isAsync=false) {
	if (!isInited || battleEnded) {
		if (isAsync && allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

		return NULL;
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("model creating...\n"));
#endif

	PyObject* Model = NULL;

	if (isAsync) {
		if (coords == nullptr) { 
			if (allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return NULL;
		}

		PyObject* __partial = PyString_FromStringAndSize("partial", 7);

		PyObject* coords_p = PyLong_FromVoidPtr((void*)coords); //передаем указатель на 3 координаты

		PyObject* partialized = PyObject_CallMethodObjArgs(functools, __partial, onModelCreatedPyMeth, coords_p, NULL);

		Py_DECREF(__partial);

		if (!partialized) {
			if (allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return NULL;
		}

		PyObject* __fetchModel = PyString_FromStringAndSize("fetchModel", 10U);

		Model = PyObject_CallMethodObjArgs(BigWorld, __fetchModel, PyString_FromString(path), partialized, NULL); //запускаем асинхронное добавление модели

		Py_XDECREF(Model);
		Py_DECREF(__fetchModel);

		return NULL;
	}

	PyObject* __Model = PyString_FromStringAndSize("Model", 5U);

	Model = PyObject_CallMethodObjArgs(BigWorld, __Model, PyString_FromString(path), NULL);

	Py_DECREF(__Model);

	if (!Model) {
		return NULL;
	}
	
	if (coords != nullptr) {
		if (!setModelPosition(Model, coords)) { //ставим на нужную позицию
			return NULL;
		}
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("model creating OK!\n"));
#endif

	return Model;
};

static PyObject* event_onModelCreated(PyObject *self, PyObject *args) { //принимает аргументы: указатель на координаты и саму модель
	if (!isInited || battleEnded || models.size() >= allModelsCreated) {
		Py_RETURN_NONE;
	}

	if (!EVENT_ALL_MODELS_CREATED->hEvent) {
#if debug_log && extended_debug_log
		OutputDebugString(_T("AMCEvent or createModelsPyMeth event is NULL!\n"));
#endif

		Py_RETURN_NONE;
	}

	//рабочая часть

	PyObject* coords_pointer = NULL;
	PyObject* Model    = NULL;

	if (!PyArg_ParseTuple(args, "OO", &coords_pointer, &Model)) {
		Py_RETURN_NONE;
	}

	if (!Model || Model == Py_None) {
		Py_XDECREF(Model);

		Py_RETURN_NONE;
	}

	if(!coords_pointer) {
		Py_DECREF(Model);

		Py_RETURN_NONE;
	}

	void* coords_vptr = PyLong_AsVoidPtr(coords_pointer);

	if (!coords_vptr) {
		Py_DECREF(coords_pointer);
		Py_DECREF(Model);

		Py_RETURN_NONE;
	}

	float* coords_f = (float*)(coords_vptr);

	if (!setModelPosition(Model, coords_f)) { //ставим на нужную позицию
		Py_RETURN_NONE;
	}

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

	if (models.size() >= allModelsCreated) { //если число созданных моделей - столько же или больше, чем надо
		//сигналим о том, что все модели были успешно созданы

		if (!SetEvent(EVENT_ALL_MODELS_CREATED->hEvent))
		{
#if debug_log && extended_debug_log
			OutputDebugString(_T("AMCEvent event not setted!\n"));
#endif

			Py_RETURN_NONE;
		}
	}

	Py_RETURN_NONE;
}

uint8_t create_models() {
	if (!isInited || battleEnded || !onModelCreatedPyMeth) {
		return 1U;
	}

	for (auto it = current_map.modelsSects.begin(); //первый проход - получаем число всех созданных моделей
		it != current_map.modelsSects.end();
		it++) {

		if (!it->isInitialised || it->models.empty()) {
			continue;
		}

		auto it2 = it->models.begin();

		while (it2 != it->models.end()) {
			if (*it2 == nullptr) {
				it2 = it->models.erase(it2);

				continue;
			}

			allModelsCreated++;

			it2++;
		}
	}

	for (auto it = current_map.modelsSects.cbegin(); //второй проход - создаем модели
		it != current_map.modelsSects.cend();
		it++) {
		if (!it->isInitialised || it->models.empty()) {
			continue;
		}
		
		for (auto it2 = it->models.cbegin(); it2 != it->models.cend(); it2++) {
#if debug_log && extended_debug_log && super_extended_debug_log
			OutputDebugString("[");
#endif

			event_model(it->path, *it2, true);

#if debug_log && extended_debug_log && super_extended_debug_log
			OutputDebugString("], ");
#endif
		}
	}
#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("], \n"));
#endif

	return NULL;
}

uint8_t init_models() {
	if (!isInited || first_check || request || battleEnded || models.empty()) {
		return 1U;
	}
#if debug_log  && extended_debug_log
	OutputDebugString(_T("[NY_Event]: models adding...\n"));
#endif

	for (uint16_t i = NULL; i < models.size(); i++) {
		if (models[i] == nullptr) {
			continue;
		}

		if (models[i]->model == Py_None || !models[i]->model || models[i]->processed) {
			Py_XDECREF(models[i]->model);

			models[i]->model = NULL;
			models[i]->coords = nullptr;
			models[i]->processed = false;

			delete models[i];
			models[i] = nullptr;

			continue;
		}

#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("[NY_Event]: addModel debug 2.1\n"));
#endif

		PyObject* __addModel = PyString_FromStringAndSize("addModel", 8U);

		PyObject* result = PyObject_CallMethodObjArgs(BigWorld, __addModel, models[i]->model, NULL);

		Py_DECREF(__addModel);

#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("[NY_Event]: addModel debug 2.2\n"));
#endif
		if (result) {
			Py_DECREF(result);

			models[i]->processed = true;

#if debug_log && extended_debug_log && super_extended_debug_log
			OutputDebugString(_T("True\n"));
#endif
		}
#if debug_log && extended_debug_log && super_extended_debug_log
		else OutputDebugString(_T("False\n"));
		OutputDebugString(_T("[NY_Event]: addModel debug 2.3\n"));
#endif
	}

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: models adding OK!\n"));
#endif

	return NULL;
}

uint8_t set_visible(bool isVisible) {
	if (!isInited || first_check || request || battleEnded || models.empty()) {
		return 1U;
	}

	PyObject* py_visible = PyBool_FromLong(isVisible);

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: Models visiblity changing...\n"));
#endif

	for (uint16_t i = NULL; i < models.size(); i++) {
		if (models[i] == nullptr) {
			continue;
		}

		if (!models[i]->model || models[i]->model == Py_None || !models[i]->processed) {
			Py_XDECREF(models[i]->model);

			models[i]->model = NULL;
			models[i]->coords = nullptr;
			models[i]->processed = false;

			delete models[i];
			models[i] = nullptr;

			continue;
		}

		PyObject* __visible = PyString_FromStringAndSize("visible", 7U);

		PyObject_SetAttr(models[i]->model, __visible, py_visible);

		Py_DECREF(__visible);
	}

	Py_DECREF(py_visible);

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: Models visiblity changing OK!\n"));
#endif

	return NULL;
};

uint8_t handle_battle_event(uint8_t eventID) {
	if (!isInited || first_check || request || battleEnded || !g_self || eventID == EventsID.IN_HANGAR) {
		return 1U;
	}

#if debug_log  && extended_debug_log
	OutputDebugString(_T("[NY_Event]: parsing...\n"));
#endif

	uint8_t parsing_result = NULL;

	Py_BEGIN_ALLOW_THREADS

	if      (eventID == EventsID.IN_BATTLE_GET_FULL) parsing_result = parse_config();
	else if (eventID == EventsID.IN_BATTLE_GET_SYNC) parsing_result = parse_sync();
	else if (eventID == EventsID.DEL_LAST_MODEL)     parsing_result = parse_del_model();

	Py_END_ALLOW_THREADS

	if (parsing_result) {
#if debug_log && extended_debug_log
		PySys_WriteStdout("[NY_Event]: parsing FAILED! Error code: %d\n", (uint32_t)parsing_result);
#endif

		GUI_setError(parsing_result);

		return 2U;
	}


#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("[NY_Event]: parsing OK!\n"));
#endif
	
	if (current_map.time_preparing)  //выводим время
		GUI_setTime(current_map.time_preparing);
		
	if (current_map.stageID >= 0 && current_map.stageID < STAGES_COUNT) {    //выводим сообщение
		if (
			current_map.stageID == StagesID.WAITING      ||
			current_map.stageID == StagesID.START        ||
			current_map.stageID == StagesID.COMPETITION  ||
			current_map.stageID == StagesID.END_BY_TIME  ||
			current_map.stageID == StagesID.END_BY_COUNT ||
			current_map.stageID == StagesID.STREAMER_MODE
			) {
			if (lastStageID != StagesID.GET_SCORE && lastStageID != StagesID.ITEMS_NOT_EXISTS) GUI_setMsg(current_map.stageID);

			if(current_map.stageID == StagesID.END_BY_TIME || current_map.stageID == StagesID.END_BY_COUNT) {
				current_map.time_preparing = NULL;

				GUI_setTime(NULL);

				if (current_map.stageID == StagesID.END_BY_COUNT) {
					GUI_setTimerVisible(false);

					isTimeVisible = false;
				}

				GUI_setMsg(current_map.stageID);

				uint8_t event_result = event_fini();

				if (event_result) {
#if debug_log && extended_debug_log
					PySys_WriteStdout("[NY_Event]: Warning - handle_battle_event - event_fini - Error code %d\n", event_result);
#endif

					GUI_setWarning(event_result);
				}
			}
			else {
				if (!isTimeVisible) {
					GUI_setTimerVisible(true);

					isTimeVisible = true;
				}
			}

			if (current_map.stageID == StagesID.START          ||
				current_map.stageID == StagesID.COMPETITION    || 
				current_map.stageID == StagesID.STREAMER_MODE) {
				if (isModelsAlreadyCreated && !isModelsAlreadyInited && current_map.minimap_count && current_map.modelsSects.size()) {
					if      (eventID == EventsID.IN_BATTLE_GET_FULL) {
#if debug_log && extended_debug_log && super_extended_debug_log
						PySys_WriteStdout("sect count: %d\npos count: %d\n", (uint32_t)current_map.modelsSects.size(), (uint32_t)current_map.minimap_count);
#endif

						Py_BEGIN_ALLOW_THREADS;

#if debug_log && extended_debug_log
						OutputDebugString(_T("[NY_Event]: creating...\n"));
#endif

						models.~vector();
						lights.~vector();

						Py_END_ALLOW_THREADS;

						/*
						Первый способ - нативный вызов в main-потоке добавлением в очередь. Ненадёжно!

						int creating_result = Py_AddPendingCall(&create_models, nullptr); //create_models();

						if (creating_result == -1) {
#if debug_log && extended_debug_log
							OutputDebugString(_T("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - create_models - failed to start PendingCall of creating models!\n"));
#endif

							return 3U;
						}
						*/

						/*
						Второй способ - вызов асинхронной функции BigWorld.fetchModel(path, onLoadedMethod)

						Более-менее надежно, выполняется на уровне движка
						*/

						request = create_models();

						if (request) {
#if debug_log && extended_debug_log
							PySys_WriteStdout("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - create_models - Error code %d\n", request);
#endif

							return 3U;
						}

						PyThreadState *_save;        //глушим GIL, пока ожидаем
						Py_UNBLOCK_THREADS;

						//ожидаем события полного создания моделей

						DWORD EVENT_ALL_MODELS_CREATED_WaitResult = WaitForSingleObject(
							EVENT_ALL_MODELS_CREATED->hEvent, // event handle
							INFINITE);                        // indefinite wait

						switch (EVENT_ALL_MODELS_CREATED_WaitResult)
						{
							// Event object was signaled
						case WAIT_OBJECT_0:
#if debug_log && extended_debug_log
							OutputDebugString(_T("AllModelsCreatedEvent was signaled!\n"));
#endif

							//очищаем ивент

							ResetEvent(EVENT_ALL_MODELS_CREATED->hEvent);

							//-------------

							//место для рабочего кода

							Py_BLOCK_THREADS;

#if debug_log  && extended_debug_log
							OutputDebugString(_T("[NY_Event]: creating OK!\n"));
#endif

							request = init_models();

							if (request) {
								if (request > 9U) {
#if debug_log && extended_debug_log
									PySys_WriteStdout("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - init_models - Error code %d\n", request);
#endif

									GUI_setError(request);

									return 5U;
								}

#if debug_log && extended_debug_log
								PySys_WriteStdout("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - init_models - Warning code %d\n", request);
#endif

								GUI_setWarning(request);

								return 4U;
							}

							request = set_visible(true);

							if (request) {
								if (request > 9U) {
#if debug_log && extended_debug_log
									PySys_WriteStdout("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - set_visible - Error code %d\n", request);
#endif

									GUI_setError(request);

									return 5U;
								}

#if debug_log && extended_debug_log
								PySys_WriteStdout("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - set_visible - Warning code %d\n", request);
#endif

								GUI_setWarning(request);

								return 4U;
							}

							isModelsAlreadyInited = true;

							break;

							// An error occurred
						default:
							ResetEvent(EVENT_ALL_MODELS_CREATED->hEvent);

#if debug_log && extended_debug_log
							OutputDebugString(_T("[NY_Event][ERROR]: IN_HANGAR - something wrong with WaitResult!\n"));
#endif

							Py_BLOCK_THREADS;

							return 3U;
						}
					}

					return NULL;
				}
			}

			if (isModelsAlreadyCreated && isModelsAlreadyInited && eventID == EventsID.IN_BATTLE_GET_SYNC && sync_map.all_models_count && !sync_map.modelsSects_deleting.empty()) {
				uint8_t res = NULL;
				
				std::vector<ModelsSection>::iterator it_sect_sync = sync_map.modelsSects_deleting.begin();

				while (it_sect_sync != sync_map.modelsSects_deleting.end()) {
					if (it_sect_sync->isInitialised) {
						std::vector<float*>::iterator it_model = it_sect_sync->models.begin();

						while (it_model != it_sect_sync->models.end()) {
							if (*it_model == nullptr) {
								it_model = it_sect_sync->models.erase(it_model);

								continue;
							}

							res = delModelPy(*it_model);

							if (res) {
#if debug_log && extended_debug_log
								PySys_WriteStdout("[NY_Event]: Error - create_models - delModelPy - Error code %d\n", (uint32_t)res);
#endif

								GUI_setError(res);

								it_model++;

								continue;
							}

							/*

							res = delModelCoords(it_sect_sync->ID, *it_model);

							if (res) {
#if debug_log && extended_debug_log
								PySys_WriteStdout("[NY_Event]: Error - create_models - delModelCoords - Error code %d\n", (uint32_t)res);
#endif

								GUI_setError(res);

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
				}

				sync_map.modelsSects_deleting.~vector();
			}
			else {
				return NULL;
			}
		}
		else {
#if debug_log && extended_debug_log
			OutputDebugString(_T("[NY_Event]: Warning - StageID is not right for this event\n"));
#endif
		}
	}
	else {
#if debug_log && extended_debug_log
		OutputDebugString(_T("[NY_Event]: Warning - StageID is not correct\n"));
#endif
	}

	return NULL;
}

//threads functions

/*
ПОЧЕМУ НЕЛЬЗЯ ЗАКРЫВАТЬ ТАЙМЕРЫ В САМИХ СЕБЕ

-поток открыл ожидающий таймер
-таймер и говорит ему: поток, у меня тут ашыпка
-таймер сделал харакири
-поток: ТАЙМЕР!!!
-программа ушла в вечное ожидание
*/

VOID CALLBACK TimerAPCProc(
	LPVOID lpArg,               // Data value
	DWORD dwTimerLowValue,      // Timer low value
	DWORD dwTimerHighValue)    // Timer high value

{
	// Formal parameters not used in this example.
	UNREFERENCED_PARAMETER(lpArg);

	UNREFERENCED_PARAMETER(dwTimerLowValue);
	UNREFERENCED_PARAMETER(dwTimerHighValue);

	uint32_t databaseID = EVENT_START_TIMER->databaseID;
	uint32_t map_ID     = EVENT_START_TIMER->map_ID;
	uint32_t eventID    = EVENT_START_TIMER->eventID;

	if (!isTimerStarted) {
		isTimerStarted = true;
	}

	request = send_token(databaseID, map_ID, eventID, NULL, nullptr);

	//включаем GIL для этого потока

	PyGILState_STATE gstate = PyGILState_Ensure();

	//-----------------------------

	if (request) {
		if (request > 9U) {
#if debug_log && extended_debug_log
			PySys_WriteStdout("[NY_Event][ERROR]: TIMER - send_token - Error code %d\n", request);
#endif

			GUI_setError(request);

			timerLastError = 1;

			PyGILState_Release(gstate);

			return;
		}

#if debug_log && extended_debug_log
		PySys_WriteStdout("[NY_Event][WARNING]: TIMER - send_token - Warning code %d\n", request);
#endif

		GUI_setWarning(request);

		PyGILState_Release(gstate);

		return;
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("[NY_Event]: generating token OK!\n"));
#endif

	request = handle_battle_event(eventID);

	if (request) {
#if debug_log && extended_debug_log
		PySys_WriteStdout("[NY_Event][ERROR]: TIMER - create_models - Error code %d\n", request);
#endif

		GUI_setError(request);

		timerLastError = 2;

		PyGILState_Release(gstate);

		return;
	}

	//выключаем GIL для этого потока

	PyGILState_Release(gstate);

	//------------------------------
}

DWORD WINAPI SecondThread(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	if (!isInited) {
		return 1U;
	}

	wchar_t msgBuf[255];

	uint32_t databaseID;
	uint8_t  map_ID;
	uint8_t  eventID;

	PyGILState_STATE gstate;

	DWORD EVENT_IN_HANGAR_WaitResult = WaitForSingleObject(
		EVENT_IN_HANGAR->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_IN_HANGAR_WaitResult)
	{
		// Event object was signaled
	case WAIT_OBJECT_0:
#if debug_log && extended_debug_log
		OutputDebugString(_T("HangarEvent was signaled!\n"));
#endif

		//место для рабочего кода

		databaseID = EVENT_IN_HANGAR->databaseID;
		map_ID = EVENT_IN_HANGAR->map_ID;
		eventID = EVENT_IN_HANGAR->eventID;

		if (eventID != EventsID.IN_HANGAR) {
			ResetEvent(EVENT_IN_HANGAR->hEvent); //если ивент не совпал с нужным - что-то идет не так, глушим тред, следующий запуск треда при входе в ангар

#if debug_log && extended_debug_log
			OutputDebugString(_T("[NY_Event][ERROR]: IN_HANGAR - eventID not equal!\n"));
#endif

			return 2U;
		}

		//рабочая часть

		first_check = send_token(databaseID, map_ID, eventID, NULL, nullptr);

		//включаем GIL для этого потока

		gstate = PyGILState_Ensure();

		//-----------------------------

		if (first_check) {
			if (first_check > 9U) {
#if debug_log && extended_debug_log
				PySys_WriteStdout("[NY_Event][ERROR]: IN_HANGAR - Error code %d\n", request);
#endif

				GUI_setError(first_check);

				return 5U;
			}

#if debug_log && extended_debug_log
			PySys_WriteStdout("[NY_Event][WARNING]: IN_HANGAR - Warning code %d\n", request);
#endif

			GUI_setWarning(first_check);

			return 4U;
		}

		//выключаем GIL для этого потока

		PyGILState_Release(gstate);

		//------------------------------

		//очищаем ивент

		ResetEvent(EVENT_IN_HANGAR->hEvent);

		break;

		// An error occurred
	default:
		ResetEvent(EVENT_IN_HANGAR->hEvent);

#if debug_log && extended_debug_log
		OutputDebugString(_T("[NY_Event][ERROR]: IN_HANGAR - something wrong with WaitResult!\n"));
#endif

		return 3U;
	}
	if (first_check) {
#if debug_log && extended_debug_log
		wsprintfW(msgBuf, _T("[NY_Event][ERROR]: IN_HANGAR - Error %d!\n"), (uint32_t)first_check);

		OutputDebugString(msgBuf);
#endif

		return 4U;
	}

	/*do {
		DWORD EVENT_IN_HANGAR_WaitResult = WaitForSingleObject(
			EVENT_IN_HANGAR->hEvent, // event handle
			INFINITE);               // indefinite wait

		switch (EVENT_IN_HANGAR_WaitResult)
		{
			// Event object was signaled
		case WAIT_OBJECT_0:
#if debug_log && extended_debug_log
			OutputDebugString(_T("HangarEvent was signaled!\n"));
#endif

			//место для рабочего кода

			databaseID = EVENT_IN_HANGAR->databaseID;
			map_ID = EVENT_IN_HANGAR->map_ID;
			eventID = EVENT_IN_HANGAR->eventID;

			if (eventID != EventsID.IN_HANGAR) {
				ResetEvent(EVENT_IN_HANGAR);

				return 2U;
			}

			//рабочая часть

			request = send_token(databaseID, map_ID, eventID, NULL, nullptr);

			//включаем GIL для этого потока

			gstate = PyGILState_Ensure();

			//-----------------------------

			if (eventID == EventsID.IN_HANGAR) {
				if (request) {
					if (request > 9U) {
#if debug_log && extended_debug_log
						PySys_WriteStdout("[NY_Event][ERROR]: IN_HANGAR - Error code %d\n", request);
#endif

						GUI_setError(request);

						return 4U;
					}

#if debug_log && extended_debug_log
					PySys_WriteStdout("[NY_Event][WARNING]: IN_HANGAR - Warning code %d\n", request);
#endif

					GUI_setWarning(request);

					return 3U;
				}
			}
			/*else if (eventID == EventsID.IN_BATTLE_GET_FULL || eventID == EventsID.IN_BATTLE_GET_SYNC) {
				if (request) {
					if (request > 9U) {
	#if debug_log && extended_debug_log
						PySys_WriteStdout("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - send_token - Error code %d\n", request);
	#endif

						GUI_setError(request);

						return 6U;
					}

	#if debug_log && extended_debug_log
					PySys_WriteStdout("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - send_token - Warning code %d\n", request);
	#endif

					GUI_setWarning(request);

					return 5U;
				}

	#if debug_log && extended_debug_log
				OutputDebugString(_T("[NY_Event]: generating token OK!\n"));
	#endif

				request = create_models(eventID);

				if (request) {
					if (request > 9U) {
	#if debug_log && extended_debug_log
						PySys_WriteStdout("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - create_models - Error code %d\n", request);
	#endif

						GUI_setError(request);

						return 4U;
					}

	#if debug_log && extended_debug_log
					PySys_WriteStdout("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - create_models - Warning code %d\n", request);
	#endif

					GUI_setWarning(request);

					return 3U;
				}
			}


			//выключаем GIL для этого потока

			PyGILState_Release(gstate);

			//------------------------------

			//очищаем ивент

			ResetEvent(EVENT_IN_HANGAR);

			break;

			// An error occurred
		default:
			return NULL;
		}
	} //запускаем таймер
	while (request || !battleEnded);*/


	BOOL            bSuccess;
	__int64         qwDueTime;
	LARGE_INTEGER   liDueTime;

	DWORD EVENT_START_TIMER_WaitResult = WaitForSingleObject(
		EVENT_START_TIMER->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_START_TIMER_WaitResult)
	{
		// Event object was signaled
	case WAIT_OBJECT_0:
#if debug_log && extended_debug_log
		OutputDebugString(_T("STEvent was signaled!\n"));
#endif

		//место для рабочего кода

		if (EVENT_START_TIMER->eventID != EventsID.IN_BATTLE_GET_FULL && EVENT_START_TIMER->eventID != EventsID.IN_BATTLE_GET_SYNC) {
			ResetEvent(EVENT_START_TIMER->hEvent); //если ивент не совпал с нужным - что-то идет не так, глушим тред, следующий запуск треда при входе в ангар

#if debug_log && extended_debug_log
			OutputDebugString(_T("[NY_Event][ERROR]: START_TIMER - eventID not equal!\n"));
#endif

			return 2U;
		}

		if (first_check || battleEnded) {
#if debug_log && extended_debug_log
			OutputDebugString(_T("[NY_Event][ERROR]: START_TIMER - first_check or battleEnded!\n"));
#endif

			return 3U;
		}

		//рабочая часть

		//инициализация таймера для получения полного списка моделей и синхронизации

		hTimer = CreateWaitableTimer(
			NULL,                   // Default security attributes
			FALSE,                  // Create auto-reset timer
			TEXT("BattleTimer"));   // Name of waitable timer

		if (hTimer != NULL)
		{
			__try
			{
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
					while (!first_check && !battleEnded && !timerLastError)
					{
						SleepEx(
							INFINITE,     // Wait forever
							TRUE);        // Put thread in an alertable state
					}

					if (timerLastError) {
#if debug_log && extended_debug_log
						OutputDebugString(_T("Timer got the error\n"));
#endif

						CancelWaitableTimer(hTimer);
					}
				}
				else
				{
#if debug_log && extended_debug_log
					OutputDebugString(_T("SetWaitableTimer failed\n"));
#endif
				}
			}
			__finally
			{
				CloseHandle(hTimer);
			}
		}
		else
		{
			printf("CreateWaitableTimer failed with error %d\n", GetLastError());
		}

		//очищаем ивент

		ResetEvent(EVENT_START_TIMER->hEvent);

		break;

		// An error occurred
	default:
		ResetEvent(EVENT_START_TIMER->hEvent);

#if debug_log && extended_debug_log
		OutputDebugString(_T("[NY_Event][ERROR]: START_TIMER - something wrong with WaitResult!\n"));
#endif

		return 3U;
	}

	//закрываем поток

#if debug_log && extended_debug_log
	wsprintfW(msgBuf, _T("Closing thread %d\n"), secondThreadID);

	OutputDebugString(msgBuf);
#endif

	ExitThread(NULL); //завершаем поток

	return NULL;
}

//-----------------

uint8_t makeEventInThread(uint8_t map_ID, uint8_t eventID) { //переводим ивенты в сигнальные состояния
	if (!isInited || !databaseID || battleEnded) {
		return 1U;
	}

	if (eventID == EventsID.IN_HANGAR || eventID == EventsID.IN_BATTLE_GET_FULL || eventID == EventsID.IN_BATTLE_GET_SYNC) { //посылаем ивент и обрабатываем в треде
		if      (eventID == EventsID.IN_HANGAR) {
			if (!EVENT_IN_HANGAR) {
				return 4;
			}

			EVENT_IN_HANGAR->databaseID = databaseID; //заполняем буфер для ангара
			EVENT_IN_HANGAR->map_ID = map_ID;
			EVENT_IN_HANGAR->eventID = eventID;

			if (!SetEvent(EVENT_IN_HANGAR->hEvent))
			{
#if debug_log && extended_debug_log
				OutputDebugString(_T("HangarEvent not setted!\n"));
#endif

				return 5;
			}
		}
		else if (eventID == EventsID.IN_BATTLE_GET_FULL || eventID == EventsID.IN_BATTLE_GET_SYNC) {
			if (!EVENT_START_TIMER) {
				return 4;
			}

			EVENT_START_TIMER->databaseID = databaseID;
			EVENT_START_TIMER->map_ID     = map_ID;
			EVENT_START_TIMER->eventID    = eventID;

			if (!SetEvent(EVENT_START_TIMER->hEvent))
			{
#if debug_log && extended_debug_log
				OutputDebugString(_T("STEvent event not setted!\n"));
#endif

				return 5;
			}
		}

		return NULL;
	}

	return 2U;
};

static PyObject* event_start(PyObject *self, PyObject *args) {
	if (first_check || !mapID || !databaseID) {
		first_check = NULL;
		battleEnded = false;
		mapID = 217U;
		//mapID = 115U;
		databaseID = 2274297;
	}

	if (!isInited || first_check) {
		return PyInt_FromSize_t(1U);
	}

	PyObject* __player = PyString_FromStringAndSize("player", 6U);

	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	isModelsAlreadyCreated = false;

	if (!player) {
		return PyInt_FromSize_t(2U);
	}

	PyObject* __arena = PyString_FromStringAndSize("arena", 5U);
	PyObject* arena = PyObject_GetAttr(player, __arena);

	Py_DECREF(__arena);

	Py_DECREF(player);

	if (!arena) {
		return PyInt_FromSize_t(3U);
	}

	PyObject* __arenaType = PyString_FromStringAndSize("arenaType", 9U);
	PyObject* arenaType = PyObject_GetAttr(arena, __arenaType);

	Py_DECREF(__arenaType);
	Py_DECREF(arena);

	if (!arenaType) {
		return PyInt_FromSize_t(4U);
	}

	PyObject* __geometryName = PyString_FromStringAndSize("geometryName", 12U);
	PyObject* map_PS = PyObject_GetAttr(arenaType, __geometryName);

	Py_DECREF(__geometryName);
	Py_DECREF(arenaType);

	if (!map_PS) {
		return PyInt_FromSize_t(5U);
	}

	char* map_s = PyString_AS_STRING(map_PS);

	Py_DECREF(map_PS);

	char map_ID_s[4];
	memcpy(map_ID_s, map_s, 3U);
	if (map_ID_s[2] == '_') map_ID_s[2] = NULL;
	map_ID_s[3] = NULL;

	mapID = atoi(map_ID_s);

#if debug_log && extended_debug_log
	PySys_WriteStdout("mapID = %d\n", (uint32_t)mapID);
#endif

	battleEnded = false;

	GUI_setTimerVisible(true);
	GUI_setVisible(true);

	isTimeVisible = true;

	request = makeEventInThread(mapID, EventsID.IN_BATTLE_GET_FULL);

	if (request) {
#if debug_log && extended_debug_log
		PySys_WriteStdout("[NY_Event]: Error in get: %d\n", request);
#endif

		return PyInt_FromSize_t(6U);
	}

	set_visible(false);

	Py_RETURN_NONE;
};

uint8_t del_models() {
	if (!isInited || first_check || battleEnded /*|| request*/) {
		return 1U;
	}

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: models deleting...\n"));
#endif

	std::vector<ModModel*>::iterator it_model = models.begin();

	while (it_model != models.end()) {
		if (*it_model == nullptr) {
			it_model = models.erase(it_model);

			continue;
		}

		if (!(*it_model)->model || (*it_model)->model == Py_None || !(*it_model)->processed) {
#if debug_log && extended_debug_log && super_extended_debug_log
			OutputDebugString(_T("NULL\n"));
#endif
			Py_XDECREF((*it_model)->model);

			(*it_model)->model = NULL;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			continue;
		}
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("[NY_Event]: del debug 1.1\n"));
#endif

		PyObject* __delModel = PyString_FromStringAndSize("delModel", 8U);

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
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("[NY_Event]: del debug 1.2\n"));
#endif
	}

	std::vector<ModLight*>::iterator it_light = lights.begin();

	while (it_light != lights.end()) {
		if (*it_light == nullptr) {
			it_light = lights.erase(it_light);

			continue;
		}

#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("[NY_Event]: del debug 1.1\n"));
#endif

		if (!(*it_light)->model || (*it_light)->model == Py_None) {
#if debug_log && extended_debug_log && super_extended_debug_log
			OutputDebugString(_T("NULL\n"));
#endif
			Py_XDECREF((*it_light)->model);

			(*it_light)->model = NULL;
			(*it_light)->coords = nullptr;

			delete *it_light;
			*it_light = nullptr;

			it_light = lights.erase(it_light);

			continue;
		}

		it_light++;
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("[NY_Event]: del debug 1.2\n"));
#endif
	}

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: models deleting OK!\n"));
#endif

	return NULL;
}

uint8_t event_fini() {
	if (!isInited || first_check) {
		return 1U;
	}

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: fini...\n"));
#endif

	if (!models.empty()) {
		request = NULL;

		uint8_t delete_models = del_models();

		if (delete_models) {
#if debug_log && extended_debug_log
			PySys_WriteStdout("[NY_Event]: Warning: del_models: %d\n", (uint32_t)delete_models);
#endif
		}

		std::vector<ModModel*>::iterator it_model = models.begin();

		while (it_model != models.end()) {
			if (*it_model == nullptr) {
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
		}

		models.~vector();
	}

	isModelsAlreadyInited = false;

	if (!lights.empty()) {
		std::vector<ModLight*>::iterator it_light = lights.begin();

		while (it_light != lights.end()) {
			if (*it_light == nullptr) {
				it_light = lights.erase(it_light);

				continue;
			}

			Py_XDECREF((*it_light)->model);

			(*it_light)->model = NULL;
			(*it_light)->coords = nullptr;

			delete *it_light;
			*it_light = nullptr;

			it_light = lights.erase(it_light);
		}

		lights.~vector();
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("[NY_Event]: fini debug 1\n"));
#endif

	if (!current_map.modelsSects.empty() && current_map.minimap_count) {
#if debug_log && extended_debug_log
		OutputDebugString(_T("[PositionsMod_Free]: fini debug 3\n"));
#endif

		Py_BEGIN_ALLOW_THREADS;
		clearModelsSections();
		Py_END_ALLOW_THREADS;


	}

	isModelsAlreadyCreated = false;

	current_map.minimap_count = NULL;

	if (isTimeVisible) {
		GUI_setTime(NULL);
		GUI_setTimerVisible(false);

		isTimeVisible = false;

		current_map.time_preparing = NULL;
	}

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: fini OK!\n"));
#endif

	return NULL;
}

void closeEvent1(PEVENTDATA_1* pEvent) {
	if (*pEvent != NULL) { //если уже была инициализирована структура - удаляем
		if ((*pEvent)->hEvent != NULL) {
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		}

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	}
}

void closeEvent2(PEVENTDATA_2* pEvent) {
	if (*pEvent != NULL) { //если уже была инициализирована структура - удаляем
		if ((*pEvent)->hEvent != NULL) {
			CloseHandle((*pEvent)->hEvent);

			(*pEvent)->hEvent = NULL;
		}

		HeapFree(GetProcessHeap(), NULL, *pEvent);

		*pEvent = NULL;
	}
}

static PyObject* event_fini_py(PyObject *self, PyObject *args) {
	uint8_t fini_result = event_fini();

	if (fini_result) {
		return PyInt_FromSize_t(fini_result);
	}
	else {
		battleEnded = true;

		current_map.stageID = StagesID.COMPETITION;

		PyObject* delLabelCBID_p = GUI_getAttr("delLabelCBID");

		if (!delLabelCBID_p || delLabelCBID_p == Py_None) {
			delLabelCBID = NULL;

			Py_XDECREF(delLabelCBID_p);
		}
		else {
			delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);

			Py_DECREF(delLabelCBID_p);
		}

		cancelCallback(&delLabelCBID);

		if (hTimer != NULL) { //закрываем таймер, если он был создан
			CloseHandle(hTimer);

			hTimer = NULL;
		}

		isTimerStarted = false;

		closeEvent1(&EVENT_START_TIMER);
		closeEvent1(&EVENT_IN_HANGAR);

		closeEvent2(&EVENT_ALL_MODELS_CREATED);

		allModelsCreated = NULL;

		GUI_setVisible(false);
		GUI_clearText();

		request = NULL;
		mapID = NULL;

		Py_RETURN_NONE;
	}
};

static PyObject* event_err_code(PyObject *self, PyObject *args) {
	return PyInt_FromSize_t(first_check);
};

bool createEvent1(PEVENTDATA_1* pEvent, uint8_t eventID) {
	closeEvent1(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_1)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выделяем память в куче для ивента
		sizeof(EVENTDATA_1));

	if (*pEvent == NULL) //нехватка памяти, завершаем работу
	{
		ExitProcess(1);
	}

	(*pEvent)->hEvent = CreateEvent(
		NULL,                      // default security attributes
		TRUE,                      // manual-reset event
		FALSE,                     // initial state is nonsignaled
		EVENT_NAMES[eventID]       // object name
	);

	if ((*pEvent)->hEvent == NULL)
	{
		OutputDebugString(TEXT("Primary event creating error\n"));

		return false;
	}

	return true;
}

bool createEvent2(PEVENTDATA_2* pEvent, LPCWSTR eventName) {
	closeEvent2(pEvent); //закрываем ивент, если существует

	*pEvent = (PEVENTDATA_2)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, //выделяем память в куче для ивента
		sizeof(EVENTDATA_2));

	if (*pEvent == NULL) //нехватка памяти, завершаем работу
	{
		ExitProcess(1);
	}

	(*pEvent)->hEvent = CreateEvent(
		NULL,                      // default security attributes
		TRUE,                      // manual-reset event
		FALSE,                     // initial state is nonsignaled
		eventName                  // object name
	);

	if ((*pEvent)->hEvent == NULL)
	{
		OutputDebugString(TEXT("Secondary event creating error\n"));

		return false;
	}

	return true;
}

bool createEventsAndSecondThread() {
	if (!createEvent1(&EVENT_IN_HANGAR, EventsID.IN_HANGAR)) {
		return false;
	}
	if (!createEvent1(&EVENT_START_TIMER, EventsID.IN_BATTLE_GET_FULL)) {
		return false;
	}

	if (!createEvent2(&EVENT_ALL_MODELS_CREATED, L"NY_Event_AllModelsCreatedEvent")) {
		return false;
	}
	//TODO: сделать ивент - удаление ближайшей модели

	//Thread creating

	if (hSecondThread) {
		CloseHandle(hSecondThread);

		hSecondThread = NULL;
	}

	hSecondThread = CreateThread( //создаем второй поток
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		SecondThread,                           // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&secondThreadID);                       // returns the thread identifier 

	if (hSecondThread == NULL)
	{
		OutputDebugString(TEXT("CreateThread: error 1\n"));

		return false;
	}

	return true;
}

uint8_t event_сheck() {
	if (!isInited) {
		return 1U;
	}

	// инициализация второго потока, если не существует, иначе - завершить второй поток и начать новый

	if (!createEventsAndSecondThread()) {
		return 2U;
	}

	//------------------------------------------------------------------------------------------------

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: checking...\n"));
#endif

	PyObject* __player = PyString_FromStringAndSize("player", 6U);
	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	if (!player) {
		return 3U;
	}

	PyObject* __databaseID = PyString_FromStringAndSize("databaseID", 10U);
	PyObject* DBID_string = PyObject_GetAttr(player, __databaseID);

	Py_DECREF(__databaseID);
	Py_DECREF(player);

	if (!DBID_string) {
		return 4U;
	}

	PyObject* DBID_int = PyNumber_Int(DBID_string);

	Py_DECREF(DBID_string);

	if (!DBID_int) {
		return 5U;
	}

	databaseID = PyInt_AS_LONG(DBID_int);

	Py_DECREF(DBID_int);

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: DBID created\n"));
#endif

	battleEnded = false;

	first_check = makeEventInThread(NULL, EventsID.IN_HANGAR);

	if (first_check) {
		return 6U;
	}
	else {
		return NULL;
	}
}

static PyObject* event_сheck_py(PyObject *self, PyObject *args) {
	uint8_t res = event_сheck();

	if (res) {
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
};

uint8_t event_init(PyObject* template_, PyObject* apply, PyObject* byteify) {
	if (!template_ || !apply || !byteify) {
		return 1U;
	}

	if (g_gui && PyCallable_Check(template_) && PyCallable_Check(apply)) {
		Py_INCREF(template_);
		Py_INCREF(apply);

		PyObject* __register = PyString_FromStringAndSize("register", 8U);
		PyObject* result = PyObject_CallMethodObjArgs(g_gui, __register, PyString_FromString(g_self->ids), template_, g_self->data, apply, NULL);

		Py_XDECREF(result);
		Py_DECREF(__register);
		Py_DECREF(apply);
		Py_DECREF(template_);
	}

	if (!g_gui && PyCallable_Check(byteify)) {
		Py_INCREF(byteify);

		PyObject* args1 = PyTuple_New(1U);
		PyTuple_SET_ITEM(args1, NULL, g_self->i18n);

		PyObject* result = PyObject_CallObject(byteify, args1);

		if (result) {
			PyObject* old = g_self->i18n;

			g_self->i18n = result;

			PyDict_Clear(old);
			Py_DECREF(old);
		}

		Py_DECREF(byteify);
	}

	return NULL;
}

static PyObject* event_init_py(PyObject *self, PyObject *args) {
	if (!isInited) {
		return PyInt_FromSize_t(1U);
	}

	PyObject* template_ = NULL;
	PyObject* apply     = NULL;
	PyObject* byteify   = NULL;

	if (!PyArg_ParseTuple(args, "OOO", &template_, &apply, &byteify)) {
		return PyInt_FromSize_t(2U);
	}

	uint8_t res = event_init(template_, apply, byteify);

	if (res) {
		return PyInt_FromSize_t(res);
	}
	else Py_RETURN_NONE;
};

static PyObject* event_inject_handle_key_event(PyObject *self, PyObject *args) {
	if (!isInited || first_check || request || !databaseID || !mapID || !spaceKey || isStreamer) {
		Py_RETURN_NONE;
	}

	PyObject* event_ = PyTuple_GET_ITEM(args, NULL);
	PyObject* isKeyGetted_Space = NULL;

	if (g_gui) {
		PyObject* __get_key = PyString_FromStringAndSize("get_key", 7U);
		
		isKeyGetted_Space = PyObject_CallMethodObjArgs(g_gui, __get_key, spaceKey, NULL);

		Py_DECREF(__get_key);
	}
	else {
		PyObject* __key = PyString_FromStringAndSize("key", 3U);
		PyObject* key = PyObject_GetAttr(event_, __key);

		Py_DECREF(__key);

		if (!key) {
			Py_RETURN_NONE;
		}

		PyObject* ____contains__ = PyString_FromStringAndSize("__contains__", 12U);

		isKeyGetted_Space = PyObject_CallMethodObjArgs(spaceKey, ____contains__, key, NULL);

		Py_DECREF(____contains__);
	}

	if (isKeyGetted_Space == Py_True) {
		uint8_t modelID;
		float* coords = new float[3];

		uint8_t find_result = findLastModelCoords(5.0, &modelID, &coords);

		if (!find_result) {
			uint8_t server_req = send_token(databaseID, mapID, EventsID.DEL_LAST_MODEL, modelID, coords);

			if (server_req) {
				if (server_req > 9U) {
#if debug_log && extended_debug_log
					PySys_WriteStdout("[NY_Event][ERROR]: DEL_LAST_MODEL - send_token - Error code %d\n", server_req);
#endif

					GUI_setError(server_req);

					Py_RETURN_NONE;
				}

#if debug_log && extended_debug_log
				PySys_WriteStdout("[NY_Event][WARNING]: DEL_LAST_MODEL - send_token - Warning code %d\n", server_req);
#endif

				GUI_setWarning(server_req);

				Py_RETURN_NONE;
			}

			uint8_t deleting_py_models = delModelPy(coords);

			if (deleting_py_models) {
#if debug_log && extended_debug_log
				PySys_WriteStdout("[NY_Event][ERROR]: DEL_LAST_MODEL - delModelPy - Error code %d\n", deleting_py_models);
#endif

				GUI_setError(deleting_py_models);

				Py_RETURN_NONE;
			}

			scoreID = modelID;
			current_map.stageID = StagesID.GET_SCORE;

			/*
			uint8_t deleting_coords = delModelCoords(modelID, coords);

			if (deleting_coords) {
#if debug_log && extended_debug_log
					PySys_WriteStdout("[NY_Event][ERROR]: DEL_LAST_MODEL - delModelCoords - Error code %d\n", deleting_coords);
#endif

					GUI_setError(deleting_coords);

					Py_RETURN_NONE;
			}
			*/
		}
		else if (find_result == 7U) {
			current_map.stageID = StagesID.ITEMS_NOT_EXISTS;
		}

		if (current_map.stageID == StagesID.GET_SCORE && scoreID != -1) {
			GUI_setMsg(current_map.stageID, scoreID, 5.0f);

			scoreID = -1;
		}
		else if (current_map.stageID == StagesID.ITEMS_NOT_EXISTS) {
			GUI_setMsg(current_map.stageID);
		}

		//current_map.stageID = StagesID.COMPETITION;
	}

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

	if (!BigWorld) {
		return;
	}

	PyObject* appLoader = PyImport_ImportModule("gui.app_loader");

	if (!appLoader) {
		return;
	}

	PyObject* __g_appLoader = PyString_FromStringAndSize("g_appLoader", 11U);

	g_appLoader = PyObject_GetAttr(appLoader, __g_appLoader);

	Py_DECREF(__g_appLoader);
	Py_DECREF(appLoader);

	if (!g_appLoader) {
		return;
	}

	functools = PyImport_ImportModule("functools");

	if (!functools) {
		return;
	}

	json = PyImport_ImportModule("json");

	if (!json) {
		Py_DECREF(g_appLoader);
		return;
	}

#if debug_log
	OutputDebugString(_T("Config init...\n"));
#endif

	if (PyType_Ready(&Config_p)) {
		Py_DECREF(g_appLoader);
		return;
	}

	Py_INCREF(&Config_p);

#if debug_log
	OutputDebugString(_T("Config init OK\n"));
#endif

	PyObject* g_config = PyObject_CallObject((PyObject*)&Config_p, NULL);

	Py_DECREF(&Config_p);

	if (!g_config) {
		Py_DECREF(g_appLoader);
		return;
	}

	event_module = Py_InitModule3("event",
		event_methods,
		event_methods__doc__);

	if (!event_module) {
		Py_DECREF(g_appLoader);
		return;
	}

	if (PyModule_AddObject(event_module, "l", g_config)) {
		Py_DECREF(g_appLoader);
		return;
	}

	//

	PyObject* __omc = PyString_FromStringAndSize("omc", 3);

	onModelCreatedPyMeth = PyObject_GetAttr(event_module, __omc);

	Py_DECREF(__omc);

	if (!onModelCreatedPyMeth) {
		Py_DECREF(g_appLoader);
		return;
	}

	//Space key

	spaceKey = PyList_New(1U);

	if (spaceKey) {
		PyList_SET_ITEM(spaceKey, 0U, PyInt_FromSize_t(57U));
	}

	//---------

#if debug_log
	OutputDebugString(_T("Mod_GUI module loading...\n"));
#endif

	PyObject* mGUI_module = PyImport_ImportModule("NY_Event.native.mGUI");

	if (!mGUI_module) {
		Py_DECREF(g_appLoader);
		return;
	}

#if debug_log
	OutputDebugString(_T("Mod_GUI class loading...\n"));
#endif

	PyObject* __Mod_GUI = PyString_FromStringAndSize("Mod_GUI", 7U);

	modGUI = PyObject_CallMethodObjArgs(mGUI_module, __Mod_GUI, NULL);
	
	Py_DECREF(__Mod_GUI);
	Py_DECREF(mGUI_module);

	if (!modGUI) {
		Py_DECREF(g_appLoader);
		return;
	}

#if debug_log
	OutputDebugString(_T("Mod_GUI class loaded OK!\n"));
#endif

#if debug_log
	OutputDebugString(_T("g_gui module loading...\n"));
#endif

	PyObject* mod_mods_gui = PyImport_ImportModule("gui.mods.mod_mods_gui");

	if (!mod_mods_gui) {
		PyErr_Clear();
		g_gui = NULL;
#if debug_log
		OutputDebugString(_T("mod_mods_gui is NULL!\n"));
#endif
	}
	else {
		PyObject* __g_gui = PyString_FromStringAndSize("g_gui", 5U);

		g_gui = PyObject_GetAttr(mod_mods_gui, __g_gui);

		Py_DECREF(__g_gui);
		Py_DECREF(mod_mods_gui);

		if (!g_gui) {
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			return;
		}

#if debug_log
		OutputDebugString(_T("mod_mods_gui loaded OK!\n"));
#endif
	}

	if (!g_gui) {
		_mkdir("mods/configs");
		_mkdir("mods/configs/pavel3333");
		_mkdir("mods/configs/pavel3333/NY_Event");
		_mkdir("mods/configs/pavel3333/NY_Event/i18n");

		if (!read_data(true) || !read_data(false)) {
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			return;
		}
	}
	else {
		PyObject* ids = PyString_FromString(g_self->ids);

		if (!ids) {
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			Py_DECREF(g_gui);
			return;
		}

		PyObject* __register_data = PyString_FromStringAndSize("register_data", 13U);
		PyObject* __pavel3333 = PyString_FromStringAndSize("pavel3333", 9U);
		PyObject* data_i18n = PyObject_CallMethodObjArgs(g_gui, __register_data, ids, g_self->data, g_self->i18n, __pavel3333, NULL);

		Py_DECREF(__pavel3333);
		Py_DECREF(__register_data);
		Py_DECREF(ids);

		if (!data_i18n) {
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			Py_DECREF(g_gui);
			Py_DECREF(ids);
			return;
		}

		PyObject* old = g_self->data;

		g_self->data = PyTuple_GET_ITEM(data_i18n, NULL);

		PyDict_Clear(old);

		Py_DECREF(old);

		old = g_self->i18n;

		g_self->i18n = PyTuple_GET_ITEM(data_i18n, 1U);

		PyDict_Clear(old);

		Py_DECREF(old);
		Py_DECREF(data_i18n);
	}
	
	uint32_t curl_init_result = curl_init();

	if (curl_init_result) {
		OutputDebugString(_T("Error while initialising CURL handle!\n"));

		return;
	}

	isInited = true;
};
