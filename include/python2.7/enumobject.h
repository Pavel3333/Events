#ifndef Py_ENUMOBJECT_H
#define Py_ENUMOBJECT_H

/* Enumerate Object */

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyEnum_Type;
#define PyEnum_Type (*PyEnum_Type)

PyAPI_DATA(PyTypeObject) PyReversed_Type;
#define PyReversed_Type (*PyReversed_Type)

#ifdef __cplusplus
}
#endif

#endif /* !Py_ENUMOBJECT_H */
