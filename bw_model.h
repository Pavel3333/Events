#pragma once
#include <vector>
#include <string>
#include <functional>
#include "python2.7/Python.h"


#define USE_ASYNC_MODEL_LOADING true


namespace BW
{
    using std::string;
    using std::to_string;
    using std::vector;
    using std::function;

    class Vector3D {
    public:
        float x, y, z;
        Vector3D(float x, float y, float z) {
            this->x = x;
            this->y = y;
            this->z = z;
        }
        Vector3D(float *arr) {
            x = arr[0];
            y = arr[1];
            z = arr[2];
        }
        string AsString() const {
            return to_string(x) + " " + to_string(y) + " " + to_string(z);
        }
    };

    class Model {
    public:
        PyObject* model;
		Vector3D* pos;

    public:
        Model(PyObject* model, Vector3D* pos);
        ~Model();

        void ShowDebugInfo(const string& title) const;

        void SetVisible(bool visible);
        void SetPosition(Vector3D* pos);

        bool IsInit() const;
        void Init();
    };

    class ModelSet {
    private:
        size_t now_loaded = 0;
        size_t size;
        vector<Model*> models;
        function<void()> on_created;

#if USE_ASYNC_MODEL_LOADING
        PyObject* pyOnModelLoadedCallback;
        static PyObject* OnModelLoaded(PyObject*, PyObject*);
        PyObject* pyOnLoadingCompletedCallback;
        static PyObject* OnLoadingCompleted(PyObject*, PyObject*);
#endif

    public:
        ModelSet(size_t size, function<void()> on_created);
        ~ModelSet();

        int InitAll();
        int Add(std::string_view path, Vector3D* pos, long index);
    };
}
