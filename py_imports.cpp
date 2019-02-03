#include"py_imports.h"
#include "common.h"


PyObject* Py::BigWorld = nullptr;


void Py::init_imports()
{
    BigWorld = PyImport_AddModule("BigWorld");
}
