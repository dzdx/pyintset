#include <Python.h>
#include <longintrepr.h>

#include <structmember.h>
#include "number.h"


#include "intset.h"

#if PY_MAJOR_VERSION >= 3
#define PY3
#endif


#define MIN(a, b) (a>b?b:a)
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define NUM_SCALE (NUM_SHIFT/PyLong_SHIFT)


#ifdef PY3
#define PyNumber_Check(op)       (PyLong_Check(op))
#else
#define PyNumber_Check(op)       (PyInt_Check(op) || PyLong_Check(op))
#endif


#ifdef PY3
#define PyString_FromFormat PyUnicode_FromFormat
#endif

Number *PyNumber_AsNumber(PyObject *obj) {
#ifndef PY3
    if (PyInt_Check(obj)) {
        return number_from_long(PyInt_AsLong(obj));
    }
#endif
    if (PyLong_Check(obj)) {
        int ob_size = Py_SIZE(obj);
        #if NUM_SCALE == 1
        int size  = ob_size;
        Number *n = number_new(size);
        PyLongObject *v = (PyLongObject *) obj;
        memcpy(n->digits, v->ob_digit, ABS(size) * sizeof(digit));
        return n;
        #else
        int size  = (ABS(ob_size)-1)/NUM_SCALE+1;
        if(ob_size<0)
            size = -size;
        Number *n = number_new(size);
        PyLongObject *v = (PyLongObject *) obj;
        for(int i=0;i<ABS(size);i++){
            num x=0;
            for(int j=0;j<NUM_SCALE;j++){
                int z = NUM_SCALE*i+j;
                if(z<ABS(ob_size)){
                    x += (((num)(v->ob_digit[z]))<<(PyLong_SHIFT*j));
                }
            }
            n->digits[i] = x;
        }
        return n;
        #endif
    } else {
        PyErr_Format(PyExc_TypeError, "require int or long");
        return NULL;
    }
}
PyLongObject *pynumber_normalize(PyLongObject *v) {
    int i = ABS(Py_SIZE(v));
    while (i > 0 && v->ob_digit[i - 1] == 0) {
        i--;
    }
    Py_SIZE(v) = (Py_SIZE(v) < 0) ? -i : i;
    return v;
}

PyObject *PyNumber_FromNumber(Number *number) {
    int size = ABS(number->size);
#ifndef PY3
    if (ABS(number->size)<=1) {
        return PyInt_FromLong(number_as_long(number));
    }
#endif
    #if NUM_SCALE == 1
    PyLongObject *v = PyObject_NEW_VAR(PyLongObject, &PyLong_Type, size);
    Py_SIZE(v) = number->size;
    memcpy(v->ob_digit, number->digits, size * sizeof(digit));
    return (PyObject *) v;
   #else
    int ob_size = size*NUM_SCALE;
    PyLongObject *v = PyObject_NEW_VAR(PyLongObject, &PyLong_Type, ob_size);
    if(number->size<0)
        ob_size = -ob_size;
    Py_SIZE(v) = ob_size;
    for(int i=0;i<size;i++){
        for(int j=0;j<NUM_SCALE;j++){
            digit x = (digit)(number->digits[i]>>(PyLong_SHIFT*j));
            v->ob_digit[NUM_SCALE*i+j] = x;
        }
    }
    v= pynumber_normalize(v);
    return (PyObject *)v;

   #endif
}

typedef struct {
    PyObject_HEAD;
    IntSetIter *iter;
} IterObject;

static PyObject *iter_iternext(IterObject *iter_obj) {
    int stopped = 0;

    Number *val = intsetiter_next(iter_obj->iter, &stopped);
    if (stopped == 1) {
        return NULL;
    } else {
        PyObject *r = PyNumber_FromNumber(val);
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
        (destructor) iter_dealloc,                  /* tp_dealloc */
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
        (iternextfunc) iter_iternext,               /* tp_iternext */
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
    Number *buffer[buffer_size];
    PyObject *it, *key;
    it = PyObject_GetIter(obj);
    IntSet *intset = intset_new();
    if (it == NULL)return intset;
    while (count > 0) {
        for (int i = 0; i < buffer_size && (key = PyIter_Next(it)) != NULL; i++) {
            if (!PyNumber_Check(key)) {
                Py_DECREF(key);
                PyErr_Format(PyExc_TypeError, "a Integer is required");
                Py_DECREF(it);
                return intset;
            }
            Number *x = PyNumber_AsNumber(key);
            buffer[i] = x;
            Py_DECREF(key);
        }
        if (count < buffer_size) {
            intset_add_array(intset, buffer, count);
        } else {
            intset_add_array(intset, buffer, buffer_size);
        }
        for (int i = 0; i < MIN(count, buffer_size); i++) {
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
        if (PyErr_Occurred()) {
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
        return PyString_FromFormat("%s(...)", Py_TYPE(set_obj)->tp_name);
    }

    keys = PySequence_List((PyObject *) set_obj);
    if (keys == NULL)
        goto done;
    PyObject *listrepr = PyObject_Repr(keys);
    Py_DECREF(keys);
    if (listrepr == NULL)
        goto done;

#ifdef PY3
    result = PyUnicode_FromFormat("%s(%U)", Py_TYPE(set_obj)->tp_name, listrepr);
#else
    result = PyString_FromFormat("%s(%s)", Py_TYPE(set_obj)->tp_name,
                                 PyString_AS_STRING(listrepr));
#endif
   Py_DECREF(listrepr);
    done:
    Py_ReprLeave((PyObject *) set_obj);
    return result;
}


static PyObject *set_add(IntSetObject *set_obj, PyObject *obj) {
    if (!PyNumber_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
    Number *x = PyNumber_AsNumber(obj);
    intset_add(set_obj->intset, x);
    number_clear(x);
    Py_RETURN_NONE;
}
PyDoc_STRVAR(add_doc, "add an Integer to a intset.\n return None.");

static PyObject *set_remove(IntSetObject *set_obj, PyObject *obj) {
    if (!PyNumber_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
    Number *x = PyNumber_AsNumber(obj);
    int r = intset_remove(set_obj->intset, x);
    number_clear(x);
    if (r == 0) {
        PyErr_SetObject(PyExc_KeyError, obj);
        return NULL;
    }
    Py_RETURN_NONE;
}
PyDoc_STRVAR(remove_doc, "remove an Integer from a intset.\n If the Integer is not a member, raise a KeyError.");

static PyObject *set_discard(IntSetObject *set_obj, PyObject *obj) {

    if (!PyNumber_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }

    Number *x = PyNumber_AsNumber(obj);
    intset_remove(set_obj->intset, x);
    number_clear(x);
    Py_RETURN_NONE;
}
PyDoc_STRVAR(discard_doc, "remove an Integer from a intset.\n If the Integer is not a member, do noting.");

static Py_ssize_t set_len(PyObject *set_obj) {
    int r = intset_len(((IntSetObject *) set_obj)->intset);
    return (Py_ssize_t) r;
}

static int set_contains(IntSetObject *set_obj, PyObject *obj) {
    if (!PyNumber_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return -1;
    }
    Number *x = PyNumber_AsNumber(obj);
    int r = intset_has(set_obj->intset, x);
    number_clear(x);
    return r;
}

static PyObject *set_get_item(IntSetObject *set_obj, Py_ssize_t i) {
    int error;
    Number *x = intset_get_item(set_obj->intset, i, &error);
    if (error != 0) {
        PyErr_Format(PyExc_KeyError, "%ld", i);
        return NULL;
    }
    PyObject *r = PyNumber_FromNumber(x);
    number_clear(x);
    return r;

}

static PyObject *set_get_slice(IntSetObject *set_obj, Py_ssize_t ilow, Py_ssize_t ihight) {
    IntSet *intset = intset_get_slice(set_obj->intset, ilow, ihight);
    return make_new_set(Py_TYPE(set_obj), intset);
}



static PyObject *set_subscript(IntSetObject *set_obj, PyObject *item) {
    if(PyIndex_Check(item)){
        Py_ssize_t i;
        i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
         if (i < 0)
            i += intset_len(set_obj->intset);
         return set_get_item(set_obj, i);
    }else if(PySlice_Check(item)){
        Py_ssize_t start, stop, step;
#ifdef PY3
        if (PySlice_GetIndices(item, intset_len(set_obj->intset),
                    &start, &stop, &step) < 0) {
#else
        if (PySlice_GetIndices((PySliceObject*)item, intset_len(set_obj->intset),
                    &start, &stop, &step) < 0) {
#endif
            return NULL;
        }
        if(step==1){
            return set_get_slice(set_obj, start, stop);
        }else{
            PyErr_Format(PyExc_KeyError, "only support step=1");
            return NULL;
        }
    }else{
         PyErr_Format(PyExc_TypeError,
                 "intset indices must by integers, not %.200s", Py_TYPE(item)->tp_name);
         return NULL;
    }
}




static PyObject *set_max(IntSetObject *set_obj) {
    int error;
    Number *result = intset_max(set_obj->intset, &error);
    if (error) {
        PyErr_Format(PyExc_ValueError, "intset is empty");
        return NULL;
    }
    PyObject *r = PyNumber_FromNumber(result);
    number_clear(result);
    return r;
}
PyDoc_STRVAR(max_doc, "Get the max Integer in a intset.\n If the intset is empty, raise a ValueError.");

static PyObject *set_min(IntSetObject *set_obj) {
    int error;
    Number *result = intset_min(set_obj->intset, &error);
    if (error != 0) {
        PyErr_Format(PyExc_ValueError, "intset is empty");
        return NULL;
    }
    PyObject *r = PyNumber_FromNumber(result);
    number_clear(result);
    return r;
}
PyDoc_STRVAR(min_doc, "Get the min Integer in a intset.\n If the intset is empty, raise a ValueError.");

static PyObject *set_or(IntSetObject *set_obj, PyObject *other) {
    if(!IntSetObj_Check(other)){
        PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for |: 'IntSet' and '%s'", Py_TYPE(other)->tp_name);
        return NULL;
    }
    IntSet *s = intset_or(set_obj->intset, ((IntSetObject *) other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject *set_and(IntSetObject *set_obj, PyObject *other) {
    if(!IntSetObj_Check(other)){
        PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for &: 'IntSet' and '%s'", Py_TYPE(other)->tp_name);
        return NULL;
    }
    IntSet *s = intset_and(set_obj->intset, ((IntSetObject *) other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject *set_sub(IntSetObject *set_obj, PyObject *other) {
    if(!IntSetObj_Check(other)){
        PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for -: 'IntSet' and '%s'", Py_TYPE(other)->tp_name);
        return NULL;
    }

    IntSet *s = intset_sub(set_obj->intset, ((IntSetObject *) other)->intset);
    return make_new_set(Py_TYPE(set_obj), s);
}

static PyObject *set_xor(IntSetObject *set_obj, PyObject *other) {
    if(!IntSetObj_Check(other)){
        PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for ^: 'IntSet' and '%s'", Py_TYPE(other)->tp_name);
        return NULL;
    }
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
        if (PyErr_Occurred()) {
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

PyDoc_STRVAR(union_doc, "Return the union of an intset and an iterable object as a new intset.\n");

static PyObject *set_update(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        intset_merge(set_obj->intset, ((IntSetObject *) other)->intset);
        Py_RETURN_NONE;
    }
    if (Allow_Type_Check(other)) {
        IntSet *intset = get_intset_from_obj(other);
        if (PyErr_Occurred()) {
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
PyDoc_STRVAR(update_doc, "update a intset with the union of itself and other iterable object.\n");

static PyObject *set_intersection(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_and(set_obj->intset, ((IntSetObject *) other)->intset);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if (PyErr_Occurred()) {
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
PyDoc_STRVAR(intersection_doc, "Return the intersection of an intset and an iterable object as a new intset.\n");


static PyObject *set_intersection_update(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_and(set_obj->intset, ((IntSetObject *) other)->intset);
        Free_IntSet(set_obj->intset);
        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if (PyErr_Occurred()) {
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
PyDoc_STRVAR(intersection_update_doc, "update a intset with the intersection of itself and other iterable object.\n");

static PyObject *set_difference(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_sub(set_obj->intset, ((IntSetObject *) other)->intset);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if (PyErr_Occurred()) {
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
PyDoc_STRVAR(difference_doc, "Return the difference of an intset and an iterable object as a new intset.\n");

static PyObject *set_difference_update(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_sub(set_obj->intset, ((IntSetObject *) other)->intset);
        Free_IntSet(set_obj->intset);
        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if (PyErr_Occurred()) {
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
PyDoc_STRVAR(difference_update_doc, "update a intset with the difference of itself and other iterable object.\n");

static PyObject *set_symmetric_difference(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_xor(set_obj->intset, ((IntSetObject *) other)->intset);
        return make_new_set(Py_TYPE(set_obj), rs);
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if (PyErr_Occurred()) {
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
PyDoc_STRVAR(symmetric_difference_doc, "Return the symmetric_difference of an intset and an iterable object as a new intset.\n");

static PyObject *set_symmetric_difference_update(IntSetObject *set_obj, PyObject *other) {
    if (IntSetObj_Check(other)) {
        IntSet *rs = intset_xor(set_obj->intset, ((IntSetObject *) other)->intset);
        Free_IntSet(set_obj->intset);
        set_obj->intset = rs;
        Py_RETURN_NONE;
    }
    if (Allow_Type_Check(other)) {
        IntSet *set = get_intset_from_obj(other);
        if (PyErr_Occurred()) {
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
PyDoc_STRVAR(symmetric_difference_update_doc, "update a intset with the symmetric_difference of itself and other iterable object.\n");

static PyObject *set_clear(IntSetObject *set_obj) {
    intset_clear(set_obj->intset);
    Py_RETURN_NONE;
}
PyDoc_STRVAR(clear_doc, "Remove all elements from this intset.");


static PyObject *set_copy(IntSetObject *set_obj) {
    IntSet *set = intset_copy(set_obj->intset);
    return make_new_set(Py_TYPE(set_obj), set);
}
PyDoc_STRVAR(copy_doc, "Return a copy of a intset.");


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
PyDoc_STRVAR(issubset_doc, "Report whether another intset contains this intset.");


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
PyDoc_STRVAR(issuperset_doc, "Report whether this intset contains another intset.");


static PyObject *set_direct_contains(IntSetObject *set_obj, PyObject *obj) {
    if (!PyNumber_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
    long result = set_contains(set_obj, obj);
    if (result == 0)return NULL;
    return PyBool_FromLong(result);
}
PyDoc_STRVAR(contains_doc, "x.__contains__(y) <==> y in x.");


static PyObject *set_richcompare(IntSetObject *set_obj, PyObject *obj, int op) {
    if (!IntSetObj_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }

    IntSetObject *other = (IntSetObject *) obj;
    switch (op) {
        case Py_EQ:
            if (intset_equals(set_obj->intset, other->intset)) {
                Py_RETURN_TRUE;
            }
            Py_RETURN_FALSE;
        case Py_NE:
            if (!intset_equals(set_obj->intset, other->intset)) {
                Py_RETURN_TRUE;
            }
            Py_RETURN_FALSE;
        case Py_LE:
            if (intset_issubset(set_obj->intset, other->intset))
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        case Py_GE:
            if (intset_issuperset(set_obj->intset, other->intset))
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        case Py_LT:
            if (intset_len(set_obj->intset) >= intset_len(other->intset))
                Py_RETURN_FALSE;
            if (intset_issubset(set_obj->intset, other->intset))
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        case Py_GT:
            if (intset_len(set_obj->intset) <= intset_len(other->intset))
                Py_RETURN_FALSE;
            if (intset_issuperset(set_obj->intset, other->intset))
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
    }
    PyErr_Format(PyExc_TypeError, "%s", Py_TYPE(obj)->tp_name);
    return NULL;
}

static PyMethodDef set_methods[] = {
        {"add",                         (PyCFunction) set_add,                         METH_O,      add_doc},
        {"remove",                      (PyCFunction) set_remove,                      METH_O,      remove_doc},
        {"discard",                     (PyCFunction) set_discard,                     METH_O,      discard_doc},
        {"max",                         (PyCFunction) set_max,                         METH_NOARGS, max_doc},
        {"min",                         (PyCFunction) set_min,                         METH_NOARGS, min_doc},
        {"clear",                       (PyCFunction) set_clear,                       METH_NOARGS, clear_doc},
        {"copy",                        (PyCFunction) set_copy,                        METH_NOARGS, copy_doc},
        {"issubset",                    (PyCFunction) set_issubset,                    METH_O,      issubset_doc},
        {"issuperset",                  (PyCFunction) set_issuperset,                  METH_O,      issuperset_doc},
        {"intersection",                (PyCFunction) set_intersection,                METH_O,      intersection_doc},
        {"intersection_update",         (PyCFunction) set_intersection_update,         METH_O,      intersection_update_doc},
        {"difference",                  (PyCFunction) set_difference,                  METH_O,      difference_doc},
        {"difference_update",           (PyCFunction) set_difference_update,           METH_O,      difference_update_doc},
        {"union",                       (PyCFunction) set_union,                       METH_O,      union_doc},
        {"update",                      (PyCFunction) set_update,                      METH_O,      update_doc},
        {"symmetric_difference",        (PyCFunction) set_symmetric_difference,        METH_O,      symmetric_difference_doc},
        {"symmetric_difference_update", (PyCFunction) set_symmetric_difference_update, METH_O,      symmetric_difference_update_doc},
        {"__contains__",                (PyCFunction) set_direct_contains,             METH_O,      contains_doc},
        {NULL}
};
static PyNumberMethods set_as_number = {
        0,                                  /*nb_add*/
        (binaryfunc) set_sub,               /*nb_subtract*/
        0,                                  /*nb_multiply*/
#ifndef PY3
        0,                                  /*nb_divide*/
#endif
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
        (binaryfunc) set_and,               /*nb_and*/
        (binaryfunc) set_xor,               /*nb_xor*/
        (binaryfunc) set_or,                /*nb_or*/
};


static PySequenceMethods set_as_sequence = {
        set_len,                            /* sq_length */
        0,                                  /* sq_concat */
        0,                                  /* sq_repeat */
        (ssizeargfunc) set_get_item,        /* sq_item */
        (ssizessizeargfunc) set_get_slice,  /* sq_slice */
        0,                                  /* sq_ass_item */
        0,                                  /* sq_ass_slice */
        (objobjproc) set_contains,          /* sq_contains */
};

static PyMappingMethods set_as_mapping = {
     0,
    (binaryfunc)set_subscript,
    0
};

PyDoc_STRVAR(intset_doc,
        "IntSet() -> new empty intset object\n\
        IntSet(iterable) -> new intset object\n\
        \n\
        Build an ordered collection of unique Integers.");

static PyTypeObject IntSetObject_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "IntSet",                          /* tp_name */
        sizeof(IntSetObject),              /* tp_basicsize */
        0,                                 /* tp_itemsize */
        (destructor) set_dealloc,          /* tp_dealloc */
        0,                                 /* tp_print */
        0,                                 /* tp_getattr */
        0,                                 /* tp_setattr */
        0,                                 /* tp_compare */
        (reprfunc) set_repr,               /* tp_repr */
        &set_as_number,                    /* tp_as_number */
        &set_as_sequence,                  /* tp_as_sequence */
        &set_as_mapping,                                 /* tp_as_mapping */
        PyObject_HashNotImplemented,       /* tp_hash */
        0,                                 /* tp_call */
        0,                                 /* tp_str */
        PyObject_GenericGetAttr,           /* tp_getattro */
        0,                                 /* tp_setattro */
        0,                                 /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                /* tp_flags */
        intset_doc,                        /* tp_doc */
        0,                                 /* tp_traverse */
        0,                                 /* tp_clear */
        (richcmpfunc) set_richcompare,     /* tp_richcompare */
        0,                                 /* tp_weaklistoffset */
        (getiterfunc) set_iter,            /* tp_iter */
        0,                                 /* tp_iternext */
        set_methods,                       /* tp_methods */
        0,                                 /* tp_members */
        0,                                 /* tp_getset */
        0,                                 /* tp_base */
        0,                                 /* tp_dict */
        0,                                 /* tp_descr_get */
        0,                                 /* tp_descr_set */
        0,                                 /* tp_dictoffset */
        (initproc) set_init,               /* tp_init */
        PyType_GenericAlloc,               /* tp_alloc */
        set_new,                           /* tp_new */
};

static PyMethodDef module_methods[] = {
        {NULL}  /* Sentinel */
};

#ifdef PY3

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "intset",
    0,              /* m_doc */
    -1,             /* m_size */
    module_methods,   /* m_methods */
    NULL,           /* m_reload */
    NULL,           /* m_traverse */
    NULL,           /* m_clear */
    NULL            /* m_free */
};

#define PYMODINITFUNC       PyObject *PyInit_intset(void)
#define PYMODULE_CREATE()   PyModule_Create(&moduledef)
#define MODINITERROR        return NULL

#else

#define PYMODINITFUNC       PyMODINIT_FUNC initintset(void)
#define PYMODULE_CREATE()   Py_InitModule("intset", module_methods);
#define MODINITERROR        return

#endif

PYMODINITFUNC
{
    PyObject *m;
    IntSetObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&IntSetObject_Type) < 0)
        MODINITERROR;
    m = PYMODULE_CREATE();
    if(m == NULL){
         MODINITERROR;
    }
    Py_INCREF(&IntSetObject_Type);
    PyModule_AddObject(m, "IntSet", (PyObject * ) & IntSetObject_Type);
    #ifdef PY3
    return m;
    #endif
};
