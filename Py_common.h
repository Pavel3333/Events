#pragma once

#include "API_functions.h"

#define PyObject_CallMethod_increfed(result_name, obj, meth, format, ...)        \
	Py_INCREF(obj);                                                              \
	PyObject* result_name = PyObject_CallMethod(obj, meth, format, __VA_ARGS__); \
	Py_DECREF(obj);

#define PyObject_CallMethodObjArgs_increfed(result_name, obj, meth, ...)        \
	Py_INCREF(obj);                                                             \
	PyObject* result_name = PyObject_CallMethodObjArgs(obj, meth, __VA_ARGS__); \
	Py_DECREF(obj);

#define PyObject_CallFunction_increfed(result_name, obj, format, ...)        \
	Py_INCREF(obj);                                                          \
	PyObject* result_name = PyObject_CallFunction(obj, format, __VA_ARGS__); \
	Py_DECREF(obj);

#define PyObject_CallFunctionObjArgs_increfed(result_name, obj, ...)        \
	Py_INCREF(obj);                                                         \
	PyObject* result_name = PyObject_CallFunctionObjArgs(obj, __VA_ARGS__); \
	Py_DECREF(obj);