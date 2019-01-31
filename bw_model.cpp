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

/*
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
*/


ModelSet::ModelSet(size_t size)
{
    this->size = size;
    this->models_for_bg_loading.reserve(size);
}


ModelSet::~ModelSet()
{
}


void ModelSet::Add(std::string_view path)
{
    PyObject* pyPath = PyString_FromStringAndSize(path.data(), path.size());
    models_for_bg_loading.push_back(pyPath);
}


void ModelSet::OnModelLoaded(PyObject* arg1, PyObject* arg2)
{
    ModelSet* self = reinterpret_cast<ModelSet*>(arg2);
}

void ModelSet::StartBGLoading()
{
    PyObject* pyLoadModels = PyTuple_New(size);

    size_t i = 0;
    for (PyObject* model : models_for_bg_loading) {
        PyTuple_SET_ITEM(pyLoadModels, i++, model);
    }

    PyMethodDef pyOnModelLoadedDef = {
        "OnModelLoaded",
        (PyCFunction)this->OnModelLoaded,
        METH_VARARGS,
        ""
    };

    PyObject* pyOnModelLoadedFunc = PyCFunction_New(
        &pyOnModelLoadedDef,
        reinterpret_cast<PyObject*>(this)
    );

    static PyObject* loadResourceListBGMethodName = PyString_FromString("loadResourceListBG");
    PyObject* res = PyObject_CallMethodObjArgs(Py::BigWorld, loadResourceListBGMethodName, pyLoadModels, pyOnModelLoadedFunc, NULL);

	Py_DECREF(pyLoadModels);

    models_for_bg_loading.clear();
}
