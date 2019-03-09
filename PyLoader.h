#pragma once

#include "API_functions.h"

class PyLoader {
public:
	static bool inited;

	static PyObject* py_module;
	static PyObject* keyDelLastModel;
	static PyObject* onModelCreatedPyMeth;


	static MyErr init();
	static void  fini();

	static PyObject* start(PyObject*, PyObject*);
	static PyObject* fini(PyObject*, PyObject*);
	static PyObject* check(PyObject*, PyObject*);
	static PyObject* initCfg(PyObject*, PyObject*);
	static PyObject* keyHandler(PyObject*, PyObject*);

	static struct PyMethodDef event_methods[];
};