#pragma once

#include "API_functions.h"
#include "python2.7/Python.h"

extern PyObject* m_BigWorld;
extern PyObject* m_Model;
extern PyObject* m_fetchModel;
extern PyObject* m_callback;
extern PyObject* m_cancelCallback;
extern PyObject* m_g_gui;
extern PyObject* m_g_appLoader;
extern PyObject* m_partial;
extern PyObject* m_json;

uint8_t initNative();
void    finiNative();

void callback(long*, PyObject*, float time_f = 1.0);
void cancelCallback(long*);