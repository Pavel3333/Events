#pragma once

#include "API_functions.h"

#define PyObject_CallObject_increfed(result_name, obj, args)        \
	Py_INCREF(obj);                                                          \
	PyObject* result_name = PyObject_CallObject(obj, args); \
	Py_DECREF(obj);

#define PyObject_CallFunction_increfed(result_name, obj, format, ...)        \
	Py_INCREF(obj);                                                          \
	PyObject* result_name = PyObject_CallFunction(obj, format, __VA_ARGS__); \
	Py_DECREF(obj);

#define PyObject_CallFunctionObjArgs_increfed(result_name, obj, ...)        \
	Py_INCREF(obj);                                                         \
	PyObject* result_name = PyObject_CallFunctionObjArgs(obj, __VA_ARGS__); \
	Py_DECREF(obj);