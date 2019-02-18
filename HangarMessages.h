#pragma once

#include "API_functions.h"
#include "python2.7/Python.h"

extern PyObject* m_SM_TYPE;
extern PyObject* m_pushMessage;

bool initHangarMessages();
void finiHangarMessages();

uint8_t showMessage(PyObject*);