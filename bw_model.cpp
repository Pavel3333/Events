#include "bw_model.h"
#include "common.h"
#include "py_imports.h"


using namespace BW;


Model::Model(PyObject* model, Vector3D* pos)
{
    this->model = model;
    this->pos = pos;
}


Model::~Model()
{
	if (!inited) {
		Py_DECREF(this->model);
		return;
	}

	static PyObject* delModelMethodName = PyString_FromString("delModel");
	PyObject* result = PyObject_CallMethodObjArgs(Py::BigWorld, delModelMethodName, this->model, NULL);

	if (result) {
		Py_DECREF(result);
		Py_DECREF(this->model);
	}
}

void Model::Init()
{
	static PyObject* addModelMethodName = PyString_FromString("addModel");

	PyObject* result = PyObject_CallMethodObjArgs(Py::BigWorld, addModelMethodName, this->model, NULL);

	if (result) {
		Py_DECREF(result);
		this->inited = true;
	}
}

Model* Model::Open(const std::string& path, Vector3D* pos)
{
    static PyObject* modelMethodName = PyString_FromString("Model");

    PyObject* pyModel = PyObject_CallMethodObjArgs(Py::BigWorld, modelMethodName, PyString_FromString(path.c_str()), NULL);

    if (!pyModel) {
        return nullptr;
    }

    PyObject* pyPosition = PyTuple_New(3);
    PyTuple_SET_ITEM(pyPosition, 0, PyFloat_FromDouble(pos->x));
    PyTuple_SET_ITEM(pyPosition, 1, PyFloat_FromDouble(pos->y));
    PyTuple_SET_ITEM(pyPosition, 2, PyFloat_FromDouble(pos->z));

	static PyObject* positionAttrName = PyString_FromString("position");
	if (PyObject_SetAttr(pyModel, positionAttrName, pyPosition) == -1) {
        Py_DECREF(pyPosition);
        Py_DECREF(pyModel);
		return nullptr;
	}

	Py_DECREF(pyPosition);

	return new Model(pyModel, pos);
}
