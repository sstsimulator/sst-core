// -*- c++ -*-

// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <string.h>
#include <sstream>

#include <sst/core/warnmacros.h>
#include <sst/core/model/python/pymacros.h>
#include <sst/core/model/python/pymodel.h>
#include <sst/core/model/python/pymodel_unitalgebra.h>

#include <sst/core/unitAlgebra.h>

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;

extern "C" {


static int unitAlgebraInit(UnitAlgebraPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *init_str = NULL;
    // PyObject* obj;
    UnitAlgebraPy_t* new_obj;


    if ( PyArg_ParseTuple(args, "s", &init_str) ) {
        self->obj = init_str;
        return 0;
    }
    PyErr_Clear();
    if ( PyArg_ParseTuple(args, "O!", &PyModel_UnitAlgebraType, &new_obj) ) {
        self->obj = new_obj->obj;
        return 0;
    }
    PyErr_SetString(PyExc_TypeError,"sst.UnitAlgebra can only be initialized with another sst.UnitAlgebra or with a formatted string");
    return -1;
}

static void unitAlgebraDealloc(UnitAlgebraPy_t *self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* unitAlgebraStr(PyObject* self)
{
    UnitAlgebraPy_t *ua = (UnitAlgebraPy_t*)self;
    return SST_ConvertToPythonString(ua->obj.toStringBestSI().c_str());
}


static PyObject* unitAlgebraRichCmp (PyObject *self, PyObject *other, int op)
{
    UnitAlgebraPy_t *self_ua = (UnitAlgebraPy_t*)self;
    if ( Py_TYPE(other) == &PyModel_UnitAlgebraType ) {
        UnitAlgebraPy_t *other_ua = (UnitAlgebraPy_t*)other;
        switch (op) {
        case Py_LT:
            if ( self_ua->obj < other_ua->obj ) return Py_True;
            else return Py_False;
            break;
        case Py_LE:
            if ( self_ua->obj <= other_ua->obj ) return Py_True;
            else return Py_False;
            break;
        case Py_GT:
            if ( self_ua->obj > other_ua->obj ) return Py_True;
            else return Py_False;
            break;
        case Py_GE:
            if ( self_ua->obj >= other_ua->obj ) return Py_True;
            else return Py_False;
            break;
        case Py_EQ:
            if ( self_ua->obj == other_ua->obj ) return Py_True;
            else return Py_False;
            break;
        case Py_NE:
            if ( self_ua->obj != other_ua->obj ) return Py_True;
            else return Py_False;
            break;
        default:
            break;
        }
    }

    // Generate an error message.  Either they types don't match, or
    // it was an invalid operation.  Either way, the same error
    // message is created.
    std::stringstream ss;
    switch (op) {
    case Py_LT:
        ss << "'<'";
        break;
    case Py_LE:
        ss << "'<='";
        break;
    case Py_GT:
        ss << "'>'";
        break;
    case Py_GE:
        ss << "'>='";
            break;
    case Py_EQ:
        ss << "'=='";
        break;
    case Py_NE:
        ss << "'!='";
        break;
    }
    ss << " operator not supported between instances of " << Py_TYPE(self)->tp_name << " and " << Py_TYPE(other)->tp_name;
    PyErr_SetString(PyExc_TypeError, ss.str().c_str());
    return nullptr;
}

// Utility function for creating a new UnitAlgebra from an existing
// one
static PyObject* createUnitAlgebra(PyObject* self)
{
    PyObject *argList = Py_BuildValue("(O)", self);
    PyObject* ret = PyObject_CallObject((PyObject*)&PyModel_UnitAlgebraType, argList);
    Py_DECREF(argList);
    return ret;
}


// Numerical functions

// Utility function to do all the basic math operations.  Using this
// will make the operation somewhat slower to execute, but will make
// the code cleaner and more maintainable.  Since this is only used
// during intialization that's probably the right trade-off.  We can
// revisit later if it becomes an issue.

static PyObject* unitAlgebraMathOps(PyObject* self, PyObject* other, char op, bool in_place) {

    if ( Py_TYPE(other) != &PyModel_UnitAlgebraType || Py_TYPE(self) != &PyModel_UnitAlgebraType ) {
        std::stringstream ss;
        const char* ip = in_place ? "=\0" : "\0";
        ss << "'" << op << ip << "'" << " operator not supported between instance of " << Py_TYPE(self)->tp_name << " and " << Py_TYPE(other)->tp_name;
        PyErr_SetString(PyExc_TypeError, ss.str().c_str());
        return nullptr;
    }

    PyObject* ret = createUnitAlgebra(self);
    UnitAlgebraPy_t *ret_ua = (UnitAlgebraPy_t*)ret;

    UnitAlgebraPy_t *other_ua = (UnitAlgebraPy_t*)other;

    switch ( op ) {
    case '+':
        ret_ua->obj += other_ua->obj;
        break;
    case '-':
        ret_ua->obj -= other_ua->obj;
        break;
    case '*':
        ret_ua->obj *= other_ua->obj;
        break;
    case '/':
        ret_ua->obj /= other_ua->obj;
        break;
    default:
        Output::getDefaultObject().fatal(CALL_INFO_LONG,1,"Internal error encountered, terminating.\n");
        break;
    };

    return ret;
}

// NOTE: Because the python semantics require that the in-place
// operators return a new reference, the in-place and regular
// operators end up being the same function.  However, in order to
// create technically correct error messages if the wrong type of
// operands are used, there are functions for both, but the inplace
// functions will just call the normal functions after error checking.

// Add
static PyObject* unitAlgebraAdd (PyObject *self, PyObject *other)
{
    return unitAlgebraMathOps(self,other,'+', false);
}

static PyObject* unitAlgebraAddIP (PyObject *self, PyObject *other)
{
    return unitAlgebraMathOps(self,other,'+', true);
}

// Sub
static PyObject* unitAlgebraSub (PyObject *self, PyObject *other)
{
    return unitAlgebraMathOps(self,other,'-', false);
}

static PyObject* unitAlgebraSubIP (PyObject *self, PyObject *other)
{
    return unitAlgebraMathOps(self,other,'-', true);
}

// Mult
static PyObject* unitAlgebraMul (PyObject *self, PyObject *other)
{
    return unitAlgebraMathOps(self,other,'*', false);
}

static PyObject* unitAlgebraMulIP (PyObject *self, PyObject *other)
{
    return unitAlgebraMathOps(self,other,'*', true);
}

// Div
static PyObject* unitAlgebraDiv (PyObject *self, PyObject *other)
{
    return unitAlgebraMathOps(self,other,'/', false);
}

static PyObject* unitAlgebraDivIP (PyObject *self, PyObject *other)
{
    return unitAlgebraMathOps(self,other,'/', true);
}


static PyObject* unitAlgebraToLong(PyObject *self)
{
    UnitAlgebraPy_t *self_ua = (UnitAlgebraPy_t*)self;
    int64_t val = self_ua->obj.getRoundedValue();
    PyObject* ret = SST_ConvertToPythonLong(val);
    return ret;
}

static PyObject* unitAlgebraToFloat(PyObject *self)
{
    UnitAlgebraPy_t *self_ua = (UnitAlgebraPy_t*)self;
    double val = self_ua->obj.getDoubleValue();
    PyObject* ret = PyFloat_FromDouble(val);
    return ret;
}

static int unitAlgebraToBool(PyObject *self)
{
    UnitAlgebraPy_t *self_ua = (UnitAlgebraPy_t*)self;
    bool val = !self_ua->obj.isValueZero();
    return val;
}

static PyObject* unitAlgebraNegate(PyObject *self)
{
    PyObject* ret = createUnitAlgebra(self);

    UnitAlgebraPy_t *ret_ua = (UnitAlgebraPy_t*)ret;
    ret_ua->obj *= -1;
    return ret;
}


PyNumberMethods PyModel_UnitAlgebraNumMeth = {
    (binaryfunc)unitAlgebraAdd,    // binaryfunc nb_add
    (binaryfunc)unitAlgebraSub,    // binaryfunc nb_subtract
    (binaryfunc)unitAlgebraMul,    // binaryfunc nb_multiply
    SST_NB_DIVIDE((binaryfunc)unitAlgebraDiv)    // binaryfunc nb_divide, py2 only
    nullptr,                // binaryfunc nb_remainder
    nullptr,                // binaryfunc nb_divmod
    nullptr,                // ternaryfunc nb_power
    (unaryfunc)unitAlgebraNegate,                // unaryfunc nb_negative
    nullptr,                // unaryfunc nb_positive
    nullptr,                // unaryfunc nb_absolute
    unitAlgebraToBool,      // inquiry nb_nonzero (py2) nb_bool (py3)       /* Used by PyObject_IsTrue */
    nullptr,                // unaryfunc nb_invert
    nullptr,                // binaryfunc nb_lshift
    nullptr,                // binaryfunc nb_rshift
    nullptr,                // binaryfunc nb_and
    nullptr,                // binaryfunc nb_xor
    nullptr,                // binaryfunc nb_or
    SST_NB_COERCE                // coercion nb_coerce Python 2 only
    SST_NB_INTLONG((unaryfunc)unitAlgebraToLong) // unaryfunc nb_int (py1 nb_ling for py2)
    SST_NB_RESERVED           // nb_reserved, py3 only
    unitAlgebraToFloat,       // unaryfunc nb_float
    SST_NB_OCT                // unaryfunc nb_oct
    SST_NB_HEX                // unaryfunc nb_hex

    /* Added in release 2.0 */
    (binaryfunc)unitAlgebraAddIP,  // binaryfunc nb_inplace_add
    (binaryfunc)unitAlgebraSubIP,  // binaryfunc nb_inplace_subtract
    (binaryfunc)unitAlgebraMulIP,  // binaryfunc nb_inplace_multiply
    SST_NB_INPLACE_DIVIDE((binaryfunc)unitAlgebraDivIP)   // binaryfunc nb_inplace_divide, py2 only
    nullptr,                // binaryfunc nb_inplace_remainder
    nullptr,                // ternaryfunc nb_inplace_power
    nullptr,                // binaryfunc nb_inplace_lshift
    nullptr,                // binaryfunc nb_inplace_rshift
    nullptr,                // binaryfunc nb_inplace_and
    nullptr,                // binaryfunc nb_inplace_xor
    nullptr,                // binaryfunc nb_inplace_or

    /* Added in release 2.2 */
    nullptr,                // binaryfunc nb_floor_divide
    (binaryfunc)unitAlgebraDiv, // binaryfunc nb_true_divide
    nullptr,                // binaryfunc nb_inplace_floor_divide
    (binaryfunc)unitAlgebraDivIP,  // binaryfunc nb_inplace_true_divide

    /* Added in release 2.5 */
    nullptr,                // unaryfunc nb_index
    SST_NB_MATRIX_MULTIPLY     // py3 only
    SST_NB_INPLACE_MATRIX_MULTIPLY // py3 only
};

// Other methods

static PyObject* unitAlgebraGetRoundedValue(PyObject *self, PyObject *UNUSED(args))
{
    return unitAlgebraToLong(self);
}

static PyObject* unitAlgebraGetFloatValue(PyObject *self, PyObject *UNUSED(args))
{
    return unitAlgebraToFloat(self);
}

static PyObject* unitAlgebraIsValueZero(PyObject *self, PyObject *UNUSED(args))
{
    bool val = !unitAlgebraToBool(self);
    return PyBool_FromLong(val);
}

static PyObject* unitAlgebraHasUnits(PyObject* self, PyObject* args)
{
    char *units = NULL;

    if ( PyArg_ParseTuple(args, "s", &units) ) {
        UnitAlgebraPy_t *ua = (UnitAlgebraPy_t*)self;
        if ( ua->obj.hasUnits(units) ) Py_RETURN_TRUE;
    }
    else {
        PyErr_Clear();
        PyErr_SetString(PyExc_TypeError,"sst.UnitAlgebra.hasUnits() must be called with a string as it's only argument");
        return nullptr;
    }
    Py_RETURN_FALSE;
}

static PyObject* unitAlgebraInvert(PyObject *self, PyObject *UNUSED(args))
{
    PyObject* ret = createUnitAlgebra(self);
    UnitAlgebraPy_t *ret_ua = (UnitAlgebraPy_t*)ret;
    ret_ua->obj.invert();
    return ret;
}


static PyMethodDef unitAlgebraMethods[] = {
    {   "getRoundedValue",
        unitAlgebraGetRoundedValue, METH_NOARGS,
        "Rounds value of UnitAlgebra to nearest whole number and returns a long"},
    {   "getFloatValue",
        unitAlgebraGetFloatValue, METH_NOARGS,
        "Returns value portion of UnitAlgebra as a float"},
    {   "isValueZero",
        unitAlgebraIsValueZero, METH_NOARGS,
        "Returns True if value is zero, false otherwise"},
    {   "hasUnits",
        unitAlgebraHasUnits, METH_VARARGS,
        "Checks to see if the UnitAlgebra has the specified units"},
    {   "invert",
        unitAlgebraInvert, METH_NOARGS,
        "Inverts the UnitAlgebra value and units"},
    {   NULL, NULL, 0, NULL }
};


#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
DISABLE_WARN_DEPRECATED_DECLARATION
#endif
#endif
PyTypeObject PyModel_UnitAlgebraType = {
    SST_PY_OBJ_HEAD
    "sst.UnitAlgebra",         /* tp_name */
    sizeof(UnitAlgebraPy_t),   /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)unitAlgebraDealloc,   /* tp_dealloc */
    SST_TP_VECTORCALL_OFFSET         /* Python3 only */
    SST_TP_PRINT                     /* Python2 only */
    nullptr,                         /* tp_getattr */
    nullptr,                         /* tp_setattr */
    SST_TP_COMPARE(nullptr)          /* Python2 only */
    SST_TP_AS_SYNC                   /* Python3 only */
    (reprfunc)unitAlgebraStr,        /* tp_repr */
    &PyModel_UnitAlgebraNumMeth,     /* tp_as_number */
    nullptr,                         /* tp_as_sequence */
    nullptr,                         /* tp_as_mapping */
    nullptr,                         /* tp_hash */
    nullptr,                         /* tp_call */
    (reprfunc)unitAlgebraStr,        /* tp_str */
    nullptr,                         /* tp_getattro */
    nullptr,                         /* tp_setattro */
    nullptr,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,              /* tp_flags */
    "SST UnitAlgebra",               /* tp_doc */
    nullptr,                         /* tp_traverse */
    nullptr,                         /* tp_clear */
    unitAlgebraRichCmp,              /* Python3 only */
    0,                               /* tp_weaklistoffset */
    nullptr,                         /* tp_iter */
    nullptr,                         /* tp_iternext */
    unitAlgebraMethods,              /* tp_methods */
    nullptr,                         /* tp_members */
    nullptr,                         /* tp_getset */
    nullptr,                         /* tp_base */
    nullptr,                         /* tp_dict */
    nullptr,                         /* tp_descr_get */
    nullptr,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)unitAlgebraInit,     /* tp_init */
    nullptr,                         /* tp_alloc */
    nullptr,                         /* tp_new */
    nullptr,                         /* tp_free */
    nullptr,                         /* tp_is_gc */
    nullptr,                         /* tp_bases */
    nullptr,                         /* tp_mro */
    nullptr,                         /* tp_cache */
    nullptr,                         /* tp_subclasses */
    nullptr,                         /* tp_weaklist */
    nullptr,                         /* tp_del */
    0,                         /* tp_version_tag */
    SST_TP_FINALIZE                /* Python3 only */
    SST_TP_VECTORCALL              /* Python3 only */
    SST_TP_PRINT_DEP               /* Python3.8 only */
};

#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
REENABLE_WARNING
#endif
#endif


}  /* extern C */


