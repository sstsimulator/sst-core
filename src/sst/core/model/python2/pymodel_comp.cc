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

#include <string.h>

#include "sst/core/model/python2/pymodel.h"
#include "sst/core/model/python2/pymodel_comp.h"
#include "sst/core/model/python2/pymodel_link.h"

#include "sst/core/sst_types.h"
#include "sst/core/simulation.h"
#include "sst/core/component.h"
#include "sst/core/subcomponent.h"
#include "sst/core/configGraph.h"

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;


extern "C" {


ConfigComponent* ComponentHolder::getSubComp(const std::string& name, int slot_num)
{
    for ( auto &sc : getComp()->subComponents ) {
        if ( sc.name == name && sc.slot_num == slot_num)
            return &sc;
    }
    return nullptr;
}

ComponentId_t ComponentHolder::getID()
{
    return id;
}

std::string ComponentHolder::getName() {
    return getComp()->name;
}

ConfigComponent* ComponentHolder::getComp() {
    return gModel->getGraph()->findComponent(id);
}
    

int ComponentHolder::compare(ComponentHolder *other) {
    if (id < other->id) return -1;
    else if (id > other->id ) return 1;
    else return 0;    
}



int PySubComponent::getSlot() {
    return getComp()->slot_num;
}


static int compInit(ComponentPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *name, *type;
    ComponentId_t useID = UNSET_COMPONENT_ID;
    if ( !PyArg_ParseTuple(args, "ss|k", &name, &type, &useID) )
        return -1;

    PyComponent *obj;
    if ( useID == UNSET_COMPONENT_ID ) {
        char* prefixed_name = gModel->addNamePrefix(name);
        ComponentId_t id = gModel->addComponent(prefixed_name, type);
        obj = new PyComponent(self,id);
        gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating component [%s] of type [%s]: id [%" PRIu64 "]\n", name, type, id);
    } else {
        obj = new PyComponent(self,useID);
    }
    self->obj = obj;

    return 0;
}


static void compDealloc(ComponentPy_t *self)
{
    if ( self->obj ) delete self->obj;
    self->ob_type->tp_free((PyObject*)self);
}



static PyObject* compAddParam(PyObject *self, PyObject *args)
{
    char* param = nullptr;
    PyObject *value = nullptr;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) )
        return nullptr;

    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    // Get the string-ized value by calling __str__ function of the
    // value object
    PyObject *vstr = PyObject_CallMethod(value, (char*)"__str__", nullptr);
    c->addParameter(param, PyString_AsString(vstr), true);
    Py_XDECREF(vstr);

    return PyLong_FromLong(0);
}


static PyObject* compAddParams(PyObject *self, PyObject *args)
{
    ConfigComponent *c = getComp(self);
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
        c->addParameter(PyString_AsString(kstr), PyString_AsString(vstr), true);
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return PyLong_FromLong(count);
}


static PyObject* compSetRank(PyObject *self, PyObject *args)
{
    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    PyErr_Clear();

    unsigned long rank = (unsigned long)-1;
    unsigned long thread = (unsigned long)0;

    if ( !PyArg_ParseTuple(args, "k|k", &rank, &thread) ) {
        return nullptr;
    }

    c->setRank(RankInfo(rank, thread));

    return PyLong_FromLong(0);
}


static PyObject* compSetWeight(PyObject *self, PyObject *arg)
{
    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    PyErr_Clear();
    double weight = PyFloat_AsDouble(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    c->setWeight(weight);

    return PyLong_FromLong(0);
}


static PyObject* compAddLink(PyObject *self, PyObject *args)
{
    ConfigComponent *c = getComp(self);
    ComponentId_t id = c->id;

    PyObject *plink = nullptr;
    char *port = nullptr, *lat = nullptr;


    if ( !PyArg_ParseTuple(args, "O!s|s", &PyModel_LinkType, &plink, &port, &lat) ) {
        return nullptr;
    }
    LinkPy_t* link = (LinkPy_t*)plink;
    if ( nullptr == lat ) lat = link->latency;
    if ( nullptr == lat ) return nullptr;


    gModel->getOutput()->verbose(CALL_INFO, 4, 0, "Connecting component %" PRIu64 " to Link %s (lat: %s)\n", id, link->name, lat);
    gModel->addLink(id, link->name, port, lat, link->no_cut);

    return PyLong_FromLong(0);
}


static PyObject* compGetFullName(PyObject *self, PyObject *UNUSED(args))
{
    return PyString_FromString(getComp(self)->name.c_str());
}

static int compCompare(PyObject *obj0, PyObject *obj1) {
    return ((ComponentHolder*)obj0)->compare(((ComponentHolder*)obj1));
}


static PyObject* compSetSubComponent(PyObject *self, PyObject *args)
{
    char *name = nullptr, *type = nullptr;
    int slot = 0;

    
    if ( !PyArg_ParseTuple(args, "ss|i", &name, &type, &slot) )
        return nullptr;

    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    ComponentId_t subC_id = c->getNextSubComponentID();
    ConfigComponent* sub = c->addSubComponent(subC_id, name, type, slot);
    if ( nullptr != sub ) {
        PyObject *argList = Py_BuildValue("Ok", self, subC_id);
        PyObject *subObj = PyObject_CallObject((PyObject*)&PyModel_SubComponentType, argList);
        Py_DECREF(argList);
        return subObj;
    }

    char errMsg[1024] = {0};
    snprintf(errMsg, sizeof(errMsg)-1, "Failed to create subcomponent %s on %s.  Already attached a subcomponent at that slot name and number?\n", name, c->name.c_str());
    PyErr_SetString(PyExc_RuntimeError, errMsg);
    return nullptr;
}

static PyObject* compSetCoords(PyObject *self, PyObject *args)
{
    std::vector<double> coords(3, 0.0);
    if ( !PyArg_ParseTuple(args, "d|dd", &coords[0], &coords[1], &coords[2]) ) {
        PyObject* list = nullptr;
        if ( PyArg_ParseTuple(args, "O!", &PyList_Type, &list) && PyList_Size(list) > 0 ) {
            coords.clear();
            for ( Py_ssize_t i = 0 ; i < PyList_Size(list) ; i++ ) {
                coords.push_back(PyFloat_AsDouble(PyList_GetItem(list, 0)));
                if ( PyErr_Occurred() ) goto error;
            }
        } else if ( PyArg_ParseTuple(args, "O!", &PyTuple_Type, &list) && PyTuple_Size(list) > 0 ) {
            coords.clear();
            for ( Py_ssize_t i = 0 ; i < PyTuple_Size(list) ; i++ ) {
                coords.push_back(PyFloat_AsDouble(PyTuple_GetItem(list, 0)));
                if ( PyErr_Occurred() ) goto error;
            }
        } else {
error:
            PyErr_SetString(PyExc_TypeError, "compSetCoords() expects arguments of 1-3 doubles, or a list/tuple of doubles");
            return nullptr;
        }
    }

    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;
    c->setCoordinates(coords);

    return PyLong_FromLong(0);
}

static PyObject* compSetStatisticLoadLevel(PyObject *self, PyObject *args) {
    int           argOK = 0;
    uint8_t       loadLevel = STATISTICLOADLEVELUNINITIALIZED;
    ConfigComponent *c = getComp(self);
    bool          apply_to_children = false;

    PyErr_Clear();

    argOK = PyArg_ParseTuple(args, "H|i", &loadLevel, &apply_to_children);
    loadLevel = loadLevel & 0xff;
    
    if (argOK) {
        c->setStatisticLoadLevel(loadLevel,apply_to_children);
    }
    else {
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyObject* compEnableAllStatistics(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    PyObject*     statParamDict = nullptr;
    ConfigComponent *c = getComp(self);
    bool          apply_to_children = false;

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!i", &PyDict_Type, &statParamDict,&apply_to_children);

    if (argOK) {
        c->enableStatistic(STATALLFLAG,apply_to_children);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            c->addStatisticParameter(STATALLFLAG, p.first, p.second, apply_to_children);
        }

    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyObject* compEnableStatistics(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    PyObject*     statList = nullptr;
    char*         stat_str = nullptr;
    PyObject*     statParamDict = nullptr;
    Py_ssize_t    numStats = 0;
    bool          apply_to_children = false;
    ConfigComponent *c = getComp(self);

    PyErr_Clear();

    // Can either have a single string, or a list of strings.  Try single string first
    argOK = PyArg_ParseTuple(args,"s|O!i", &stat_str, &PyDict_Type, &statParamDict, &apply_to_children);
    if ( argOK ) {
        statList = PyList_New(1);
        PyList_SetItem(statList,0,PyString_FromString(stat_str));
    }
    else  {
        PyErr_Clear();
        // Try list version
        argOK = PyArg_ParseTuple(args, "O!|O!i", &PyList_Type, &statList, &PyDict_Type, &statParamDict, &apply_to_children);
        if ( argOK )  Py_INCREF(statList);
    }
        
    if (argOK) {
        // Generate the Statistic Parameters
        auto params = generateStatisticParameters(statParamDict);

        // Make sure we have a list
        if ( !PyList_Check(statList) ) {
            return nullptr;
        }

        // Get the Number of Stats in the list, and enable them separately,
        // also set their parameters
        numStats = PyList_Size(statList);
        for (uint32_t x = 0; x < numStats; x++) {
            PyObject* pylistitem = PyList_GetItem(statList, x);
            PyObject* pyname = PyObject_CallMethod(pylistitem, (char*)"__str__", nullptr);

            c->enableStatistic(PyString_AsString(pyname),apply_to_children);

            // Add the parameters
            for ( auto p : params ) {
                c->addStatisticParameter(PyString_AsString(pyname), p.first, p.second, apply_to_children);
            }

        }
        Py_XDECREF(statList);
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
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
    {   "getFullName",
        compGetFullName, METH_NOARGS,
        "Returns the full name, after any prefix, of the component."},
    {   "setStatisticLoadLevel",
        compSetStatisticLoadLevel, METH_VARARGS,
        "Sets the statistics load level for this component"},
    {   "enableAllStatistics",
        compEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the component with optional parameters"},
    {   "enableStatistics",
        compEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the component with optional parameters"},
    {   "setSubComponent",
        compSetSubComponent, METH_VARARGS,
        "Bind a subcomponent to slot <name>, with type <type>"},
    {   "setCoordinates",
        compSetCoords, METH_VARARGS,
        "Set (X,Y,Z) coordinates of this component, for use with visualization"},
    {   nullptr, nullptr, 0, nullptr }
};


PyTypeObject PyModel_ComponentType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "sst.Component",           /* tp_name */
    sizeof(ComponentPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)compDealloc,   /* tp_dealloc */
    nullptr,                         /* tp_print */
    nullptr,                         /* tp_getattr */
    nullptr,                         /* tp_setattr */
    compCompare,               /* tp_compare */
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
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST Component",           /* tp_doc */
    nullptr,                         /* tp_traverse */
    nullptr,                         /* tp_clear */
    nullptr,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    nullptr,                         /* tp_iter */
    nullptr,                         /* tp_iternext */
    componentMethods,          /* tp_methods */
    nullptr,                         /* tp_members */
    nullptr,                         /* tp_getset */
    nullptr,                         /* tp_base */
    nullptr,                         /* tp_dict */
    nullptr,                         /* tp_descr_get */
    nullptr,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)compInit,        /* tp_init */
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
};





static int subCompInit(ComponentPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    ComponentId_t id;
    PyObject *parent;
    // if ( !PyArg_ParseTuple(args, "Ossii", &parent, &name, &type, &slot, &id) )
    if ( !PyArg_ParseTuple(args, "Ok", &parent, &id) )
        return -1;

    PySubComponent *obj = new PySubComponent(self,id);
    
    self->obj = obj;

    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating subcomponent [%s] of type [%s]]\n", getComp(self)->name.c_str(), getComp(self)->type.c_str());

    return 0;
}


static void subCompDealloc(ComponentPy_t *self)
{
    if ( self->obj ) {
        delete self->obj;
    }
    self->ob_type->tp_free((PyObject*)self);
}



static PyMethodDef subComponentMethods[] = {
    {   "addParam",
        compAddParam, METH_VARARGS,
        "Adds a parameter(name, value)"},
    {   "addParams",
        compAddParams, METH_O,
        "Adds Multiple Parameters from a dict"},
    {   "addLink",
        compAddLink, METH_VARARGS,
        "Connects this subComponent to a Link"},
    {   "setStatisticLoadLevel",
        compSetStatisticLoadLevel, METH_VARARGS,
        "Sets the statistics load level for this component"},
    {   "enableAllStatistics",
        compEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the component with optional parameters"},
    {   "enableStatistics",
        compEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the component with optional parameters"},
    {   "setSubComponent",
        compSetSubComponent, METH_VARARGS,
        "Bind a subcomponent to slot <name>, with type <type>"},
    {   "setCoordinates",
        compSetCoords, METH_VARARGS,
        "Set (X,Y,Z) coordinates of this component, for use with visualization"},
    {   nullptr, nullptr, 0, nullptr }
};


PyTypeObject PyModel_SubComponentType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "sst.SubComponent",        /* tp_name */
    sizeof(ComponentPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)subCompDealloc,/* tp_dealloc */
    nullptr,                         /* tp_print */
    nullptr,                         /* tp_getattr */
    nullptr,                         /* tp_setattr */
    compCompare,               /* tp_compare */
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
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST SubComponent",        /* tp_doc */
    nullptr,                         /* tp_traverse */
    nullptr,                         /* tp_clear */
    nullptr,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    nullptr,                         /* tp_iter */
    nullptr,                         /* tp_iternext */
    subComponentMethods,       /* tp_methods */
    nullptr,                         /* tp_members */
    nullptr,                         /* tp_getset */
    nullptr,                         /* tp_base */
    nullptr,                         /* tp_dict */
    nullptr,                         /* tp_descr_get */
    nullptr,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)subCompInit,     /* tp_init */
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
};


}  /* extern C */


