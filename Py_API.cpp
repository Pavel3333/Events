#include "pch.h"
#include "Py_API.h"
#include "PyLoader.h"
#include "MyLogger.h"


INIT_LOCAL_MSG_BUFFER;


uint16_t allModelsCreated = 0;

uint8_t  first_check = 100;
uint32_t request     = 100;

uint8_t  mapID      = 0;
uint32_t databaseID = 0;

bool isInited = false;

bool battleEnded = true;

bool isModelsAlreadyCreated = false;
bool isModelsAlreadyInited  = false;

bool isTimerStarted = false;
bool isTimeVisible  = false;

bool isStreamer = false;

STAGE_ID lastStageID = STAGE_ID::COMPETITION;
EVENT_ID lastEventID = EVENT_ID::IN_HANGAR;

std::vector<ModModel*> models;
//std::vector<ModLight*> lights;

void clearModelsSections() { traceLog
	std::vector<ModelsFullSection>::iterator it_sect = current_map.modelsSects.begin();

	while (it_sect != current_map.modelsSects.end()) {
		if (!it_sect->models.empty() && it_sect->isInitialised) {
			std::vector<float*>::iterator it_model = it_sect->models.begin();

			while (it_model != it_sect->models.end()) {
				if (*it_model == nullptr) { traceLog
					extendedDebugLogEx(WARNING, "clearModelsSections - *it_model is NULL!");

					it_model = it_sect->models.erase(it_model);

					continue;
				}

				for (uint8_t counter = 0; counter < 3; counter++) {
					(*it_model)[counter] = 0.0f;
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
	} traceLog

	current_map.modelsSects.~vector();

	std::vector<ModelsSyncSection>::iterator it_sect_sync = sync_map.modelsSects_deleting.begin();

	while (it_sect_sync != sync_map.modelsSects_deleting.end()) {
		if (it_sect_sync->isInitialised) {
			std::vector<ModelSync>::iterator it_model = it_sect_sync->models.begin();

			while (it_model != it_sect_sync->models.end()) {
				if (it_model->coords != nullptr) { traceLog
					delete[] it_model->coords;
					it_model->coords = nullptr;
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
	} traceLog

	sync_map.modelsSects_deleting.~vector();
}

MyErr delModelPy(float* coords) { traceLog
	INIT_LOCAL_MSG_BUFFER;
	
	if (coords == nullptr) { traceLog
		extendedDebugLogEx(WARNING, "delModelPy - coords is NULL!");

		return_err 1;
	} traceLog

	std::vector<ModModel*>::iterator it_model = models.begin();

	while (it_model != models.end()) {
		if (*it_model == nullptr) { traceLog
			extendedDebugLogEx(WARNING, "delModelPy - *it_model is NULL!");

			it_model = models.erase(it_model);
			
			continue;
		}

		if (!(*it_model)->model || (*it_model)->model == Py_None || !(*it_model)->processed) {
			superExtendedDebugLog("SYNC: PyModel is NULL or None or processed is None!");

			Py_XDECREF((*it_model)->model);

			(*it_model)->model  = nullptr;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			continue;
		}

		if ((*it_model)->coords[0] == coords[0] &&
			(*it_model)->coords[1] == coords[1] &&
			(*it_model)->coords[2] == coords[2]) {
			superExtendedDebugLog("SYNC: model found!");

			PyObject* py_visible = PyBool_FromLong(false);

			if (!PyObject_SetAttrString((*it_model)->model, "visible", py_visible)) { traceLog
				Py_DECREF(py_visible);

				return_ok;
			}

			Py_DECREF(py_visible);

			return_err 2;

			/*PyObject* __delModel = PyString_FromString("delModel");

			PyObject_CallMethodObjArgs_increfed(result, BW_Native->m_BigWorld, __delModel, (*it_model)->model, nullptr);

			Py_DECREF(__delModel);

			if (result) {
				Py_DECREF(result);

				Py_XDECREF((*it_model)->model);

				(*it_model)->model  = nullptr;
				(*it_model)->coords = nullptr;
				(*it_model)->processed = false;

				delete *it_model;
				*it_model = nullptr;

				it_model = models.erase(it_model);

				return_ok;
			}*/
		}

		it_model++;
	} traceLog

	return_err 3;
}

uint8_t delModelCoords(MODEL_ID ID, float* coords)
{
	INIT_LOCAL_MSG_BUFFER;

	traceLog
	if (coords == nullptr) { traceLog
		extendedDebugLogEx(WARNING, "delModelCoords - coords is NULL!");

		return 1;
	}

	std::vector<ModelsFullSection>::iterator it_sect = current_map.modelsSects.begin();

	try {
		while (it_sect != current_map.modelsSects.end()) {
			if (!it_sect->models.empty() && it_sect->isInitialised && it_sect->ID == ID) {
				std::vector<float*>::iterator it_model = it_sect->models.begin();

				while (it_model != it_sect->models.end()) {
					if ((*it_model) == nullptr) { traceLog
						extendedDebugLogEx(WARNING, "delModelCoords - *it_model is NULL!");

						it_model = it_sect->models.erase(it_model); traceLog

						continue;
					}

					if ((*it_model)[0] == coords[0] &&
						(*it_model)[1] == coords[1] &&
						(*it_model)[2] == coords[2]
						) { traceLog
						for (uint8_t counter = 0; counter < 3; counter++) {
							(*it_model)[counter] = 0.0f;
						}

						delete[] *it_model;
						*it_model = nullptr;

						it_model = it_sect->models.erase(it_model);

						return 0;
					}

					it_model++;
				}
			}

			it_sect++;
		}
	}
	catch (const char* msg) {
		debugLog("%s", msg);

		return 3;
	}

	return 2;
}

PyObject* event_light(float coords[3]) {
	if (!isInited || battleEnded) { traceLog
		return nullptr;
	}

	superExtendedDebugLog("light creating...");

	PyObject* Light = PyObject_CallMethod(BigWorldUtils::m_BigWorld, "PyOmniLight", nullptr);
	if (!Light) { traceLog
		superExtendedDebugLog("PyOmniLight creating FAILED");
		return nullptr;
	}

	//---------inner radius---------

	PyObject* innerRadius = PyFloat_FromDouble(0.75);

	if (PyObject_SetAttrString(Light, "innerRadius", innerRadius)) { traceLog
		superExtendedDebugLog("PyOmniLight innerRadius setting FAILED");

		Py_DECREF(innerRadius);
		Py_DECREF(Light);

		return nullptr;
	}

	//---------outer radius---------

	PyObject* outerRadius = PyFloat_FromDouble(1.5);

	if (PyObject_SetAttrString(Light, "outerRadius", outerRadius)) { traceLog
		superExtendedDebugLog("PyOmniLight outerRadius setting FAILED");

		Py_DECREF(outerRadius);
		Py_DECREF(Light);

		return nullptr;
	}

	//----------multiplier----------

	PyObject* multiplier = PyFloat_FromDouble(500.0);

	if (PyObject_SetAttrString(Light, "multiplier", multiplier)) { traceLog
		superExtendedDebugLog("PyOmniLight multiplier setting FAILED");

		Py_DECREF(multiplier);
		Py_DECREF(Light);

		return nullptr;
	}

	//-----------position-----------

	PyObject* coords_p = PyTuple_New(3);

	if (!coords_p) { traceLog
		superExtendedDebugLog("PyOmniLight coords creating FAILED");

		Py_DECREF(Light);

		return nullptr;
	}

	for (uint8_t i = 0; i < 3; i++) {
		if (i == 1) {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i] + 0.5));
		}
		else {
			PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i]));
		}
	}

	if (PyObject_SetAttrString(Light, "position", coords_p)) { traceLog
		superExtendedDebugLog("PyOmniLight coords setting FAILED");

		Py_DECREF(coords_p);
		Py_DECREF(Light);

		return nullptr;
	}

	//------------colour------------

	PyObject* colour_p = PyTuple_New(4);

	if (!colour_p) { traceLog
		superExtendedDebugLog("PyOmniLight colour creating FAILED");

		Py_DECREF(Light);

		return nullptr;
	}

	double* colour = new double[5];

	colour[0] = 255.0;
	colour[1] = 255.0;
	colour[2] = 255.0;
	colour[3] = 0.0;

	for (uint8_t i = 0; i < 4; i++) PyTuple_SET_ITEM(colour_p, i, PyFloat_FromDouble(colour[i]));

	delete[] colour;

	//------------------------------

	if (PyObject_SetAttrString(Light, "colour", colour_p)) { traceLog
		superExtendedDebugLog("PyOmniLight colour setting FAILED");

		Py_DECREF(colour_p);
		Py_DECREF(Light);

		return nullptr;
	}

	superExtendedDebugLog("light creating OK!");

	return Light;
}

bool setModelPosition(PyObject* Model, const float coords_f[3])
{
	PyObject* coords_p = PyTuple_New(3);

	for (uint8_t i = 0; i < 3; i++) PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords_f[i]));

	if (PyObject_SetAttrString(Model, "position", coords_p)) { traceLog
		Py_DECREF(coords_p);
		Py_DECREF(Model);

		return false;
	}

	return true;
}

PyObject* event_model(char* path, float coords[3], bool isAsync)
{
	if (!isInited || battleEnded) { traceLog
		if (isAsync && allModelsCreated > 0) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

		return nullptr;
	}

	superExtendedDebugLog("model creating...");

	if (isAsync) { //traceLog
		if (coords == nullptr) { traceLog
			extendedDebugLogEx(WARNING, "event_model - coords is NULL!");

			if (allModelsCreated > 0) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return nullptr;
		}

		PyObject* coords_p = PyLong_FromVoidPtr((void*)coords); //передаем указатель на 3 координаты

		PyObject_CallFunctionObjArgs_increfed(partialized, BigWorldUtils::m_partial, PyLoader::onModelCreatedPyMeth, coords_p, nullptr);

		if (!partialized) { traceLog
			if (allModelsCreated > 0) allModelsCreated--; //создать модель невозможно, убавляем счетчик числа моделей, которые должны быть созданы

			return nullptr;
		}

		PyObject_CallFunctionObjArgs_increfed(Model, BigWorldUtils::m_fetchModel, PyString_FromString(path), partialized, nullptr); //запускаем асинхронное добавление модели

		Py_XDECREF(Model);

		return nullptr;
	}

	PyObject_CallFunctionObjArgs_increfed(Model, BigWorldUtils::m_Model, PyString_FromString(path), nullptr);

	if (!Model) { traceLog
		return nullptr;
	}

	if (coords != nullptr) { traceLog
		if (!setModelPosition(Model, coords)) { traceLog //ставим на нужную позицию
			return nullptr;
		}
	}

	superExtendedDebugLog("model creating OK!");

	return Model;
}

PyObject* event_onModelCreated(PyObject *self, PyObject *args) { traceLog //принимает аргументы: указатель на координаты и саму модель
	if (!isInited || battleEnded || models.size() >= allModelsCreated) { traceLog
		Py_RETURN_NONE;
	}

	UNREFERENCED_PARAMETER(self);
	UNREFERENCED_PARAMETER(args);

	if (!EVENT_ALL_MODELS_CREATED->hEvent) { traceLog
		extendedDebugLogEx(ERROR, "AMCEvent or createModelsPyMeth event is NULL!");

		Py_RETURN_NONE;
	}

	//рабочая часть

	PyObject* coords_pointer = nullptr;
	PyObject* Model          = nullptr;

	if (!PyArg_ParseTuple(args, "OO", &coords_pointer, &Model)) { traceLog
		Py_RETURN_NONE;
	}

	if (!Model || Model == Py_None) { traceLog
		Py_XDECREF(Model);

		Py_RETURN_NONE;
	}

	if (!coords_pointer) { traceLog
		Py_DECREF(Model);

		Py_RETURN_NONE;
	}

	void* coords_vptr = PyLong_AsVoidPtr(coords_pointer);

	if (!coords_vptr) { traceLog
		Py_DECREF(coords_pointer);
		Py_DECREF(Model);

		Py_RETURN_NONE;
	}

	float* coords_f = (float*)(coords_vptr);

	if (!setModelPosition(Model, coords_f)) { traceLog //ставим на нужную позицию
		Py_RETURN_NONE;
	}

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

	if (models.size() >= allModelsCreated) { traceLog               //если число созданных моделей - столько же или больше, чем надо
		if (!SetEvent(EVENT_ALL_MODELS_CREATED->hEvent)) { traceLog //сигналим о том, что все модели были успешно созданы
			extendedDebugLogEx(ERROR, "event_onModelCreated: AMCEvent event not setted!");

			Py_RETURN_NONE;
		} traceLog

		allModelsCreated = 0;
	} traceLog

	Py_RETURN_NONE;
}

uint8_t create_models() { traceLog
	if (!isInited || battleEnded || !PyLoader::onModelCreatedPyMeth) { traceLog
		return 1;
	} traceLog

	INIT_LOCAL_MSG_BUFFER;

	g_models_mutex.lock();
	superExtendedDebugLog("MODELS_USING");

	for (auto it = current_map.modelsSects.begin(); //первый проход - получаем число всех созданных моделей
		it != current_map.modelsSects.end();
		it++) {

		if (!it->isInitialised || it->models.empty()) { traceLog
			continue;
		}

		auto it2 = it->models.begin();

		while (it2 != it->models.end()) {
			if (*it2 == nullptr) { traceLog
				extendedDebugLogEx(WARNING, "create_models - *it2 is NULL!");

				it2 = it->models.erase(it2);

				continue;
			}

			allModelsCreated++;

			it2++;
		}
	} traceLog

	for (auto it = current_map.modelsSects.cbegin(); //второй проход - создаем модели
		it != current_map.modelsSects.cend();
		it++) {
		if (!it->isInitialised || it->models.empty()) { traceLog
			continue;
		}

		for (float* it2 : it->models) {
			superExtendedDebugLog("[");

			event_model(it->path, it2, true);

			superExtendedDebugLog("], ");
		}
	} traceLog

	superExtendedDebugLog("], ");

	superExtendedDebugLog("MODELS_NOT_USING");
	g_models_mutex.unlock();

	return 0;
}

uint8_t init_models()
{
	INIT_LOCAL_MSG_BUFFER;
	
	traceLog
	if (!isInited || first_check || battleEnded || models.empty()) { traceLog
		return 10;
	} traceLog

	extendedDebugLog("models adding...");

	for (uint16_t i = 0; i < models.size(); i++) {
		if (models[i] == nullptr) { traceLog
			extendedDebugLogEx(WARNING, "init_models - models[i] is NULL!");

			continue;
		}

		if (models[i]->model == Py_None || !models[i]->model || models[i]->processed) { traceLog
			Py_XDECREF(models[i]->model);

			models[i]->model  = nullptr;
			models[i]->coords = nullptr;
			models[i]->processed = false;

			delete models[i];
			models[i] = nullptr;

			continue;
		}

		PyObject_CallFunctionObjArgs_increfed(result, BigWorldUtils::m_addModel, models[i]->model, nullptr);

		if (result) {
			Py_DECREF(result);

			models[i]->processed = true;

			superExtendedDebugLog("True");
		}
		else superExtendedDebugLog("False");
	} traceLog

	extendedDebugLog("models adding OK!");

	return 0;
}

uint8_t set_visible(bool isVisible) {
	INIT_LOCAL_MSG_BUFFER;

	traceLog
	if (!isInited || first_check || battleEnded || models.empty()) { traceLog
		return 10;
	} traceLog

	PyObject* py_visible = PyBool_FromLong(isVisible);

	extendedDebugLog("Models visiblity changing...");

	for (uint16_t i = 0; i < models.size(); i++) {
		if (models[i] == nullptr) { traceLog
			extendedDebugLogEx(WARNING, "set_visible - models[i] is NULL!");

			continue;
		}

		if (!models[i]->model || models[i]->model == Py_None || !models[i]->processed) { traceLog
			Py_XDECREF(models[i]->model);

			models[i]->model  = nullptr;
			models[i]->coords = nullptr;
			models[i]->processed = false;

			delete models[i];
			models[i] = nullptr;

			continue;
		}

		PyObject* __visible = PyString_FromString("visible");

		PyObject_SetAttr(models[i]->model, __visible, py_visible);

		Py_DECREF(__visible);
	} traceLog

	Py_DECREF(py_visible);

	extendedDebugLog("Models visiblity changing OK!");

	return 0;
};

uint8_t del_models()
{
	traceLog
	if (!isInited || first_check || battleEnded) { traceLog
		return 1;
	} traceLog

	extendedDebugLog("models deleting...");

	std::vector<ModModel*>::iterator it_model = models.begin();

	while (it_model != models.end()) {
		if (*it_model == nullptr) { traceLog
			extendedDebugLogEx(WARNING, "del_models - *it_model is NULL!");

			it_model = models.erase(it_model);

			continue;
		}

		if (!(*it_model)->model || (*it_model)->model == Py_None || !(*it_model)->processed) { traceLog
			extendedDebugLogEx(WARNING, "(*it_model)->model is NULL or None or not processed!");

			Py_XDECREF((*it_model)->model);

			(*it_model)->model  = nullptr;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			continue;
		}

		PyObject_CallFunctionObjArgs_increfed(result, BigWorldUtils::m_delModel, (*it_model)->model, nullptr);

		if (result) {
			Py_DECREF(result);

			Py_XDECREF((*it_model)->model);

			(*it_model)->model  = nullptr;
			(*it_model)->coords = nullptr;
			(*it_model)->processed = false;

			delete *it_model;
			*it_model = nullptr;

			it_model = models.erase(it_model);

			continue;
		}

		it_model++;
	} traceLog

	/*std::vector<ModLight*>::iterator it_light = lights.begin();

	while (it_light != lights.end()) {
		if (*it_light == nullptr) { traceLog
			extendedDebugLogEx(WARNING, "del_models - *it_light is NULL!");
			
			it_light = lights.erase(it_light);

			continue;
		}

		if (!(*it_light)->model || (*it_light)->model == Py_None) { traceLog
			extendedDebugLogEx(WARNING, "(*it_light)->model is NULL or None or not processed!");

			Py_XDECREF((*it_light)->model);

			(*it_light)->model  = nullptr;
			(*it_light)->coords = nullptr;

			delete *it_light;
			*it_light = nullptr;

			it_light = lights.erase(it_light);

			continue;
		}

		it_light++;
	} traceLog*/

	extendedDebugLog("models deleting OK!");

	return 0;
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
