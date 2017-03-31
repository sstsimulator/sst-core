// -*- c++ -*-

// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include <Python.h>

#include <string.h>
#include <sstream>

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


static int sgInit(StatGroupPy_t *self, PyObject *args, PyObject *kwds __attribute__((unused)))
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


static PyObject* sgAddStat(PyObject *self, PyObject *args)
{
    PyErr_Clear();

    char *statName = NULL;
    PyObject *paramsDict = NULL;

    int argOK = PyArg_ParseTuple(args, "s|O!", &statName, &PyDict_Type, &paramsDict);
    if ( !argOK )
        return NULL;

    Params params = convertToParams(paramsDict);

    if ( !((StatGroupPy_t*)self)->ptr->addStatistic(statName, params) ) {
        PyErr_SetString(PyExc_RuntimeError, "Unable to create statistic");
        return NULL;
    }
    return PyLong_FromLong(0);
}


static PyObject* sgAddComp(PyObject *self, PyObject *args)
{
    PyErr_Clear();

    if ( PyObject_TypeCheck(args, &PyModel_ComponentType) ||
            PyObject_TypeCheck(args, &PyModel_SubComponentType) ) {
        ((StatGroupPy_t*)self)->ptr->addComponent(((ComponentPy_t*)args)->obj->getID());
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected Componet or SubComponent type");
        return NULL;
    }
    return PyLong_FromLong(0);
}


static PyObject* sgSetOutput(PyObject *self, PyObject *args)
{
    char *type = NULL;
    PyObject *paramsDict = NULL;

    int argOK = PyArg_ParseTuple(args, "s|O!", &type, &PyDict_Type, &paramsDict);
    if ( !argOK )
        return NULL;

    Params params = convertToParams(paramsDict);
    if ( !((StatGroupPy_t*)self)->ptr->setOutput(type, params) ) {
        PyErr_SetString(PyExc_RuntimeError, "Unable to set Statistic Output");
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
    {   "setOutput", sgSetOutput, METH_VARARGS,
        "Configure how the stats should be written" },
    {   "setFrequency", sgSetFreq, METH_O,
        "Set the frequency or rate (ie: \"10ms\", \"25khz\") to write out the statistics" },
    {   NULL, NULL, 0, NULL }
};


PyTypeObject PyModel_StatGroupType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.Link",                /* tp_name */
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



}  /* extern C */


