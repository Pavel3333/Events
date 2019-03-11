#include "pch.h"
#include "BW_native.h"
#include "MyLogger.h"
#include "Py_common.h"


INIT_LOCAL_MSG_BUFFER;


bool BigWorldUtils::inited                = false;
PyObject* BigWorldUtils::m_BigWorld       = nullptr;
PyObject* BigWorldUtils::m_Model          = nullptr;
PyObject* BigWorldUtils::m_fetchModel     = nullptr;
PyObject* BigWorldUtils::m_addModel       = nullptr;
PyObject* BigWorldUtils::m_delModel       = nullptr;
PyObject* BigWorldUtils::m_callback       = nullptr;
PyObject* BigWorldUtils::m_cancelCallback = nullptr;
PyObject* BigWorldUtils::m_g_appLoader    = nullptr;
PyObject* BigWorldUtils::m_partial        = nullptr;
PyObject* BigWorldUtils::m_json           = nullptr;



// initialization
MyErr BigWorldUtils::init()
{
	// получение BigWorld
	m_BigWorld = PyImport_AddModule("BigWorld");

	// получение Model
	m_Model = PyObject_GetAttrString(m_BigWorld, "Model");

	// получение fetchModel
	m_fetchModel = PyObject_GetAttrString(m_BigWorld, "fetchModel");

	// получение addModel
	m_addModel = PyObject_GetAttrString(m_BigWorld, "addModel");

	// получение delModel
	m_delModel = PyObject_GetAttrString(m_BigWorld, "delModel");

	// получение callback
	m_callback = PyObject_GetAttrString(m_BigWorld, "callback");

	// получение cancelCallback
	m_cancelCallback = PyObject_GetAttrString(m_BigWorld, "cancelCallback");

	// получение g_appLoader
	PyObject* appLoader = PyImport_ImportModule("gui.app_loader");
	m_g_appLoader = PyObject_GetAttrString(appLoader, "g_appLoader");
	Py_DECREF(appLoader);

	// получение partial
	PyObject* functools = PyImport_ImportModule("functools");
	m_partial = PyObject_GetAttrString(functools, "partial");
	Py_DECREF(functools);

	// получение json
	m_json = PyImport_ImportModule("json");

	inited = true;

	return_ok;
}


void BigWorldUtils::fini()
{
	if (!inited)
		return;

	Py_XDECREF(m_json);
	Py_XDECREF(m_partial);
	Py_XDECREF(m_g_appLoader);
	Py_XDECREF(m_cancelCallback);
	Py_XDECREF(m_callback);
	Py_XDECREF(m_delModel);
	Py_XDECREF(m_addModel);
	Py_XDECREF(m_fetchModel);
	Py_XDECREF(m_Model);
}


// BigWorld.player() private impementation
PyObject* BigWorldUtils::getPlayer_p()
{
	return PyObject_CallMethod(m_BigWorld, "player", nullptr);
}

// BigWorld.callback() private implementation
MyErr BigWorldUtils::callback_p(long& callbackID, PyObject* func, float delay)
{
	if (!func) {
		debugLogEx(ERROR, "Error: func argument is NULL");
		return_err -1;
	}

	PyObject_CallFunction_increfed(res, m_callback, "fO", delay, func);
	if (!res) {
		debugLogEx(ERROR, "Error with BigWorld.callback(%f, func)", delay);
		return_err -2;
	}

	callbackID = PyInt_AS_LONG(res);
	Py_DECREF(res);

	return_ok;
}

// BigWorld.cancelCallback() private implementation
MyErr BigWorldUtils::cancelCallback_p(long callbackID)
{
	if(!callbackID) 
		return_err -1;

	PyObject_CallFunction_increfed(res, m_cancelCallback, "l", callbackID);
	if (!res) {
		debugLogEx(ERROR, "Error with BigWorld.cancelCallback(%d)", callbackID);
		return_err -2;
	}

	Py_DECREF(res);

	return_ok;
}

// BigWorld.player().arena.arenaType.geometryName private implementation
MyErr BigWorldUtils::getMapID_p(uint8_t& mapID)
{
	PyObject* player = getPlayer_p();
	if (!player) {
		debugLogEx(ERROR, "Error getting BigWorld.player()");
		return_err -1;
	}

	PyObject* arena = PyObject_GetAttrString(player, "arena");
	Py_DECREF(player);
	if (!arena) {
		debugLogEx(ERROR, "Error getting BigWorld.player().arena");
		return_err -2;
	}

	PyObject* arenaType = PyObject_GetAttrString(arena, "arenaType");
	Py_DECREF(arena);
	if (!arenaType) {
		debugLogEx(ERROR, "Error getting BigWorld.player().arena.arenaType");
		return_err -3;
	}

	PyObject* map_PS = PyObject_GetAttrString(arenaType, "geometryName");
	Py_DECREF(arenaType);
	if (!map_PS) {
		debugLogEx(ERROR, "Error getting BigWorld.player().arena.arenaType.geometryName");
		return_err -4;
	}

	char* map_s = _strdup(PyString_AS_STRING(map_PS));
	Py_DECREF(map_PS);

	if (map_s[2] == '_') map_s[2] = '\0';
	map_s[3] = '\0';

	mapID = static_cast<uint8_t>(atoi(map_s));
	delete map_s;

	return_ok;
}

// BigWorld.player().databaseID private implementation
MyErr BigWorldUtils::getDBID_p(uint32_t& DBID)
{
	PyObject* player = getPlayer_p();
	if (!player) {
		debugLogEx(ERROR, "Error getting BigWorld.player()");
		return_err -1;
	}

	PyObject* databaseID = PyObject_GetAttrString(player, "databaseID");
	Py_DECREF(player);
	if (!databaseID) {
		debugLogEx(ERROR, "Error getting BigWorld.player().databaseID");
		return_err -2;
	}

	DBID = PyInt_AS_LONG(databaseID);
	Py_DECREF(databaseID);

	return_ok;
}

MyErr BigWorldUtils::getLastModelCoords_p(float dist_equal, MODEL_ID* modelID, float** coords)
{
	PyObject* player = getPlayer_p();
	if (!player) {
		debugLogEx(ERROR, "Error getting BigWorld.player()");
		return_err -1;
	}

	PyObject* vehicle = PyObject_GetAttrString(player, "vehicle");
	Py_DECREF(player);
	if (!vehicle) {
		debugLogEx(ERROR, "Error getting BigWorld.player().vehicle");
		return_err -2;
	}

	PyObject* model_p = PyObject_GetAttrString(vehicle, "model");
	Py_DECREF(vehicle);
	if (!model_p) {
		debugLogEx(ERROR, "Error getting BigWorld.player().vehicle.model");
		return_err -3;
	}

	PyObject* position_Vec3 = PyObject_GetAttrString(model_p, "position");
	Py_DECREF(model_p);
	if (!position_Vec3) {
		debugLogEx(ERROR, "Error getting BigWorld.player().vehicle.model.position");
		return_err -4;
	}

	PyObject* position = PyObject_CallMethod(position_Vec3, "tuple", nullptr);
	Py_DECREF(position_Vec3);
	if (!position) {
		debugLogEx(ERROR, "Error getting BigWorld.player().vehicle.model.position.tuple()");
		return_err -5;
	}

	float coords_pos[3];

	for (auto i = 0; i < 3; i++) {
		PyObject* coord_p = PyTuple_GetItem(position, i);
		coords_pos[i] = static_cast<float>(PyFloat_AsDouble(coord_p));
	}

	float distTemp;
	float dist = -1.0;
	MODEL_ID modelTypeLast = MODEL_ID::UNKNOWN;
	float* coords_res = nullptr;


	// TODO: Это нужно заоптимизировать!!!
	for (const auto& it : current_map.modelsSects) {
		if (!it.isInitialised) {
			extendedDebugLogEx(WARNING, "sect %d is not initialized!", it.ID);

			continue;
		}

		for (float* it2 : it.models) {
			if (it2 == nullptr) {
				extendedDebugLogEx(WARNING, "getLastModelCoords - it2 is NULL!");
				continue;
			}

			distTemp = getDist2Points(coords_pos, it2);

			if (dist == -1.0 || distTemp < dist) {
				dist = distTemp;
				modelTypeLast = it.ID;

				coords_res = it2;
			}
		}
	} traceLog

	if (dist == -1.0 || modelTypeLast == MODEL_ID::UNKNOWN || coords_res == nullptr) { traceLog //модели с такой координатой не найдено
		extendedDebugLogEx(WARNING, "getLastModelCoords - model not found!");

	return_err 1;
	} traceLog

	if (dist > dist_equal) { traceLog
		return_err 2;
	} traceLog

	*modelID = modelTypeLast;
	*coords = coords_res;

	return_ok;
}


// public methods


MyErr BigWorldUtils::callback(long& callbackID, PyObject* func, float delay)
{
	if (!inited)
		return_err -1;

	return callback_p(callbackID, func, delay);
}

MyErr BigWorldUtils::cancelCallback(long callbackID)
{
	if (!inited)
		return_err -1;

	return cancelCallback_p(callbackID);
}

MyErr BigWorldUtils::getMapID(uint8_t& mapID)
{
	if (!inited)
		return_err -1;

	return getMapID_p(mapID);
}

MyErr BigWorldUtils::getDBID(uint32_t& DBID)
{
	if (!inited)
		return_err -1;

	return getDBID_p(DBID);
}

MyErr BigWorldUtils::getLastModelCoords(float dist_equal, MODEL_ID* modelID, float** coords)
{
	if (!inited)
		return_err -1;

	return getLastModelCoords_p(dist_equal, modelID, coords);
}
