// -*- c++ -*-

// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
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
#include <sst/core/model/python/pymacros.h>

#include <string.h>

#include "sst/core/model/python/pymodel.h"
#include "sst/core/model/python/pymodel_stat.h"

#include "sst/core/sst_types.h"
#include "sst/core/configGraph.h"

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;

namespace SST {

extern "C" {

StatisticId_t PyStatistic::getID()
{
    return id;
}

ConfigStatistic* PyStatistic::getStat() {
    return gModel->getGraph()->findStatistic(id);
}

int PyStatistic::compare(PyStatistic *other) {
    if (id < other->id) return -1;
    else if (id > other->id ) return 1;
    else return 0;
}

static int statInit(StatisticPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    StatisticId_t id = 0;
    if ( !PyArg_ParseTuple(args, "k", &id) )
        return -1;

    PyStatistic *obj = new PyStatistic(id);
    self->obj = obj;

    //gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating statistic [%s]]\n", getStat((PyObject*)self)->name.c_str());

    return 0;
}

static void statDealloc(StatisticPy_t *self)
{
    if ( self->obj ) delete self->obj;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* statAddParam(PyObject *self, PyObject *args)
{
    char* param = nullptr;
    PyObject *value = nullptr;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) )
        return nullptr;

    ConfigStatistic *c = getStat(self);
    if ( nullptr == c ) return nullptr;

    // Get the string-ized value by calling __str__ function of the
    // value object
    PyObject *vstr = PyObject_CallMethod(value, (char*)"__str__", nullptr);
    c->addParameter(param, SST_ConvertToCppString(vstr), true);
    Py_XDECREF(vstr);

    return SST_ConvertToPythonLong(0);
}

static PyObject* statAddParams(PyObject *self, PyObject *args)
{

    ConfigStatistic *c = getStat(self);
    if ( nullptr == c ) return nullptr;

    if ( !PyDict_Check(args) ) {
        return nullptr;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *val;
    long count = 0;

    while ( PyDict_Next(args, &pos, &key, &val) ) {
        PyObject *kstr = PyObject_CallMethod(key, (char*)"__str__", nullptr);
        PyObject *vstr = PyObject_CallMethod(val, (char*)"__str__", nullptr);
        c->addParameter(SST_ConvertToCppString(kstr), SST_ConvertToCppString(vstr), true);
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return SST_ConvertToPythonLong(count);
}

#if PY_MAJOR_VERSION >= 3
static PyObject* statCompare(PyObject *obj0, PyObject *obj1, int op) {
    PyObject *result;
    bool cmp = false;
    switch(op) {
        case Py_LT: cmp = ((PyStatistic*)obj0)->compare(((StatisticPy_t*)obj1)->obj) == -1; break;
        case Py_LE: cmp = ((PyStatistic*)obj0)->compare(((StatisticPy_t*)obj1)->obj) != 1; break;
        case Py_EQ: cmp = ((PyStatistic*)obj0)->compare(((StatisticPy_t*)obj1)->obj) == 0; break;
        case Py_NE: cmp = ((PyStatistic*)obj0)->compare(((StatisticPy_t*)obj1)->obj) != 0; break;
        case Py_GT: cmp = ((PyStatistic*)obj0)->compare(((StatisticPy_t*)obj1)->obj) == 1; break;
        case Py_GE: cmp = ((PyStatistic*)obj0)->compare(((StatisticPy_t*)obj1)->obj) != -1; break;
    }
    result = cmp ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}
#else
static int statCompare(PyObject *obj0, PyObject *obj1) {
    return ((PyStatistic*)obj0)->compare(((PyStatistic*)obj1));
}
#endif

static PyMethodDef statisticMethods[] = {
    {   "addParam",
        statAddParam, METH_VARARGS,
        "Adds a parameter(name, value)"},
    {   "addParams",
        statAddParams, METH_O,
        "Adds Multiple Parameters from a dict"},
     {   nullptr, nullptr, 0, nullptr }
};


PyTypeObject PyModel_StatType = {
    SST_PY_OBJ_HEAD
    "sst.Statistic",           /* tp_name */
    sizeof(StatisticPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)statDealloc,/* tp_dealloc */
    SST_TP_VECTORCALL_OFFSET       /* Python3 only */
    SST_TP_PRINT                   /* Python2 only */
    nullptr,                         /* tp_getattr */
    nullptr,                         /* tp_setattr */
    SST_TP_COMPARE(nullptr)        /* Python2 only */
    SST_TP_AS_SYNC                 /* Python3 only */
    nullptr,                         /* tp_repr */
    nullptr,                         /* tp_as_number */
    nullptr,                         /* tp_as_sequence */
    nullptr,                         /* tp_as_mapping */
    nullptr,                         /* tp_hash */
    nullptr,                         /* tp_call */
    nullptr,                         /* tp_str */
    nullptr,                         /* tp_getattro */
    nullptr,                         /* tp_setattro */
    nullptr,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,              /* tp_flags */
    "SST Statistic",                 /* tp_doc */
    nullptr,                         /* tp_traverse */
    nullptr,                         /* tp_clear */
    SST_TP_RICH_COMPARE(statCompare)    /* Python3 only */
    0,                               /* tp_weaklistoffset */
    nullptr,                         /* tp_iter */
    nullptr,                         /* tp_iternext */
    statisticMethods,                /* tp_methods */
    nullptr,                         /* tp_members */
    nullptr,                         /* tp_getset */
    nullptr,                         /* tp_base */
    nullptr,                         /* tp_dict */
    nullptr,                         /* tp_descr_get */
    nullptr,                         /* tp_descr_set */
    0,                               /* tp_dictoffset */
    (initproc)statInit,              /* tp_init */
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
    0,                               /* tp_version_tag */
    SST_TP_FINALIZE                /* Python3 only */
    SST_TP_VECTORCALL              /* Python3 only */
};


}  /* extern C */

}
