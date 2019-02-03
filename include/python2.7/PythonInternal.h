/**
* This file is part of the XVM project.
*
* Copyright (c) 2017-2018 XVM contributors.
*
* This file is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation, version 3.
*
* This file is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Python.h"

typedef struct {
    FILE *fp;
    int error;  /* see WFERR_* values */
    int depth;
    /* If fp == NULL, the following are valid: */
    PyObject *str;
    char *ptr;
    char *end;
    PyObject *strings; /* dict on marshal, list on unmarshal */
    int version;
} WFILE;

typedef WFILE RFILE;

typedef struct {
    char *name;
    unsigned char *code;
    int size;
} _frozen;

PyObject * buffer_from_memory(PyObject *, Py_ssize_t, Py_ssize_t, void *, int);
PyObject * buffer_from_object(PyObject *, Py_ssize_t, Py_ssize_t, int);
PyObject * bytearray_iter(PyObject *);
int case_ok(char *, Py_ssize_t, Py_ssize_t, char *);
PyObject * codec_getstreamcodec(const char *, PyObject *, const char *, const int);
_frozen * find_frozen(char *);
struct filedescr * find_module(char *, char *, PyObject *, char *, size_t, FILE **, PyObject **);
int is_builtin(char *name);
PyObject * load_source_module(char *, char *, FILE *);
void mywrite(char *, FILE *, const char *, va_list);
PyObject * r_object(RFILE *);
PyCodeObject * read_compiled_module(char *, FILE *);
void tstate_delete_common(PyThreadState *);
void update_code_filenames(PyCodeObject *, PyObject *, PyObject *);

