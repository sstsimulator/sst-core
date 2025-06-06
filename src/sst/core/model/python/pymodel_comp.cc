// -*- c++ -*-

// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/model/python/pymodel_comp.h"

#include "sst/core/component.h"
#include "sst/core/componentInfo.h"
#include "sst/core/configGraph.h"
#include "sst/core/model/python/pymacros.h"
#include "sst/core/model/python/pymodel.h"
#include "sst/core/model/python/pymodel_link.h"
#include "sst/core/model/python/pymodel_stat.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_types.h"
#include "sst/core/stringize.h"
#include "sst/core/subcomponent.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <string.h>

using namespace SST;
using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition* gModel;

ConfigComponent*
ComponentHolder::getSubComp(const std::string& name, int slot_num)
{
    for ( auto sc : getComp()->subComponents ) {
        if ( sc->name == name && sc->slot_num == slot_num ) return sc;
    }
    return nullptr;
}

ComponentId_t
ComponentHolder::getID()
{
    return id;
}

std::string
ComponentHolder::getName()
{
    return getComp()->name;
}

ConfigComponent*
ComponentHolder::getComp()
{
    return gModel->getGraph()->findComponent(id);
}

int
ComponentHolder::compare(ComponentHolder* other)
{
    if ( id < other->id )
        return -1;
    else if ( id > other->id )
        return 1;
    else
        return 0;
}

int
PySubComponent::getSlot()
{
    return getComp()->slot_num;
}

static int
compInit(ComponentPy_t* self, PyObject* args, PyObject* UNUSED(kwds))
{
    char *        name, *type;
    ComponentId_t useID = UNSET_COMPONENT_ID;
    if ( !PyArg_ParseTuple(args, "ss|k", &name, &type, &useID) ) return -1;

    PyComponent* obj;
    if ( useID == UNSET_COMPONENT_ID ) {
        char*         prefixed_name = gModel->addNamePrefix(name);
        ComponentId_t id            = gModel->addComponent(prefixed_name, type);
        obj                         = new PyComponent(self, id);
        free(prefixed_name);
        gModel->getOutput()->verbose(
            CALL_INFO, 3, 0, "Creating component [%s] of type [%s]: id [%" PRIu64 "]\n", name, type, id);
    }
    else {
        obj = new PyComponent(self, useID);
    }
    self->obj = obj;

    return 0;
}

static void
compDealloc(ComponentPy_t* self)
{
    if ( self->obj ) delete self->obj;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
compAddParam(PyObject* self, PyObject* args)
{
    char*     param = nullptr;
    PyObject* value = nullptr;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) ) return nullptr;

    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;

    // Get the string-ized value by calling __str__ function of the
    // value object
    PyObject* vstr = PyObject_CallMethod(value, (char*)"__str__", nullptr);
    c->addParameter(param, SST_ConvertToCppString(vstr), true);
    Py_XDECREF(vstr);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
compAddParams(PyObject* self, PyObject* args)
{
    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;

    if ( !PyDict_Check(args) ) {
        return nullptr;
    }

    Py_ssize_t pos = 0;
    PyObject * key, *val;
    long       count = 0;

    while ( PyDict_Next(args, &pos, &key, &val) ) {
        PyObject* kstr = PyObject_CallMethod(key, (char*)"__str__", nullptr);
        PyObject* vstr = PyObject_CallMethod(val, (char*)"__str__", nullptr);
        c->addParameter(SST_ConvertToCppString(kstr), SST_ConvertToCppString(vstr), true);
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return SST_ConvertToPythonLong(count);
}

static PyObject*
compSetRank(PyObject* self, PyObject* args)
{
    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;

    PyErr_Clear();

    unsigned long rank   = (unsigned long)-1;
    unsigned long thread = (unsigned long)0;

    if ( !PyArg_ParseTuple(args, "k|k", &rank, &thread) ) {
        return nullptr;
    }

    c->setRank(RankInfo(rank, thread));

    return SST_ConvertToPythonLong(0);
}

static PyObject*
compSetWeight(PyObject* self, PyObject* arg)
{
    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;

    PyErr_Clear();
    double weight = PyFloat_AsDouble(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    c->setWeight(weight);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
compAddLink(PyObject* self, PyObject* args)
{
    ConfigComponent* c  = getComp(self);
    ComponentId_t    id = c->id;

    PyObject*   plink = nullptr;
    PyObject *  plat = nullptr, *lstr = nullptr;
    char*       port = nullptr;
    const char* lat  = nullptr;

    if ( !PyArg_ParseTuple(args, "O!s|O", &PyModel_LinkType, &plink, &port, &plat) ) {
        return nullptr;
    }
    LinkPy_t* link = (LinkPy_t*)plink;

    if ( nullptr != plat ) {
        lstr = PyObject_CallMethod(plat, (char*)"__str__", nullptr);
        lat  = SST_ConvertToCppString(lstr);
    }
    if ( nullptr == lat ) lat = link->latency;
    if ( nullptr == lat ) return nullptr;

    gModel->getOutput()->verbose(
        CALL_INFO, 4, 0, "Connecting component %" PRIu64 " to Link %s (lat: %s)\n", id, link->name, lat);
    gModel->addLink(id, link->name, port, lat, link->no_cut);

    Py_XDECREF(lstr);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
compAddPortModule(PyObject* self, PyObject* args)
{
    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;
    PyErr_Clear();
    // we can have 2 or 3 arguments
    // mandatory port
    // mandatory type
    // optional parameters

    char*     port      = nullptr;
    char*     type      = nullptr;
    PyObject* py_params = nullptr;

    if ( !PyArg_ParseTuple(args, "ss|O", &port, &type, &py_params) ) return nullptr;
    auto params = pythonToCppParams(py_params);
    c->addPortModule(port, type, params);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
compGetFullName(PyObject* self, PyObject* UNUSED(args))
{
    return SST_ConvertToPythonString(getComp(self)->getFullName().c_str());
}

#if PY_MAJOR_VERSION >= 3
static PyObject*
compCompare(PyObject* obj0, PyObject* obj1, int op)
{
    PyObject* result;
    bool      cmp = false;
    switch ( op ) {
    case Py_LT:
        cmp = ((ComponentHolder*)obj0)->compare(((ComponentPy_t*)obj1)->obj) == -1;
        break;
    case Py_LE:
        cmp = ((ComponentHolder*)obj0)->compare(((ComponentPy_t*)obj1)->obj) != 1;
        break;
    case Py_EQ:
        cmp = ((ComponentHolder*)obj0)->compare(((ComponentPy_t*)obj1)->obj) == 0;
        break;
    case Py_NE:
        cmp = ((ComponentHolder*)obj0)->compare(((ComponentPy_t*)obj1)->obj) != 0;
        break;
    case Py_GT:
        cmp = ((ComponentHolder*)obj0)->compare(((ComponentPy_t*)obj1)->obj) == 1;
        break;
    case Py_GE:
        cmp = ((ComponentHolder*)obj0)->compare(((ComponentPy_t*)obj1)->obj) != -1;
        break;
    }
    result = cmp ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}
#else
static int
compCompare(PyObject* obj0, PyObject* obj1)
{
    return ((ComponentHolder*)obj0)->compare(((ComponentHolder*)obj1));
}
#endif

static PyObject*
compGetType(PyObject* self, PyObject* UNUSED(args))
{
    return PyUnicode_FromString(getComp(self)->type.c_str());
}

static PyObject*
compSetSubComponent(PyObject* self, PyObject* args)
{
    char *name = nullptr, *type = nullptr;
    int   slot = 0;

    if ( !PyArg_ParseTuple(args, "ss|i", &name, &type, &slot) ) return nullptr;

    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;

    ConfigComponent* sub = c->addSubComponent(name, type, slot);
    if ( nullptr != sub ) {
        PyObject* argList = Py_BuildValue("Ok", self, sub->id);
        PyObject* subObj  = PyObject_CallObject((PyObject*)&PyModel_SubComponentType, argList);
        Py_DECREF(argList);
        return subObj;
    }

    std::string errMsg = format_string(
        "Failed to create subcomponent %s on %s.  Already attached a subcomponent at that slot name and number?\n",
        name, c->name.c_str());

    PyErr_SetString(PyExc_RuntimeError, errMsg.c_str());
    return nullptr;
}

/**
 * @brief compSetStatistic Fill a stat slot with a shared statistic
 * @param self The Python component
 * @param args The name of the stat slot to fill and the Python stat object o use
 * @return The Python stat object is returned back on success
 */
static PyObject*
compSetStatistic(PyObject* self, PyObject* args)
{
    PyObject* statObj;
    char*     name = nullptr;
    if ( !PyArg_ParseTuple(args, "sO", &name, &statObj) ) return nullptr;

    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;

    ConfigStatistic* s     = getStat(statObj);
    bool             valid = c->reuseStatistic(name, s->id);
    if ( valid ) {
        Py_INCREF(statObj);
        return statObj;
    }
    else {
        return nullptr;
    }
}

/**
 * @brief compEnableStatistic. Creates a new statistic object unique to this slot
 *        rather than using a shared statistic created by compCreateStatistic
 * @param self The Python component
 * @param args The name of the stat slot to fill and optionally a params dictionary
 * @return The Python stat object assigned to this slot
 */
static PyObject*
compEnableStatistic(PyObject* self, PyObject* args)
{
    char*     name      = nullptr;
    PyObject* py_params = nullptr;

    if ( !PyArg_ParseTuple(args, "s|O", &name, &py_params) ) {
        return nullptr;
    }
    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;

    return buildEnabledStatistic(c, name, py_params, false /*do not apply to children*/);
}

static PyObject*
compSetCoords(PyObject* self, PyObject* args)
{
    std::vector<double> coords(3, 0.0);
    if ( !PyArg_ParseTuple(args, "d|dd", &coords[0], &coords[1], &coords[2]) ) {
        PyObject* list = nullptr;
        if ( PyArg_ParseTuple(args, "O!", &PyList_Type, &list) && PyList_Size(list) > 0 ) {
            coords.clear();
            for ( Py_ssize_t i = 0; i < PyList_Size(list); i++ ) {
                coords.push_back(PyFloat_AsDouble(PyList_GetItem(list, 0)));
                if ( PyErr_Occurred() ) goto error;
            }
        }
        else if ( PyArg_ParseTuple(args, "O!", &PyTuple_Type, &list) && PyTuple_Size(list) > 0 ) {
            coords.clear();
            for ( Py_ssize_t i = 0; i < PyTuple_Size(list); i++ ) {
                coords.push_back(PyFloat_AsDouble(PyTuple_GetItem(list, 0)));
                if ( PyErr_Occurred() ) goto error;
            }
        }
        else {
error:
            PyErr_SetString(
                PyExc_TypeError, "compSetCoords() expects arguments of 1-3 doubles, or a list/tuple of doubles");
            return nullptr;
        }
    }

    ConfigComponent* c = getComp(self);
    if ( nullptr == c ) return nullptr;
    c->setCoordinates(coords);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
compSetStatisticLoadLevel(PyObject* self, PyObject* args)
{
    int              argOK             = 0;
    int              loadLevel         = STATISTICLOADLEVELUNINITIALIZED;
    ConfigComponent* c                 = getComp(self);
    int              apply_to_children = 0;

    PyErr_Clear();

    argOK     = PyArg_ParseTuple(args, "i|i", &loadLevel, &apply_to_children);
    loadLevel = loadLevel & 0xff;

    if ( argOK ) {
        c->setStatisticLoadLevel(loadLevel, apply_to_children);
    }
    else {
        return nullptr;
    }

    return SST_ConvertToPythonLong(0);
}

/**
 * @brief compEnableAllStatistics
 * @param self The Python component
 * @param args Optionally a params dictionary
 * @return Just a flag (0) indicating success. The enable all functions
 *         do not return Python stat handles.
 */
static PyObject*
compEnableAllStatistics(PyObject* self, PyObject* args)
{
    int              argOK             = 0;
    PyObject*        statParamDict     = nullptr;
    ConfigComponent* c                 = getComp(self);
    int              apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!i", &PyDict_Type, &statParamDict, &apply_to_children);

    if ( argOK ) {
        c->enableStatistic(STATALLFLAG, pythonToCppParams(statParamDict), apply_to_children);
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

/**
 * @brief compEnableStatistics
 * @param self  The Python component
 * @param args  The statistic slot to enable (single) or a list of slots to enable, optionally also params dictionary
 * @return A list of Python statistic templates.  Each Statistic template will generate a unique object
 *         in C++ rather than a shared object.  If wanted a shared object, use compCreateStatistic
 *         and then assigned the created statistic to slots.
 */
static PyObject*
compEnableStatistics(PyObject* self, PyObject* args)
{
    int              argOK             = 0;
    PyObject*        statList          = nullptr;
    char*            stat_str          = nullptr;
    PyObject*        statParamDict     = nullptr;
    int              apply_to_children = 0;
    ConfigComponent* cc                = getComp(self);

    PyErr_Clear();

    // Can either have a single string, or a list of strings.  Try single string first
    argOK = PyArg_ParseTuple(args, "s|O!i", &stat_str, &PyDict_Type, &statParamDict, &apply_to_children);
    if ( argOK ) {
        statList = PyList_New(1);
        PyList_SetItem(statList, 0, SST_ConvertToPythonString(stat_str));
    }
    else {
        PyErr_Clear();
        apply_to_children = 0;
        // Try list version
        argOK =
            PyArg_ParseTuple(args, "O!|O!i", &PyList_Type, &statList, &PyDict_Type, &statParamDict, &apply_to_children);
        if ( argOK ) Py_INCREF(statList);
    }

    if ( argOK ) {
        // Generate the Statistic Parameters
        // Make sure we have a list
        if ( !PyList_Check(statList) ) {
            return nullptr;
        }
        PyObject* statObjList = buildEnabledStatistics(cc, statList, statParamDict, apply_to_children);
        Py_XDECREF(statList);
        return statObjList;
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

/**
 * @brief compCreateStatistic
 * @param self The Python component
 * @param args The name assigned to the statistic in Python (not C++). Optionally params.
 * @return A Python object representing the statistic. All statistic slots this is assigned to
 *         will be linked to the same statistic object.
 */
static PyObject*
compCreateStatistic(PyObject* self, PyObject* args)
{
    //    char* param = nullptr;
    ConfigComponent* comp = getComp(self);
    if ( nullptr == comp ) return nullptr;

    // we can have 1 or 2 arguments
    // mandatory name
    // optional parameters
    char*     name      = nullptr;
    PyObject* py_params = nullptr;

    if ( !PyArg_ParseTuple(args, "s|O", &name, &py_params) ) return nullptr;

    if ( nullptr == comp ) return nullptr;

    ConfigStatistic* cs = comp->createStatistic();
    if ( cs == nullptr ) {
        std::string errMsg = format_string("Failed to create statistic '%s' on '%s'", name, comp->name.c_str());
        PyErr_SetString(PyExc_RuntimeError, errMsg.c_str());
        return nullptr;
    }

    if ( py_params ) {
        cs->params.insert(pythonToCppParams(py_params));
    }

    cs->shared = true;
    cs->name   = name;

    return buildStatisticObject(cs->id);
}

static PyObject*
compAddSharedParamSet(PyObject* self, PyObject* arg)
{
    const char* set = nullptr;
    PyErr_Clear();
    set = SST_ConvertToCppString(arg);

    ConfigComponent* c = getComp(self);

    if ( set != nullptr ) {
        c->addSharedParamSet(set);
    }
    else {
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyMethodDef componentMethods[] = { { "addParam", compAddParam, METH_VARARGS, "Adds a parameter(name, value)" },
    { "addParams", compAddParams, METH_O, "Adds Multiple Parameters from a dict" },
    { "setRank", compSetRank, METH_VARARGS, "Sets which rank on which this component should sit" },
    { "setWeight", compSetWeight, METH_O, "Sets the weight of the component" },
    { "addLink", compAddLink, METH_VARARGS, "Connects this component to a Link" },
    { "addPortModule", compAddPortModule, METH_VARARGS, "Connect Port Module to this component" },
    { "getFullName", compGetFullName, METH_NOARGS, "Returns the full name of the component." },
    { "getType", compGetType, METH_NOARGS, "Returns the type of the component." },
    { "setStatisticLoadLevel", compSetStatisticLoadLevel, METH_VARARGS,
        "Sets the statistics load level for this component" },
    { "createStatistic", compCreateStatistic, METH_VARARGS,
        "Create a Statistic Object in the component with optional parameters" },
    { "enableAllStatistics", compEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the component with optional parameters" },
    { "enableStatistic", compEnableStatistic, METH_VARARGS,
        "Enable a statistic with a name and return a handle to it" },
    { "enableStatistics", compEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the component with optional parameters" },
    { "setStatistic", compSetStatistic, METH_VARARGS, "Bind a statistic with name <name> to a statistic object" },
    { "setSubComponent", compSetSubComponent, METH_VARARGS, "Bind a subcomponent to slot <name>, with type <type>" },
    { "setCoordinates", compSetCoords, METH_VARARGS,
        "Set (X,Y,Z) coordinates of this component, for use with visualization" },
    { "addSharedParamSet", compAddSharedParamSet, METH_O, "Add shared parameter set to the component" },
    { "addGlobalParamSet", compAddSharedParamSet, METH_O, "Add shared parameter set to the component" },
    { nullptr, nullptr, 0, nullptr } };

#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
DISABLE_WARN_DEPRECATED_DECLARATION
#endif
#endif
PyTypeObject PyModel_ComponentType = {
    SST_PY_OBJ_HEAD "sst.Component", /* tp_name */
    sizeof(ComponentPy_t),           /* tp_basicsize */
    0,                               /* tp_itemsize */
    (destructor)compDealloc,         /* tp_dealloc */
    0,                               /* tp_vectorcall_offset */
    nullptr,                         /* tp_getattr */
    nullptr,                         /* tp_setattr */
    nullptr,                         /* tp_as_sync */
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
    "SST Component",                 /* tp_doc */
    nullptr,                         /* tp_traverse */
    nullptr,                         /* tp_clear */
    compCompare,                     /* tp_rich_compare */
    0,                               /* tp_weaklistoffset */
    nullptr,                         /* tp_iter */
    nullptr,                         /* tp_iternext */
    componentMethods,                /* tp_methods */
    nullptr,                         /* tp_members */
    nullptr,                         /* tp_getset */
    nullptr,                         /* tp_base */
    nullptr,                         /* tp_dict */
    nullptr,                         /* tp_descr_get */
    nullptr,                         /* tp_descr_set */
    0,                               /* tp_dictoffset */
    (initproc)compInit,              /* tp_init */
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
    nullptr,                         /* tp_finalize */
    SST_TP_VECTORCALL                /* Python3.8+ */
    SST_TP_PRINT_DEP                 /* Python3.8 only */
    SST_TP_WATCHED                   /* Python3.12+ only */
    SST_TP_VERSIONS_USED             /* Python3.13+ only */
};
#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
REENABLE_WARNING
#endif
#endif

static int
subCompInit(ComponentPy_t* self, PyObject* args, PyObject* UNUSED(kwds))
{
    ComponentId_t id;
    PyObject*     parent;
    // if ( !PyArg_ParseTuple(args, "Ossii", &parent, &name, &type, &slot, &id) )
    if ( !PyArg_ParseTuple(args, "Ok", &parent, &id) ) return -1;

    PySubComponent* obj = new PySubComponent(self, id);

    self->obj = obj;

    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating subcomponent [%s] of type [%s]]\n",
        getComp((PyObject*)self)->name.c_str(), getComp((PyObject*)self)->type.c_str());

    return 0;
}

static void
subCompDealloc(ComponentPy_t* self)
{
    if ( self->obj ) {
        delete self->obj;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMethodDef subComponentMethods[] = { { "addParam", compAddParam, METH_VARARGS,
                                                 "Adds a parameter(name, value)" },
    { "addParams", compAddParams, METH_O, "Adds Multiple Parameters from a dict" },
    { "addLink", compAddLink, METH_VARARGS, "Connects this subComponent to a Link" },
    { "getFullName", compGetFullName, METH_NOARGS, "Returns the full name, after any prefix, of the component." },
    { "getType", compGetType, METH_NOARGS, "Returns the type of the component." },
    { "setStatisticLoadLevel", compSetStatisticLoadLevel, METH_VARARGS,
        "Sets the statistics load level for this component" },
    { "enableAllStatistics", compEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the component with optional parameters" },
    { "enableStatistics", compEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the component with optional parameters" },
    { "enableStatistic", compEnableStatistic, METH_VARARGS,
        "Enable a statistic with a name and return a handle to it" },
    { "setStatistic", compSetStatistic, METH_VARARGS, "Reuse a statistic for the binding" },
    { "setSubComponent", compSetSubComponent, METH_VARARGS, "Bind a subcomponent to slot <name>, with type <type>" },
    { "addSharedParamSet", compAddSharedParamSet, METH_O, "Add shared parameter set to the component" },
    { "addGlobalParamSet", compAddSharedParamSet, METH_O, "Add shared parameter set to the component" },
    { "setCoordinates", compSetCoords, METH_VARARGS,
        "Set (X,Y,Z) coordinates of this component, for use with visualization" },
    { nullptr, nullptr, 0, nullptr } };

#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
DISABLE_WARN_DEPRECATED_DECLARATION
#endif
#endif
PyTypeObject PyModel_SubComponentType = {
    SST_PY_OBJ_HEAD "sst.SubComponent", /* tp_name */
    sizeof(ComponentPy_t),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)subCompDealloc,         /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    nullptr,                            /* tp_getattr */
    nullptr,                            /* tp_setattr */
    nullptr,                            /* tp_as_sync */
    nullptr,                            /* tp_repr */
    nullptr,                            /* tp_as_number */
    nullptr,                            /* tp_as_sequence */
    nullptr,                            /* tp_as_mapping */
    nullptr,                            /* tp_hash */
    nullptr,                            /* tp_call */
    nullptr,                            /* tp_str */
    nullptr,                            /* tp_getattro */
    nullptr,                            /* tp_setattro */
    nullptr,                            /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    "SST SubComponent",                 /* tp_doc */
    nullptr,                            /* tp_traverse */
    nullptr,                            /* tp_clear */
    compCompare,                        /* tp_rich_compare */
    0,                                  /* tp_weaklistoffset */
    nullptr,                            /* tp_iter */
    nullptr,                            /* tp_iternext */
    subComponentMethods,                /* tp_methods */
    nullptr,                            /* tp_members */
    nullptr,                            /* tp_getset */
    nullptr,                            /* tp_base */
    nullptr,                            /* tp_dict */
    nullptr,                            /* tp_descr_get */
    nullptr,                            /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)subCompInit,              /* tp_init */
    nullptr,                            /* tp_alloc */
    nullptr,                            /* tp_new */
    nullptr,                            /* tp_free */
    nullptr,                            /* tp_is_gc */
    nullptr,                            /* tp_bases */
    nullptr,                            /* tp_mro */
    nullptr,                            /* tp_cache */
    nullptr,                            /* tp_subclasses */
    nullptr,                            /* tp_weaklist */
    nullptr,                            /* tp_del */
    0,                                  /* tp_version_tag */
    nullptr,                            /* tp_finalize */
    SST_TP_VECTORCALL                   /* Python3.8+ */
    SST_TP_PRINT_DEP                    /* Python3.8 only */
    SST_TP_WATCHED                      /* Python3.12+ */
    SST_TP_VERSIONS_USED                /* Python3.13+ only */
};
#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
REENABLE_WARNING
#endif
#endif
