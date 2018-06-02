// -*- c++ -*-

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include <Python.h>

#include <string.h>
#include <sstream>

#include <sst/core/warnmacros.h>
#include <sst/core/model/pymodel.h>
#include <sst/core/model/pymodel_comp.h>
#include <sst/core/model/pymodel_statgroup.h>

#include <sst/core/sst_types.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/configGraph.h>

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;

extern "C" {



static Params convertToParams(PyObject *dict)
{
    Params res;
    if ( dict != NULL ) {
        for ( auto p : generateStatisticParameters(dict) ) {
            res.insert(p.first, p.second);
        }
    }
    return res;
}




static int sgInit(StatGroupPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *name = NULL;
    if ( !PyArg_ParseTuple(args, "s", &name) ) return -1;

    self->ptr = gModel->getGraph()->getStatGroup(name);
	gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating Stat Group %s\n", name);

    return 0;
}


static void sgDealloc(StatGroupPy_t *self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject* sgAddStat(PyObject *self, PyObject *args)
{
    PyErr_Clear();

    char *statName = NULL;
    PyObject *paramsDict = NULL;

    int argOK = PyArg_ParseTuple(args, "s|O!", &statName, &PyDict_Type, &paramsDict);
    if ( !argOK )
        return NULL;

    Params params = convertToParams(paramsDict);

    ConfigStatGroup *csg = ((StatGroupPy_t*)self)->ptr;
    if ( !csg->addStatistic(statName, params) ) {
        PyErr_SetString(PyExc_RuntimeError, "Unable to create statistic");
        return NULL;
    }
    bool verified;
    std::string reason;
    std::tie(verified, reason) = csg->verifyStatsAndComponents(gModel->getGraph());
    if ( !verified ) {
        PyErr_SetString(PyExc_RuntimeError, reason.c_str());
        return NULL;
    }
    return PyLong_FromLong(0);
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
        return NULL;
    }

    bool verified;
    std::string reason;
    std::tie(verified, reason) = csg->verifyStatsAndComponents(gModel->getGraph());
    if ( !verified ) {
        PyErr_SetString(PyExc_RuntimeError, reason.c_str());
        return NULL;
    }
    return PyLong_FromLong(0);
}


static PyObject* sgSetOutput(PyObject *self, PyObject *args)
{

    if ( PyObject_TypeCheck(args, &PyModel_StatOutputType) ) {
        StatOutputPy_t *out = (StatOutputPy_t*)args;
        if ( !((StatGroupPy_t*)self)->ptr->setOutput(out->id) ) {
            PyErr_SetString(PyExc_RuntimeError, "Unable to set Statistic Output");
            return NULL;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected sst.StatisticOutput type");
        return NULL;
    }

    return PyLong_FromLong(0);
}


static PyObject* sgSetFreq(PyObject *self, PyObject *args)
{
    if ( !PyString_Check(args) ) {
        return NULL;
    }
    if ( !((StatGroupPy_t*)self)->ptr->setFrequency(PyString_AsString(args)) ) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid frequency");
        return NULL;
    }

    return PyLong_FromLong(0);
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
    {   NULL, NULL, 0, NULL }
};


PyTypeObject PyModel_StatGroupType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.StatisticGroup",      /* tp_name */
    sizeof(StatGroupPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)sgDealloc,     /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST Statistic Group",     /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    sgMethods,                 /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)sgInit,          /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
    0,                         /* tp_free */
    0,                         /* tp_is_gc */
    0,                         /* tp_bases */
    0,                         /* tp_mro */
    0,                         /* tp_cache */
    0,                         /* tp_subclasses */
    0,                         /* tp_weaklist */
    0,                         /* tp_del */
    0,                         /* tp_version_tag */
};



static int soInit(StatOutputPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *type = NULL;
    PyObject *params = NULL;
    if ( !PyArg_ParseTuple(args, "s|O!", &type, &PyDict_Type, &params) ) return -1;

    std::vector<ConfigStatOutput>& vec = gModel->getGraph()->getStatOutputs();

    self->id = vec.size();
    vec.emplace_back(type);
    self->ptr = &vec.back();

    if ( params != NULL ) {
        self->ptr->params = convertToParams(params);
    }

	gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating Stat Output %s\n", type);

    return 0;
}


static void soDealloc(StatOutputPy_t *self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject* soAddParam(PyObject *self, PyObject *args)
{
    char *param = NULL;
    PyObject *value = NULL;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) )
        return NULL;

    ConfigStatOutput *so = ((StatOutputPy_t*)self)->ptr;
    if ( NULL == so ) return NULL;

    PyObject *vstr = PyObject_CallMethod(value, (char*)"__str__", NULL);
    so->addParameter(param, PyString_AsString(vstr));
    Py_XDECREF(vstr);

    return PyInt_FromLong(0);
}


static PyObject* soAddParams(PyObject *self, PyObject *args)
{
    ConfigStatOutput *so = ((StatOutputPy_t*)self)->ptr;
    if ( NULL == so ) return NULL;

    if ( !PyDict_Check(args) ) {
        return NULL;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *val;
    long count = 0;

    while ( PyDict_Next(args, &pos, &key, &val) ) {
        PyObject *kstr = PyObject_CallMethod(key, (char*)"__str__", NULL);
        PyObject *vstr = PyObject_CallMethod(val, (char*)"__str__", NULL);
        so->addParameter(PyString_AsString(kstr), PyString_AsString(vstr));
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return PyInt_FromLong(count);
}



static PyMethodDef soMethods[] = {
    { "addParam",
      soAddParam, METH_VARARGS,
      "Adds a parameter(name, value)"},
    { "addParams",
      soAddParams, METH_O,
      "Adds Multiple Parameters from a dict"},
    { NULL, NULL, 0, NULL }
};


PyTypeObject PyModel_StatOutputType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.StatisticOutput",     /* tp_name */
    sizeof(StatOutputPy_t),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)soDealloc,     /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST Statistic Output",     /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    soMethods,                 /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)soInit,          /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
    0,                         /* tp_free */
    0,                         /* tp_is_gc */
    0,                         /* tp_bases */
    0,                         /* tp_mro */
    0,                         /* tp_cache */
    0,                         /* tp_subclasses */
    0,                         /* tp_weaklist */
    0,                         /* tp_del */
    0,                         /* tp_version_tag */
};


}  /* extern C */


