#include <Python.h>

#include "../src/intset.h"



static PyObject *intset_intset(PyObject *self, PyObject *args) {
    PyObject *obj_list;
    if (!PyArg_ParseTuple(args, "O", &obj_list))
        return NULL;
    long length = PyList_Size(obj_list);
    long *data = malloc(sizeof(long)*length);
    for (int i = 0; i < length; i++) {
        PyObject *obj = PyList_GetItem(obj_list, i);
        long x = PyLong_AsLong(obj);

        data[i] = x;
    }

    printf("hello world");
    struct IntSet *set = intset_new(data, length);
    free(data);


    return Py_BuildValue("O", set);
}

static char intset_docs[] = "create intset\n";

static PyMethodDef intset_funcs[] = {
        {"intset", (PyCFunction) intset_intset, METH_VARARGS, intset_docs},
        {NULL,     NULL,                        0,            NULL}
};

PyMODINIT_FUNC initintset(void) {
    Py_InitModule3("intset", intset_funcs, "intset");
}
