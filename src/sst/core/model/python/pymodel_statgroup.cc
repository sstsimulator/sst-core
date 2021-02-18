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
#include <sst/core/model/python/pymacros.h>

#include <string.h>

#include "sst/core/model/python/pymodel.h"
#include "sst/core/model/python/pymodel_comp.h"
#include "sst/core/model/python/pymodel_statgroup.h"

#include "sst/core/sst_types.h"
#include "sst/core/simulation.h"
#include "sst/core/component.h"
#include "sst/core/configGraph.h"

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;

extern "C" {



static Params convertToParams(PyObject *dict)
{
    Params res;
    if ( dict != nullptr ) {
        for ( auto p : generateStatisticParameters(dict) ) {
            res.insert(p.first, p.second);
        }
    }
    return res;
}




static int sgInit(StatGroupPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *name = nullptr;
    if ( !PyArg_ParseTuple(args, "s", &name) ) return -1;

    self->ptr = gModel->getGraph()->getStatGroup(name);
    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating Stat Group %s\n", name);

    return 0;
}


static void sgDealloc(StatGroupPy_t *self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* sgAddStat(PyObject *self, PyObject *args)
{
    PyErr_Clear();

    char *statName = nullptr;
    PyObject *paramsDict = nullptr;

    int argOK = PyArg_ParseTuple(args, "s|O!", &statName, &PyDict_Type, &paramsDict);
    if ( !argOK )
        return nullptr;

    Params params = convertToParams(paramsDict);

    ConfigStatGroup *csg = ((StatGroupPy_t*)self)->ptr;
    if ( !csg->addStatistic(statName, params) ) {
        PyErr_SetString(PyExc_RuntimeError, "Unable to create statistic");
        return nullptr;
    }
    bool verified;
    std::string reason;
    std::tie(verified, reason) = csg->verifyStatsAndComponents(gModel->getGraph());
    if ( !verified ) {
        PyErr_SetString(PyExc_RuntimeError, reason.c_str());
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}


static PyObject* sgAddComp(PyObject *self, PyObject *args)
{
    PyErr_Clear();
    ConfigStatGroup *csg = ((StatGroupPy_t*)self)->ptr;

    if ( PyObject_TypeCheck(args, &PyModel_ComponentType) ||
            PyObject_TypeCheck(args, &PyModel_SubComponentType) ) {
        csg->addComponent(((ComponentPy_t*)args)->obj->getID());
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected Component or SubComponent type");
        return nullptr;
    }

    bool verified;
    std::string reason;
    std::tie(verified, reason) = csg->verifyStatsAndComponents(gModel->getGraph());
    if ( !verified ) {
        PyErr_SetString(PyExc_RuntimeError, reason.c_str());
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}


static PyObject* sgSetOutput(PyObject *self, PyObject *args)
{

    if ( PyObject_TypeCheck(args, &PyModel_StatOutputType) ) {
        StatOutputPy_t *out = (StatOutputPy_t*)args;
        if ( !((StatGroupPy_t*)self)->ptr->setOutput(out->id) ) {
            PyErr_SetString(PyExc_RuntimeError, "Unable to set Statistic Output");
            return nullptr;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected sst.StatisticOutput type");
        return nullptr;
    }

    return SST_ConvertToPythonLong(0);
}


static PyObject* sgSetFreq(PyObject *self, PyObject *args)
{
    if ( !PyUnicode_Check(args) ) {
        return nullptr;
    }
    if ( !((StatGroupPy_t*)self)->ptr->setFrequency(SST_ConvertToCppString(args)) ) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid frequency");
        return nullptr;
    }

    return SST_ConvertToPythonLong(0);
}




static PyMethodDef sgMethods[] = {
    {   "addStatistic", sgAddStat, METH_VARARGS,
        "Add a new statistic to the group" },
    {   "addComponent", sgAddComp, METH_O,
        "Add a component to the group" },
    {   "setOutput", sgSetOutput, METH_O,
        "Configure how the stats should be written" },
    {   "setFrequency", sgSetFreq, METH_O,
        "Set the frequency or rate (ie: \"10ms\", \"25khz\") to write out the statistics" },
    {   nullptr, nullptr, 0, nullptr }
};


#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
DISABLE_WARN_DEPRECATED_DECLARATION
#endif
#endif
PyTypeObject PyModel_StatGroupType = {
    SST_PY_OBJ_HEAD
    "sst.StatisticGroup",      /* tp_name */
    sizeof(StatGroupPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)sgDealloc,     /* tp_dealloc */
    SST_TP_VECTORCALL_OFFSET       /* Python3 only */
    SST_TP_PRINT                   /* Python2 only */
    nullptr,                   /* tp_getattr */
    nullptr,                   /* tp_setattr */
    SST_TP_COMPARE(nullptr)        /* Python2 only */
    SST_TP_AS_SYNC                 /* Python3 only */
    nullptr,                   /* tp_repr */
    nullptr,                   /* tp_as_number */
    nullptr,                   /* tp_as_sequence */
    nullptr,                   /* tp_as_mapping */
    nullptr,                   /* tp_hash */
    nullptr,                   /* tp_call */
    nullptr,                   /* tp_str */
    nullptr,                   /* tp_getattro */
    nullptr,                   /* tp_setattro */
    nullptr,                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST Statistic Group",     /* tp_doc */
    nullptr,                   /* tp_traverse */
    nullptr,                   /* tp_clear */
    SST_TP_RICH_COMPARE(nullptr)   /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    nullptr,                   /* tp_iter */
    nullptr,                   /* tp_iternext */
    sgMethods,                 /* tp_methods */
    nullptr,                   /* tp_members */
    nullptr,                   /* tp_getset */
    nullptr,                   /* tp_base */
    nullptr,                   /* tp_dict */
    nullptr,                   /* tp_descr_get */
    nullptr,                   /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)sgInit,          /* tp_init */
    nullptr,                   /* tp_alloc */
    nullptr,                   /* tp_new */
    nullptr,                   /* tp_free */
    nullptr,                   /* tp_is_gc */
    nullptr,                   /* tp_bases */
    nullptr,                   /* tp_mro */
    nullptr,                   /* tp_cache */
    nullptr,                   /* tp_subclasses */
    nullptr,                   /* tp_weaklist */
    nullptr,                   /* tp_del */
    0,                         /* tp_version_tag */
    SST_TP_FINALIZE            /* Python3 only */
    SST_TP_VECTORCALL          /* Python3 only */
    SST_TP_PRINT_DEP               /* Python3.8 only */
};
#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
REENABLE_WARNING
#endif
#endif



static int soInit(StatOutputPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *type = nullptr;
    PyObject *params = nullptr;
    if ( !PyArg_ParseTuple(args, "s|O!", &type, &PyDict_Type, &params) ) return -1;

    std::vector<ConfigStatOutput>& vec = gModel->getGraph()->getStatOutputs();

    self->id = vec.size();
    vec.emplace_back(type);
    self->ptr = &vec.back();

    if ( params != nullptr ) {
        self->ptr->params = convertToParams(params);
    }

    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating Stat Output %s\n", type);

    return 0;
}


static void soDealloc(StatOutputPy_t *self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* soAddParam(PyObject *self, PyObject *args)
{
    char *param = nullptr;
    PyObject *value = nullptr;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) )
        return nullptr;

    ConfigStatOutput *so = ((StatOutputPy_t*)self)->ptr;
    if ( nullptr == so ) return nullptr;

    PyObject *vstr = PyObject_CallMethod(value, (char*)"__str__", nullptr);
    so->addParameter(param, SST_ConvertToCppString(vstr));
    Py_XDECREF(vstr);

    return SST_ConvertToPythonLong(0);
}


static PyObject* soAddParams(PyObject *self, PyObject *args)
{
    ConfigStatOutput *so = ((StatOutputPy_t*)self)->ptr;
    if ( nullptr == so ) return nullptr;

    if ( !PyDict_Check(args) ) {
        return nullptr;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *val;
    long count = 0;

    while ( PyDict_Next(args, &pos, &key, &val) ) {
        PyObject *kstr = PyObject_CallMethod(key, (char*)"__str__", nullptr);
        PyObject *vstr = PyObject_CallMethod(val, (char*)"__str__", nullptr);
        so->addParameter(SST_ConvertToCppString(kstr), SST_ConvertToCppString(vstr));
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return SST_ConvertToPythonLong(count);
}



static PyMethodDef soMethods[] = {
    { "addParam",
      soAddParam, METH_VARARGS,
      "Adds a parameter(name, value)"},
    { "addParams",
      soAddParams, METH_O,
      "Adds Multiple Parameters from a dict"},
    { nullptr, nullptr, 0, nullptr }
};


#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
DISABLE_WARN_DEPRECATED_DECLARATION
#endif
#endif
PyTypeObject PyModel_StatOutputType = {
    SST_PY_OBJ_HEAD
    "sst.StatisticOutput",       /* tp_name */
    sizeof(StatOutputPy_t),      /* tp_basicsize */
    0,                           /* tp_itemsize */
    (destructor)soDealloc,       /* tp_dealloc */
    SST_TP_VECTORCALL_OFFSET       /* Python3 only */
    SST_TP_PRINT                  /* Python2 only */
    nullptr,                     /* tp_getattr */
    nullptr,                     /* tp_setattr */
    SST_TP_COMPARE(nullptr)        /* Python2 only */
    SST_TP_AS_SYNC                 /* Python3 only */
    nullptr,                     /* tp_repr */
    nullptr,                     /* tp_as_number */
    nullptr,                     /* tp_as_sequence */
    nullptr,                     /* tp_as_mapping */
    nullptr,                     /* tp_hash */
    nullptr,                     /* tp_call */
    nullptr,                     /* tp_str */
    nullptr,                     /* tp_getattro */
    nullptr,                     /* tp_setattro */
    nullptr,                     /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,          /* tp_flags */
    "SST Statistic Output",      /* tp_doc */
    nullptr,                     /* tp_traverse */
    nullptr,                     /* tp_clear */
    SST_TP_RICH_COMPARE(nullptr)   /* Python3 only */
    0,                           /* tp_weaklistoffset */
    nullptr,                     /* tp_iter */
    nullptr,                     /* tp_iternext */
    soMethods,                   /* tp_methods */
    nullptr,                     /* tp_members */
    nullptr,                     /* tp_getset */
    nullptr,                     /* tp_base */
    nullptr,                     /* tp_dict */
    nullptr,                     /* tp_descr_get */
    nullptr,                     /* tp_descr_set */
    0,                           /* tp_dictoffset */
    (initproc)soInit,            /* tp_init */
    nullptr,                     /* tp_alloc */
    nullptr,                     /* tp_new */
    nullptr,                     /* tp_free */
    nullptr,                     /* tp_is_gc */
    nullptr,                     /* tp_bases */
    nullptr,                     /* tp_mro */
    nullptr,                     /* tp_cache */
    nullptr,                     /* tp_subclasses */
    nullptr,                     /* tp_weaklist */
    nullptr,                     /* tp_del */
    0,                           /* tp_version_tag */
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


