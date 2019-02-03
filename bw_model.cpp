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
    delete pos;

    if (!IsInit()) {
        Py_DECREF(model);
        return;
    }

    static PyObject* delModelMethodName = PyString_InternFromString("delModel");
    PyObject_CallMethodObjArgs(Py::BigWorld, delModelMethodName, model, nullptr);

    Py_DECREF(model);
}


void Model::ShowDebugInfo(const string& title) const
{
#if _BW_MODEL_DEBUG
    PyObject* sources = PyObject_GetAttrString(model, "sources");
    PyObject* position = PyObject_GetAttrString(model, "position");

    char* src = PyString_AsString(PyTuple_GetItem(sources, 0));

    double pos_x = PyFloat_AsDouble(PyTuple_GetItem(position, 0));
    double pos_y = PyFloat_AsDouble(PyTuple_GetItem(position, 1));
    double pos_z = PyFloat_AsDouble(PyTuple_GetItem(position, 2));

    string s = \
        "ptr: " + to_string((long)model) + "\n" +
        "refcnt: " + to_string(Py_REFCNT(model)) + "\n" +
        "source: " + src + "\n" +
        "pos in game: " + to_string(pos_x) + " " + to_string(pos_y) + " " + to_string(pos_z) + "\n" +
        "pos in pyd: " + (pos ? pos->AsString() : "null");

    MessageBoxA(nullptr, s.c_str(), title.c_str(), 0);
#endif
}

void Model::SetPosition(Vector3D* pos)
{
    this->pos = pos;

    if (!pos)
        return;

    PyObject* pyPosition = PyTuple_New(3);
    PyTuple_SET_ITEM(pyPosition, 0, PyFloat_FromDouble(pos->x));
    PyTuple_SET_ITEM(pyPosition, 1, PyFloat_FromDouble(pos->y));
    PyTuple_SET_ITEM(pyPosition, 2, PyFloat_FromDouble(pos->z));

    if (PyObject_SetAttrString(model, "position", pyPosition)) {
        ShowDebugInfo("set position failed!");
        return;
    }

    Py_DECREF(pyPosition);
}


bool Model::IsInit() const
{
    PyObject* res = PyObject_GetAttrString(model, "inWorld");
    return res == Py_True;
}

void Model::Init()
{
    if (IsInit()) {
        ShowDebugInfo("PyModel is already inited!");
        return;
    }

    static PyObject* addModelMethodName = PyString_InternFromString("addModel");
    PyObject_CallMethodObjArgs(Py::BigWorld, addModelMethodName, model, nullptr);
}

void Model::SetVisible(bool visible)
{
    PyObject_SetAttrString(model, "visible", visible ? Py_True : Py_False);
}


ModelSet::ModelSet(size_t size, function<void()> on_created)
{
    this->size = size;
    this->models.resize(size);
    std::fill(begin(this->models), end(this->models), nullptr);
    this->on_created = on_created;

#if _BW_MODEL_USE_ASYNC_LOADING

    // ну а как еще?...
    PyObject* self = PyLong_FromVoidPtr(static_cast<void*>(this));

    static PyMethodDef pyOnModelLoadedDef = {
        "OnModelLoaded", OnModelLoaded, METH_VARARGS, "" };

    pyOnModelLoadedCallback = PyCFunction_New(
        &pyOnModelLoadedDef, self);

    static PyMethodDef pyOnLoadingCompletedDef = {
        "OnLoadingCompleted", OnLoadingCompleted, METH_NOARGS, "" };

    pyOnLoadingCompletedCallback = PyCFunction_New(
        &pyOnLoadingCompletedDef, self);

#endif
}


ModelSet::~ModelSet()
{
    for (auto* model : models) {
        delete model;
    }
}


int ModelSet::InitAll()
{
    for (auto* model : models) {
        if (model) {
            model->Init();
            // model->SetVisible(true); // true by default
        }
    }

    return 0;
}

int ModelSet::Add(std::string_view path, Vector3D* pos, long index)
{
    PyObject* pyPath = PyString_FromStringAndSize(path.data(), path.size());

#if _BW_MODEL_USE_ASYNC_LOADING

    PyObject* pyPos = PyLong_FromVoidPtr(static_cast<void*>(pos));
    PyObject* pyIndex = PyInt_FromLong(index);

    static PyObject* m_functools = PyImport_ImportModule("functools");
    static PyObject* partialMethodName = PyString_InternFromString("partial");
    PyObject* callback = PyObject_CallMethodObjArgs(m_functools, partialMethodName, pyOnModelLoadedCallback, pyPos, pyIndex, nullptr);

    static PyObject* fetchModelMethodName = PyString_InternFromString("fetchModel");
    PyObject_CallMethodObjArgs(Py::BigWorld, fetchModelMethodName, pyPath, callback, nullptr);

#else

    static PyObject* ModelMethodName = PyString_InternFromString("Model");
    PyObject* pyModel = PyObject_CallMethodObjArgs(Py::BigWorld, ModelMethodName, pyPath, nullptr);

    models.at(index) = new Model(pyModel, pos);
    now_loaded++;

    if (now_loaded == size)
        on_created();

#endif

    return 0;
}


#if _BW_MODEL_USE_ASYNC_LOADING

PyObject* ModelSet::OnModelLoaded(PyObject* selfPtr, PyObject* args)
{
    PyObject* pyPos;
    long index;
    PyObject* pyModel;

    if (!PyArg_ParseTuple(args, "OlO", &pyPos, &index, &pyModel)) {
        LOG_debug("parse arg tuple failed!");
        return nullptr;
    }

    if (pyModel == Py_None) {
        LOG_debug("fetch model failed!"); // Возможно был указан неправильный путь к файлу
        return nullptr;
    }

    Vector3D* pos = static_cast<Vector3D*>(PyLong_AsVoidPtr(pyPos));

    ModelSet* self = static_cast<ModelSet*>(PyLong_AsVoidPtr(selfPtr));

    self->models.at(index) = new Model(pyModel, pos);
    self->now_loaded++;

    if (self->now_loaded == self->size) {
        static PyObject* callbackTime = PyFloat_FromDouble(0.0);
        static PyObject* callbackMethodName = PyString_InternFromString("callback");
        PyObject_CallMethodObjArgs(Py::BigWorld, callbackMethodName, callbackTime, self->pyOnLoadingCompletedCallback, nullptr);
    }

    Py_RETURN_NONE;
}

PyObject* ModelSet::OnLoadingCompleted(PyObject* selfPtr, PyObject* args)
{
    ModelSet* self = static_cast<ModelSet*>(PyLong_AsVoidPtr(selfPtr));
    self->on_created();

    Py_RETURN_NONE;
}

#endif
