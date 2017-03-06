// -*- c++ -*-

// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
#include <sst/core/model/pymodel_link.h>

#include <sst/core/sst_types.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/configGraph.h>

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;


extern "C" {


ConfigComponent* ComponentHolder::getSubComp(const std::string &name)
{
    for ( auto &sc : getComp().subComponents ) {
        if ( sc.first == name )
            return &(sc.second);
    }
    return NULL;
}



const char* PyComponent::getName() const {
    return name;
}


ConfigComponent& PyComponent::getComp() {
    return gModel->getGraph()->getComponentMap()[id];
}


PyComponent* PyComponent::getBaseObj() {
    return this;
}

int PyComponent::compare(ComponentHolder *other) {
    PyComponent *o = dynamic_cast<PyComponent*>(other);
    if ( o ) {
        return (id < o->id) ? -1 : (id > o->id) ? 1 : 0;
    }
    return 1;
}


const char* PySubComponent::getName() const {
    return name;
}


ConfigComponent& PySubComponent::getComp() {
    return *(parent->getSubComp(name));
}


PyComponent* PySubComponent::getBaseObj() {
    return parent->getBaseObj();
}


int PySubComponent::compare(ComponentHolder *other) {
    PySubComponent *o = dynamic_cast<PySubComponent*>(other);
    if ( o ) {
        int pCmp = parent->compare(o->parent);
        if ( pCmp == 0 ) /* Parents are equal */
            pCmp = strcmp(name, o->name);
        return pCmp;
    }
    return -11;
}


static inline ConfigComponent& getComp(PyObject *pobj) {
    return ((ComponentPy_t*)pobj)->obj->getComp();
}


static int compInit(ComponentPy_t *self, PyObject *args, PyObject *kwds)
{
    char *name, *type;
    ComponentId_t useID = UNSET_COMPONENT_ID;
    if ( !PyArg_ParseTuple(args, "ss|k", &name, &type, &useID) )
        return -1;

    PyComponent *obj = new PyComponent(self);
    self->obj = obj;
    if ( useID == UNSET_COMPONENT_ID ) {
        obj->name = gModel->addNamePrefix(name);
        obj->id = gModel->addComponent(obj->name, type);
        gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating component [%s] of type [%s]: id [%lu]\n", name, type, obj->id);
    } else {
        obj->name = name;
        obj->id = useID;
    }

    return 0;
}


static void compDealloc(ComponentPy_t *self)
{
    if ( self->obj ) delete self->obj;
    self->ob_type->tp_free((PyObject*)self);
}



static PyObject* compAddParam(PyObject *self, PyObject *args)
{
    char *param = NULL;
    PyObject *value = NULL;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) )
        return NULL;

    ConfigComponent &c = getComp(self);

    PyObject *vstr = PyObject_CallMethod(value, (char*)"__str__", NULL);
    c.addParameter(param, PyString_AsString(vstr), true);
    Py_XDECREF(vstr);

    return PyInt_FromLong(0);
}


static PyObject* compAddParams(PyObject *self, PyObject *args)
{
    ConfigComponent &c = getComp(self);

    if ( !PyDict_Check(args) ) {
        return NULL;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *val;
    long count = 0;

    while ( PyDict_Next(args, &pos, &key, &val) ) {
        PyObject *kstr = PyObject_CallMethod(key, (char*)"__str__", NULL);
        PyObject *vstr = PyObject_CallMethod(val, (char*)"__str__", NULL);
        c.addParameter(PyString_AsString(kstr), PyString_AsString(vstr), true);
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return PyInt_FromLong(count);
}


static PyObject* compSetRank(PyObject *self, PyObject *args)
{
    ConfigComponent &c = getComp(self);

    PyErr_Clear();

    unsigned long rank = (unsigned long)-1;
    unsigned long thread = (unsigned long)0;

    if ( !PyArg_ParseTuple(args, "k|k", &rank, &thread) ) {
        return NULL;
    }

    c.setRank(RankInfo(rank, thread));

    return PyInt_FromLong(0);
}


static PyObject* compSetWeight(PyObject *self, PyObject *arg)
{
    ConfigComponent &c = getComp(self);

    PyErr_Clear();
    double weight = PyFloat_AsDouble(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    c.setWeight(weight);

    return PyInt_FromLong(0);
}


static PyObject* compAddLink(PyObject *self, PyObject *args)
{
    ComponentId_t id = ((PyComponent*)(((ComponentPy_t*)self)->obj))->id;;

    PyObject *plink = NULL;
    char *port = NULL, *lat = NULL;


    if ( !PyArg_ParseTuple(args, "O!s|s", &PyModel_LinkType, &plink, &port, &lat) ) {
        return NULL;
    }
    LinkPy_t* link = (LinkPy_t*)plink;
    if ( NULL == lat ) lat = link->latency;
    if ( NULL == lat ) return NULL;


	gModel->getOutput()->verbose(CALL_INFO, 4, 0, "Connecting component %lu to Link %s\n", id, link->name);
    gModel->addLink(id, link->name, port, lat, link->no_cut);

    return PyInt_FromLong(0);
}


static PyObject* compGetFullName(PyObject *self, PyObject *args)
{
    return PyString_FromString(getComp(self).name.c_str());
}

static int compCompare(PyObject *obj0, PyObject *obj1) {
    return ((ComponentPy_t*)obj0)->obj->compare(((ComponentPy_t*)obj1)->obj);
}


static PyObject* compSetSubComponent(PyObject *self, PyObject *args)
{
    char *name = NULL, *type = NULL;

    if ( !PyArg_ParseTuple(args, "ss", &name, &type) )
        return NULL;

    ConfigComponent &c = ((ComponentHolder*)self)->getComp();
    if ( NULL != c.addSubComponent(name, type) ) {
        PyObject *argList = Py_BuildValue("Oss", self, name, type);
        ComponentPy_t *subObj = (ComponentPy_t*)PyObject_CallObject((PyObject*)&PyModel_SubComponentType, argList);
        return (PyObject*)subObj;
    }

    char errMsg[1024] = {0};
    snprintf(errMsg, sizeof(errMsg)-1, "Failed to create subcomponent %s on %s.  Name already exists?\n", name, c.name.c_str());
    PyErr_SetString(PyExc_RuntimeError, errMsg);
    return NULL;
}


static PyObject* compEnableAllStatistics(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    PyObject*     statParamDict = NULL;
    ConfigComponent &c = getComp(self);

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!", &PyDict_Type, &statParamDict);

    if (argOK) {
        c.enableStatistic(STATALLFLAG);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            c.addStatisticParameter(STATALLFLAG, p.first, p.second);
        }

    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* compEnableStatistics(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    PyObject*     statList = NULL;
    PyObject*     statParamDict = NULL;
    Py_ssize_t    numStats = 0;
    ConfigComponent &c = getComp(self);

    PyErr_Clear();

    // Parse the Python Args and get A List Object and the optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "O!|O!", &PyList_Type, &statList, &PyDict_Type, &statParamDict);

    if (argOK) {
        // Generate the Statistic Parameters
        auto params = generateStatisticParameters(statParamDict);

        // Make sure we have a list
        if ( !PyList_Check(statList) ) {
            return NULL;
        }

        // Get the Number of Stats in the list, and enable them separatly,
        // also set their parameters
        numStats = PyList_Size(statList);
        for (uint32_t x = 0; x < numStats; x++) {
            PyObject* pylistitem = PyList_GetItem(statList, x);
            PyObject* pyname = PyObject_CallMethod(pylistitem, (char*)"__str__", NULL);

            c.enableStatistic(PyString_AsString(pyname));

            // Add the parameters
            for ( auto p : params ) {
                c.addStatisticParameter(PyString_AsString(pyname), p.first, p.second);
            }

            Py_XDECREF(pyname);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyMethodDef componentMethods[] = {
    {   "addParam",
        compAddParam, METH_VARARGS,
        "Adds a parameter(name, value)"},
    {   "addParams",
        compAddParams, METH_O,
        "Adds Multiple Parameters from a dict"},
    {   "setRank",
        compSetRank, METH_VARARGS,
        "Sets which rank on which this component should sit"},
    {   "setWeight",
        compSetWeight, METH_O,
        "Sets the weight of the component"},
    {   "addLink",
        compAddLink, METH_VARARGS,
        "Connects this component to a Link"},
	{	"getFullName",
		compGetFullName, METH_NOARGS,
		"Returns the full name, after any prefix, of the component."},
    {   "enableAllStatistics",
        compEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the component with optional parameters"},
    {   "enableStatistics",
        compEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the component with optional parameters"},
    {   "setSubComponent",
        compSetSubComponent, METH_VARARGS,
        "Bind a subcomponent to slot <name>, with type <type>"},
    {   NULL, NULL, 0, NULL }
};


PyTypeObject PyModel_ComponentType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.Component",           /* tp_name */
    sizeof(ComponentPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)compDealloc,   /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    compCompare,               /* tp_compare */
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
    "SST Component",           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    componentMethods,          /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)compInit,        /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};





static int subCompInit(ComponentPy_t *self, PyObject *args, PyObject *kwds)
{
    char *name, *type;
    PyObject *parent;
    ComponentId_t useID = UNSET_COMPONENT_ID;
    if ( !PyArg_ParseTuple(args, "Oss", &parent, &name, &type) )
        return -1;

    PySubComponent *obj = new PySubComponent(self);
    obj->parent = (ComponentHolder*)parent;

    obj->name = (char*)calloc(1, strlen(obj->parent->getName()) + 2 + strlen(name));
    sprintf(obj->name, "%s.%s", obj->parent->getName(), name);

    self->obj = obj;
    Py_INCREF(obj->parent->pobj);

    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating subcomponent [%s] of type [%s]]\n", name, type);

    return 0;
}


static void subCompDealloc(ComponentPy_t *self)
{
    if ( self->obj ) {
        PySubComponent *obj = (PySubComponent*)self->obj;
        Py_XDECREF(obj->parent->pobj);
        delete self->obj;
    }
    self->ob_type->tp_free((PyObject*)self);
}



static PyObject* subCompAddLink(PyObject *self, PyObject *args)
{
    /* TODO */
    gModel->getOutput()->fatal(CALL_INFO, 1, "SubComponent Links not yet implemented.\n");
    return NULL;
}



static PyMethodDef subComponentMethods[] = {
    {   "addParam",
        compAddParam, METH_VARARGS,
        "Adds a parameter(name, value)"},
    {   "addParams",
        compAddParams, METH_O,
        "Adds Multiple Parameters from a dict"},
    {   "addLink",
        subCompAddLink, METH_VARARGS,
        "Connects this component to a Link"},
	{	"getFullName",
		compGetFullName, METH_NOARGS,
		"Returns the full name, after any prefix, of the component."},
    {   "enableAllStatistics",
        compEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the component with optional parameters"},
    {   "enableStatistics",
        compEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the component with optional parameters"},
    {   "setSubComponent",
        compSetSubComponent, METH_VARARGS,
        "Bind a subcomponent to slot <name>, with type <type>"},
    {   NULL, NULL, 0, NULL }
};


PyTypeObject PyModel_SubComponentType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.SubComponent",        /* tp_name */
    sizeof(ComponentPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)subCompDealloc,/* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    compCompare,               /* tp_compare */
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
    "SST SubComponent",        /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    subComponentMethods,       /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)subCompInit,     /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};


}  /* extern C */


