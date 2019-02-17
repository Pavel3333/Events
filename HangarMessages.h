#pragma once

#include "API_functions.h"
#include "python2.7/Python.h"

extern PyObject* SM_TYPE;
extern PyObject* pushMessage;

uint8_t showMessage(PyObject*);