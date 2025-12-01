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

#include "sst/core/model/python/pymodel_portmodule.h"

#include "sst/core/model/configGraph.h"
#include "sst/core/model/python/pymacros.h"
#include "sst/core/model/python/pymodel.h"
#include "sst/core/model/python/pymodel_stat.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_types.h"
#include "sst/core/stringize.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <string.h>

using namespace SST;
using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition* gModel;

SST::ConfigPortModule*
PyPortModule::getPortModule()
{
    // Pointer to element in vector is temporarily safe because when this code is called,
    // no other entity can be modifying the vector
    return &(gModel->getGraph()->findComponent(id)->port_modules.find(port)->second[lkup]);
}


int
PyPortModule::compare(PyPortModule* other)
{
    // Just sort the ptrs, and then indices
    if ( id < other->id )
        return -1;
    else if ( id > other->id )
        return 1;
    else if ( port < other->port )
        return -1;
    else if ( port > other->port )
        return 1;
    else if ( lkup < other->lkup )
        return -1;
    else if ( lkup > other->lkup )
        return 1;
    else
        return 0;
}

static int
portModInit(PortModulePy_t* self, PyObject* args, PyObject* UNUSED(kwds))
{
    unsigned           num;
    SST::ComponentId_t id;
    const char*        pstr;
    if ( !PyArg_ParseTuple(args, "KIs", &id, &num, &pstr) ) return -1;

    PyPortModule* obj = new PyPortModule(self, id, num, pstr);
    self->obj         = obj;
    return 0;
}

static void
portModDealloc(PortModulePy_t* self)
{
    if ( self->obj ) delete self->obj;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
portModAddParam(PyObject* self, PyObject* args)
{
    char*     param = nullptr;
    PyObject* value = nullptr;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) ) return nullptr;

    SST::ConfigPortModule* pm = getPortModule(self);
    if ( nullptr == pm ) return nullptr; // Should never happen
    PyObject* vstr = PyObject_Str(value);
    pm->addParameter(param, SST_ConvertToCppString(vstr));
    Py_XDECREF(vstr);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
portModAddParams(PyObject* self, PyObject* args)
{
    SST::ConfigPortModule* pm = getPortModule(self);
    if ( nullptr == pm ) return nullptr;

    if ( !PyDict_Check(args) ) {
        return nullptr;
    }

    Py_ssize_t pos = 0;
    PyObject * key, *val;
    long       count = 0;

    while ( PyDict_Next(args, &pos, &key, &val) ) {
        PyObject* kstr = PyObject_Str(key);
        PyObject* vstr = PyObject_Str(val);
        pm->addParameter(SST_ConvertToCppString(kstr), SST_ConvertToCppString(vstr));
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return SST_ConvertToPythonLong(count);
}

#if PY_MAJOR_VERSION >= 3
static PyObject*
portModCompare(PyObject* obj0, PyObject* obj1, int op)
{
    PyObject* result;
    bool      cmp = false;
    switch ( op ) {
    case Py_LT:
        cmp = ((PyPortModule*)obj0)->compare(((PortModulePy_t*)obj1)->obj) == -1;
        break;
    case Py_LE:
        cmp = ((PyPortModule*)obj0)->compare(((PortModulePy_t*)obj1)->obj) != 1;
        break;
    case Py_EQ:
        cmp = ((PyPortModule*)obj0)->compare(((PortModulePy_t*)obj1)->obj) == 0;
        break;
    case Py_NE:
        cmp = ((PyPortModule*)obj0)->compare(((PortModulePy_t*)obj1)->obj) != 0;
        break;
    case Py_GT:
        cmp = ((PyPortModule*)obj0)->compare(((PortModulePy_t*)obj1)->obj) == 1;
        break;
    case Py_GE:
        cmp = ((PyPortModule*)obj0)->compare(((PortModulePy_t*)obj1)->obj) != -1;
        break;
    }
    result = cmp ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}
#else
static int
portModCompare(PyObject* obj0, PyObject* obj1)
{
    return ((PyPortModule*)obj0)->compare(((PyPortModule)obj1));
}
#endif

/**
 * @brief portModEnableStatistic. Enables a specific statistic on the port module.
 * @param self The Python port module
 * @param args The name of the stat slot to fill and optionally a params dictionary
 * @return The Python stat object assigned to this slot
 */
static PyObject*
portModEnableStatistic(PyObject* self, PyObject* args)
{
    char*     name      = nullptr;
    PyObject* py_params = nullptr;

    if ( !PyArg_ParseTuple(args, "s|O", &name, &py_params) ) {
        return nullptr;
    }
    SST::ConfigPortModule* pm = getPortModule(self);
    if ( nullptr == pm ) return nullptr;
    pm->enableStatistic(name, pythonToCppParams(py_params));

    return SST_ConvertToPythonLong(0);
}

/**
 * @brief portModEnableAllStatistics
 * @param self The Python PortModule
 * @param args Optionally a params dictionary
 * @return Just a flag (0) indicating success. The enable all functions
 *         do not return Python stat handles.
 */
static PyObject*
portModEnableAllStatistics(PyObject* self, PyObject* args)
{
    int       argOK           = 0;
    PyObject* stat_param_dict = nullptr;

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O", &PyDict_Type, &stat_param_dict);

    if ( argOK ) {
        SST::ConfigPortModule* pm = getPortModule(self);
        pm->enableAllStatistics(pythonToCppParams(stat_param_dict));
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

/**
 * @brief portModEnableStatistics
 * @param self  The Python PortModule
 * @param args  The statistic slot to enable (single) or a list of slots to enable, optionally also params dictionary
 * @return A list of Python statistic templates.  Each Statistic template will generate a unique object
 *         in C++.
 */
static PyObject*
portModEnableStatistics(PyObject* self, PyObject* args)
{
    int                    arg_ok          = 0;
    PyObject*              stat_list       = nullptr;
    char*                  stat_str        = nullptr;
    PyObject*              stat_param_dict = nullptr;
    SST::ConfigPortModule* pm              = getPortModule(self);

    PyErr_Clear();

    // Can either have a single string, or a list of strings.  Try single string first
    arg_ok = PyArg_ParseTuple(args, "s|O", &stat_str, &PyDict_Type, &stat_param_dict);
    if ( arg_ok ) {
        stat_list = PyList_New(1);
        PyList_SetItem(stat_list, 0, SST_ConvertToPythonString(stat_str));
    }
    else { // Try list version
        PyErr_Clear();
        arg_ok = PyArg_ParseTuple(args, "O!|O", &PyList_Type, &stat_list, &PyDict_Type, &stat_param_dict);
        if ( arg_ok ) Py_INCREF(stat_list);
    }

    // Check for arg parse failure and that we got a list
    if ( !arg_ok || !PyList_Check(stat_list) || pm == nullptr ) {
        return nullptr;
    }

    // Add each stat in the list
    Py_ssize_t num_stats = PyList_Size(stat_list);
    auto       params    = pythonToCppParams(stat_param_dict);

    for ( uint32_t x = 0; x < num_stats; x++ ) {
        PyObject*   pylistitem = PyList_GetItem(stat_list, x);
        PyObject*   pyname     = PyObject_Str(pylistitem);
        std::string name       = SST_ConvertToCppString(pyname);
        pm->enableStatistic(name, params);
    }

    Py_XDECREF(stat_list);
    return SST_ConvertToPythonLong(0);
}


static PyObject*
portModSetStatisticLoadLevel(PyObject* self, PyObject* args)
{
    int                    arg_ok = 0;
    int                    level  = STATISTICLOADLEVELUNINITIALIZED;
    SST::ConfigPortModule* pm     = getPortModule(self);

    PyErr_Clear();
    arg_ok = PyArg_ParseTuple(args, "i", &level);
    level  = level & 0xff;

    if ( arg_ok ) {
        pm->setStatisticLoadLevel(level);
    }
    else {
        return nullptr;
    }

    return SST_ConvertToPythonLong(0);
}


static PyObject*
portModAddSharedParamSet(PyObject* self, PyObject* arg)
{
    const char* set = nullptr;
    PyErr_Clear();
    set = SST_ConvertToCppString(arg);

    SST::ConfigPortModule* pm = getPortModule(self);

    if ( set != nullptr ) {
        pm->addSharedParamSet(set);
    }
    else {
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}


static PyMethodDef portModuleMethods[] = { { "addParam", portModAddParam, METH_VARARGS,
                                               "Adds a parameter(name, value)" },
    { "addParams", portModAddParams, METH_O, "Adds Multiple Parameters from a dict" },
    { "setStatisticLoadLevel", portModSetStatisticLoadLevel, METH_VARARGS,
        "Sets the statistics load level for this PortModule" },
    { "enableAllStatistics", portModEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the PortModule with optional parameters" },
    { "enableStatistic", portModEnableStatistic, METH_VARARGS,
        "Enable a statistic with a name and return a handle to it" },
    { "enableStatistics", portModEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the PortModule with optional parameters" },
    { "addSharedParamSet", portModAddSharedParamSet, METH_O, "Add shared parameter set to the PortModule" },
    { nullptr, nullptr, 0, nullptr } };

#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
DISABLE_WARN_DEPRECATED_DECLARATION
#endif
#endif
PyTypeObject PyModel_PortModuleType = {
    SST_PY_OBJ_HEAD "sst.PortModule", /* tp_name */
    sizeof(PortModulePy_t),           /* tp_basicsize */
    0,                                /* tp_itemsize */
    (destructor)portModDealloc,       /* tp_dealloc */
    0,                                /* tp_vectorcall_offset */
    nullptr,                          /* tp_getattr */
    nullptr,                          /* tp_setattr */
    nullptr,                          /* tp_as_sync */
    nullptr,                          /* tp_repr */
    nullptr,                          /* tp_as_number */
    nullptr,                          /* tp_as_sequence */
    nullptr,                          /* tp_as_mapping */
    nullptr,                          /* tp_hash */
    nullptr,                          /* tp_call */
    nullptr,                          /* tp_str */
    nullptr,                          /* tp_getattro */
    nullptr,                          /* tp_setattro */
    nullptr,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,               /* tp_flags */
    "SST PortModule",                 /* tp_doc */
    nullptr,                          /* tp_traverse */
    nullptr,                          /* tp_clear */
    portModCompare,                   /* tp_rich_compare */
    0,                                /* tp_weaklistoffset */
    nullptr,                          /* tp_iter */
    nullptr,                          /* tp_iternext */
    portModuleMethods,                /* tp_methods */
    nullptr,                          /* tp_members */
    nullptr,                          /* tp_getset */
    nullptr,                          /* tp_base */
    nullptr,                          /* tp_dict */
    nullptr,                          /* tp_descr_get */
    nullptr,                          /* tp_descr_set */
    0,                                /* tp_dictoffset */
    (initproc)portModInit,            /* tp_init */
    nullptr,                          /* tp_alloc */
    nullptr,                          /* tp_new */
    nullptr,                          /* tp_free */
    nullptr,                          /* tp_is_gc */
    nullptr,                          /* tp_bases */
    nullptr,                          /* tp_mro */
    nullptr,                          /* tp_cache */
    nullptr,                          /* tp_subclasses */
    nullptr,                          /* tp_weaklist */
    nullptr,                          /* tp_del */
    0,                                /* tp_version_tag */
    nullptr,                          /* tp_finalize */
    SST_TP_VECTORCALL                 /* Python3.8+ */
    SST_TP_PRINT_DEP                  /* Python3.8 only */
    SST_TP_WATCHED                    /* Python3.12+ only */
    SST_TP_VERSIONS_USED              /* Python3.13+ only */
};
#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
REENABLE_WARNING
#endif
#endif
