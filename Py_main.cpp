#include "common.h"
#include "py_imports.h"
#include "bw_model.h"


BW::ModelSet* modelSet = nullptr;


int init_models()
{
    return modelSet->InitAll();
}


void on_created()
{
	init_models();
}


int create_models()
{
    modelSet = new BW::ModelSet(3, on_created);

    if (modelSet->Add("demo1.model", new BW::Vector3D(0.0, 0.0, 0.0), 0))
        return -1;

    if (modelSet->Add("demo2.model", new BW::Vector3D(0.0, 0.0, 1.0), 1))
        return -1;

    if (modelSet->Add("demo3.model", new BW::Vector3D(1.0, 0.0, 0.0), 2))
        return -1;

    return 0;
}


int delete_models()
{
    delete modelSet;
    modelSet = nullptr;

    return 0;
}


PyMODINIT_FUNC initevent()
{
    Py::init_imports();

	create_models();
}
