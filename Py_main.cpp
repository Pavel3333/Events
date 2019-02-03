#include "common.h"
#include "py_imports.h"
#include "bw_model.h"
#include <random>


static BW::ModelSet* modelSet = nullptr;



static PyObject* create_models(PyObject* self, PyObject* args)
{
#define TEST_COUNT 2000
#define TEST_MODEL "content/Hangars/hangar_v3/enviroment/normal/lod0/hd_hv3_026_Fieldkitchen.model"

    static std::default_random_engine rand_engine;
    static std::uniform_real_distribution<> dis(-5, 5);

    modelSet = new BW::ModelSet(TEST_COUNT, [=]() {
        modelSet->InitAll();
    });

	for (long i = 0; i < TEST_COUNT; i++) {
		modelSet->Add(
			TEST_MODEL,
			new BW::Vector3D(dis(rand_engine), 0.0, dis(rand_engine)),
			i
		);
	}

#undef TEST_MODEL
#undef TEST_COUNT

    Py_RETURN_NONE;
}


static PyObject* delete_models(PyObject* self, PyObject* args)
{
    if (modelSet) {
        delete modelSet;
        modelSet = nullptr;
    }

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
