#pragma once

#include "API_functions.h"
#include "python2.7/Python.h"

extern PyObject* BigWorld;
extern PyObject* g_gui;
extern PyObject* g_appLoader;
extern PyObject* functools;
extern PyObject* json;

void callback(long*, PyObject*, float time_f = 1.0);
void cancelCallback(long*);