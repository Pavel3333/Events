#pragma once
#include "API_functions.h"


class BigWorldUtils {
	static bool inited;

public:
	static PyObject* m_BigWorld;
	static PyObject* m_Model;
	static PyObject* m_fetchModel;
	static PyObject* m_addModel;
	static PyObject* m_delModel;
	static PyObject* m_callback;
	static PyObject* m_cancelCallback;
	static PyObject* m_g_appLoader;
	static PyObject* m_partial;
	static PyObject* m_json;

	static MyErr init();
	static void fini();

	// BigWorld.callback()
	// Registers a callback function to be called after a certain time, but not before the next tick
	static MyErr callback(long&, PyObject*, float delay = 1.0);

	// BigWorld.cancelCallback()
	// Cancels a previously registered callback
	static MyErr cancelCallback(long);

	static MyErr getMapID(uint8_t&);
	static MyErr getDBID(uint32_t&);
	static MyErr getLastModelCoords(float, MODEL_ID*, float**);

private:
	static PyObject* getPlayer_p();

	static MyErr callback_p(long&, PyObject*, float);
	static MyErr cancelCallback_p(long);
	static MyErr getMapID_p(uint8_t&);
	static MyErr getDBID_p(uint32_t&);
	static MyErr getLastModelCoords_p(float, MODEL_ID*, float**);
};
