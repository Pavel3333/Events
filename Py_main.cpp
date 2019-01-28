#define CONSOLE_VER1

#include "ModThreads.h"
#include "Py_config.h"
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

PyObject* BigWorld = NULL;
PyObject* g_gui = NULL;
PyObject* g_appLoader = NULL;
PyObject* json = NULL;

PyObject* modGUI = NULL;

PyObject* tickTimerMethod = NULL;
PyObject* spaceKey = NULL;

PyThreadState* _save = NULL;

uint8_t first_check = 100U;
uint8_t mapID = NULL;
uint32_t request = NULL;
uint32_t databaseID = NULL;

extern EVENT_ID EventsID;
extern STAGE_ID StagesID;

bool isInited = false;

bool battleEnded = true;

bool isModelsAlreadyCreated = false;
bool isModelsAlreadyInited = false;

bool isTimerStarted = false;
bool isTimeON = false;

bool isStreamer = false;

long timerCBID    = NULL;
long delLabelCBID = NULL;

//HANDLE getEvent = NULL;

uint8_t lastStageID = StagesID.COMPETITION;
uint8_t lastEventID = EventsID.IN_HANGAR;

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

		it_sect_sync = sync_map.modelsSects_deleting.erase(it_sect_sync); //������� ������ �� ������� ������ �������������
	}

	sync_map.modelsSects_deleting.~vector();
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

//native methods

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

	float distTemp;
	float dist = -1.0;
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
		return false;
	}

	if (dist > dist_equal) {
		return 7U;
	}

	*modelID = modelTypeLast;
	*coords  = coords_res;

	return NULL;
}

void callback(long* CBID, PyObject* func, float time_f=1.0) {
	if (!func) {
		return;
	}

	PyObject* time_p;

	if (!time_f) time_p = PyFloat_FromDouble(0.0);
	else         time_p = PyFloat_FromDouble(time_f);

	if (!time_p) {
		return;
	}

	PyObject* __callback_text = PyString_FromStringAndSize("callback", 8U);

	//Py_INCREF(func);

	PyObject* res = PyObject_CallMethodObjArgs(BigWorld, __callback_text, time_p, func, NULL);

	Py_DECREF(__callback_text);
	
	if (!res) {
		return;
	}

	*CBID = PyInt_AS_LONG(res);

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("[NY_Event]: Callback created!\n"));
#endif

	Py_DECREF(res);
}

void cancelCallback(long* CBID) {
	if (!*CBID) {
		return;
	}

	PyObject* __cancelCallback = PyString_FromStringAndSize("cancelCallback", 14U);

	PyObject* res = PyObject_CallMethodObjArgs(BigWorld, __cancelCallback, PyInt_FromLong(*CBID), NULL);

	Py_DECREF(__cancelCallback);
	Py_XDECREF(res);

	*CBID = NULL;
}

//GUI methods

PyObject* GUI_getAttr(char* attribute) {
	if (!modGUI) {
		return NULL;
	}

	return PyObject_GetAttrString(modGUI, attribute);
}

bool GUI_setAttr(char* attribute, PyObject* value) {
	if (!modGUI) {
		return false;
	}

	return PyObject_SetAttrString(modGUI, attribute, value);
}

void GUI_setWarning(uint8_t warningCode) {
#if debug_log && extended_debug_log
	if (!isInited || !modGUI) {
		return;
	}

	PyObject* __setWarning = PyString_FromStringAndSize("setWarning", 10U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setWarning, PyInt_FromSize_t((size_t)warningCode), NULL);

	Py_DECREF(__setWarning);
	Py_XDECREF(res);
#endif

	return;
}

void GUI_setError(uint8_t errorCode) {
#if debug_log && extended_debug_log
	if (!isInited || !modGUI) {
		return;
	}

	PyObject* __setError = PyString_FromStringAndSize("setError", 8U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setError, PyInt_FromSize_t((size_t)errorCode), NULL);

	Py_DECREF(__setError);
	Py_XDECREF(res);
#endif

	return;
}

void GUI_setVisible(bool visible) {
	if (!isInited || battleEnded || !modGUI) {
		return;
	}

	PyObject* __setVisible = PyString_FromStringAndSize("setVisible", 10U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setVisible, PyBool_FromLong((long)visible), NULL);

	Py_DECREF(__setVisible);
	Py_XDECREF(res);

	return;
}

void GUI_setTimerVisible(bool visible) {
	if (!isInited || battleEnded || !modGUI) {
		return;
	}

	PyObject* __setTimerVisible = PyString_FromStringAndSize("setTimerVisible", 15U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setTimerVisible, PyBool_FromLong((long)visible), NULL);

	Py_DECREF(__setTimerVisible);
	Py_XDECREF(res);

	return ;
}

void GUI_setTime(uint32_t time_preparing) {
	if (!isInited || battleEnded || !modGUI) {
		return;
	}

	char new_time[30];

	sprintf_s(new_time, 30U, "Time: %02d:%02d", time_preparing / 60, time_preparing % 60);

	PyObject* __setTime = PyString_FromStringAndSize("setTime", 7U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setTime, PyString_FromString(new_time), NULL);

	Py_DECREF(__setTime);
	Py_XDECREF(res);

	return;
}

void GUI_setText(char* msg, float time_f=NULL) {
	if (!isInited || battleEnded || !modGUI) {
		return;
	}

	PyObject* __setText = PyString_FromStringAndSize("setText", 7U);

	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setText, PyString_FromString(msg), NULL);

	Py_DECREF(__setText);
	Py_XDECREF(res);

	PyObject* delLabelCBID_p = GUI_getAttr("delLabelCBID");

	if (!delLabelCBID_p || delLabelCBID_p == Py_None) {
		delLabelCBID = NULL;

		Py_XDECREF(delLabelCBID_p);
	}
	else {
		delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);

		Py_DECREF(delLabelCBID_p);
	}

	if (delLabelCBID) {
		cancelCallback(&delLabelCBID);

		Py_INCREF(Py_None);

		if (!GUI_setAttr("delLabelCBID", Py_None)) {
			return;
		}
	}

	if (time_f) {
		PyObject* __clearTextCB = PyString_FromStringAndSize("clearTextCB", 11U);
		PyObject* res2 = PyObject_CallMethodObjArgs(modGUI, __clearTextCB, PyFloat_FromDouble(time_f), NULL);

		Py_DECREF(__clearTextCB);

		Py_XDECREF(res2);

		PyObject* delLabelCBID_p = GUI_getAttr("delLabelCBID");

		if (!delLabelCBID_p || delLabelCBID_p == Py_None) delLabelCBID = NULL;
		else delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);
	}

	return;
}

void GUI_setMsg(uint8_t msgID, uint8_t scoreID = NULL, float time_f = NULL) {
	if (!isInited || battleEnded || !modGUI || msgID >= MESSAGES_COUNT || scoreID >= MESSAGES_COUNT) {
		return;
	}

	if (lastStageID == current_map.stageID && lastStageID != StagesID.GET_SCORE && lastStageID != StagesID.ITEMS_NOT_EXISTS) {
		return;
	}

	// �������� �� ������� ����������� ������ ������

	PyObject* __UI_messages = PyString_FromStringAndSize("UI_messages", 11U);

	/*if (PyDict_Contains(g_self->i18n, __UI_messages) != 1U) {
		Py_DECREF(__UI_messages);
		return ;
	}*/

	PyObject* messagesList = PyDict_GetItem(g_self->i18n, __UI_messages);

	Py_DECREF(__UI_messages);
	 
	if (!messagesList) {
		return;
	}

	//----------------------------------------------

	//������� ��������� �� ������

	PyObject* msg_p = PyList_GetItem(messagesList, msgID);

	if (!msg_p) {
		return;
	}

	char* msg = PyString_AsString(msg_p);

	if (!msg) {
		return;
	}

	//---------------------------

	char new_msg[255];

	if (msgID == StagesID.GET_SCORE) { // ���� ��� - ��������� � ���, ��� �������� �����
		sprintf_s(new_msg, 255U, msg, COLOURS[msgID], SCORE[scoreID]);
	}
	else {
		sprintf_s(new_msg, 255U, msg, COLOURS[msgID]);
	}

	GUI_setText(new_msg, time_f);

	lastStageID = current_map.stageID;

	return;
}

void GUI_clearText() {
	if (!isInited || !modGUI) {
		return;
	}

	PyObject* __clearText = PyString_FromStringAndSize("clearText", 9U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __clearText, NULL);

	Py_DECREF(__clearText);
	Py_XDECREF(res);

	lastStageID = StagesID.COMPETITION;

	delLabelCBID = NULL;

	return;
}

void GUI_tickTimer() {
	timerCBID = NULL;

	if (!isInited || first_check || battleEnded) {
		return;
	}

	if(isModelsAlreadyCreated && isModelsAlreadyInited) request = get(mapID, EventsID.IN_BATTLE_GET_SYNC);
	else                                                request = get(mapID, EventsID.IN_BATTLE_GET_FULL);

	if (request) {
#if debug_log
		PySys_WriteStdout("[NY_Event]: Error in tickTimer: %d\n", request);
#endif

		return;
	}

	callback(&timerCBID, tickTimerMethod, 1.0);

	return;
};

static PyObject* GUI_tickTimerMethod(PyObject *self, PyObject *args) {
	GUI_tickTimer();

	Py_RETURN_NONE;
};

//-----------

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

static PyObject* event_model(char* path, float coords[3]) {
	if (!isInited || battleEnded) {
		return NULL;
	}

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("model creating...\n"));
#endif

	PyObject* __Model = PyString_FromStringAndSize("Model", 5U);

	PyObject* Model = PyObject_CallMethodObjArgs(BigWorld, __Model, PyString_FromString(path), NULL);

	Py_DECREF(__Model);

	if (!Model) {
		return NULL;
	}

	PyObject* coords_p = PyTuple_New(3U);

	if (!coords_p) {
		Py_DECREF(Model);
		return NULL;
	}

	for (uint8_t i = NULL; i < 3U; i++) {
		PyTuple_SET_ITEM(coords_p, i, PyFloat_FromDouble(coords[i]));
	}

	PyObject* __position = PyString_FromStringAndSize("position", 8U);

	if (PyObject_SetAttr(Model, __position, coords_p)) {
		Py_DECREF(__position);
		Py_DECREF(coords_p);
		Py_DECREF(Model);

		return NULL;
	}

	Py_DECREF(__position);

#if debug_log && extended_debug_log && super_extended_debug_log
	OutputDebugString(_T("model creating OK!\n"));
#endif

	return Model;
};

uint8_t init_models() {
	if (!isInited || first_check || request || battleEnded || models.empty()) {
		return 1U;
	}
#if debug_log 
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

#if debug_log
	OutputDebugString(_T("[NY_Event]: models adding OK!\n"));
#endif

	return NULL;
}

uint8_t set_visible(bool isVisible) {
	if (!isInited || first_check || request || battleEnded) {
		return 1U;
	}

	PyObject* py_visible = PyBool_FromLong(isVisible);

	if (models.empty()) return 2U;
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

uint8_t create_models(uint8_t eventID) {
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
#if debug_log && extended_debug_log && super_extended_debug_log
		PySys_WriteStdout("[NY_Event]: parsing FAILED! Error code: %d\n", (uint32_t)parsing_result);
#endif

		GUI_setError(parsing_result);

		return 2U;
	}


#if debug_log  && extended_debug_log
		OutputDebugString(_T("[NY_Event]: parsing OK!\n"));
#endif
	
	if (current_map.time_preparing)  //������� �����
		GUI_setTime(current_map.time_preparing);
		
	if (current_map.stageID >= 0 && current_map.stageID < STAGES_COUNT) {    //������� ���������
		if (
			current_map.stageID == StagesID.WAITING      ||
			current_map.stageID == StagesID.START        ||
			current_map.stageID == StagesID.COMPETITION  ||
			current_map.stageID == StagesID.END_BY_TIME  ||
			current_map.stageID == StagesID.END_BY_COUNT ||
			current_map.stageID == StagesID.STREAMER_MODE
			) {
			if (!isTimerStarted) {
				isTimerStarted = true;

				GUI_tickTimer();
			}

			if (lastStageID != StagesID.GET_SCORE && lastStageID != StagesID.ITEMS_NOT_EXISTS) GUI_setMsg(current_map.stageID);

			if(current_map.stageID == StagesID.END_BY_TIME || current_map.stageID == StagesID.END_BY_COUNT) {
				current_map.time_preparing = NULL;

				GUI_setTime(NULL);

				if (current_map.stageID == StagesID.END_BY_COUNT) {
					GUI_setTimerVisible(false);

					isTimeON = false;
				}

				GUI_setMsg(current_map.stageID);

				uint8_t event_result = event_fini();

				if (event_result) {
#if debug_log
					PySys_WriteStdout("[NY_Event]: Warning - create_models - event_fini - Error code %d\n", event_result);
#endif

					GUI_setWarning(event_result);
				}


			}
			else {
				if (!isTimeON) {
					GUI_setTimerVisible(true);

					isTimeON = true;
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

#if debug_log && extended_debug_log
						OutputDebugString(_T("[NY_Event]: creating...\n"));
#endif

						Py_BEGIN_ALLOW_THREADS;

						models.~vector();
						lights.~vector();

						models.resize(current_map.minimap_count);
						lights.resize(current_map.minimap_count);

						for (uint16_t i = NULL; i < current_map.minimap_count; i++) {
							if (models[i] != nullptr) {
								Py_XDECREF(models[i]->model);

								models[i]->model = NULL;
								models[i]->coords = nullptr;
								models[i]->processed = false;

								delete models[i];
								models[i] = nullptr;
							}

							if (lights[i] != nullptr) {
								Py_XDECREF(lights[i]->model);

								lights[i]->model = NULL;
								lights[i]->coords = nullptr;

								delete lights[i];
								lights[i] = nullptr;
							}
						}

						Py_END_ALLOW_THREADS;

						uint16_t counter_model = NULL;

						for (auto it = current_map.modelsSects.cbegin();
							it != current_map.modelsSects.cend();
							it++) {
							if (!it->isInitialised || it->models.empty()) {
								continue;
							}

							for (auto it2 = it->models.cbegin();
								it2 != it->models.cend();
								it2++) {
								if (*it2 == nullptr) {
#if debug_log && extended_debug_log && super_extended_debug_log
									OutputDebugString("NULL, ");
#endif

									counter_model++;

									continue;
								}
#if debug_log && extended_debug_log && super_extended_debug_log
								OutputDebugString("[");
#endif
								models[counter_model] = new ModModel{
									false,
									event_model(it->path, *it2),
									*it2
								};

								lights[counter_model] = new ModLight{
									event_light(*it2),
									*it2
								};

								counter_model++;

#if debug_log && extended_debug_log && super_extended_debug_log
								OutputDebugString("], ");
#endif
							}
						}
#if debug_log && extended_debug_log
						OutputDebugString(_T("], \n"));
#endif

#if debug_log  && extended_debug_log
						OutputDebugString(_T("[NY_Event]: creating OK!\n"));
#endif

						request = init_models();

						if (request) {
							if (request > 9U) {
#if debug_log
								PySys_WriteStdout("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - init_models - Error code %d\n", request);
#endif

								GUI_setError(request);

								return 4U;
							}

#if debug_log
							PySys_WriteStdout("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - init_models - Warning code %d\n", request);
#endif

							GUI_setWarning(request);

							return 3U;
						}

						request = set_visible(true);

						if (request) {
							if (request > 9U) {
#if debug_log
								PySys_WriteStdout("[NY_Event][ERROR]: IN_BATTLE_GET_FULL - init_models - Error code %d\n", request);
#endif

								GUI_setError(request);

								return 4U;
							}

#if debug_log
							PySys_WriteStdout("[NY_Event][WARNING]: IN_BATTLE_GET_FULL - init_models - Warning code %d\n", request);
#endif

							GUI_setWarning(request);

							return 3U;
						}

						isModelsAlreadyInited = true;
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

					it_sect_sync = sync_map.modelsSects_deleting.erase(it_sect_sync); //������� ������ �� ������� ������ �������������
				}

				sync_map.modelsSects_deleting.~vector();
			}
			else {
				return NULL;
			}
		}
		else {
#if debug_log
			OutputDebugString(_T("[NY_Event]: Warning - StageID is not right for this event\n"));
#endif
		}
	}
	else {
#if debug_log
		OutputDebugString(_T("[NY_Event]: Warning - StageID is not correct\n"));
#endif
	}

	return NULL;
}

//threads functions

DWORD WINAPI Thread1_2_3(LPVOID lpParam)
{
	if (!isInited || !databaseID || battleEnded) {
		return 1U;
	}

	wchar_t msgBuf[32];

	PMYDATA_1 pDataArray = (PMYDATA_1)lpParam;

	DWORD ID            = pDataArray->ID;

	uint32_t databaseID = pDataArray->databaseID;
	uint32_t map_ID     = pDataArray->map_ID;
	uint32_t eventID    = pDataArray->eventID;

	//������� ��������� � ���, ��� ����� ��������

#if debug_log && extended_debug_log
	wsprintfW(msgBuf, _T("Thread %d working!\n"), ID);

	OutputDebugString(msgBuf);
#endif

	//������� �����

	request = send_token(databaseID, map_ID, eventID, NULL, nullptr);

	//�������� GIL ��� ����� ������

	PyGILState_STATE gstate = PyGILState_Ensure();

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
	else if (eventID == EventsID.IN_BATTLE_GET_FULL || eventID == EventsID.IN_BATTLE_GET_SYNC) {
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

	//��������� GIL ��� ����� ������

	PyGILState_Release(gstate);

	//------------------------------

	//������� ������ ����� ���������� �������� ����

	if (pDataArray != NULL)
	{
		HeapFree(GetProcessHeap(), 0, pDataArray);
		pDataArray = NULL;    // Ensure address is not reused.
	}

	//��������� �����

#if debug_log && extended_debug_log
	wsprintfW(msgBuf, _T("Closing thread %d\n"), ID);

	OutputDebugString(msgBuf);
#endif

	ExitThread(NULL); //��������� �����

	return NULL;
}

//-----------------

uint8_t get(uint8_t map_ID, uint8_t eventID) {
	if (!isInited || !databaseID || battleEnded) {
		return 1U;
	}

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: generating token...\n"));
#endif

	if (eventID == EventsID.IN_HANGAR || eventID == EventsID.IN_BATTLE_GET_FULL || eventID == EventsID.IN_BATTLE_GET_SYNC) { //������� 1 ����� ��� �������� ������� � ���������
		//_save = NULL;

		//Py_UNBLOCK_THREADS; //��������� GIL

		/*getEvent = CreateEvent(
			NULL,               // default security attributes
			TRUE,               // manual-reset event
			FALSE,              // initial state is nonsignaled
			TEXT("WriteEvent")  // object name
		);

		if (getEvent == NULL)
		{
			printf("CreateEvent failed (%d)\n", GetLastError());

			return;
		}*/

		uint8_t threadCount = 1U; //��������� ������� ������ � 1 ������

		//HANDLE* hThreads = new HANDLE[threadCount];

		//threads_1.resize(threads_1.size() + threadCount);

		for (size_t i = NULL; i < threadCount; i++)
		{
			// Allocate memory for thread data.

			PMYDATA_1 pMyData = (PMYDATA_1)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				sizeof(MYDATA_1));

			if (pMyData == NULL)
			{
				// If the array allocation fails, the system is out of memory
				// so there is no point in trying to print an error message.
				// Just terminate execution.
				ExitProcess(2);
			}

			// Generate unique data for each thread to work with.

			pMyData->databaseID = databaseID;
			pMyData->map_ID     = map_ID;
			pMyData->eventID    = eventID;

			// Create the thread to begin execution on its own.

			HANDLE hThread = CreateThread(
				NULL,                                   // default security attributes
				0,                                      // use default stack size  
				Thread1_2_3,                            // thread function name
				pMyData,                                // argument to thread function 
				0,                                      // use default creation flags 
				&(pMyData->ID));                        // returns the thread identifier 

			if (hThread == NULL)
			{
				OutputDebugString(TEXT("CreateThread: error 1\n"));

				if (pMyData != NULL)   //������� ������
				{
					HeapFree(GetProcessHeap(), 0, pMyData);
					pMyData = NULL;
				}

				return 1U;
			}
		} // End of main thread creation loop.

		//delete[] hThreads;

		/*if (_save) { //�������� GIL
			Py_BLOCK_THREADS;

			_save = NULL;
		}*/

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

	if (!map_PS) return PyInt_FromSize_t(5U);

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

	isTimeON = true;

	request = get(mapID, EventsID.IN_BATTLE_GET_FULL);

	if (request) {
#if debug_log
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
#if debug_log
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

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: fini debug 1\n"));
#endif

	if (current_map.modelsSects.size() && current_map.minimap_count) {
#if debug_log && extended_debug_log
		OutputDebugString(_T("[PositionsMod_Free]: fini debug 3\n"));
#endif
		Py_BEGIN_ALLOW_THREADS
			clearModelsSections();
		Py_END_ALLOW_THREADS
	}

	GUI_setTime(NULL);
	GUI_setTimerVisible(false);

	isTimeON       = false;

	current_map.minimap_count  = NULL;
	current_map.time_preparing = NULL;

	isModelsAlreadyCreated = false;

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: fini OK!\n"));
#endif

	return NULL;
}

static PyObject* event_fini_py(PyObject *self, PyObject *args) {
	uint8_t fini_result = event_fini();
	
	if (fini_result) return PyInt_FromSize_t(fini_result);
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

		cancelCallback(&timerCBID);
		cancelCallback(&delLabelCBID);

		isTimerStarted = false;

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

static PyObject* event_�heck(PyObject *self, PyObject *args) {
	if (!isInited) {
		return PyInt_FromSize_t(1U);
	}

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: checking...\n"));
#endif
	PyObject* __player = PyString_FromStringAndSize("player", 6U);
	PyObject* player = PyObject_CallMethodObjArgs(BigWorld, __player, NULL);

	Py_DECREF(__player);

	if (!player) {
		return PyInt_FromSize_t(2U);
	}

	PyObject* __databaseID = PyString_FromStringAndSize("databaseID", 10U);
	PyObject* DBID_string = PyObject_GetAttr(player, __databaseID);

	Py_DECREF(__databaseID);
	Py_DECREF(player);

	if (!DBID_string) {
		return PyInt_FromSize_t(3U);
	}

	PyObject* DBID_int = PyNumber_Int(DBID_string);

	Py_DECREF(DBID_string);

	if (!DBID_int) {
		return PyInt_FromSize_t(4U);
	}

	databaseID = PyInt_AS_LONG(DBID_int);

	Py_DECREF(DBID_int);

#if debug_log && extended_debug_log
	OutputDebugString(_T("[NY_Event]: DBID created\n"));
#endif

	battleEnded = false;

	first_check = get(NULL, EventsID.IN_HANGAR);

	if (first_check) {
		return PyInt_FromSize_t(2U);
	}
	else {
		Py_RETURN_NONE;
	}
};

static PyObject* event_init(PyObject *self, PyObject *args) {
	if (!isInited) {
		return PyInt_FromSize_t(1U);
	}

	PyObject* template_;
	PyObject* apply;
	PyObject* byteify;

	if (!PyArg_ParseTuple(args, "OOO", &template_, &apply, &byteify)) {
		return PyInt_FromSize_t(2U);
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

	Py_RETURN_NONE;
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
#if debug_log
					PySys_WriteStdout("[NY_Event][ERROR]: DEL_LAST_MODEL - send_token - Error code %d\n", server_req);
#endif

					GUI_setError(server_req);

					Py_RETURN_NONE;
				}

#if debug_log
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
	{ "b",  event_�heck, METH_VARARGS, ":P" }, //check
	{ "c",  event_start, METH_NOARGS, ":P" },//start
	{ "d",  event_fini_py, METH_NOARGS, ":P" },//fini
	{ "e",  event_err_code, METH_NOARGS, ":P" },//get_error_code
	{ "g",  event_init, METH_VARARGS, ":P" },//init
	{ "cb", GUI_tickTimerMethod, METH_NOARGS, ":P" },//tickTimer
	{ "event_handler", event_inject_handle_key_event, METH_VARARGS, ":P" },//init
	{ NULL, NULL, 0, NULL }
};

PyDoc_STRVAR(event_methods__doc__,
	"Trajectory Mod module"
);

//---------------------------INITIALIZATION--------------------------

PyMODINIT_FUNC initevent(void)
{
	BigWorld = PyImport_AddModule("BigWorld");

	if (!BigWorld) return;

	PyObject* appLoader = PyImport_ImportModule("gui.app_loader");

	if (!appLoader) return;

	PyObject* __g_appLoader = PyString_FromStringAndSize("g_appLoader", 11U);

	g_appLoader = PyObject_GetAttr(appLoader, __g_appLoader);

	Py_DECREF(__g_appLoader);
	Py_DECREF(appLoader);

	if (!g_appLoader) return;

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

	//tick timer method

	PyObject* __cb = PyString_FromStringAndSize("cb", 2U);

	tickTimerMethod = PyObject_GetAttr(event_module, __cb);

	Py_DECREF(__cb);

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
