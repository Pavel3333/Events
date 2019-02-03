#include "common.h"
#include "py_imports.h"
#include "bw_model.h"


static BW::ModelSet* modelSet = nullptr;


static void on_created()
{
    LOG_extended_debug("On created");

    modelSet->InitAll();
}


static PyObject* create_models(PyObject* self, PyObject* args)
{
    LOG_extended_debug("Create models");

    modelSet = new BW::ModelSet(3, on_created);

    modelSet->Add("content/Hangars/hangar_v3/enviroment/normal/lod0/hd_hv3_026_Fieldkitchen.model", new BW::Vector3D(0.0, 0.0, 0.0), 0);
    modelSet->Add("content/Hangars/hangar_v3/enviroment/normal/lod0/hd_hv3_026_Fieldkitchen.model", new BW::Vector3D(0.0, 0.0, 0.5), 1);
    modelSet->Add("content/Hangars/hangar_v3/enviroment/normal/lod0/hd_hv3_026_Fieldkitchen.model", new BW::Vector3D(0.0, 0.0, 1.0), 2);

    Py_RETURN_NONE;
}


static PyObject* delete_models(PyObject* self, PyObject* args)
{
    LOG_debug("Delete models");

    if (!modelSet)
        return nullptr;

    delete modelSet;
    modelSet = nullptr;

    Py_RETURN_NONE;
}


static PyMethodDef eventMethods[] = {
    {"create_models", create_models, METH_NOARGS, ""},
    {"delete_models", delete_models, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC initevent()
{
    (void)Py_InitModule("event", eventMethods);

    Py::init_imports();
}
