#include <Python.h>
#include "intset.h"

#include <structmember.h>
#include "number.h"


#define MIN(a,b) (a>b?b:a)
#define ABS(x) ((x) < 0 ? -(x) : (x))

Number* PyInt_AsNumber(PyObject *obj){
    if(PyInt_Check(obj)){
        return number_from_long(PyInt_AsLong(obj));
    }else if(PyLong_Check(obj)){
    //    PyLongObject* long_o = (PyLongObject *)obj;
    //    int size = Py_SIZE(long_o);
    //    Number * n = number_new(ABS(size));
    //    for(int i=0;i<size;i++){
    //        n->digits[i] = long_o->ob_digit[i];
    //    }
    }else{
        PyErr_Format(PyExc_TypeError, "require int or long");
    }
}

PyObject* PyInt_FromNumber(Number *obj){
    return PyInt_FromLong(number_as_long(obj));
}

typedef struct {
    PyObject_HEAD;
    IntSetIter *iter;
} IterObject;

static PyObject *iter_iternext(IterObject *iter_obj) {
    int stopped = 0;

    Number* val = intsetiter_next(iter_obj->iter, &stopped);
    if (stopped == 1) {
        return NULL;
    } else {
        PyObject * r = PyInt_FromNumber(val);
        number_clear(val);
        return r;
    }
}


static void iter_dealloc(IterObject *iter_obj) {
    free(iter_obj->iter);
    iter_obj->iter = NULL;
    PyObject_Del(iter_obj);
}


static PyMethodDef iter_methods[] = {
        {NULL}
};

static PyTypeObject IterObject_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "intset_iterator",                          /* tp_name */
        sizeof(IterObject),                         /* tp_basicsize */
        0,                                          /* tp_itemsize */
        (destructor) iter_dealloc,                   /* tp_dealloc */
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
        (iternextfunc) iter_iternext,                /* tp_iternext */
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

#define IntSetObj_Check(obj)   (Py_TYPE(obj) == &IntSetObject_Type)

#define Allow_Type_Check(other) (PyAnySet_Check(other) \
                                || PySequence_Check(other) \
                                || PyDict_Check(other))
#define Free_IntSet(set) intset_clear(set);free(set);


typedef struct {
    PyObject_HEAD
            IntSet * intset;
} IntSetObject;


static void set_dealloc(IntSetObject *set_obj) {

    Free_IntSet(set_obj->intset)
    Py_TYPE(set_obj)->tp_free(set_obj);
};


static PyObject *make_new_set(PyTypeObject *type, IntSet *intset) {
    IntSetObject *set_obj;
    set_obj = (IntSetObject *) type->tp_alloc(type, 0);
    if (set_obj == NULL)return NULL;
    set_obj->intset = intset;
    return (PyObject *) set_obj;
}

static PyObject *set_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    return make_new_set(type, NULL);
};


IntSet *get_intset_from_obj(PyObject *obj) {
    int count = 0;
    if (PySequence_Check(obj)) {
        count = PySequence_Length(obj);
    } else if (PyAnySet_Check(obj)) {
        count = PySet_Size(obj);
    } else if (PyDict_Check(obj)) {
        count = PyDict_Size(obj);
    }
    int buffer_size = 1024;
    Number* buffer[buffer_size];
    PyObject *it, *key;
    it = PyObject_GetIter(obj);
    IntSet *intset = intset_new();
    if (it == NULL)return intset;
    while (count > 0) {
        for (int i = 0; i < buffer_size && (key = PyIter_Next(it)) != NULL; i++) {
            if(!PyInt_Check(key) && !PyLong_Check(key)){
                Py_DECREF(key);
                PyErr_Format(PyExc_TypeError, "a Integer is required");
                Py_DECREF(it);
                return intset;
            }
            Number * x = PyInt_AsNumber(key);
            buffer[i] = x;
            Py_DECREF(key);
        }
        if (count < buffer_size) {
            intset_add_array(intset, buffer, count);
        } else {
            intset_add_array(intset, buffer, buffer_size);
        }
        for(int i=0;i<MIN(count, buffer_size);i++){
            number_clear(buffer[i]);
        }
        count -= buffer_size;
    }
    Py_DECREF(it);
    return intset;
};


static int set_init(IntSetObject *set_obj, PyObject *args, PyObject *kwds) {
    PyObject *iterable = NULL;
    if (!_PyArg_NoKeywords("IntSet()", kwds))return -1;
    if (!PyArg_UnpackTuple(args, Py_TYPE(set_obj)->tp_name, 0, 1, &iterable))return -1;
    if (iterable == NULL) {
        set_obj->intset = intset_new();
        return 0;
    }
    if (IntSetObj_Check(iterable)) {
        set_obj->intset = intset_copy(((IntSetObject *) iterable)->intset);
        return 0;
    }
    if (Allow_Type_Check(iterable)) {
        IntSet *intset = get_intset_from_obj(iterable);
        if(PyErr_Occurred()){
            Py_INCREF(set_obj);
            return -1;
        }
        set_obj->intset = intset;
        return 0;
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(iterable)->tp_name);
    Py_INCREF(set_obj);
    return -1;
};


static PyObject *set_iter(IntSetObject *set_obj) {
    IterObject *iter_obj = PyObject_New(IterObject, &IterObject_Type);
    if (iter_obj == NULL)return NULL;
    iter_obj->iter = intset_iter(set_obj->intset);
    return (PyObject *) iter_obj;
}


static PyObject *set_repr(IntSetObject *set_obj) {
    PyObject *keys, *result = NULL;
    int status = Py_ReprEnter((PyObject *) set_obj);

    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyString_FromFormat("%s(...)", set_obj->ob_type->tp_name);
    }

    keys = PySequence_List((PyObject *) set_obj);
    if (keys == NULL)
        goto done;
    PyObject *listrepr = PyObject_Repr(keys);
    Py_DECREF(keys);
    if (listrepr == NULL)
        goto done;

    result = PyString_FromFormat("%s(%s)", set_obj->ob_type->tp_name,
                                 PyString_AS_STRING(listrepr));
    Py_DECREF(listrepr);
    done:
    Py_ReprLeave((PyObject *) set_obj);
    return result;
}


static PyObject *set_add(IntSetObject *set_obj, PyObject *obj) {
    if (!PyInt_Check(obj) && !PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
    Number *x= PyInt_AsNumber(obj);
    intset_add(set_obj->intset, x);
    number_clear(x);
    Py_RETURN_NONE;
}

static PyObject *set_remove(IntSetObject *set_obj, PyObject *obj) {
    if (!PyInt_Check(obj) && !PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
    Number * x = PyInt_AsNumber(obj);
    int r = intset_remove(set_obj->intset, x);
    number_clear(x);
    if (r == 0) {
        PyErr_Format(PyExc_KeyError, "TODO");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *set_discard(IntSetObject *set_obj, PyObject *obj) {

    if (!PyInt_Check(obj) && !PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }

    Number * x = PyInt_AsNumber(obj);
    intset_remove(set_obj->intset, x);
    number_clear(x);
    Py_RETURN_NONE;
}

static Py_ssize_t set_len(PyObject *set_obj) {
    int r = intset_len(((IntSetObject *) set_obj)->intset);
    return (Py_ssize_t) r;
}

static int set_contains(IntSetObject *set_obj, PyObject *obj) {
    if (!PyInt_Check(obj) && !PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return -1;
    }
    Number *x = PyInt_AsNumber(obj);
    int r =  intset_has(set_obj->intset, x);
    number_clear(x);
    return r;
}


static PyObject *set_get_slice(IntSetObject *set_obj, Py_ssize_t ilow, Py_ssize_t ihight) {
    IntSet *intset = intset_get_slice(set_obj->intset, ilow, ihight);
    return make_new_set(Py_TYPE(set_obj), intset);
}

static PyObject *set_get_item(IntSetObject *set_obj, Py_ssize_t i) {
    int error;
    Number* x = intset_get_item(set_obj->intset, i, &error);
    if (error != 0) {
        PyErr_Format(PyExc_KeyError, "%ld", i);
        return NULL;
    }
    return PyInt_FromNumber(x);
}


static PyObject *set_max(IntSetObject *set_obj) {
    int error;
    Number * result = intset_max(set_obj->intset, &error);
    if (error) {
        PyErr_Format(PyExc_ValueError, "intset is empty");
        return NULL;
    }
    return PyInt_FromNumber(result);
}

static PyObject *set_min(IntSetObject *set_obj) {
    int error;
    Number* result = intset_min(set_obj->intset, &error);
    if (error != 0) {
        PyErr_Format(PyExc_ValueError, "intset is empty");
        return NULL;
    }
    return PyInt_FromNumber(result);
}

static PyObject *set_or(IntSetObject *set_obj, PyObject *other) {
    IntSet *s = intset_or(set_obj->intset, ((IntSetObject *) other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject *set_and(IntSetObject *set_obj, PyObject *other) {
    IntSet *s = intset_and(set_obj->intset, ((IntSetObject *) other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject *set_sub(IntSetObject *set_obj, PyObject *other) {
    IntSet *s = intset_sub(set_obj->intset, ((IntSetObject *) other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject *set_xor(IntSetObject *set_obj, PyObject *other) {
    IntSet *s = intset_xor(set_obj->intset, ((IntSetObject *) other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject *set_union(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_or(set_obj->intset, ((IntSetObject *) other)->intset);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if(PyErr_Occurred()){
            Free_IntSet(set);
            return NULL;
        }

        IntSet *rs = intset_or(set_obj->intset, set);
        Free_IntSet(set);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
    return NULL;
}

static PyObject *set_update(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        intset_merge(set_obj->intset, ((IntSetObject *) other)->intset);
        Py_RETURN_NONE;
    }
    if (Allow_Type_Check(other)) {
        IntSet *intset = get_intset_from_obj(other);
        if(PyErr_Occurred()){
            Free_IntSet(intset);
            return NULL;
        }

        intset_merge(set_obj->intset, intset);
        Free_IntSet(intset)
        Py_RETURN_NONE;
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
    return NULL;
}

static PyObject *set_intersection(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_and(set_obj->intset, ((IntSetObject *) other)->intset);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if(PyErr_Occurred()){
            Free_IntSet(set);
            return NULL;
        }
        IntSet *rs = intset_and(set_obj->intset, set);
        Free_IntSet(set);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
    return NULL;
}


static PyObject *set_intersection_update(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_and(set_obj->intset, ((IntSetObject *) other)->intset);
        Free_IntSet(set_obj->intset);
        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if(PyErr_Occurred()){
            Free_IntSet(set);
            return NULL;
        }
        IntSet *rs = intset_and(set_obj->intset, set);

        Free_IntSet(set);
        Free_IntSet(set_obj->intset);

        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
    return NULL;
}

static PyObject *set_difference(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_sub(set_obj->intset, ((IntSetObject *) other)->intset);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if(PyErr_Occurred()){
            Free_IntSet(set);
            return NULL;
        }

        IntSet *rs = intset_sub(set_obj->intset, set);
        Free_IntSet(set);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
    return NULL;
}

static PyObject *set_difference_update(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_sub(set_obj->intset, ((IntSetObject *) other)->intset);
        Free_IntSet(set_obj->intset);
        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if(PyErr_Occurred()){
            Free_IntSet(set);
            return NULL;
        }

        IntSet *rs = intset_sub(set_obj->intset, set);

        Free_IntSet(set);
        Free_IntSet(set_obj->intset);

        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
    return NULL;
}

static PyObject *set_symmetric_difference(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_xor(set_obj->intset, ((IntSetObject *) other)->intset);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if(PyErr_Occurred()){
            Free_IntSet(set);
            return NULL;
        }

        IntSet *rs = intset_xor(set_obj->intset, set);
        Free_IntSet(set);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
    return NULL;
}

static PyObject *set_symmetric_difference_update(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_xor(set_obj->intset, ((IntSetObject *) other)->intset);
        Free_IntSet(set_obj->intset);
        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if(PyErr_Occurred()){
            Free_IntSet(set);
            return NULL;
        }

        IntSet *rs = intset_xor(set_obj->intset, set);

        Free_IntSet(set);
        Free_IntSet(set_obj->intset);

        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    PyErr_Format(PyExc_TypeError, "args type %s is not support", Py_TYPE(other)->tp_name);
    return NULL;
}

static PyObject *set_clear(IntSetObject *set_obj) {
    intset_clear(set_obj->intset);
    Py_RETURN_NONE;
}

static PyObject *set_copy(IntSetObject *set_obj) {
    IntSet *set = intset_copy(set_obj->intset);
    return make_new_set(Py_TYPE(set_obj), set);
}

static PyObject *set_issubset(IntSetObject *set_obj, PyObject *other) {
    if (!IntSetObj_Check(other)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(other)->tp_name);
        return NULL;
    }
    int r = intset_issubset(set_obj->intset, ((IntSetObject *) other)->intset);
    if (r) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject *set_issuperset(IntSetObject *set_obj, PyObject *other) {
    if (!IntSetObj_Check(other)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(other)->tp_name);
        return NULL;
    }
    int r = intset_issuperset(set_obj->intset, ((IntSetObject *) other)->intset);
    if (r) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}


static PyObject *set_direct_contains(IntSetObject *set_obj, PyObject *obj) {
    if (!PyInt_Check(obj) && !PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
    long result = set_contains(set_obj, obj);
    if (result == 0)return NULL;
    return PyBool_FromLong(result);
}


static PyObject *set_richcompare(IntSetObject *set_obj, PyObject *obj, int op) {
    if (!IntSetObj_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }

    IntSetObject *other = (IntSetObject *) obj;
    switch (op) {
        case Py_EQ:
            if (intset_equals(set_obj->intset, other->intset)) {
                return Py_True;
            }
            return Py_False;
        case Py_NE:
            if (!intset_equals(set_obj->intset, other->intset)) {
                return Py_True;
            }
            return Py_False;
        case Py_LE:
            if (intset_issubset(set_obj->intset, other->intset))
                return Py_True;
            return Py_False;
        case Py_GE:
            if (intset_issuperset(set_obj->intset, other->intset))
                return Py_True;
            return Py_False;
        case Py_LT:
            if (intset_len(set_obj->intset) >= intset_len(other->intset))
                return Py_False;
            if (intset_issubset(set_obj->intset, other->intset))
                return Py_True;
            return Py_False;
        case Py_GT:
            if (intset_len(set_obj->intset) <= intset_len(other->intset))
                return Py_False;
            if (intset_issuperset(set_obj->intset, other->intset))
                return Py_True;
            return Py_False;
    }
    PyErr_Format(PyExc_TypeError, "not support action");
    return NULL;
}

static PyMethodDef set_methods[] = {
        {"add",                         (PyCFunction) set_add,                         METH_O,      "intset add"},
        {"remove",                      (PyCFunction) set_remove,                      METH_O,      "intset remove"},
        {"discard",                     (PyCFunction) set_discard,                     METH_O,      "intset discard"},
        {"max",                         (PyCFunction) set_max,                         METH_NOARGS, "intset max"},
        {"min",                         (PyCFunction) set_min,                         METH_NOARGS, "intset min"},
        {"clear",                       (PyCFunction) set_clear,                       METH_NOARGS, "intset clear"},
        {"copy",                        (PyCFunction) set_copy,                        METH_NOARGS, "intset copy"},
        {"issubset",                    (PyCFunction) set_issubset,                    METH_O,      "intset issubset"},
        {"issuperset",                  (PyCFunction) set_issuperset,                  METH_O,      "intset issuperset"},
        {"intersection",                (PyCFunction) set_intersection,                METH_O,      "intset intersection"},
        {"intersection_update",         (PyCFunction) set_intersection_update,         METH_O,      "intset intersection_update"},
        {"difference",                  (PyCFunction) set_difference,                  METH_O,      "intset difference"},
        {"difference_update",           (PyCFunction) set_difference_update,           METH_O,      "intset difference_update"},
        {"union",                       (PyCFunction) set_union,                       METH_O,      "intset union"},
        {"update",                      (PyCFunction) set_update,                      METH_O,      "intset update"},
        {"symmetric_difference",        (PyCFunction) set_symmetric_difference,        METH_O,      "intset symmetric_difference"},
        {"symmetric_difference_update", (PyCFunction) set_symmetric_difference_update, METH_O,      "intset symmetric_difference_update"},
        {"__contains__",                (PyCFunction) set_direct_contains,             METH_O,      "intset __contains__"},
        {NULL}
};
static PyNumberMethods set_as_number = {
        0,                                  /*nb_add*/
        (binaryfunc) set_sub,                /*nb_subtract*/
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
        (binaryfunc) set_and,                /*nb_and*/
        (binaryfunc) set_xor,                /*nb_xor*/
        (binaryfunc) set_or,                 /*nb_or*/
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
        (ssizeargfunc) set_get_item,                                  /* sq_item */
        (ssizessizeargfunc) set_get_slice,                                  /* sq_slice */
        0,                                  /* sq_ass_item */
        0,   /* sq_ass_slice */
        (objobjproc) set_contains,           /* sq_contains */
};

static PyTypeObject IntSetObject_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "IntSet",             /* tp_name */
        sizeof(IntSetObject),             /* tp_basicsize */
        0,                         /* tp_itemsize */
        (destructor) set_dealloc, /* tp_dealloc */
        0,                         /* tp_print */
        0,                         /* tp_getattr */
        0,                         /* tp_setattr */
        0,                         /* tp_compare */
        (reprfunc) set_repr,                         /* tp_repr */
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
        (richcmpfunc) set_richcompare,                         /* tp_richcompare */
        0,                         /* tp_weaklistoffset */
        (getiterfunc) set_iter,     /* tp_iter */
        0,                         /* tp_iternext */
        set_methods,               /* tp_methods */
        0,               /* tp_members */
        0,                         /* tp_getset */
        0,                         /* tp_base */
        0,                         /* tp_dict */
        0,                         /* tp_descr_get */
        0,                         /* tp_descr_set */
        0,                         /* tp_dictoffset */
        (initproc) set_init,        /* tp_init */
        PyType_GenericAlloc,       /* tp_alloc */
        set_new,                   /* tp_new */
};

static PyMethodDef module_methods[] = {
        {NULL}  /* Sentinel */
};

PyMODINIT_FUNC initintset(void) {
    PyObject *m;
    IntSetObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&IntSetObject_Type) < 0)
        return;

    m = Py_InitModule3("intset", module_methods,
                       "Example module that creates an extension type.");

    Py_INCREF(&IntSetObject_Type);
    PyModule_AddObject(m, "IntSet", (PyObject * ) & IntSetObject_Type);
};
