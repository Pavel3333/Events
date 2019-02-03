#pragma once
#include <vector>
#include <string>
#include <functional>
#include "python2.7/Python.h"


namespace BW
{
    class Vector3D {
    public:
        float x, y, z;
        Vector3D(float x, float y, float z) {
            this->x = x;
            this->y = y;
            this->z = z;
        }
        Vector3D(float *arr) {
            this->x = arr[0];
            this->y = arr[1];
            this->z = arr[2];
        }
    };

    class Model {
    public:
        PyObject* model;
        Vector3D* pos;
        bool inited = false;

    public:
        Model(PyObject* model, Vector3D* pos);
        ~Model();

        void SetPosition(Vector3D* pos);
        void Init();
    };

    class ModelSet {
    private:
        size_t now_loaded = 0;
        size_t size;

        std::function<void()> on_created;
        std::vector<Model*> models;
        PyObject* pyOnModelLoadedCallback;
        static PyObject* OnModelLoaded(PyObject*, PyObject*);

    public:
        ModelSet(size_t size, std::function<void()> on_created);
        ~ModelSet();

        int InitAll();
        int Add(std::string_view path, Vector3D* pos, long index);
    };
}
