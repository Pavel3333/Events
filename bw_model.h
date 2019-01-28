#pragma once
#include <memory>
#include <string>
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
        ~Vector3D() {
            x = y = z = 0;
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
		
		void Init();
		static Model* Open(const std::string& path, Vector3D* pos);
    };
}
