
#include <Python.h>
#include "intset.h"

#include <structmember.h>

typedef struct{
    PyObject_HEAD
    IntSetIter * iter;
}IterObject;

static PyObject * iter_iternext(IterObject *iter_obj){
    int stopped = 0;

    long val = intsetiter_next(iter_obj->iter, &stopped);
    if(stopped==1){
        return NULL;
    }else{
        return PyInt_FromLong(val);
    }
}

static void iter_dealloc(IterObject *iter_obj){
    free(iter_obj->iter);
    iter_obj->iter= NULL;
    PyObject_Del(iter_obj);
}


static PyMethodDef iter_methods[] = {
    {NULL}
};

static PyTypeObject IterObject_Type ={
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "intset_iterator",                          /* tp_name */
    sizeof(IterObject),                         /* tp_basicsize */
    0,                                          /* tp_itemsize */
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
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)iter_iternext,                /* tp_iternext */
    iter_methods,                               /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
};



static PyTypeObject IntSetObject_Type;

#define IntSet_Check(obj)   (Py_TYPE(obj) == &IntSetObject_Type)

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

static int set_update_internal(IntSetObject * set_obj, PyObject * other){
	if(IntSet_Check(other)){
		set_obj->intset = intset_copy(((IntSetObject *)other)->intset);
		return 0;
	}
	int count = 0;
	if(PySequence_Check(other)){
		count = PySequence_Length(other);
	}else if(PyAnySet_Check(other)){
		count = PySet_Size(other);
	}else if(PyDict_Check(other)){
		count = PyDict_Size(other);
	}else{
		PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
		PyErr_Occurred();
	}
	long data[count];
	set_obj->intset = intset_new();

	PyObject * it , *key;
	it =  PyObject_GetIter(other);
	if(it == NULL)return -1;
	for(int i=0;(key = PyIter_Next(it))!= NULL&&i < count;i++){
		long x = PyInt_AsLong(key);
		data[i] = x;
		Py_DECREF(key);
	}
	Py_DECREF(it);
	intset_add_array(set_obj->intset, data, count);
    return 0;
};


static int set_init(IntSetObject * set_obj, PyObject *args, PyObject *kwds){
    PyObject *iterable = NULL;
    if(!_PyArg_NoKeywords("IntSet()", kwds))return -1;
    if(!PyArg_UnpackTuple(args, Py_TYPE(set_obj)->tp_name, 0, 1, &iterable))return -1;
	if(iterable == NULL){
		set_obj->intset = intset_new();
		return 0;
	}
    return set_update_internal(set_obj, iterable);
};


static PyObject * set_iter(IntSetObject *set_obj){
    IterObject * iter_obj = PyObject_New(IterObject, &IterObject_Type);
    if(iter_obj == NULL)return NULL;
    iter_obj->iter = intset_iter(set_obj->intset);
    return (PyObject *)iter_obj;
}


static PyObject* set_repr(IntSetObject * set_obj){
  PyObject *keys, *result=NULL, *listrepr;
    int status = Py_ReprEnter((PyObject*)set_obj);

    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyString_FromFormat("%s(...)", set_obj->ob_type->tp_name);
    }

    keys = PySequence_List((PyObject *)set_obj);
    if (keys == NULL)
        goto done;
    listrepr = PyObject_Repr(keys);
    Py_DECREF(keys);
    if (listrepr == NULL)
        goto done;

    result = PyString_FromFormat("%s(%s)", set_obj->ob_type->tp_name,
        PyString_AS_STRING(listrepr));
    Py_DECREF(listrepr);
done:
    Py_ReprLeave((PyObject*)set_obj);
    return result;
}


static PyObject* set_add(IntSetObject *set_obj, PyObject *obj){
	if(!PyInt_Check(obj) && !PyLong_Check(obj)){
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        PyErr_Occurred();
        return  NULL;
	}
    long x = PyInt_AsLong(obj);
    intset_add(set_obj->intset, x);
    Py_RETURN_NONE;
}

static PyObject* set_remove(IntSetObject *set_obj, PyObject *obj){
	if(!PyInt_Check(obj) && !PyLong_Check(obj)){
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        PyErr_Occurred();
        return  NULL;
	}
    long x = PyInt_AsLong(obj);
    int r = intset_remove(set_obj->intset, x);
    if(r==0){
        PyErr_Format(PyExc_KeyError, "%ld", x);
        PyErr_Occurred();
        return  NULL;
    }
    Py_RETURN_NONE;
}

static PyObject* set_discard(IntSetObject *set_obj, PyObject *obj){

 	if(!PyInt_Check(obj) && !PyLong_Check(obj)){
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        PyErr_Occurred();
        return  NULL;
	}

    long x = PyInt_AsLong(obj);
    intset_remove(set_obj->intset, x);
    Py_RETURN_NONE;
}

static Py_ssize_t set_len(PyObject *set_obj){
    int r = intset_len(((IntSetObject* )set_obj)->intset);
    return (Py_ssize_t)r;
}

static int set_contains(IntSetObject *set_obj, PyObject * obj){
 	if(!PyInt_Check(obj) && !PyLong_Check(obj)){
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        PyErr_Occurred();
        return  -1;
	}
    return intset_has(set_obj->intset, PyInt_AsLong(obj));
}



static PyObject * set_max(IntSetObject * set_obj){
	int error;
	long result = intset_max(set_obj->intset, &error);
	if(error){
		PyErr_Format(PyExc_ValueError, "intset is empty");
		PyErr_Occurred();
		return NULL;
	}
	return PyInt_FromLong(result);
}

static PyObject * set_min(IntSetObject * set_obj){
	int error;
	long result = intset_min(set_obj->intset, &error);
	if(error!=0){
		PyErr_Format(PyExc_ValueError, "intset is empty");
		PyErr_Occurred();
		return NULL;
	}
	return PyInt_FromLong(result);
}

static PyObject * set_or(IntSetObject *set_obj, PyObject * other){
    IntSet* s = intset_or(set_obj->intset, ((IntSetObject *)other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_and(IntSetObject *set_obj, PyObject * other){
    IntSet* s = intset_and(set_obj->intset, ((IntSetObject *)other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_sub(IntSetObject *set_obj, PyObject * other){
    IntSet* s = intset_sub(set_obj->intset, ((IntSetObject *)other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_xor(IntSetObject *set_obj, PyObject * other){
    IntSet* s = intset_xor(set_obj->intset, ((IntSetObject *)other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_union(IntSetObject *set_obj, PyObject * other){
	if(!IntSet_Check(other)){
		PyErr_Format(PyExc_TypeError, "args must be IntSet type");
		PyErr_Occurred();
        return NULL;
	}
    IntSet* s = intset_or(set_obj->intset, ((IntSetObject *)other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_update(IntSetObject *set_obj, PyObject * other){
	Py_RETURN_NONE;
}

static PyObject * set_intersection(IntSetObject *set_obj, PyObject * other){
	if(!IntSet_Check(other)){
		PyErr_Format(PyExc_TypeError, "args must be IntSet type");
		PyErr_Occurred();
        return NULL;
	}
    IntSet* s = intset_and(set_obj->intset, ((IntSetObject *)other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}


static PyObject * set_intersection_update(IntSetObject * set_obj, PyObject *other){
	Py_RETURN_NONE;
}

static PyObject * set_difference(IntSetObject *set_obj, PyObject * other){
	if(!IntSet_Check(other)){
		PyErr_Format(PyExc_TypeError, "args must be IntSet type");
		PyErr_Occurred();
        return NULL;
	}
    IntSet* s = intset_sub(set_obj->intset, ((IntSetObject *)other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_difference_update(IntSetObject *set_obj, PyObject * other){
	Py_RETURN_NONE;
}

static PyObject * set_symmetric_difference(IntSetObject *set_obj, PyObject * other){
	if(!IntSet_Check(other)){
		PyErr_Format(PyExc_TypeError, "args must be IntSet type");
		PyErr_Occurred();
        return NULL;
	}
    IntSet* s = intset_xor(set_obj->intset, ((IntSetObject *)other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject * set_symmetric_difference_update(IntSetObject *set_obj, PyObject * other){
	Py_RETURN_NONE;
}

static PyObject * set_clear(IntSetObject * set_obj){
	intset_clear(set_obj->intset);
	Py_RETURN_NONE;
}

static PyObject * set_copy(IntSetObject * set_obj){
    IntSet * set = intset_copy(set_obj->intset);
    return make_new_set(Py_TYPE(set_obj), set);
}

static PyObject * set_issubset(IntSetObject * set_obj, PyObject * other){
    if(!IntSet_Check(other)){
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(other)->tp_name);
        PyErr_Occurred();
        Py_INCREF(Py_NotImplemented);
        return NULL;
    }
    int r = intset_issubset(set_obj->intset, ((IntSetObject *)other)->intset);
    if(r){
        Py_RETURN_TRUE;
    }else{
        Py_RETURN_FALSE;
    }
}

static PyObject * set_issuperset(IntSetObject * set_obj, PyObject * other){
    if(!IntSet_Check(other)){
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(other)->tp_name);
        PyErr_Occurred();
        Py_INCREF(Py_NotImplemented);
        return NULL;
    }
    int r = intset_issuperset(set_obj->intset, ((IntSetObject *)other)->intset);
    if(r){
        Py_RETURN_TRUE;
    }else{
        Py_RETURN_FALSE;
    }
}


static PyObject * set_direct_contains(IntSetObject * set_obj, PyObject *obj){
	if(!PyInt_Check(obj) && !PyLong_Check(obj)){
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        PyErr_Occurred();
        return  NULL;
	}
    long result = set_contains(set_obj, obj);
    if(result==0)return NULL;
    return PyBool_FromLong(result);
}


static PyObject * set_richcompare(IntSetObject *set_obj, PyObject *obj, int op){
    if(!IntSet_Check(obj)){
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        PyErr_Occurred();
        return NULL;
    }

	IntSetObject * other = (IntSetObject *)obj;
	switch(op){
		case Py_EQ:
            if(intset_equals(set_obj->intset, other->intset)){
                return Py_True;
            }
			return Py_False;
		case Py_NE:
           if(!intset_equals(set_obj->intset, other->intset)){
				 return Py_True;
            }
			return Py_False;
		case Py_LE:
            if(intset_issubset(set_obj->intset, other->intset))
                return Py_True;
			return Py_False;
		case Py_GE:
            if(intset_issuperset(set_obj->intset, other->intset))
                return Py_True;
			return Py_False;
		case Py_LT:
            if(intset_len(set_obj->intset)>=intset_len(other->intset))
                return Py_False;
            if(intset_issubset(set_obj->intset, other->intset))
                return Py_True;
			return Py_False;
		case Py_GT:
            if(intset_len(set_obj->intset)<=intset_len(other->intset))
                return Py_False;
            if(intset_issuperset(set_obj->intset, other->intset))
                return Py_True;
			return Py_False;
	}
	PyErr_Format(PyExc_TypeError, "not support action");
	PyErr_Occurred();
	return NULL;
}

static PyMethodDef set_methods[] = {
    {"add", (PyCFunction)set_add, METH_O, "intset add"},
    {"remove", (PyCFunction)set_remove, METH_O, "intset remove"},
    {"discard", (PyCFunction)set_discard, METH_O, "intset discard"},
    {"max", (PyCFunction)set_max, METH_NOARGS, "intset max"},
    {"min", (PyCFunction)set_min, METH_NOARGS, "intset min"},
    {"clear", (PyCFunction)set_clear, METH_NOARGS, "intset clear"},
    {"copy", (PyCFunction)set_copy, METH_NOARGS, "intset copy"},
    {"issubset", (PyCFunction)set_issubset, METH_O, "intset issubset"},
    {"issuperset", (PyCFunction)set_issuperset, METH_O, "intset issuperset"},
    {"intersection", (PyCFunction)set_intersection, METH_O, "intset intersection"},
    {"intersection_update", (PyCFunction)set_intersection_update, METH_O, "intset intersection_update"},
    {"difference", (PyCFunction)set_difference, METH_O, "intset difference"},
    {"difference_update", (PyCFunction)set_difference_update, METH_O, "intset difference_update"},
    {"union", (PyCFunction)set_union, METH_O, "intset union"},
    {"update", (PyCFunction)set_update, METH_O, "intset update"},
    {"symmetric_difference", (PyCFunction)set_symmetric_difference, METH_O, "intset symmetric_difference"},
    {"symmetric_difference_update", (PyCFunction)set_symmetric_difference_update, METH_O, "intset symmetric_difference_update"},
    {"__contains__",(PyCFunction)set_direct_contains, METH_O , "intset __contains__"},
    {NULL}
};
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
    0,                                  /*nb_nonzero*/
    0,                                  /*nb_invert*/
    0,                                  /*nb_lshift*/
    0,                                  /*nb_rshift*/
    (binaryfunc)set_and,                /*nb_and*/
    (binaryfunc)set_xor,                /*nb_xor*/
    (binaryfunc)set_or,                 /*nb_or*/
    0,                                  /*nb_coerce*/
    0,                                  /*nb_int*/
    0,                                  /*nb_long*/
    0,                                  /*nb_float*/
    0,                                  /*nb_oct*/
    0,                                  /*nb_hex*/
    0,                                  /*nb_inplace_add*/
    0,               /*nb_inplace_subtract*/
    0,                                  /*nb_inplace_multiply*/
    0,                                  /*nb_inplace_divide*/
    0,                                  /*nb_inplace_remainder*/
    0,                                  /*nb_inplace_power*/
    0,                                  /*nb_inplace_lshift*/
    0,                                  /*nb_inplace_rshift*/
    0,               /*nb_inplace_and*/
    0,               /*nb_inplace_xor*/
    0,                /*nb_inplace_or*/
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
    (reprfunc)set_repr,                         /* tp_repr */
    &set_as_number,                         /* tp_as_number */
    &set_as_sequence,           /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    PyObject_HashNotImplemented,  /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    PyObject_GenericGetAttr,   /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
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
