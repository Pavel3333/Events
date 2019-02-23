#include "BW_native.h"


// constructor
BigWorldUtils::BigWorldUtils()
{
	init();
}

// destructor
BigWorldUtils::~BigWorldUtils()
{
	if (!inited) return;

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


// private methods


// initialization
void BigWorldUtils::init()
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
}

// BigWorld.player() private impementation
PyObject* BigWorldUtils::getPlayer_p() const
{
	return PyObject_CallMethod(m_BigWorld, "player", nullptr);
}

// BigWorld.callback() private implementation
int BigWorldUtils::callback_p(long* callbackID, PyObject* func, float delay)
{
	if (!func) {
		lastErrorStr = "Error: func argument is NULL";
		return -1;
	}

	PyObject* res = PyObject_CallFunction(m_callback, "fO", delay, func);
	if (!res) {
		lastErrorStr = "Error with BigWorld.callback(delay, func)";
		return -2;
	}

	*callbackID = PyInt_AS_LONG(res);
	Py_DECREF(res);

	return 0;
}

// BigWorld.cancelCallback() private implementation
int BigWorldUtils::cancelCallback_p(long callbackID)
{
	PyObject* res = PyObject_CallFunction(m_cancelCallback, "l", callbackID);
	if (!res) {
		lastErrorStr = "Error with BigWorld.cancelCallback(callbackID)";
		return -1;
	}

	Py_DECREF(res);

	return 0;
}

// BigWorld.player().arena.arenaType.geometryName private implementation
int BigWorldUtils::getMapID_p(uint8_t* mapID)
{
	PyObject* player = getPlayer_p();
	if (!player) {
		lastErrorStr = "Error getting BigWorld.player()";
		return -1;
	}

	PyObject* arena = PyObject_GetAttrString(player, "arena");
	Py_DECREF(player);
	if (!arena) {
		lastErrorStr = "Error getting BigWorld.player().arena";
		return -2;
	}

	PyObject* arenaType = PyObject_GetAttrString(arena, "arenaType");
	Py_DECREF(arena);
	if (!arenaType) {
		lastErrorStr = "Error getting BigWorld.player().arena.arenaType";
		return -3;
	}

	PyObject* map_PS = PyObject_GetAttrString(arenaType, "geometryName");
	Py_DECREF(arenaType);
	if (!map_PS) {
		lastErrorStr = "Error getting BigWorld.player().arena.arenaType.geometryName";
		return -4;
	}

	char* map_s = _strdup(PyString_AS_STRING(map_PS));
	Py_DECREF(map_PS);

	if (map_s[2] == '_') map_s[2] = '\0';
	map_s[3] = '\0';

	*mapID = atoi(map_s);
	delete map_s;

	return 0;
}

// BigWorld.player().databaseID private implementation
int BigWorldUtils::getDBID_p(uint32_t* DBID)
{
	INIT_LOCAL_MSG_BUFFER;

	PyObject* player = getPlayer_p();
	if (!player) {
		lastErrorStr = "Error getting BigWorld.player()";
		return -1;
	}

	PyObject* databaseID = PyObject_GetAttrString(player, "databaseID");
	Py_DECREF(player);
	if (!databaseID) {
		lastErrorStr = "Error getting BigWorld.player().databaseID";
		return -2;
	}

	*DBID = PyInt_AS_LONG(databaseID);
	Py_DECREF(databaseID);

	return 0;
}

int BigWorldUtils::getLastModelCoords_p(float dist_equal, uint8_t* modelID, float** coords)
{
	INIT_LOCAL_MSG_BUFFER;

	PyObject* player = getPlayer_p();
	if (!player) {
		lastErrorStr = "Error getting BigWorld.player()";
		return -1;
	}

	PyObject* vehicle = PyObject_GetAttrString(player, "vehicle");
	Py_DECREF(player);
	if (!vehicle) {
		lastErrorStr = "Error getting BigWorld.player().vehicle";
		return -2;
	}

	PyObject* model_p = PyObject_GetAttrString(vehicle, "model");
	Py_DECREF(vehicle);
	if (!model_p) {
		lastErrorStr = "Error getting BigWorld.player().vehicle.model";
		return -3;
	}

	PyObject* position_Vec3 = PyObject_GetAttrString(model_p, "position");
	Py_DECREF(model_p);
	if (!position_Vec3) {
		lastErrorStr = "Error getting BigWorld.player().vehicle.model.position";
		return -4;
	}

	PyObject* position = PyObject_CallMethod(position_Vec3, "tuple", nullptr);
	Py_DECREF(position_Vec3);
	if (!position) {
		lastErrorStr = "Error getting BigWorld.player().vehicle.model.position.tuple()";
		return -5;
	}

	float coords_pos[3];

	for (auto i = 0; i < 3; i++) {
		PyObject* coord_p = PyTuple_GetItem(position, i);
		coords_pos[i] = static_cast<float>(PyFloat_AsDouble(coord_p));
	}

	float distTemp;
	float dist = -1.0;
	int8_t modelTypeLast = -1;
	float* coords_res = nullptr;


	// TODO: Это нужно заоптимизировать!!!
	for (const auto& it : current_map.modelsSects) {
		if (!it.isInitialised) {
			extendedDebugLogFmt("[NY_Event][WARNING]: sect %d is not initialized!\n", it.ID);

			continue;
		}

		for (float* it2 : it.models) {
			if (it2 == nullptr) {
				extendedDebugLog("[NY_Event][WARNING]: getLastModelCoords - it2 is NULL!\n");
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

	if (dist == -1.0 || modelTypeLast == -1 || coords_res == nullptr) { traceLog //модели с такой координатой не найдено
		extendedDebugLog("[NY_Event][WARNING]: getLastModelCoords - model not found!\n");

		return -6;
	} traceLog

	if (dist > dist_equal) { traceLog
		return -7;
	} traceLog

	*modelID = modelTypeLast;
	*coords = coords_res;

	return 0;
}


// public methods


void BigWorldUtils::callback(long* callbackID, PyObject* func, float delay)
{
	if (!inited) return;

	lastError = callback_p(callbackID, func, delay);
}

void BigWorldUtils::cancelCallback(long callbackID)
{
	if (!inited) return;

	lastError = cancelCallback_p(callbackID);
}

void BigWorldUtils::getMapID(uint8_t* mapID)
{
	if (!inited) return;

	lastError = getMapID_p(mapID);
}

void BigWorldUtils::getDBID(uint32_t* DBID)
{
	if (!inited) return;

	lastError = getDBID_p(DBID);
}

void BigWorldUtils::getLastModelCoords(float dist_equal, uint8_t* modelID, float** coords)
{
	if (!inited) return;

	lastError = getLastModelCoords_p(dist_equal, modelID, coords);
}


// instance
BigWorldUtils* gBigWorldUtils = nullptr;
