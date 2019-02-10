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

bool isStreamer = false;

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

PEVENTDATA_2 EVENT_NETWORK_NOT_USING = NULL;
PEVENTDATA_2 EVENT_MODELS_NOT_USING  = NULL;

PEVENTDATA_2 EVENT_ALL_MODELS_CREATED = NULL;

PEVENTDATA_2 EVENT_BATTLE_ENDED = NULL;

//---------------------

STAGE_ID lastStageID = STAGE_ID::COMPETITION;
EVENT_ID lastEventID = EVENT_ID::IN_HANGAR;

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

		PyObject* data_p = PyString_FromString(data_s);

		delete[] data_s;

		PyObject* __loads = PyString_FromString("loads");

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
	PyObject* __player = PyString_FromString("player");

	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	isModelsAlreadyCreated = false;

	if (!player) {
		return 1;
	}

	PyObject* __vehicle = PyString_FromString("vehicle");
	PyObject* vehicle = PyObject_GetAttr(player, __vehicle);

	Py_DECREF(__vehicle);

	Py_DECREF(player);

	if (!vehicle) {
		return 2;
	}

	PyObject* __model = PyString_FromString("model");
	PyObject* model_p = PyObject_GetAttr(vehicle, __model);

	Py_DECREF(__model);
	Py_DECREF(vehicle);

	if (!model_p) {
		return 3;
	}

	PyObject* __position = PyString_FromString("position");
	PyObject* position_Vec3 = PyObject_GetAttr(model_p, __position);

	Py_DECREF(__position);
	Py_DECREF(model_p);

	if (!position_Vec3) {
		return 4;
	}

	double* coords_pos = new double[3];

	for (uint8_t i = NULL; i < 3; i++) {
		PyObject* __tuple = PyString_FromString("tuple");

		PyObject* position = PyObject_CallMethodObjArgs(position_Vec3, __tuple, NULL);

		Py_DECREF(__tuple);

		if (!position) {
			return 5;
		}

		PyObject* coord_p = PyTuple_GetItem(position, i);

		if (!coord_p) {
			return 6;
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
		return 8;
	}

	if (dist > dist_equal) {
		return 7;
	}

	*modelID = modelTypeLast;
	*coords = coords_res;

	return NULL;
}

uint8_t delModelPy(float* coords) {
	if (coords == nullptr) {
		return 1;
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

			PyObject* __visible = PyString_FromString("visible");

			if (!PyObject_SetAttr((*it_model)->model, __visible, py_visible)) {
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
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("[NY_Event]: del debug 1.2\n"));
#endif

	return 2;
}

uint8_t delModelCoords(uint16_t ID, float* coords) {
	if (coords == nullptr) {
		return 1;
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
	}

	return 2;
}

static PyObject* event_light(float coords[3]) {
	if (!isInited || battleEnded) {
		return NULL;
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("light creating...\n"));
#endif

	PyObject* __PyOmniLight = PyString_FromString("PyOmniLight");

	PyObject* Light = PyObject_CallMethodObjArgs(BigWorld, __PyOmniLight, NULL);

	Py_DECREF(__PyOmniLight);

	if (!Light) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight creating FAILED\n"));
#endif
		return NULL;
	}

	//---------inner radius---------

	PyObject* __innerRadius = PyString_FromString("innerRadius");
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

	PyObject* __outerRadius = PyString_FromString("outerRadius");
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

	PyObject* __multiplier = PyString_FromString("multiplier");
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

	PyObject* coords_p = PyTuple_New(3);

	if (!coords_p) {
#if debug_log && extended_debug_log && super_extended_debug_log
		OutputDebugString(_T("PyOmniLight coords creating FAILED\n"));
#endif
		Py_DECREF(Light);

		return NULL;
	}

	for (uint8_t i = NULL; i < 3; i++) {
		if (i == 1) {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i] + 0.5));
		}
		else {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i]));
		}
	}

	PyObject* __position = PyString_FromString("position");

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

	PyObject* colour_p = PyTuple_New(4);

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

	for (uint8_t i = NULL; i < 4; i++) PyTuple_SET_ITEM(colour_p, i, PyFloat_FromDouble(colour[i]));

	delete[] colour;

	//------------------------------

	PyObject* __colour = PyString_FromString("colour");

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

	PyObject* __position = PyString_FromString("position");

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

		PyObject* __partial = PyString_FromString("partial");

		PyObject* coords_p = PyLong_FromVoidPtr((void*)coords); //передаем указатель на 3 координаты

		PyObject* partialized = PyObject_CallMethodObjArgs(functools, __partial, onModelCreatedPyMeth, coords_p, NULL);

		Py_DECREF(__partial);

		if (!partialized) {
			if (allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return NULL;
		}

		PyObject* __fetchModel = PyString_FromString("fetchModel");

		Model = PyObject_CallMethodObjArgs(BigWorld, __fetchModel, PyString_FromString(path), partialized, NULL); //запускаем асинхронное добавление модели

		Py_XDECREF(Model);
		Py_DECREF(__fetchModel);

		return NULL;
	}

	PyObject* __Model = PyString_FromString("Model");

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
		extendedDebugLog("[NY_Event][ERROR]: AMCEvent or createModelsPyMeth event is NULL!\n");

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
			extendedDebugLog("[NY_Event][ERROR]: AMCEvent event not setted!\n");

			Py_RETURN_NONE;
		}
	}

	Py_RETURN_NONE;
}

uint8_t create_models() {
	if (!isInited || battleEnded || !onModelCreatedPyMeth) {
		return 1;
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
			superExtendedDebugLog("[");

			event_model(it->path, *it2, true);

			superExtendedDebugLog("], ");
		}
	}
	superExtendedDebugLog("], \n");

	return NULL;
}

uint8_t init_models() {
	if (!isInited || first_check || request || battleEnded || models.empty()) {
		return 1;
	}

	extendedDebugLog("[NY_Event]: models adding...\n");

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
	}

	extendedDebugLog("[NY_Event]: models adding OK!\n");

	return NULL;
}

uint8_t set_visible(bool isVisible) {
	if (!isInited || first_check || request || battleEnded || models.empty()) {
		return 1;
	}

	PyObject* py_visible = PyBool_FromLong(isVisible);

	extendedDebugLog("[NY_Event]: Models visiblity changing...\n");

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

		PyObject* __visible = PyString_FromString("visible");

		PyObject_SetAttr(models[i]->model, __visible, py_visible);

		Py_DECREF(__visible);
	}

	Py_DECREF(py_visible);

	extendedDebugLog("[NY_Event]: Models visiblity changing OK!\n");

	return NULL;
};

uint8_t handle_battle_event(EVENT_ID eventID) {
	if (!isInited || first_check || request || battleEnded || !g_self || eventID == EVENT_ID::IN_HANGAR) {
		NETWORK_NOT_USING;

		return 1;
	}

	extendedDebugLog("[NY_Event]: parsing...\n");

	uint8_t parsing_result = NULL;

	Py_BEGIN_ALLOW_THREADS

	if      (eventID == EVENT_ID::IN_BATTLE_GET_FULL) parsing_result = parse_config();
	else if (eventID == EVENT_ID::IN_BATTLE_GET_SYNC) parsing_result = parse_sync();
	else if (eventID == EVENT_ID::DEL_LAST_MODEL)     parsing_result = parse_del_model();

	NETWORK_NOT_USING;

	

	if (parsing_result) {
		extendedDebugLogFmt("[NY_Event]: parsing FAILED! Error code: %d\n", (uint32_t)parsing_result);

		//GUI_setError(parsing_result);

		return 2;
	}

	Py_END_ALLOW_THREADS

	superExtendedDebugLog("[NY_Event]: parsing OK!\n");
	
	if (current_map.time_preparing)  //выводим время
		GUI_setTime(current_map.time_preparing);
		
	if (current_map.stageID >= 0 && current_map.stageID < STAGES_COUNT) {    //выводим сообщение
		if (
			current_map.stageID == STAGE_ID::WAITING      ||
			current_map.stageID == STAGE_ID::START        ||
			current_map.stageID == STAGE_ID::COMPETITION  ||
			current_map.stageID == STAGE_ID::END_BY_TIME  ||
			current_map.stageID == STAGE_ID::END_BY_COUNT ||
			current_map.stageID == STAGE_ID::STREAMER_MODE
			) {
			if (lastStageID != STAGE_ID::GET_SCORE && lastStageID != STAGE_ID::ITEMS_NOT_EXISTS) GUI_setMsg(current_map.stageID);

			if(current_map.stageID == STAGE_ID::END_BY_TIME || current_map.stageID == STAGE_ID::END_BY_COUNT) {
				current_map.time_preparing = NULL;

				GUI_setTime(NULL);

				if (current_map.stageID == STAGE_ID::END_BY_COUNT) {
					GUI_setTimerVisible(false);

					isTimeVisible = false;
				}

				GUI_setMsg(current_map.stageID);

				uint8_t event_result = event_fini();

				if (event_result) {
					extendedDebugLogFmt("[NY_Event]: Warning - handle_battle_event - event_fini - Error code %d\n", (uint32_t)event_result);

					//GUI_setWarning(event_result);
				}
			}
			else {
				if (!isTimeVisible) {
					GUI_setTimerVisible(true);

					isTimeVisible = true;
				}
			}

			if (current_map.stageID == STAGE_ID::START          ||
				current_map.stageID == STAGE_ID::COMPETITION    || 
				current_map.stageID == STAGE_ID::STREAMER_MODE) {
				if (isModelsAlreadyCreated && !isModelsAlreadyInited && current_map.minimap_count && current_map.modelsSects.size()) {
					if (eventID == EVENT_ID::IN_BATTLE_GET_FULL) {
						superExtendedDebugLogFmt("sect count: %u\npos count: %u\n", current_map.modelsSects.size(), current_map.minimap_count);

						Py_BEGIN_ALLOW_THREADS;

						extendedDebugLog("[NY_Event]: creating...\n");

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

							return 3;
						}
						*/

						/*
						Второй способ - вызов асинхронной функции BigWorld.fetchModel(path, onLoadedMethod)

						Более-менее надежно, выполняется на уровне движка
						*/

						request = create_models();

						PyThreadState *_save;        //глушим GIL
						Py_UNBLOCK_THREADS;

						if (request) {
							extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - create_models - Error code %d\n", request);

							return 3;
						}

						//ожидаем события полного создания моделей

						extendedDebugLog("[NY_Event]: waiting EVENT_ALL_MODELS_CREATED\n");

						DWORD EVENT_ALL_MODELS_CREATED_WaitResult = WaitForSingleObject(
							EVENT_ALL_MODELS_CREATED->hEvent, // event handle
							INFINITE);                        // indefinite wait

						switch (EVENT_ALL_MODELS_CREATED_WaitResult)
						{
							// Event object was signaled
						case WAIT_OBJECT_0:
							extendedDebugLog("[NY_Event]: EVENT_ALL_MODELS_CREATED signaled!\n");

							//очищаем ивент

							ResetEvent(EVENT_ALL_MODELS_CREATED->hEvent);

							//-------------

							//место для рабочего кода

							Py_BLOCK_THREADS;

							extendedDebugLog("[NY_Event]: creating OK!\n");

							request = init_models();

							if (request) {
								if (request > 9) {
									extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - init_models - Error code %d\n", request);

									//GUI_setError(request);

									return 5;
								}

								extendedDebugLogFmt("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - init_models - Warning code %d\n", request);

								//GUI_setWarning(request);

								return 4;
							}

							request = set_visible(true);

							if (request) {
								if (request > 9) {
									extendedDebugLogFmt("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - set_visible - Error code %d\n", request);

									//GUI_setError(request);

									return 5;
								}

								extendedDebugLogFmt("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - set_visible - Warning code %d\n", request);

								//GUI_setWarning(request);

								return 4;
							}

							isModelsAlreadyInited = true;

							break;

							// An error occurred
						default:
							ResetEvent(EVENT_ALL_MODELS_CREATED->hEvent);

							extendedDebugLog("[NY_Event][ERROR]: IN_HANGAR - something wrong with WaitResult!\n");

							Py_BLOCK_THREADS;

							return 3;
						}
					}

					return NULL;
				}
			}

			if (isModelsAlreadyCreated && isModelsAlreadyInited && eventID == EVENT_ID::IN_BATTLE_GET_SYNC && sync_map.all_models_count && !sync_map.modelsSects_deleting.empty()) {
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
								extendedDebugLogFmt("[NY_Event][ERROR]: create_models - delModelPy - Error code %d\n", (uint32_t)res);

								//GUI_setError(res);

								it_model++;

								continue;
							}

							/*

							res = delModelCoords(it_sect_sync->ID, *it_model);

							if (res) {
#if debug_log && extended_debug_log
								PySys_WriteStdout("[NY_Event]: Error - create_models - delModelCoords - Error code %d\n", (uint32_t)res);
#endif

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
				}

				sync_map.modelsSects_deleting.~vector();
			}
			else {
				return NULL;
			}
		}
		else extendedDebugLog("[NY_Event]: Warning - StageID is not right for this event\n");
	}
	else extendedDebugLog("[NY_Event]: Warning - StageID is not correct\n");

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
	DWORD dwTimerHighValue)     // Timer high value
{
	// Formal parameters not used in this example.
	UNREFERENCED_PARAMETER(lpArg);

	UNREFERENCED_PARAMETER(dwTimerLowValue);
	UNREFERENCED_PARAMETER(dwTimerHighValue);
}

DWORD WINAPI TimerThread(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	if (!isInited) {
		return 1;
	}

	BOOL            bSuccess;
	__int64         qwDueTime;
	LARGE_INTEGER   liDueTime;

	PyGILState_STATE gstate;

	PyThreadState* _save;

	extendedDebugLog("[NY_Event]: waiting EVENT_START_TIMER\n");

	DWORD EVENT_START_TIMER_WaitResult = WaitForSingleObject(
		EVENT_START_TIMER->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_START_TIMER_WaitResult)
	{
		// Event object was signaled
	case WAIT_OBJECT_0:
		extendedDebugLog("[NY_Event]: EVENT_START_TIMER signaled!\n");

		//включаем GIL для этого потока

		gstate = PyGILState_Ensure();

		//-----------------------------

		Py_UNBLOCK_THREADS;

		//место для рабочего кода

		if (EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_FULL && EVENT_START_TIMER->eventID != EVENT_ID::IN_BATTLE_GET_SYNC) {
			ResetEvent(EVENT_START_TIMER->hEvent); //если ивент не совпал с нужным - что-то идет не так, глушим тред, следующий запуск треда при входе в ангар

			extendedDebugLog("[NY_Event][ERROR]: START_TIMER - eventID not equal!\n");

			return 2;
		}

		if (first_check || battleEnded) {
			extendedDebugLog("[NY_Event][ERROR]: START_TIMER - first_check or battleEnded!\n");

			return 3;
		}

		//рабочая часть

		//инициализация таймера для получения полного списка моделей и синхронизации

		hTimer = CreateWaitableTimer(
			NULL,                   // Default security attributes
			FALSE,                  // Create auto-reset timer
			TEXT("BattleTimer"));   // Name of waitable timer

		if (hTimer != NULL)
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
					uint32_t databaseID = EVENT_START_TIMER->databaseID;
					uint32_t map_ID = EVENT_START_TIMER->map_ID;
					EVENT_ID eventID = EVENT_START_TIMER->eventID;

					if (!isTimerStarted) {
						isTimerStarted = true;
					}

					extendedDebugLog("[NY_Event]: timer: waiting EVENT_NETWORK_NOT_USING\n");

					DWORD EVENT_NETWORK_NOT_USING_WaitResult = WaitForSingleObject(
						EVENT_NETWORK_NOT_USING->hEvent, // event handle
						INFINITE);               // indefinite wait

					switch (EVENT_NETWORK_NOT_USING_WaitResult) //ждем, когда сеть перестанет использоваться
					{
						// Event object was signaled
					case WAIT_OBJECT_0:
						extendedDebugLog("[NY_Event]: timer: EVENT_NETWORK_NOT_USING signaled!\n");

						NETWORK_USING;

						//рабочая часть

						request = send_token(databaseID, map_ID, eventID, NULL, nullptr);

						if (request) {
							if (request > 9) {
								extendedDebugLogFmt("[NY_Event][ERROR]: TIMER - send_token - Error code %d\n", request);

								//GUI_setError(request);

								timerLastError = 1;

								NETWORK_NOT_USING;

								break;
							}

							extendedDebugLogFmt("[NY_Event][WARNING]: TIMER - send_token - Warning code %d\n", request);

							//GUI_setWarning(request);

							NETWORK_NOT_USING;

							break;
						}

#if debug_log && extended_debug_log && super_extended_debug_log
						OutputDebugString(_T("[NY_Event]: generating token OK!\n"));
#endif

						Py_BLOCK_THREADS;

						request = handle_battle_event(eventID);

						Py_UNBLOCK_THREADS;

						if (request) {
							extendedDebugLogFmt("[NY_Event][ERROR]: TIMER - create_models - Error code %d\n", request);

							//GUI_setError(request);

							timerLastError = 2;

							break;
						}

						break;

						// An error occurred
					default:
						extendedDebugLog("[NY_Event][ERROR]: NetworkNotUsingEvent - something wrong with WaitResult!\n");

						break;
					}

					SleepEx(
						INFINITE,     // Wait forever
						TRUE);        // Put thread in an alertable state
				}

				if (timerLastError) {
					extendedDebugLog("[NY_Event][ERROR]: Timer got the error\n");

					CancelWaitableTimer(hTimer);
				}
			}
			else extendedDebugLog("[NY_Event][ERROR]: SetWaitableTimer failed\n");

			CloseHandle(hTimer);
		}
		else extendedDebugLog("[NY_Event][ERROR]: CreateWaitableTimer failed\n");

		Py_BLOCK_THREADS;

		//выключаем GIL для этого потока

		PyGILState_Release(gstate);

		//------------------------------

		//очищаем ивент

		ResetEvent(EVENT_START_TIMER->hEvent);

		break;

		// An error occurred
	default:
		ResetEvent(EVENT_START_TIMER->hEvent);

		extendedDebugLog("[NY_Event][ERROR]: START_TIMER - something wrong with WaitResult!\n");

		return 3;
	}

	//закрываем поток

	extendedDebugLogFmt("[NY_Event]: Closing timer thread %d\n", handlerThreadID);

	ExitThread(NULL); //завершаем поток

	return NULL;
}

DWORD WINAPI HandlerThread(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	if (!isInited) {
		return 1;
	}

	uint32_t databaseID;
	uint8_t  map_ID;
	EVENT_ID eventID;

	//включаем GIL для этого потока

	PyGILState_STATE gstate = PyGILState_Ensure();

	//-----------------------------

	PyThreadState* _save;

	Py_UNBLOCK_THREADS;

	DWORD EVENT_NETWORK_NOT_USING_WaitResult;

	extendedDebugLog("[NY_Event]: waiting EVENT_IN_HANGAR\n");

	DWORD EVENT_IN_HANGAR_WaitResult = WaitForSingleObject(
		EVENT_IN_HANGAR->hEvent, // event handle
		INFINITE);               // indefinite wait

	switch (EVENT_IN_HANGAR_WaitResult)
	{
		// Event object was signaled
	case WAIT_OBJECT_0:
		extendedDebugLog("[NY_Event]: EVENT_IN_HANGAR signaled!\n");

		//место для рабочего кода

		databaseID = EVENT_IN_HANGAR->databaseID;
		map_ID     = EVENT_IN_HANGAR->map_ID;
		eventID    = EVENT_IN_HANGAR->eventID;

		if (eventID != EVENT_ID::IN_HANGAR) {
			ResetEvent(EVENT_IN_HANGAR->hEvent); //если ивент не совпал с нужным - что-то идет не так, глушим тред, следующий запуск треда при входе в ангар

			extendedDebugLog("[NY_Event][ERROR]: IN_HANGAR - eventID not equal!\n");

			return 2;
		}

		//рабочая часть

		EVENT_NETWORK_NOT_USING_WaitResult = WaitForSingleObject(
			EVENT_NETWORK_NOT_USING->hEvent, // event handle
			INFINITE);               // indefinite wait

		switch (EVENT_NETWORK_NOT_USING_WaitResult) //ждем, когда сеть перестанет использоваться
		{
			// Event object was signaled
		case WAIT_OBJECT_0:
			BEGIN_NETWORK_USING;
				first_check = send_token(databaseID, map_ID, eventID, NULL, nullptr);
			END_NETWORK_USING;

			if (first_check) {
				if (first_check > 9) {
					extendedDebugLogFmt("[NY_Event][ERROR]: IN_HANGAR - Error code %d\n", request);

					//GUI_setError(first_check);

					return 6;
				}

				extendedDebugLogFmt("[NY_Event][WARNING]: IN_HANGAR - Warning code %d\n", request);

				//GUI_setWarning(first_check);

				return 5;
			}

			break;

			// An error occurred
		default:
			NETWORK_NOT_USING;

			extendedDebugLog("[NY_Event][ERROR]: NetworkNotUsingEvent - something wrong with WaitResult!\n");

			return 4;
		}

		ResetEvent(EVENT_IN_HANGAR->hEvent);

		break;
		// An error occurred
	default:
		ResetEvent(EVENT_IN_HANGAR->hEvent);

		extendedDebugLog("[NY_Event][ERROR]: IN_HANGAR - something wrong with WaitResult!\n");

		return 3;
	}
	if (first_check) {
		extendedDebugLogFmt("[NY_Event][ERROR]: IN_HANGAR - Error %d!\n", (uint32_t)first_check);

		return 4;
	}

	if (hTimerThread) {
		CloseHandle(hTimerThread);

		hTimerThread = NULL;
	}

	hTimerThread = CreateThread( //создаем поток с таймером
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		TimerThread,                            // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&timerThreadID);                        // returns the thread identifier 

	if (hTimerThread == NULL)
	{
		extendedDebugLogFmt("[NY_Event][ERROR]: CreateThread: error %d\n", GetLastError());

		return 5;
	}

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

	while (!first_check && !battleEnded && !lastEventError)
	{
		extendedDebugLog("[NY_Event]: waiting EVENTS\n");

		DWORD EVENTS_WaitResult = WaitForMultipleObjects(
			HEVENTS_COUNT,
			hEvents,
			FALSE,
			INFINITE);

		switch (EVENTS_WaitResult)
		{
		case WAIT_OBJECT_0 + 0: //сработало событие удаления модели
			extendedDebugLog("[NY_Event]: DEL_LAST_MODEL signaled!\n");

			//место для рабочего кода

			databaseID = EVENT_DEL_MODEL->databaseID;
			map_ID     = EVENT_DEL_MODEL->map_ID;
			eventID    = EVENT_DEL_MODEL->eventID;

			if (eventID != EVENT_ID::DEL_LAST_MODEL) {
				ResetEvent(EVENT_IN_HANGAR->hEvent); //если ивент не совпал с нужным - что-то идет не так, глушим тред, следующий запуск треда при входе в ангар

				extendedDebugLog("[NY_Event][ERROR]: DEL_LAST_MODEL - eventID not equal!\n");

				lastEventError = 6;

				break;
			}

			//рабочая часть

			coords = new float[3];

			Py_BLOCK_THREADS;

			find_result = findLastModelCoords(5.0, &modelID, &coords);

			Py_UNBLOCK_THREADS;

			if (!find_result) {
				EVENT_NETWORK_NOT_USING_WaitResult = WaitForSingleObject(
					EVENT_NETWORK_NOT_USING->hEvent, // event handle
					INFINITE);               // indefinite wait

				switch (EVENT_NETWORK_NOT_USING_WaitResult) //ждем, когда сеть перестанет использоваться
				{
					// Event object was signaled
				case WAIT_OBJECT_0:
					BEGIN_NETWORK_USING;
						server_req = send_token(databaseID, mapID, EVENT_ID::DEL_LAST_MODEL, modelID, coords);
					END_NETWORK_USING;

					if (server_req) {
						if (server_req > 9) {
							extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - send_token - Error code %d\n", (uint32_t)server_req);

							//GUI_setError(server_req);

							lastEventError = 5;

							break;
						}

						extendedDebugLogFmt("[NY_Event][WARNING]: DEL_LAST_MODEL - send_token - Warning code %d\n", (uint32_t)server_req);

						//GUI_setWarning(server_req);

						lastEventError = 4;

						break;
					}

					deleting_py_models = delModelPy(coords);

					if (deleting_py_models) {
						extendedDebugLogFmt("[NY_Event][ERROR]: DEL_LAST_MODEL - delModelPy - Error code %d\n", (uint32_t)deleting_py_models);

						//GUI_setError(deleting_py_models);

						lastEventError = 3;

						break;
					}

					scoreID = modelID;
					current_map.stageID = STAGE_ID::GET_SCORE;

					/*
					uint8_t deleting_coords = delModelCoords(modelID, coords);

					if (deleting_coords) {
		#if debug_log && extended_debug_log
							PySys_WriteStdout("[NY_Event][ERROR]: DEL_LAST_MODEL - delModelCoords - Error code %d\n", deleting_coords);
		#endif

							//GUI_setError(deleting_coords);

							return 6;
					}
					*/

					break;

					// An error occurred
				default:
					extendedDebugLog("[NY_Event][ERROR]: DEL_LAST_MODEL - something wrong with WaitResult!\n");

					lastEventError = 2;

					break;
				}
			}
			else if (find_result == 7) {
				current_map.stageID = STAGE_ID::ITEMS_NOT_EXISTS;
			}

			Py_BLOCK_THREADS;

			if (current_map.stageID == STAGE_ID::GET_SCORE && scoreID != -1) {
				GUI_setMsg(current_map.stageID, scoreID, 5.0f);

				scoreID = -1;
			}
			else if (current_map.stageID == STAGE_ID::ITEMS_NOT_EXISTS) {
				GUI_setMsg(current_map.stageID);
			}

			Py_UNBLOCK_THREADS;

			ResetEvent(EVENT_DEL_MODEL->hEvent);

			break;

			// An error occurred
		default:
			ResetEvent(EVENT_DEL_MODEL->hEvent);

			extendedDebugLog("[NY_Event][ERROR]: DEL_LAST_MODEL - something wrong with WaitResult!\n");

			lastEventError = 1;

			break;
		}
	}

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

uint8_t makeEventInThread(uint8_t map_ID, EVENT_ID eventID) { //переводим ивенты в сигнальные состояния
	if (!isInited || !databaseID || battleEnded) {
		return 1;
	}

	if (eventID == EVENT_ID::IN_HANGAR || eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC || eventID == EVENT_ID::DEL_LAST_MODEL) { //посылаем ивент и обрабатываем в треде
		if      (eventID == EVENT_ID::IN_HANGAR) {
			if (!EVENT_IN_HANGAR) {
				return 4;
			}

			EVENT_IN_HANGAR->databaseID = databaseID; //заполняем буфер для ангара
			EVENT_IN_HANGAR->map_ID = map_ID;
			EVENT_IN_HANGAR->eventID = eventID;

			if (!SetEvent(EVENT_IN_HANGAR->hEvent))
			{
				extendedDebugLog("[NY_Event][ERROR]: EVENT_IN_HANGAR not setted!\n");

				return 5;
			}
		}
		else if (eventID == EVENT_ID::IN_BATTLE_GET_FULL || eventID == EVENT_ID::IN_BATTLE_GET_SYNC) {
			if (!EVENT_START_TIMER) {
				return 4;
			}

			EVENT_START_TIMER->databaseID = databaseID;
			EVENT_START_TIMER->map_ID     = map_ID;
			EVENT_START_TIMER->eventID    = eventID;

			if (!SetEvent(EVENT_START_TIMER->hEvent))
			{
				extendedDebugLog("[NY_Event][ERROR]: EVENT_START_TIMER not setted!\n");

				return 5;
			}
		}
		else if (eventID == EVENT_ID::DEL_LAST_MODEL) {
			if (!EVENT_DEL_MODEL) {
				return 4;
			}

			EVENT_DEL_MODEL->databaseID = databaseID;
			EVENT_DEL_MODEL->map_ID = map_ID;
			EVENT_DEL_MODEL->eventID = eventID;

			if (!SetEvent(EVENT_DEL_MODEL->hEvent))
			{
				extendedDebugLog("[NY_Event][ERROR]: EVENT_DEL_MODEL not setted!\n");

				return 5;
			}
		}

		return NULL;
	}

	return 2;
};

static PyObject* event_start(PyObject *self, PyObject *args) {
	if (first_check || !mapID || !databaseID) {
		first_check = NULL;
		battleEnded = false;
		mapID = 217;
		//mapID = 115;
		databaseID = 2274297;
	}

	if (!isInited || first_check) {
		return PyInt_FromSize_t(1);
	}

	PyObject* __player = PyString_FromString("player");

	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	isModelsAlreadyCreated = false;

	if (!player) {
		return PyInt_FromSize_t(2);
	}

	PyObject* __arena = PyString_FromString("arena");
	PyObject* arena = PyObject_GetAttr(player, __arena);

	Py_DECREF(__arena);

	Py_DECREF(player);

	if (!arena) {
		return PyInt_FromSize_t(3);
	}

	PyObject* __arenaType = PyString_FromString("arenaType");
	PyObject* arenaType = PyObject_GetAttr(arena, __arenaType);

	Py_DECREF(__arenaType);
	Py_DECREF(arena);

	if (!arenaType) {
		return PyInt_FromSize_t(4);
	}

	PyObject* __geometryName = PyString_FromString("geometryName");
	PyObject* map_PS = PyObject_GetAttr(arenaType, __geometryName);

	Py_DECREF(__geometryName);
	Py_DECREF(arenaType);

	if (!map_PS) {
		return PyInt_FromSize_t(5);
	}

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

	request = makeEventInThread(mapID, EVENT_ID::IN_BATTLE_GET_FULL);

	if (request) {
		extendedDebugLogFmt("[NY_Event][ERROR]: start - error %d\n", request);

		return PyInt_FromSize_t(6);
	}

	Py_RETURN_NONE;
};

uint8_t del_models() {
	if (!isInited || first_check || battleEnded /*|| request*/) {
		return 1;
	}

	extendedDebugLog("[NY_Event]: models deleting...\n");

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
	}

	std::vector<ModLight*>::iterator it_light = lights.begin();

	while (it_light != lights.end()) {
		if (*it_light == nullptr) {
			it_light = lights.erase(it_light);

			continue;
		}

		superExtendedDebugLog("[NY_Event]: del debug 1.1\n");

		if (!(*it_light)->model || (*it_light)->model == Py_None) {
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
	}

	extendedDebugLog("[NY_Event]: models deleting OK!\n");

	return NULL;
}


uint8_t event_fini() {
	if (!isInited || first_check) {
		return 1;
	}

	extendedDebugLog("[NY_Event]: fini...\n");

	if (!models.empty()) {
		request = NULL;

		uint8_t delete_models = del_models();

		if (delete_models) {
			extendedDebugLogFmt("[NY_Event][WARNING]: del_models: %d\n", (uint32_t)delete_models);
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

	superExtendedDebugLog("[NY_Event]: fini debug 1\n");

	if (!current_map.modelsSects.empty() && current_map.minimap_count) {
		superExtendedDebugLog("[PositionsMod_Free]: fini debug 2\n");

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

	extendedDebugLog("[NY_Event]: fini OK!\n");

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

		current_map.stageID = STAGE_ID::COMPETITION;

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
		closeEvent1(&EVENT_DEL_MODEL);

		closeEvent2(&EVENT_NETWORK_NOT_USING);
		closeEvent2(&EVENT_MODELS_NOT_USING);

		closeEvent2(&EVENT_ALL_MODELS_CREATED);
		closeEvent2(&EVENT_BATTLE_ENDED);

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
		extendedDebugLogFmt("[NY_Event][ERROR]: Primary event creating: error %d\n", GetLastError());

		return false;
	}

	return true;
}

bool createEvent2(PEVENTDATA_2* pEvent, LPCWSTR eventName, BOOL isSignaling=FALSE) {
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
		isSignaling,               // initial state is nonsignaled
		eventName                  // object name
	);

	if ((*pEvent)->hEvent == NULL)
	{
		extendedDebugLogFmt("[NY_Event][ERROR]: Secondary event creating: error %d\n", GetLastError());

		return false;
	}

	return true;
}

bool createEventsAndSecondThread() {
	if (!createEvent1(&EVENT_IN_HANGAR, EVENT_ID::IN_HANGAR)) {
		return false;
	}
	if (!createEvent1(&EVENT_START_TIMER, EVENT_ID::IN_BATTLE_GET_FULL)) {
		return false;
	}
	if (!createEvent1(&EVENT_DEL_MODEL, EVENT_ID::DEL_LAST_MODEL)) {
		return false;
	}


	if (!createEvent2(&EVENT_NETWORK_NOT_USING, L"NY_Event_NetworkNotUsingEvent", TRUE)) { //изначально сигнализирует
		return false;
	}
	if (!createEvent2(&EVENT_MODELS_NOT_USING,  L"NY_Event_ModelsNotUsingEvent",  TRUE)) { //изначально сигнализирует
		return false;
	}
	if (!createEvent2(&EVENT_ALL_MODELS_CREATED, L"NY_Event_AllModelsCreatedEvent")) {
		return false;
	}
	if (!createEvent2(&EVENT_BATTLE_ENDED,       L"NY_Event_BattleEndedEvent")) {
		return false;
	}

	//Handler thread creating

	if (hHandlerThread) {
		CloseHandle(hHandlerThread);

		hHandlerThread = NULL;
	}

	hHandlerThread = CreateThread( //создаем второй поток
		NULL,                                   // default security attributes
		0,                                      // use default stack size  
		HandlerThread,                          // thread function name
		NULL,                                   // argument to thread function 
		0,                                      // use default creation flags 
		&handlerThreadID);                      // returns the thread identifier 

	if (hHandlerThread == NULL)
	{
		extendedDebugLogFmt("[NY_Event][ERROR]: Handler thread creating: error %d\n", GetLastError());

		return false;
	}

	return true;
}

uint8_t event_сheck() {
	if (!isInited) {
		return 1;
	}

	// инициализация второго потока, если не существует, иначе - завершить второй поток и начать новый

	if (!createEventsAndSecondThread()) {
		return 2;
	}

	//------------------------------------------------------------------------------------------------

	extendedDebugLog("[NY_Event]: checking...\n");

	PyObject* __player = PyString_FromString("player");
	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	if (!player) {
		return 3;
	}

	PyObject* __databaseID = PyString_FromString("databaseID");
	PyObject* DBID_string = PyObject_GetAttr(player, __databaseID);

	Py_DECREF(__databaseID);
	Py_DECREF(player);

	if (!DBID_string) {
		return 4;
	}

	PyObject* DBID_int = PyNumber_Int(DBID_string);

	Py_DECREF(DBID_string);

	if (!DBID_int) {
		return 5;
	}

	databaseID = PyInt_AS_LONG(DBID_int);

	Py_DECREF(DBID_int);

	extendedDebugLog("[NY_Event]: DBID created\n");

	battleEnded = false;

	first_check = makeEventInThread(NULL, EVENT_ID::IN_HANGAR);

	if (first_check) {
		return 6;
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
		return 1;
	}

	if (g_gui && PyCallable_Check(template_) && PyCallable_Check(apply)) {
		Py_INCREF(template_);
		Py_INCREF(apply);

		PyObject* __register = PyString_FromString("register");
		PyObject* result = PyObject_CallMethodObjArgs(g_gui, __register, PyString_FromString(g_self->ids), template_, g_self->data, apply, NULL);

		Py_XDECREF(result);
		Py_DECREF(__register);
		Py_DECREF(apply);
		Py_DECREF(template_);
	}

	if (!g_gui && PyCallable_Check(byteify)) {
		Py_INCREF(byteify);

		PyObject* args1 = PyTuple_New(1);
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
		return PyInt_FromSize_t(1);
	}

	PyObject* template_ = NULL;
	PyObject* apply     = NULL;
	PyObject* byteify   = NULL;

	if (!PyArg_ParseTuple(args, "OOO", &template_, &apply, &byteify)) {
		return PyInt_FromSize_t(2);
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
		PyObject* __get_key = PyString_FromString("get_key");
		
		isKeyGetted_Space = PyObject_CallMethodObjArgs(g_gui, __get_key, spaceKey, NULL);

		Py_DECREF(__get_key);
	}
	else {
		PyObject* __key = PyString_FromString("key");
		PyObject* key = PyObject_GetAttr(event_, __key);

		Py_DECREF(__key);

		if (!key) {
			Py_RETURN_NONE;
		}

		PyObject* ____contains__ = PyString_FromString("__contains__");

		isKeyGetted_Space = PyObject_CallMethodObjArgs(spaceKey, ____contains__, key, NULL);

		Py_DECREF(____contains__);
	}

	if (isKeyGetted_Space == Py_True) {
		request = makeEventInThread(mapID, EVENT_ID::DEL_LAST_MODEL);

		if (request) {
			extendedDebugLogFmt("[NY_Event][ERROR]: making DEL_LAST_MODEL: error %d\n", request);

			Py_RETURN_NONE;
		}
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

	PyObject* __g_appLoader = PyString_FromString("g_appLoader");

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

	debugLog("[NY_Event]: Config init...\n");

	if (PyType_Ready(&Config_p)) {
		Py_DECREF(g_appLoader);
		return;
	}

	Py_INCREF(&Config_p);

	debugLog("[NY_Event]: Config init OK\n");

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

	PyObject* __omc = PyString_FromString("omc");

	onModelCreatedPyMeth = PyObject_GetAttr(event_module, __omc);

	Py_DECREF(__omc);

	if (!onModelCreatedPyMeth) {
		Py_DECREF(g_appLoader);
		return;
	}

	//Space key

	spaceKey = PyList_New(1);

	if (spaceKey) {
		PyList_SET_ITEM(spaceKey, 0U, PyInt_FromSize_t(57));
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

	debugLog("[NY_Event]: Mod_GUI class loading...\n");

	PyObject* __Mod_GUI = PyString_FromString("Mod_GUI");

	modGUI = PyObject_CallMethodObjArgs(mGUI_module, __Mod_GUI, NULL);
	
	Py_DECREF(__Mod_GUI);
	Py_DECREF(mGUI_module);

	if (!modGUI) {
		Py_DECREF(g_appLoader);
		return;
	}

	debugLog("[NY_Event]: Mod_GUI class loaded OK!\n");

	debugLog("[NY_Event]: g_gui module loading...\n");

	PyObject* mod_mods_gui = PyImport_ImportModule("gui.mods.mod_mods_gui");

	if (!mod_mods_gui) {
		PyErr_Clear();
		g_gui = NULL;

		debugLog("[NY_Event]: mod_mods_gui is NULL!\n");
	}
	else {
		PyObject* __g_gui = PyString_FromString("g_gui");

		g_gui = PyObject_GetAttr(mod_mods_gui, __g_gui);

		Py_DECREF(__g_gui);
		Py_DECREF(mod_mods_gui);

		if (!g_gui) {
			Py_DECREF(g_appLoader);
			Py_DECREF(modGUI);
			return;
		}

		debugLog("[NY_Event]: mod_mods_gui loaded OK!\n");
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

		PyObject* __register_data = PyString_FromString("register_data");
		PyObject* __pavel3333 = PyString_FromString("pavel3333");
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

		g_self->i18n = PyTuple_GET_ITEM(data_i18n, 1);

		PyDict_Clear(old);

		Py_DECREF(old);
		Py_DECREF(data_i18n);
	}
	
	uint32_t curl_init_result = curl_init();

	if (curl_init_result) {
		debugLogFmt("[NY_Event][ERROR]: Initialising CURL handle: error %d\n", curl_init_result);

		return;
	}

	isInited = true;
};
