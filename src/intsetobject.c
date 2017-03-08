
#include <Python.h>
#include "intset.h"

#include <structmember.h>


typedef struct{
    PyObject_HEAD
    IntSetIter * iter;
}IterObject;

static PyObject * iter_iternext(IterObject *iter_obj){
    long val = 0;
    int stopped = 0;

    intsetiter_next(iter_obj->iter, &val, &stopped);
    if(stopped==1){
        return NULL;
    }else{
        return PyLong_FromLong(val);
    }
}


static void iter_dealloc(IterObject *iter_obj){
    free(iter_obj->iter);
    PyObject_GC_Del(iter_obj);
}

static PyMethodDef iter_methods[] = {
    {NULL}
};

PyTypeObject IterObject_Type ={
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "intset_iterator",                          /* tp_name */
    sizeof(IterObject),                         /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)iter_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,      /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)iter_iternext,                /* tp_iternext */
    iter_methods,                               /* tp_methods */
    0,                                          /* tp_members */
};

typedef struct{
    PyObject_HEAD
    IntSet * intset;
}IntSetObject;


static void set_dealloc(IntSetObject* set_obj){

    intset_clear(set_obj->intset);
    free(set_obj->intset);
    Py_TYPE(set_obj)->tp_free(set_obj);
};

static PyObject* make_new_set(PyTypeObject *type, IntSet *intset){
    IntSetObject * set_obj;
    set_obj = (IntSetObject *)type->tp_alloc(type, 0);
    if(set_obj== NULL)return NULL;
    set_obj->intset = intset;
    return (PyObject *)set_obj;
}

static PyObject* set_new(PyTypeObject *type, PyObject *args, PyObject *kwds){
    return make_new_set(type, NULL);
};

static int intset_update_internal(IntSet * intset, PyObject * iterable){

    PyObject * it, *key;
    it = PyObject_GetIter(iterable);

    if(it == NULL)return -1;

    while((key = PyIter_Next(it)) != NULL){
        long x = PyLong_AsLong(key);
        intset_add(intset, x);
        Py_DECREF(key);
    }
    Py_DECREF(it);
    if (PyErr_Occurred())return -1;
    return 0;
};


static int set_init(IntSetObject * set_obj, PyObject *args, PyObject *kwds){
    PyObject *iterable = NULL;
    if(!_PyArg_NoKeywords("IntSet()", kwds))return -1;
    if(!PyArg_UnpackTuple(args, Py_TYPE(set_obj)->tp_name, 0, 1, &iterable))return -1;

    set_obj->intset = malloc(sizeof(IntSet));
    set_obj->intset->root = NULL;

    if(iterable == NULL)return 0;
    return intset_update_internal(set_obj->intset, iterable);
};


static PyObject * set_iter(IntSetObject *set_obj){
    IterObject * iter_obj = PyObject_GC_New(IterObject, &IterObject_Type);
    if(iter_obj == NULL)return NULL;
    iter_obj->iter = intset_iter(set_obj->intset);
    _PyObject_GC_TRACK(iter_obj);
    return (PyObject *)iter_obj;
}


static PyObject* set_add(IntSetObject *set_obj, PyObject *obj){
    long x = PyLong_AsLong(obj);
    intset_add(set_obj->intset, x);
    Py_RETURN_NONE;
}

static PyObject* set_remove(IntSetObject *set_obj, PyObject *obj){

    long x = PyLong_AsLong(obj);
    int r = intset_remove(set_obj->intset, x);
    if(r==0){
        PyErr_Format(PyExc_KeyError, "%ld", x);
        PyErr_Occurred();
        return  NULL;
    }
    Py_RETURN_NONE;
}


static PyObject* set_discard(IntSetObject *set_obj, PyObject *obj){

    long x = PyLong_AsLong(obj);
    intset_remove(set_obj->intset, x);
    Py_RETURN_NONE;
}

static Py_ssize_t set_len(PyObject *set_obj){
    int r = intset_len(((IntSetObject* )set_obj)->intset);
    return (Py_ssize_t)r;
}

static int set_contains(IntSetObject *set_obj, PyObject * key){
    return intset_has(set_obj->intset, PyLong_AsLong(key));
}



static PyObject * set_max(IntSetObject * set_obj){
	long result;
	int error;
	intset_max(set_obj->intset, &result, &error);
	if(error){
		PyErr_Format(PyExc_ValueError, "intset is empty");
		PyErr_Occurred();
		return NULL;
	}
	return PyLong_FromLong(result);
}

static PyObject * set_min(IntSetObject * set_obj){
	long result;
	int error;
	intset_min(set_obj->intset, &result, &error);
	if(error!=0){
		PyErr_Format(PyExc_ValueError, "intset is empty");
		PyErr_Occurred();
		return NULL;
	}
	return PyLong_FromLong(result);
}


static PyObject * set_clear(IntSetObject * set_obj){
	intset_clear(set_obj->intset);
	Py_RETURN_NONE;
}


static PyObject * set_direct_contains(IntSetObject * set_obj, PyObject *key){
    long result;
    result = set_contains(set_obj, key);
    if(result==0)return NULL;
    return PyBool_FromLong(result);
}


static PyObject * set_richcompare(IntSetObject *set_obj, IntSetObject *other, int op){
	//TODO
	switch(op){
		case Py_EQ:
			return Py_True;
		case Py_NE:
			return Py_True;
		case Py_LE:
			return Py_True;
		case Py_GE:
			return Py_True;
		case Py_LT:
			return Py_True;
		case Py_GT:
			return Py_True;
	}
	return Py_NotImplemented;
}

static PyMethodDef set_methods[] = {
    {"add", (PyCFunction)set_add, METH_O, "intset add"},
    {"remove", (PyCFunction)set_remove, METH_O, "intset remove"},
    {"discard", (PyCFunction)set_discard, METH_O, "intset discard"},
	{"max", (PyCFunction)set_max, METH_NOARGS, "intset max"},
	{"min", (PyCFunction)set_min, METH_NOARGS, "intset min"},
	{"clear", (PyCFunction)set_clear, METH_NOARGS, "intset clear"},
    {"__contains__",(PyCFunction)set_direct_contains, METH_O , "intset __contains__"},
    {NULL}
};


static PyObject * set_or(IntSetObject *set_obj, IntSetObject * other){
    IntSet* s = intset_or(set_obj->intset, other->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_and(IntSetObject *set_obj, IntSetObject * other){
    IntSet* s = intset_and(set_obj->intset, other->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_sub(IntSetObject *set_obj, IntSetObject * other){
    IntSet* s = intset_sub(set_obj->intset, other->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyNumberMethods set_as_number = {
    0,                                  /*nb_add*/
    (binaryfunc)set_sub,                /*nb_subtract*/
    0,                                  /*nb_multiply*/
	0,                                  /*nb_divide*/
    0,                                  /*nb_remainder*/
    0,                                  /*nb_divmod*/
    0,                                  /*nb_power*/
    0,                                  /*nb_negative*/
    0,                                  /*nb_positive*/
    0,                                  /*nb_absolute*/
    0,                                  /*nb_bool*/
    0,                                  /*nb_invert*/
    0,                                  /*nb_lshift*/
    0,                                  /*nb_rshift*/
    (binaryfunc)set_and,                /*nb_and*/
    0,                /*nb_xor*/
    (binaryfunc)set_or,                 /*nb_or*/
    0,                                  /*nb_int*/
    0,                                  /*nb_reserved*/
    0,                                  /*nb_float*/
    0,                                  /*nb_inplace_add*/
    0,                                  /*nb_inplace_subtract*/
    0,                                  /*nb_inplace_multiply*/
    0,                                  /*nb_inplace_remainder*/
    0,                                  /*nb_inplace_power*/
    0,                                  /*nb_inplace_lshift*/
    0,                                  /*nb_inplace_rshift*/
    0,                                  /*nb_inplace_and*/
    0,                                  /*nb_inplace_xor*/
    0,                                  /*nb_inplace_or*/
};


static PySequenceMethods set_as_sequence = {
    set_len,                            /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    (objobjproc)set_contains,           /* sq_contains */
};

static PyTypeObject IntSetObject_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "IntSet",             /* tp_name */
    sizeof(IntSetObject),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)set_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    &set_as_number,                         /* tp_as_number */
    &set_as_sequence,           /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    PyObject_HashNotImplemented,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    PyObject_GenericGetAttr,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|
	Py_TPFLAGS_HAVE_INPLACEOPS,        /* tp_flags */
    "intset objects",          /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    (richcmpfunc)set_richcompare,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    (getiterfunc)set_iter,     /* tp_iter */
    0,                         /* tp_iternext */
    set_methods,               /* tp_methods */
    0,               /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)set_init,        /* tp_init */
    PyType_GenericAlloc,       /* tp_alloc */
    set_new,                   /* tp_new */
};

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

PyMODINIT_FUNC initintset(void){
    PyObject*m;
    IntSetObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&IntSetObject_Type) < 0)
        return;

    m = Py_InitModule3("intset", module_methods,
                       "Example module that creates an extension type.");

    Py_INCREF(&IntSetObject_Type);
    PyModule_AddObject(m, "IntSet", (PyObject *)&IntSetObject_Type);
};
