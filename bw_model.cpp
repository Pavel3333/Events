#include "bw_model.h"
#include "common.h"
#include "py_imports.h"


using namespace BW;


Model::Model(PyObject* model, Vector3D* pos)
{
    this->model = model;
    SetPosition(pos);
}

Model::~Model()
{
    delete this->pos;

    if (!inited) {
        Py_DECREF(this->model);
        return;
    }

    static PyObject* delModelMethodName = PyString_FromString("delModel");
    PyObject* result = PyObject_CallMethodObjArgs(Py::BigWorld, delModelMethodName, this->model, nullptr);

    if (result) {
        Py_DECREF(result);
        Py_DECREF(this->model);
    }
}

void Model::SetPosition(Vector3D* pos)
{
    this->pos = pos;

    PyObject* pyPosition = PyTuple_New(3);
    PyTuple_SET_ITEM(pyPosition, 0, PyFloat_FromDouble(pos->x));
    PyTuple_SET_ITEM(pyPosition, 1, PyFloat_FromDouble(pos->y));
    PyTuple_SET_ITEM(pyPosition, 2, PyFloat_FromDouble(pos->z));

    if (PyObject_SetAttrString(model, "position", pyPosition) == -1) {
        // TODO: log here
        return;
    }

    Py_DECREF(pyPosition);
}

void Model::Init()
{
    static PyObject* addModelMethodName = PyString_FromString("addModel");
    PyObject* result = PyObject_CallMethodObjArgs(Py::BigWorld, addModelMethodName, this->model, nullptr);

    if (result) {
        Py_DECREF(result);
        this->inited = true;
    }
}


ModelSet::ModelSet(size_t size, std::function<void()> on_created)
{
    this->size = size;
    this->models.reserve(size);
	this->on_created = on_created;

    static PyMethodDef pyOnModelLoadedDef = {
        "OnModelLoaded",
        OnModelLoaded,
        METH_VARARGS,
        ""
    };

    this->pyOnModelLoadedCallback = PyCFunction_New(
        &pyOnModelLoadedDef,
        PyLong_FromVoidPtr(static_cast<void*>(this))
    );
}


ModelSet::~ModelSet()
{
}


int ModelSet::InitAll()
{
    for (auto* model : models)
        model->Init();

    return 0;
}

int ModelSet::Add(std::string_view path, Vector3D* pos, long index)
{
    LOG_extended_debug(path.data());

    PyObject* pyPath = PyString_FromStringAndSize(path.data(), path.size());
    PyObject* pyPos = PyLong_FromVoidPtr(static_cast<void*>(pos));
    PyObject* pyIndex = PyInt_FromLong(index);

    static PyObject* m_functools = PyImport_AddModule("functools");
    static PyObject* partialMethodName = PyString_FromString("partial");
    PyObject* callback = PyObject_CallMethodObjArgs(m_functools, partialMethodName, pyOnModelLoadedCallback, pyPos, pyIndex, nullptr);

    static PyObject* fetchModelMethodName = PyString_FromString("fetchModel");
    PyObject_CallMethodObjArgs(Py::BigWorld, fetchModelMethodName, pyPath, callback, nullptr);

    return 0;
}


PyObject* ModelSet::OnModelLoaded(PyObject* selfPtr, PyObject* args)
{
    ModelSet* self = static_cast<ModelSet*>(PyLong_AsVoidPtr(selfPtr));

    PyObject* pyPos;
    long index;
    PyObject* pyModel;

    if (!PyArg_ParseTuple(args, "OlO", &pyPos, &index, &pyModel)) {
        // TODO: log here
        return nullptr;
    }

    Vector3D* pos = static_cast<Vector3D*>(PyLong_AsVoidPtr(pyPos));

    self->models[index] = new Model(pyModel, pos);
    self->now_loaded++;

    if (self->now_loaded == self->size)
        self->on_created();

    Py_RETURN_NONE;
}
