#include "Py_API.h"

PyObject* event_module = NULL;

PyObject* spaceKey = NULL;

uint16_t allModelsCreated = NULL;

PyObject* onModelCreatedPyMeth = NULL;

uint8_t  first_check = 100;
uint32_t request = 100;

uint8_t  mapID = NULL;
uint32_t databaseID = NULL;

bool isInited = false;

bool battleEnded = true;

bool isModelsAlreadyCreated = false;
bool isModelsAlreadyInited = false;

bool isTimerStarted = false;
bool isTimeVisible = false;

bool isStreamer = false;

HANDLE hTimer = NULL;

HANDLE hTimerThread = NULL;
DWORD  timerThreadID = NULL;

HANDLE hHandlerThread = NULL;
DWORD  handlerThreadID = NULL;

uint8_t timerLastError = NULL;

STAGE_ID lastStageID = STAGE_ID::COMPETITION;
EVENT_ID lastEventID = EVENT_ID::IN_HANGAR;

std::vector<ModModel*> models;
//std::vector<ModLight*> lights;

bool write_data(char* data_path, PyObject* data_p) {
	traceLog();

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
		traceLog();
		return false;
	} traceLog();

	size_t data_size = PyObject_Length(data_json_s);

	std::ofstream data_w(data_path);

	data_w.write(PyString_AS_STRING(data_json_s), data_size);

	data_w.close();

	Py_DECREF(data_json_s);

	return true;
}

bool read_data(bool isData) {
	traceLog();
	char* data_path;
	PyObject* data_src;
	if (isData) {
		traceLog();
		data_path = "mods/configs/pavel3333/NY_Event/NY_Event.json";
		data_src = g_self->data;
	}
	else {
		data_src = g_self->i18n;
		data_path = "mods/configs/pavel3333/NY_Event/i18n/ru.json";
	} traceLog();

	std::ifstream data(data_path, std::ios::binary);

	if (!data.is_open()) {
		traceLog();
		data.close();
		if (!write_data(data_path, data_src)) {
			traceLog();
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

		if (!data_json_s) {
			traceLog();
			PyErr_Clear();

			if (!write_data(data_path, data_src)) {
				traceLog();
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
					(*it_model)[counter] = NULL;
				}

				delete[] * it_model;
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
				if (*it_model != nullptr) {
					traceLog();
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

uint8_t findLastModelCoords(float dist_equal, uint8_t* modelID, float** coords) {
	traceLog();
	PyObject* __player = PyString_FromString("player");

	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	if (!player) {
		traceLog();
		return 1;
	} traceLog();

	PyObject* __vehicle = PyString_FromString("vehicle");
	PyObject* vehicle = PyObject_GetAttr(player, __vehicle);

	Py_DECREF(__vehicle);

	Py_DECREF(player);

	if (!vehicle) {
		traceLog();
		return 2;
	} traceLog();

	PyObject* __model = PyString_FromString("model");
	PyObject* model_p = PyObject_GetAttr(vehicle, __model);

	Py_DECREF(__model);
	Py_DECREF(vehicle);

	if (!model_p) {
		traceLog();
		return 3;
	} traceLog();

	PyObject* __position = PyString_FromString("position");
	PyObject* position_Vec3 = PyObject_GetAttr(model_p, __position);

	Py_DECREF(__position);
	Py_DECREF(model_p);

	if (!position_Vec3) {
		traceLog();
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

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog();
			superExtendedDebugLog("[NY_Event]: MODELS_USING\n");

			for (auto it = current_map.modelsSects.cbegin();
				it != current_map.modelsSects.cend();
				it++) {
				if (it->isInitialised) {
					for (auto it2 = it->models.cbegin();
						it2 != it->models.cend();
						it2++) {
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

			//освобождаем мутекс для этого потока

			if (!ReleaseMutex(M_MODELS_NOT_USING)) {
				traceLog();

				INIT_LOCAL_MSG_BUFFER;

				extendedDebugLogFmt("[NY_Event][ERROR]: findLastModelCoords - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

				return 14;
			}

			superExtendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog();
			extendedDebugLog("[NY_Event][ERROR]: findLastModelCoords - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			return 13;
	END_USING_MODELS;

	delete[] coords_pos;

	if (dist == -1.0 || modelTypeLast == -1 || coords_res == nullptr) {
		traceLog();
		return 8;
	} traceLog();

	if (dist > dist_equal) {
		traceLog();
		return 7;
	} traceLog();

	*modelID = modelTypeLast;
	*coords = coords_res;

	return NULL;
}

uint8_t delModelPy(float* coords) {
	traceLog();
	if (coords == nullptr) {
		traceLog();
		return 1;
	} traceLog();

	std::vector<ModModel*>::iterator it_model = models.begin();

	while (it_model != models.end()) {
		if (*it_model == nullptr) {
			traceLog();
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

			if (!PyObject_SetAttr((*it_model)->model, __visible, py_visible)) {
				traceLog();
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

uint8_t delModelCoords(uint8_t ID, float* coords) { traceLog();
	if (coords == nullptr) { traceLog();
		return 1;
	} traceLog();

	std::vector<ModelsSection>::iterator it_sect = current_map.modelsSects.begin();

	try {
		while (it_sect != current_map.modelsSects.end()) {
			if (!it_sect->models.empty() && it_sect->isInitialised && it_sect->ID == ID) { traceLog();
				std::vector<float*>::iterator it_model = it_sect->models.begin(); traceLog();

				while (it_model != it_sect->models.end()) { traceLog();
					if ((*it_model) == nullptr) { traceLog();
						it_model = it_sect->models.erase(it_model); traceLog();

						continue;
					} traceLog();
					traceLog();
					if ((*it_model)[0] == coords[0] &&
						(*it_model)[1] == coords[1] &&
						(*it_model)[2] == coords[2]
						) { traceLog();
						for (uint8_t counter = NULL; counter < 3; counter++) { traceLog();
							(*it_model)[counter] = NULL; traceLog();
						}

						delete[] *it_model;   traceLog();
						*it_model = nullptr;  traceLog();

						it_model = it_sect->models.erase(it_model);  traceLog(); //TODO: fix crash

						return NULL;
					} traceLog();

					it_model++;
				}
			}

			it_sect++;
		} traceLog();
	}
	catch (const char* msg) {
		dbg_log << msg << std::endl;

		return 3;
	}

	return 2;
}

PyObject* event_light(float coords[3]) { //traceLog();
	if (!isInited || battleEnded) {
		traceLog();
		return NULL;
	} //traceLog();

	superExtendedDebugLog("light creating...\n");

	PyObject* __PyOmniLight = PyString_FromString("PyOmniLight");

	PyObject* Light = PyObject_CallMethodObjArgs(BigWorld, __PyOmniLight, NULL);

	Py_DECREF(__PyOmniLight);

	if (!Light) {
		traceLog();
		superExtendedDebugLog("PyOmniLight creating FAILED\n");

		return NULL;
	} //traceLog();

	//---------inner radius---------

	PyObject* __innerRadius = PyString_FromString("innerRadius");
	PyObject* innerRadius = PyFloat_FromDouble(0.75);

	if (PyObject_SetAttr(Light, __innerRadius, innerRadius)) {
		traceLog();
		superExtendedDebugLog("PyOmniLight innerRadius setting FAILED\n");

		Py_DECREF(__innerRadius);
		Py_DECREF(innerRadius);
		Py_DECREF(Light);

		return NULL;
	} //traceLog();

	Py_DECREF(__innerRadius);

	//---------outer radius---------

	PyObject* __outerRadius = PyString_FromString("outerRadius");
	PyObject* outerRadius = PyFloat_FromDouble(1.5);

	if (PyObject_SetAttr(Light, __outerRadius, outerRadius)) {
		traceLog();
		superExtendedDebugLog("PyOmniLight outerRadius setting FAILED\n");

		Py_DECREF(outerRadius);
		Py_DECREF(__outerRadius);
		Py_DECREF(Light);

		return NULL;
	} //traceLog();

	Py_DECREF(__outerRadius);

	//----------multiplier----------

	PyObject* __multiplier = PyString_FromString("multiplier");
	PyObject* multiplier = PyFloat_FromDouble(500.0);

	if (PyObject_SetAttr(Light, __multiplier, multiplier)) {
		traceLog();
		superExtendedDebugLog("PyOmniLight multiplier setting FAILED\n");

		Py_DECREF(multiplier);
		Py_DECREF(__multiplier);
		Py_DECREF(Light);

		return NULL;
	} //traceLog();

	Py_DECREF(__multiplier);

	//-----------position-----------

	PyObject* coords_p = PyTuple_New(3);

	if (!coords_p) {
		traceLog();
		superExtendedDebugLog("PyOmniLight coords creating FAILED\n");

		Py_DECREF(Light);

		return NULL;
	} //traceLog();

	for (uint8_t i = NULL; i < 3; i++) {
		if (i == 1) {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i] + 0.5));
		}
		else {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i]));
		}
	} //traceLog();

	PyObject* __position = PyString_FromString("position");

	if (PyObject_SetAttr(Light, __position, coords_p)) {
		traceLog();
		superExtendedDebugLog("PyOmniLight coords setting FAILED\n");

		Py_DECREF(__position);
		Py_DECREF(coords_p);
		Py_DECREF(Light);

		return NULL;
	} //traceLog();

	Py_DECREF(__position);

	//------------colour------------

	PyObject* colour_p = PyTuple_New(4);

	if (!colour_p) {
		traceLog();
		superExtendedDebugLog("PyOmniLight colour creating FAILED\n");

		Py_DECREF(Light);

		return NULL;
	} //traceLog();

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
		traceLog();
		superExtendedDebugLog("PyOmniLight colour setting FAILED\n");

		Py_DECREF(__colour);
		Py_DECREF(colour_p);
		Py_DECREF(Light);

		return NULL;
	} //traceLog();

	Py_DECREF(__colour);

	superExtendedDebugLog("light creating OK!\n");

	return Light;
}

bool setModelPosition(PyObject* Model, float* coords_f) { //traceLog();
	PyObject* coords_p = PyTuple_New(3);

	for (uint8_t i = NULL; i < 3; i++) PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords_f[i]));

	PyObject* __position = PyString_FromString("position");

	if (PyObject_SetAttr(Model, __position, coords_p)) {
		traceLog();
		Py_DECREF(__position);
		Py_DECREF(coords_p);
		Py_DECREF(Model);

		return false;
	} //traceLog();

	Py_DECREF(__position);

	return true;
}

PyObject* event_model(char* path, float coords[3], bool isAsync) { //traceLog();
	if (!isInited || battleEnded) {
		traceLog();
		if (isAsync && allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

		return NULL;
	} //traceLog();

	superExtendedDebugLog("model creating...\n");

	PyObject* Model = NULL;

	if (isAsync) { //traceLog();
		if (coords == nullptr) {
			traceLog();
			if (allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return NULL;
		} //traceLog();

		PyObject* __partial = PyString_FromString("partial");

		PyObject* coords_p = PyLong_FromVoidPtr((void*)coords); //передаем указатель на 3 координаты

		PyObject* partialized = PyObject_CallMethodObjArgs(functools, __partial, onModelCreatedPyMeth, coords_p, NULL);

		Py_DECREF(__partial);

		if (!partialized) {
			traceLog();
			if (allModelsCreated > NULL) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return NULL;
		} //traceLog();

		PyObject* __fetchModel = PyString_FromString("fetchModel");

		Model = PyObject_CallMethodObjArgs(BigWorld, __fetchModel, PyString_FromString(path), partialized, NULL); //запускаем асинхронное добавление модели

		Py_XDECREF(Model);
		Py_DECREF(__fetchModel);

		return NULL;
	} //traceLog();

	PyObject* __Model = PyString_FromString("Model");

	Model = PyObject_CallMethodObjArgs(BigWorld, __Model, PyString_FromString(path), NULL);

	Py_DECREF(__Model);

	if (!Model) {
		traceLog();
		return NULL;
	} //traceLog();

	if (coords != nullptr) {
		traceLog();
		if (!setModelPosition(Model, coords)) {
			traceLog(); //ставим на нужную позицию
			return NULL;
		}
	} //traceLog();

	superExtendedDebugLog("model creating OK!\n");

	return Model;
};

PyObject* event_onModelCreated(PyObject *self, PyObject *args) {
	traceLog(); //принимает аргументы: указатель на координаты и саму модель
	if (!isInited || battleEnded || models.size() >= allModelsCreated) {
		traceLog();
		Py_RETURN_NONE;
	} traceLog();

	if (!EVENT_ALL_MODELS_CREATED->hEvent) {
		traceLog();
		extendedDebugLog("[NY_Event][ERROR]: AMCEvent or createModelsPyMeth event is NULL!\n");

		Py_RETURN_NONE;
	} traceLog();

	//рабочая часть

	PyObject* coords_pointer = NULL;
	PyObject* Model = NULL;

	if (!PyArg_ParseTuple(args, "OO", &coords_pointer, &Model)) {
		traceLog();
		Py_RETURN_NONE;
	} traceLog();

	if (!Model || Model == Py_None) {
		traceLog();
		Py_XDECREF(Model);

		Py_RETURN_NONE;
	} traceLog();

	if (!coords_pointer) {
		traceLog();
		Py_DECREF(Model);

		Py_RETURN_NONE;
	} traceLog();

	void* coords_vptr = PyLong_AsVoidPtr(coords_pointer);

	if (!coords_vptr) {
		traceLog();
		Py_DECREF(coords_pointer);
		Py_DECREF(Model);

		Py_RETURN_NONE;
	} traceLog();

	float* coords_f = (float*)(coords_vptr);

	if (!setModelPosition(Model, coords_f)) {
		traceLog(); //ставим на нужную позицию
		Py_RETURN_NONE;
	} traceLog();

	Py_INCREF(Model);

	ModModel* newModel = new ModModel{
		false,
		Model,
		coords_f
	};

	/*ModLight* newLight = new ModLight {
		event_light(coords_f),
		coords_f
	};*/

	models.push_back(newModel);
	//lights.push_back(newLight);

	if (models.size() >= allModelsCreated) {
		traceLog();               //если число созданных моделей - столько же или больше, чем надо
		if (!SetEvent(EVENT_ALL_MODELS_CREATED->hEvent)) {
			traceLog(); //сигналим о том, что все модели были успешно созданы
			extendedDebugLog("[NY_Event][ERROR]: event_onModelCreated: AMCEvent event not setted!\n");

			Py_RETURN_NONE;
		} traceLog();

		allModelsCreated = NULL;
	} traceLog();

	Py_RETURN_NONE;
}

uint8_t create_models() {
	traceLog();
	if (!isInited || battleEnded || !onModelCreatedPyMeth || !M_MODELS_NOT_USING) {
		traceLog();
		return 1;
	} traceLog();

	INIT_LOCAL_MSG_BUFFER;

	BEGIN_USING_MODELS;
		case WAIT_OBJECT_0: traceLog();
			superExtendedDebugLog("[NY_Event]: MODELS_USING\n");

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

			//освобождаем мутекс для этого потока

			if (!ReleaseMutex(M_MODELS_NOT_USING)) {
				traceLog();
				extendedDebugLogFmt("[NY_Event][ERROR]: create_models - MODELS_NOT_USING - ReleaseMutex: error %d!\n", GetLastError());

				return 4;
			}

			superExtendedDebugLog("[NY_Event]: MODELS_NOT_USING\n");

			break;
		case WAIT_ABANDONED: traceLog();
			extendedDebugLog("[NY_Event][ERROR]: create_models - MODELS_NOT_USING: WAIT_ABANDONED!\n");

			return 3;
			END_USING_MODELS;

			return NULL;
}

uint8_t init_models() {
	traceLog();
	if (!isInited || first_check || battleEnded || models.empty()) {
		traceLog();
		return 1;
	} traceLog();

	extendedDebugLog("[NY_Event]: models adding...\n");

	for (uint16_t i = NULL; i < models.size(); i++) {
		if (models[i] == nullptr) {
			traceLog();
			continue;
		}

		if (models[i]->model == Py_None || !models[i]->model || models[i]->processed) {
			traceLog();
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

uint8_t set_visible(bool isVisible) {
	traceLog();
	if (!isInited || first_check || battleEnded || models.empty()) {
		traceLog();
		return 1;
	} traceLog();

	PyObject* py_visible = PyBool_FromLong(isVisible);

	extendedDebugLog("[NY_Event]: Models visiblity changing...\n");

	for (uint16_t i = NULL; i < models.size(); i++) {
		if (models[i] == nullptr) {
			traceLog();
			continue;
		}

		if (!models[i]->model || models[i]->model == Py_None || !models[i]->processed) {
			traceLog();
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

	/*std::vector<ModLight*>::iterator it_light = lights.begin();

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
	} traceLog();*/

	extendedDebugLog("[NY_Event]: models deleting OK!\n");

	return NULL;
};

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
