// -*- c++ -*-

// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/model/python/pymodel.h"

#include "sst/core/component.h"
#include "sst/core/configGraph.h"
#include "sst/core/cputimer.h"
#include "sst/core/factory.h"
#include "sst/core/memuse.h"
#include "sst/core/model/element_python.h"
#include "sst/core/model/python/pymacros.h"
#include "sst/core/model/python/pymodel_comp.h"
#include "sst/core/model/python/pymodel_link.h"
#include "sst/core/model/python/pymodel_stat.h"
#include "sst/core/model/python/pymodel_statgroup.h"
#include "sst/core/model/python/pymodel_unitalgebra.h"
#include "sst/core/simulation.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include <string>

DISABLE_WARN_STRICT_ALIASING

using namespace SST;
using namespace SST::Core;

SST::Core::SSTPythonModelDefinition* gModel = nullptr;

struct ModuleLoaderPy_t
{
    PyObject_HEAD
};

static PyObject* setStatisticOutput(PyObject* self, PyObject* args);
static PyObject* setStatisticLoadLevel(PyObject* self, PyObject* args);

static PyObject* enableAllStatisticsForAllComponents(PyObject* self, PyObject* args);

static PyObject* enableAllStatisticsForComponentName(PyObject* self, PyObject* args);
static PyObject* enableStatisticsForComponentName(PyObject* self, PyObject* args);
static PyObject* enableStatisticForComponentName(PyObject* self, PyObject* args);

static PyObject* enableAllStatisticsForComponentType(PyObject* self, PyObject* args);
static PyObject* enableStatisticsForComponentType(PyObject* self, PyObject* args);
static PyObject* enableStatisticForComponentType(PyObject* self, PyObject* args);

static PyObject* setStatisticLoadLevelForComponentName(PyObject* self, PyObject* args);
static PyObject* setStatisticLoadLevelForComponentType(PyObject* self, PyObject* args);

static PyObject* setCallPythonFinalize(PyObject* self, PyObject* args);

static PyObject* mlFindModule(PyObject* self, PyObject* args);
static PyObject* mlLoadModule(PyObject* self, PyObject* args);

static PyMethodDef mlMethods[] = { { "find_module", mlFindModule, METH_VARARGS, "Finds an SST Element Module" },
                                   { "load_module", mlLoadModule, METH_VARARGS, "Loads an SST Element Module" },
                                   { nullptr, nullptr, 0, nullptr } };

#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
DISABLE_WARN_DEPRECATED_DECLARATION
#endif
#endif
static PyTypeObject ModuleLoaderType = {
    SST_PY_OBJ_HEAD "ModuleLoader", /* tp_name */
    sizeof(ModuleLoaderPy_t),       /* tp_basicsize */
    0,                              /* tp_itemsize */
    nullptr,                        /* tp_dealloc */
    SST_TP_VECTORCALL_OFFSET        /* Python3 only */
        SST_TP_PRINT                /* Python2 only */
    nullptr,                        /* tp_getattr */
    nullptr,                        /* tp_setattr */
    SST_TP_COMPARE(nullptr)         /* Python2 only */
    SST_TP_AS_SYNC                  /* Python3 only */
    nullptr,                        /* tp_repr */
    nullptr,                        /* tp_as_number */
    nullptr,                        /* tp_as_sequence */
    nullptr,                        /* tp_as_mapping */
    nullptr,                        /* tp_hash */
    nullptr,                        /* tp_call */
    nullptr,                        /* tp_str */
    nullptr,                        /* tp_getattro */
    nullptr,                        /* tp_setattro */
    nullptr,                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    "SST Module Loader",            /* tp_doc */
    nullptr,                        /* tp_traverse */
    nullptr,                        /* tp_clear */
    SST_TP_RICH_COMPARE(nullptr)    /* Python3 only */
    0,                              /* tp_weaklistoffset */
    nullptr,                        /* tp_iter */
    nullptr,                        /* tp_iternext */
    mlMethods,                      /* tp_methods */
    nullptr,                        /* tp_members */
    nullptr,                        /* tp_getset */
    nullptr,                        /* tp_base */
    nullptr,                        /* tp_dict */
    nullptr,                        /* tp_descr_get */
    nullptr,                        /* tp_descr_set */
    0,                              /* tp_dictoffset */
    nullptr,                        /* tp_init */
    nullptr,                        /* tp_alloc */
    nullptr,                        /* tp_new */
    nullptr,                        /* tp_free */
    nullptr,                        /* tp_is_gc */
    nullptr,                        /* tp_bases */
    nullptr,                        /* tp_mro */
    nullptr,                        /* tp_cache */
    nullptr,                        /* tp_subclasses */
    nullptr,                        /* tp_weaklist */
    nullptr,                        /* tp_del */
    0,                              /* tp_version_tag */
    SST_TP_FINALIZE                 /* Python3 only */
        SST_TP_VECTORCALL           /* Python3 only */
            SST_TP_PRINT_DEP        /* Python3.8 only */
};
#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
REENABLE_WARNING
#endif
#endif

// I hate having to do this through a global variable
// but there's really no other way to communicate errors from the importer
// to the parent simulator process
static std::string loadErrors;

static PyObject*
mlFindModule(PyObject* self, PyObject* args)
{
    char*     name;
    PyObject* path;
    if ( !PyArg_ParseTuple(args, "s|O", &name, &path) ) return nullptr;

    // reset any previous load errors, they apparently didn't matter
    loadErrors = "";

    std::stringstream err_sstr;
    if ( !strncmp(name, "sst.", 4) ) {
        // We know how to handle only sst.<module>
        // Check for the existence of a library
        char* modName = name + 4;
        if ( Factory::getFactory()->hasLibrary(modName, err_sstr) ) {
            // genPythonModuleFunction func = Factory::getFactory()->getPythonModule(modName);
            SSTElementPythonModule* pymod = Factory::getFactory()->getPythonModule(modName);
            if ( pymod ) {
                Py_INCREF(self);
                return self;
            }
            else {
                loadErrors += std::string("Succeeded in loading library for ") + (const char*)modName +
                              " but library does not contain a Python module\n";
                loadErrors += err_sstr.str();
            }
        }
        else {
            loadErrors += std::string("No component or Python model registered for ") + (const char*)modName + "\n";
            loadErrors += err_sstr.str();
        }
    }
    Py_RETURN_NONE;
}

static PyMethodDef emptyModMethods[] = { { nullptr, nullptr, 0, nullptr } };
#if PY_MAJOR_VERSION >= 3
/* This defines an empty module used if modName is not found during mlLoadModule() */
static struct PyModuleDef emptyModDef
{
    PyModuleDef_HEAD_INIT, /* m_base */
        "sstempty",        /* m_name */
        nullptr,           /* m_doc */
        -1,                /* m_size */
        emptyModMethods,   /* m_methods */
        nullptr,           /* m_slots */
        nullptr,           /* m_traverse */
        nullptr,           /* m_clear */
        nullptr,           /* m_free */
};
#endif

static PyObject*
mlLoadModule(PyObject* UNUSED(self), PyObject* args)
{
    char* name;
    if ( !PyArg_ParseTuple(args, "s", &name) ) return nullptr;

    if ( strncmp(name, "sst.", 4) ) {
        // We know how to handle only sst.<module>
        return nullptr; // ERROR!
    }

    char* modName = name + 4; // sst.<modName>

    // fprintf(stderr, "Loading SST module '%s' (from %s)\n", modName, name);
    // genPythonModuleFunction func = Factory::getFactory()->getPythonModule(modName);
    SSTElementPythonModule* pymod = Factory::getFactory()->getPythonModule(modName);
    PyObject*               mod   = nullptr;
    if ( !pymod ) { mod = SST_PY_INIT_MODULE(name, emptyModMethods, emptyModDef); }
    else {
        mod = static_cast<PyObject*>(pymod->load());
    }

    return mod;
}

/***** Module information *****/

static PyObject*
findComponentByName(PyObject* UNUSED(self), PyObject* args)
{
    if ( !PyUnicode_Check(args) ) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    const char*      name = SST_ConvertToCppString(args);
    ConfigComponent* cc   = gModel->findComponentByName(name);

    if ( nullptr == cc ) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if ( SUBCOMPONENT_ID_MASK(cc->id) == 0 ) {
        // Component
        PyObject* argList = Py_BuildValue("ssk", name, "irrelephant", cc->id);
        PyObject* res     = PyObject_CallObject((PyObject*)&PyModel_ComponentType, argList);
        Py_DECREF(argList);
        return res;
    }
    else {
        // SubComponent
        PyObject* argList = Py_BuildValue("Ok", Py_None, cc->id);
        PyObject* res     = PyObject_CallObject((PyObject*)&PyModel_SubComponentType, argList);
        Py_DECREF(argList);
        return res;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
setProgramOption(PyObject* UNUSED(self), PyObject* args)
{
    char *param, *value;
    PyErr_Clear();
    int argOK = PyArg_ParseTuple(args, "ss", &param, &value);

    if ( argOK ) {
        if ( gModel->setConfigEntryFromModel(param, value) )
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }
    else {
        return nullptr;
    }
}

static PyObject*
setProgramOptions(PyObject* UNUSED(self), PyObject* args)
{
    if ( !PyDict_Check(args) ) { return nullptr; }
    Py_ssize_t pos = 0;
    PyObject * key, *val;
    long       count = 0;
    while ( PyDict_Next(args, &pos, &key, &val) ) {
        if ( gModel->setConfigEntryFromModel(SST_ConvertToCppString(key), SST_ConvertToCppString(val)) ) count++;
    }
    return SST_ConvertToPythonLong(count);
}

static PyObject*
getProgramOptions(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    // Load parameters already set.
    Config* cfg = gModel->getConfig();

    PyObject* dict = PyDict_New();
    // Basic options
    PyDict_SetItem(dict, SST_ConvertToPythonString("verbose"), SST_ConvertToPythonLong(cfg->verbose()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("num-ranks"), SST_ConvertToPythonLong(cfg->num_ranks()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("num-threads"), SST_ConvertToPythonLong(cfg->num_threads()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("sdl-file"), SST_ConvertToPythonString(cfg->configFile().c_str()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("print-timing-info"), SST_ConvertToPythonBool(cfg->print_timing()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("stop-at"), SST_ConvertToPythonString(cfg->stop_at().c_str()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("exit-after"), SST_ConvertToPythonLong(cfg->exit_after()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("partitioner"), SST_ConvertToPythonString(cfg->partitioner().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("heartbeat-period"), SST_ConvertToPythonString(cfg->heartbeatPeriod().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("output-directory"),
        SST_ConvertToPythonString(cfg->output_directory().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("output-prefix-core"),
        SST_ConvertToPythonString(cfg->output_core_prefix().c_str()));

    // Configuration output options
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("output-config"),
        SST_ConvertToPythonString(cfg->output_config_graph().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("output-json"), SST_ConvertToPythonString(cfg->output_json().c_str()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("parallel-output"), SST_ConvertToPythonBool(cfg->parallel_output()));

    // Graph output options
    PyDict_SetItem(dict, SST_ConvertToPythonString("output-dot"), SST_ConvertToPythonString(cfg->output_dot().c_str()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("dot-verbosity"), SST_ConvertToPythonLong(cfg->dot_verbosity()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("output-partition"),
        SST_ConvertToPythonString(cfg->component_partition_file().c_str()));

    // Advanced options
    PyDict_SetItem(dict, SST_ConvertToPythonString("timebase"), SST_ConvertToPythonString(cfg->timeBase().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("parallel-load"), SST_ConvertToPythonString(cfg->parallel_load_str().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("time-vortex"), SST_ConvertToPythonString(cfg->timeVortex().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("interthread-links"), SST_ConvertToPythonBool(cfg->interthread_links()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("debug-file"), SST_ConvertToPythonString(cfg->debugFile().c_str()));
    PyDict_SetItem(dict, SST_ConvertToPythonString("lib-path"), SST_ConvertToPythonString(cfg->libpath().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("add-lib-path"), SST_ConvertToPythonString(cfg->addLibPath().c_str()));

    // Advanced options - profiling
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("enable-profiling"),
        SST_ConvertToPythonString(cfg->enabledProfiling().c_str()));
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("profiling-output"), SST_ConvertToPythonString(cfg->profilingOutput().c_str()));

    // Advanced options - debug
    PyDict_SetItem(dict, SST_ConvertToPythonString("run-mode"), SST_ConvertToPythonString(cfg->runMode_str().c_str()));
#ifdef USE_MEMPOOL
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("output-undeleted-events"),
        SST_ConvertToPythonString(cfg->event_dump_file().c_str()));
#endif
    PyDict_SetItem(
        dict, SST_ConvertToPythonString("force-rank-seq-startup"), SST_ConvertToPythonBool(cfg->rank_seq_startup()));

    return dict;
}

static PyObject*
pushNamePrefix(PyObject* UNUSED(self), PyObject* arg)
{
    const char* name = nullptr;
    PyErr_Clear();
    name = SST_ConvertToCppString(arg);

    if ( name != nullptr ) { gModel->pushNamePrefix(name); }
    else {
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
popNamePrefix(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    gModel->popNamePrefix();
    return SST_ConvertToPythonLong(0);
}

static PyObject*
exitsst(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    exit(-1);
    return nullptr;
}

static PyObject*
getSSTMPIWorldSize(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    int ranks = 1;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
#endif
    return SST_ConvertToPythonLong(ranks);
}

static PyObject*
getSSTMyMPIRank(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    int myrank = 0;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
#endif
    return SST_ConvertToPythonLong(myrank);
}

static PyObject*
getSSTThreadCount(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    Config* cfg = gModel->getConfig();
    return SST_ConvertToPythonLong(cfg->num_threads());
}

static PyObject*
setSSTThreadCount(PyObject* UNUSED(self), PyObject* args)
{
    Config* cfg     = gModel->getConfig();
    long    oldNThr = cfg->num_threads();
    long    nThr    = SST_ConvertToCppLong(args);
    gModel->setConfigEntryFromModel("num_threads", std::to_string(nThr));
    return SST_ConvertToPythonLong(oldNThr);
}

static PyObject*
setStatisticOutput(PyObject* UNUSED(self), PyObject* args)
{
    char*     statOutputName;
    int       argOK           = 0;
    PyObject* outputParamDict = nullptr;

    PyErr_Clear();

    // Parse the Python Args and get the StatOutputName and optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!", &statOutputName, &PyDict_Type, &outputParamDict);

    if ( argOK ) {
        gModel->setStatisticOutput(statOutputName);

        // Generate and Add the Statistic Output Parameters
        for ( auto p : generateStatisticParameters(outputParamDict) ) {
            gModel->addStatisticOutputParameter(p.first, p.second);
        }
    }
    else {
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
setStatisticOutputOption(PyObject* UNUSED(self), PyObject* args)
{
    char* param;
    char* value;
    int   argOK = 0;

    PyErr_Clear();

    argOK = PyArg_ParseTuple(args, "ss", &param, &value);

    if ( argOK ) { gModel->addStatisticOutputParameter(param, value); }
    else {
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
setStatisticOutputOptions(PyObject* UNUSED(self), PyObject* args)
{
    PyErr_Clear();

    if ( !PyDict_Check(args) ) { return nullptr; }

    // Generate and Add the Statistic Output Parameters
    for ( auto p : generateStatisticParameters(args) ) {
        gModel->addStatisticOutputParameter(p.first, p.second);
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
setStatisticLoadLevel(PyObject* UNUSED(self), PyObject* arg)
{
    PyErr_Clear();

    uint32_t loadLevel = SST_ConvertToCppLong(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    gModel->setStatisticLoadLevel(loadLevel);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
enableAllStatisticsForAllComponents(PyObject* UNUSED(self), PyObject* args)
{
    int       argOK         = 0;
    PyObject* statParamDict = nullptr;

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!", &PyDict_Type, &statParamDict);

    if ( argOK ) {
        for ( auto cc : gModel->components() ) {
            cc->enableStatistic(STATALLFLAG, pythonToCppParams(statParamDict), true /*recursive*/);
        }
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }

    return SST_ConvertToPythonLong(0);
}

static PyObject*
enableAllStatisticsForComponentName(PyObject* UNUSED(self), PyObject* args)
{
    int       argOK             = 0;
    char*     compName          = nullptr;
    PyObject* statParamDict     = nullptr;
    int       apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!i", &compName, &PyDict_Type, &statParamDict, &apply_to_children);

    if ( argOK ) {
        ConfigComponent* cc = gModel->findComponentByName(compName);
        cc->enableStatistic(STATALLFLAG, pythonToCppParams(statParamDict), apply_to_children);
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
enableStatisticForComponentName(PyObject* UNUSED(self), PyObject* args)
{
    int       argOK             = 0;
    char*     compName          = nullptr;
    char*     statName          = nullptr;
    PyObject* statParamDict     = nullptr;
    int       apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Name, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "ss|O!i", &compName, &statName, &PyDict_Type, &statParamDict, &apply_to_children);

    if ( argOK ) {
        ConfigComponent* cc = gModel->findComponentByName(compName);
        return buildEnabledStatistic(cc, statName, statParamDict, apply_to_children);
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
enableStatisticsForComponentName(PyObject* UNUSED(self), PyObject* args)
{
    int       argOK             = 0;
    char*     compName          = nullptr;
    PyObject* statList          = nullptr;
    char*     stat_str          = nullptr;
    PyObject* statParamDict     = nullptr;
    int       apply_to_children = 0;

    PyErr_Clear();

    // Can either have a single string, or a list of strings.  Try single string first
    argOK = PyArg_ParseTuple(args, "ss|O!i", &compName, &stat_str, &PyDict_Type, &statParamDict, &apply_to_children);
    if ( argOK ) {
        statList = PyList_New(1);
        PyList_SetItem(statList, 0, SST_ConvertToPythonString(stat_str));
    }
    else {
        PyErr_Clear();
        apply_to_children = 0;
        // Try list version
        argOK             = PyArg_ParseTuple(
            args, "sO!|O!i", &compName, &PyList_Type, &statList, &PyDict_Type, &statParamDict, &apply_to_children);
        if ( argOK ) Py_INCREF(statList);
    }

    if ( argOK ) {
        // Get the component
        ConfigComponent* cc = gModel->findComponentByName(compName);
        if ( nullptr == cc ) {
            gModel->getOutput()->fatal(
                CALL_INFO, 1, "component name not found in call to enableStatisticsForComponentName(): %s\n", compName);
        }
        return buildEnabledStatistics(cc, statList, statParamDict, apply_to_children);
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    // unreachable
    return SST_ConvertToPythonLong(0);
}

static void
enableStatisticForComponentType(
    ConfigComponent* cc, const std::string& compType, const std::string& statName, const SST::Params& params,
    bool is_all_types, bool apply_to_children)
{
    if ( is_all_types || cc->type == compType ) { cc->enableStatistic(statName, params, apply_to_children); }
    for ( auto sc : cc->subComponents ) {
        enableStatisticForComponentType(sc, compType, statName, params, is_all_types, apply_to_children);
    }
}

static PyObject*
enableAllStatisticsForComponentType(PyObject* UNUSED(self), PyObject* args)
{
    int       argOK             = 0;
    char*     compType          = nullptr;
    PyObject* statParamDict     = nullptr;
    int       apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Type and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!i", &compType, &PyDict_Type, &statParamDict, &apply_to_children);

    if ( argOK ) {
        bool is_all_types = std::string(compType) == STATALLFLAG;
        auto params       = pythonToCppParams(statParamDict);
        for ( ConfigComponent* cc : gModel->components() ) {
            enableStatisticForComponentType(cc, compType, STATALLFLAG, params, is_all_types, apply_to_children);
        }
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static void
enableStatisticForComponentType(
    const std::string& compType, const std::string& statName, const SST::Params& params, bool apply_to_children)
{
    bool is_all_types = compType == STATALLFLAG;
    for ( auto& cc : gModel->components() ) {
        enableStatisticForComponentType(cc, compType, statName, params, is_all_types, apply_to_children);
    }
}

static PyObject*
enableStatisticForComponentType(PyObject* UNUSED(self), PyObject* args)
{
    int       argOK             = 0;
    char*     compType          = nullptr;
    char*     statName          = nullptr;
    PyObject* statParamDict     = nullptr;
    int       apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Type, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "ss|O!i", &compType, &statName, &PyDict_Type, &statParamDict, &apply_to_children);

    if ( argOK ) {
        auto params = pythonToCppParams(statParamDict);
        enableStatisticForComponentType(compType, statName, params, apply_to_children);
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
enableStatisticsForComponentType(PyObject* UNUSED(self), PyObject* args)
{
    int       argOK             = 0;
    char*     compType          = nullptr;
    PyObject* statList          = nullptr;
    char*     stat_str          = nullptr;
    PyObject* statParamDict     = nullptr;
    int       apply_to_children = 0;

    PyErr_Clear();

    // Can either have a single string, or a list of strings.  Try single string first
    argOK = PyArg_ParseTuple(args, "ss|O!i", &compType, &stat_str, &PyDict_Type, &statParamDict, &apply_to_children);
    if ( argOK ) {
        statList = PyList_New(1);
        PyList_SetItem(statList, 0, SST_ConvertToPythonString(stat_str));
    }
    else {
        PyErr_Clear();
        apply_to_children = 0;
        // Try list version
        argOK             = PyArg_ParseTuple(
            args, "sO!|O!i", &compType, &PyList_Type, &statList, &PyDict_Type, &statParamDict, &apply_to_children);
        if ( argOK ) Py_INCREF(statList);
    }

    if ( argOK ) {
        // Figure out how many stats there are
        Py_ssize_t numStats = PyList_Size(statList);
        auto       params   = pythonToCppParams(statParamDict);
        for ( uint32_t x = 0; x < numStats; x++ ) {
            PyObject*   pylistitem = PyList_GetItem(statList, x);
            PyObject*   pyname     = PyObject_CallMethod(pylistitem, (char*)"__str__", nullptr);
            std::string statName   = SST_ConvertToCppString(pyname);
            enableStatisticForComponentType(compType, statName, params, apply_to_children);
        }
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
setStatisticLoadLevelForComponentName(PyObject* UNUSED(self), PyObject* args)
{
    int   argOK             = 0;
    char* compName          = nullptr;
    int   level             = STATISTICLOADLEVELUNINITIALIZED;
    int   apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Type, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "si|i", &compName, &level, &apply_to_children);

    if ( argOK ) {
        // Get the component
        ConfigComponent* cc = gModel->findComponentByName(compName);
        if ( nullptr == cc ) {
            gModel->getOutput()->fatal(
                CALL_INFO, 1, "component name not found in call to setStatisticLoadLevelForComponentName(): %s\n",
                compName);
        }

        cc->setStatisticLoadLevel(level, apply_to_children);
    }
    else {
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static void
setStatisticLoadLevelForComponentType(
    ConfigComponent* cc, bool is_all_types, const std::string& compType, uint8_t level, int apply_to_children)
{
    if ( is_all_types || cc->type == compType ) { cc->setStatisticLoadLevel(level, apply_to_children); }
    for ( auto sc : cc->subComponents ) {
        setStatisticLoadLevelForComponentType(sc, is_all_types, compType, level, apply_to_children);
    }
}

static PyObject*
setStatisticLoadLevelForComponentType(PyObject* UNUSED(self), PyObject* args)
{
    int   argOK             = 0;
    char* compType          = nullptr;
    int   level             = STATISTICLOADLEVELUNINITIALIZED;
    int   apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Type, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "si|i", &compType, &level, &apply_to_children);

    if ( argOK ) {
        bool is_all_types = std::string(compType) == STATALLFLAG;
        for ( ConfigComponent* cc : gModel->components() ) {
            setStatisticLoadLevelForComponentType(cc, is_all_types, compType, level, apply_to_children);
        }
    }
    else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return SST_ConvertToPythonLong(0);
}

static PyObject*
setCallPythonFinalize(PyObject* UNUSED(self), PyObject* arg)
{
    PyErr_Clear();

    bool state = SST_ConvertToCppLong(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    gModel->setCallPythonFinalize(state);
    int myrank = 0;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
#endif
    if ( state && myrank == 0 ) {
        gModel->getOutput()->output(
            "WARNING: Setting callPythonFinalize to True is EXPERIMENTAL pending further testing.\n");
    }

    return SST_ConvertToPythonLong(0);
}

static PyObject*
globalAddParam(PyObject* UNUSED(self), PyObject* args)
{
    char*     set   = nullptr;
    char*     param = nullptr;
    PyObject* value = nullptr;
    if ( !PyArg_ParseTuple(args, "ssO", &set, &param, &value) ) return nullptr;

    // Get the string-ized value by calling __str__ function of the
    // value object
    PyObject* vstr = PyObject_CallMethod(value, (char*)"__str__", nullptr);
    gModel->addGlobalParameter(set, param, SST_ConvertToCppString(vstr), true);
    Py_XDECREF(vstr);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
globalAddParams(PyObject* UNUSED(self), PyObject* args)
{
    char*     set  = nullptr;
    PyObject* dict = nullptr;
    if ( !PyArg_ParseTuple(args, "sO", &set, &dict) ) return nullptr;

    if ( !PyDict_Check(dict) ) { return nullptr; }

    Py_ssize_t pos = 0;
    PyObject * key, *val;
    long       count = 0;

    while ( PyDict_Next(dict, &pos, &key, &val) ) {
        PyObject* kstr = PyObject_CallMethod(key, (char*)"__str__", nullptr);
        PyObject* vstr = PyObject_CallMethod(val, (char*)"__str__", nullptr);
        gModel->addGlobalParameter(set, SST_ConvertToCppString(kstr), SST_ConvertToCppString(vstr), true);
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return SST_ConvertToPythonLong(count);
}

static PyObject*
getElapsedExecutionTime(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    // Get the elapsed runtime
    UnitAlgebra time     = gModel->getElapsedExecutionTime();
    std::string time_str = time.toString();

    PyObject* argList = Py_BuildValue("(s)", time_str.c_str());
    PyObject* res     = PyObject_CallObject((PyObject*)&PyModel_UnitAlgebraType, argList);
    Py_DECREF(argList);
    return res;
}

static PyObject*
getLocalMemoryUsage(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    // Get the elapsed runtime
    UnitAlgebra memsize     = gModel->getLocalMemoryUsage();
    std::string memsize_str = memsize.toString();

    PyObject* argList = Py_BuildValue("(s)", memsize_str.c_str());
    PyObject* res     = PyObject_CallObject((PyObject*)&PyModel_UnitAlgebraType, argList);
    Py_DECREF(argList);
    return res;
}

static PyMethodDef sstModuleMethods[] = {
    { "setProgramOption", setProgramOption, METH_VARARGS,
      "Sets a single program configuration option (form:  setProgramOption(name, value))" },
    { "setProgramOptions", setProgramOptions, METH_O, "Sets multiple program configuration option from a dict." },
    { "getProgramOptions", getProgramOptions, METH_NOARGS, "Returns a dict of the current program options." },
    { "pushNamePrefix", pushNamePrefix, METH_O, "Pushes a string onto the prefix of new component and link names" },
    { "popNamePrefix", popNamePrefix, METH_NOARGS,
      "Removes the most recent addition to the prefix of new component and link names" },
    { "exit", exitsst, METH_NOARGS, "Exits SST - indicates the script wanted to exit." },
    { "getMPIRankCount", getSSTMPIWorldSize, METH_NOARGS,
      "Gets the number of MPI ranks currently being used to run SST" },
    { "getMyMPIRank", getSSTMyMPIRank, METH_NOARGS, "Gets the SST MPI rank the script is running on" },
    { "getThreadCount", getSSTThreadCount, METH_NOARGS,
      "Gets the number of MPI ranks currently being used to run SST" },
    { "setThreadCount", setSSTThreadCount, METH_O, "Gets the number of MPI ranks currently being used to run SST" },
    { "setStatisticOutput", setStatisticOutput, METH_VARARGS,
      "Sets the Statistic Output - default is console output." },
    { "setStatisticLoadLevel", setStatisticLoadLevel, METH_O,
      "Sets the Statistic Load Level (0 - 10) - default is 0 (disabled)." },
    { "setStatisticOutputOption", setStatisticOutputOption, METH_VARARGS,
      "Sets a single Statistic output option (form: setStatisticOutputOption(name, value))" },
    { "setStatisticOutputOptions", setStatisticOutputOptions, METH_O,
      "Sets multiple Statistic output options from a dict." },
    { "enableAllStatisticsForAllComponents", enableAllStatisticsForAllComponents, METH_VARARGS,
      "Enables all statistics on all components with output at end of simulation." },
    { "enableAllStatisticsForComponentName", enableAllStatisticsForComponentName, METH_VARARGS,
      "Enables all statistics on a component with output occurring at defined rate." },
    { "enableStatisticForComponentName", enableStatisticForComponentName, METH_VARARGS,
      "Enables a single statistic on a component with output occurring at defined rate." },
    { "enableStatisticsForComponentName", enableStatisticsForComponentName, METH_VARARGS,
      "Enables a mulitple statistics on a component with output occurring at defined rate." },
    { "enableAllStatisticsForComponentType", enableAllStatisticsForComponentType, METH_VARARGS,
      "Enables all statistics on all components of component type with output occurring at defined rate." },
    { "enableStatisticForComponentType", enableStatisticForComponentType, METH_VARARGS,
      "Enables a single statistic on all components of component type with output occurring at defined rate." },
    { "enableStatisticsForComponentType", enableStatisticsForComponentType, METH_VARARGS,
      "Enables a list of statistics on all components of component type with output occurring at defined rate." },
    { "setStatisticLoadLevelForComponentName", setStatisticLoadLevelForComponentName, METH_VARARGS,
      "Sets the statistic load level for the specified component name." },
    { "setStatisticLoadLevelForComponentType", setStatisticLoadLevelForComponentType, METH_VARARGS,
      "Sets the statistic load level for all components of the specified type." },
    { "findComponentByName", findComponentByName, METH_O,
      "Looks up to find a previously created component/subcomponent, based off of its name.  Returns None if none "
      "are to be found." },
    { "addGlobalParam", globalAddParam, METH_VARARGS, "Add a parameter to the specified global set." },
    { "addGlobalParams", globalAddParams, METH_VARARGS, "Add parameters in dictionary to the specified global set." },
    { "getElapsedExecutionTime", getElapsedExecutionTime, METH_NOARGS,
      "Gets the real elapsed time since simluation start, returned as a UnitAlgebra.  Not precise enough for "
      "getting fine timings.  For that, use the built-in time module." },
    { "getLocalMemoryUsage", getLocalMemoryUsage, METH_NOARGS,
      "Gets the current memory use, returned as a UnitAlgebra" },
    { "setCallPythonFinalize", setCallPythonFinalize, METH_O,
      "Sets whether or not Py_Finalize will be called after SST model generation is done.  Py_Finalize will be "
      "called by default if this function is not called." },
    { nullptr, nullptr, 0, nullptr }
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef sstModuleDef
{
    PyModuleDef_HEAD_INIT, /* m_base */
        "sst",             /* m_name */
        nullptr,           /* m_doc */
        -1,                /* m_size */
        sstModuleMethods,  /* m_methods */
        nullptr,           /* m_slots */
        nullptr,           /* m_traverse */
        nullptr,           /* m_clear */
        nullptr,           /* m_free */
};
#endif

static PyObject*
PyInit_sst(void)
{
    // Initialize our types
    PyModel_ComponentType.tp_new    = PyType_GenericNew;
    PyModel_SubComponentType.tp_new = PyType_GenericNew;
    PyModel_StatType.tp_new         = PyType_GenericNew;
    PyModel_LinkType.tp_new         = PyType_GenericNew;
    PyModel_UnitAlgebraType.tp_new  = PyType_GenericNew;
    PyModel_StatGroupType.tp_new    = PyType_GenericNew;
    PyModel_StatOutputType.tp_new   = PyType_GenericNew;
    ModuleLoaderType.tp_new         = PyType_GenericNew;
    if ( (PyType_Ready(&PyModel_ComponentType) < 0) || (PyType_Ready(&PyModel_SubComponentType) < 0) ||
         (PyType_Ready(&PyModel_LinkType) < 0) || (PyType_Ready(&PyModel_UnitAlgebraType) < 0) ||
         (PyType_Ready(&PyModel_StatType) < 0) || (PyType_Ready(&PyModel_StatGroupType) < 0) ||
         (PyType_Ready(&PyModel_StatOutputType) < 0) || (PyType_Ready(&ModuleLoaderType) < 0) ) {
        return nullptr; // TODO better error message
    }

    // Create the module
    PyObject* module = SST_PY_INIT_MODULE("sst", sstModuleMethods, sstModuleDef);
    if ( !module ) return nullptr;

    Py_INCREF(&PyModel_ComponentType);
    Py_INCREF(&PyModel_SubComponentType);
    Py_INCREF(&PyModel_StatType);
    Py_INCREF(&PyModel_LinkType);
    Py_INCREF(&PyModel_UnitAlgebraType);
    Py_INCREF(&PyModel_StatGroupType);
    Py_INCREF(&PyModel_StatOutputType);
    Py_INCREF(&ModuleLoaderType);

    // Add the types
    PyModule_AddObject(module, "Link", (PyObject*)&PyModel_LinkType);
    PyModule_AddObject(module, "UnitAlgebra", (PyObject*)&PyModel_UnitAlgebraType);
    PyModule_AddObject(module, "Component", (PyObject*)&PyModel_ComponentType);
    PyModule_AddObject(module, "SubComponent", (PyObject*)&PyModel_SubComponentType);
    PyModule_AddObject(module, "Statistic", (PyObject*)&PyModel_StatType);
    PyModule_AddObject(module, "StatisticGroup", (PyObject*)&PyModel_StatGroupType);
    PyModule_AddObject(module, "StatisticOutput", (PyObject*)&PyModel_StatOutputType);
    PyModule_AddObject(module, "ModuleLoader", (PyObject*)&ModuleLoaderType);
    PyModule_AddObject(module, "__path__", Py_BuildValue("()"));

    return module;
}

void
SSTPythonModelDefinition::initModel(
    const std::string& script_file, int verbosity, Config* UNUSED(config), int argc, char** argv)
{
    output = new Output("SSTPythonModel: ", verbosity, 0, SST::Output::STDOUT);

    if ( gModel ) { output->fatal(CALL_INFO, 1, "A Python Config Model is already in progress.\n"); }
    gModel = this;

    graph           = new ConfigGraph();
    nextComponentId = 0;

    std::string local_script_name;
    int         substr_index = 0;

    for ( substr_index = script_file.size() - 1; substr_index >= 0; --substr_index ) {
        if ( script_file.at(substr_index) == '/' ) {
            substr_index++;
            break;
        }
    }

    const std::string file_name_only = script_file.substr(std::max(0, substr_index));
    local_script_name                = file_name_only.substr(0, file_name_only.size() - 3);

    output->verbose(
        CALL_INFO, 2, 0, "SST loading a Python model from script: %s / [%s]\n", script_file.c_str(),
        local_script_name.c_str());


#if PY_MAJOR_VERSION >= 3
    // Add sst module to the Python interpreter as a built in
    PyImport_AppendInittab("sst", &PyInit_sst);

#if PY_MINOR_VERSION >= 11
    {
        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        // Set to zero so that we can get environment variables in the
        // script
        config.isolated   = 0;
        // Set to zero so it won't parse out the first argument of
        // argv
        config.parse_argv = 0;
        // Set argc and argv
        PyConfig_SetBytesArgv(&config, argc, argv);
        // Get the Python scripting engine started
        Py_InitializeFromConfig(&config);
    }
#else
    // Set arguments; Python3 takes wchar_t* arg instead of char*
    wchar_t** wargv = (wchar_t**)PyMem_Malloc(sizeof(wchar_t*) * argc);
    for ( int i = 0; i < argc; i++ ) {
        wargv[i] = Py_DecodeLocale(argv[i], nullptr);
    }

    // Get the Python scripting engine started
    Py_Initialize();
    PySys_SetArgv(argc, wargv);
#endif
    PyRun_SimpleString("import sys\n"
                       "import sst\n"
                       "sys.meta_path.append(sst.ModuleLoader())\n");
#else
    // Get the Python scripting engine started
    Py_Initialize();
    PySys_SetArgv(argc, argv);
    // Add sst module to the Python interpreter as a built in
    PyInit_sst();

    // Add our custom loader
    PyObject* main_module = PyImport_ImportModule("__main__");
    PyModule_AddObject(main_module, "ModuleLoader", (PyObject*)&ModuleLoaderType);
    PyRun_SimpleString("def loadLoader():\n"
                       "\timport sys\n"
                       "\tsys.meta_path.append(ModuleLoader())\n"
                       "\timport sst\n"
                       "\tsst.__path__ = []\n" // Must be here or else meta_path won't be questioned
                       "loadLoader()\n");
#endif
}

SSTPythonModelDefinition::SSTPythonModelDefinition(
    const std::string& script_file, int verbosity, Config* configObj, double start_time) :
    SSTModelDescription(configObj),
    scriptName(script_file),
    config(configObj),
    namePrefix(nullptr),
    namePrefixLen(0),
    start_time(start_time),
    callPythonFinalize(false)
{
    std::vector<std::string> argv_vector;
    argv_vector.push_back("sstsim.x");

    const int   input_len = configObj->model_options().length();
    std::string temp      = "";
    bool        in_string = false;

    for ( int i = 0; i < input_len; ++i ) {
        if ( configObj->model_options().substr(i, 1) == "\"" ) {
            if ( in_string ) {
                // We are ending a string
                if ( !(temp == "") ) {
                    argv_vector.push_back(temp);
                    temp = "";
                }

                in_string = false;
            }
            else {
                // We are starting a string
                in_string = true;
            }
        }
        else if ( configObj->model_options().substr(i, 1) == " " ) {
            if ( in_string ) { temp += " "; }
            else {
                if ( !(temp == "") ) { argv_vector.push_back(temp); }

                temp = "";
            }
        }
        else {
            temp += configObj->model_options().substr(i, 1);
        }
    }

    // need to handle the last argument with a special case
    if ( temp != "" ) {
        if ( in_string ) {
            // this maybe en error?
        }
        else {
            if ( !(temp == "") ) { argv_vector.push_back(temp); }
        }
    }

    // generate C main style inputs to the Python program based on our processing above.
    char**    argv = (char**)malloc(sizeof(char*) * argv_vector.size());
    const int argc = argv_vector.size();

    for ( int i = 0; i < argc; ++i ) {
        size_t slen = argv_vector[i].size() + 1;
        argv[i]     = (char*)malloc(sizeof(char) * slen);
        snprintf(argv[i], slen, "%s", argv_vector[i].c_str());
    }

    // Init the model
    initModel(script_file, verbosity, configObj, argc, argv);

    // Free the vector
    free(argv);
}

SSTPythonModelDefinition::~SSTPythonModelDefinition()
{
    delete output;
    gModel = nullptr;

    if ( nullptr != namePrefix ) free(namePrefix);
    if ( callPythonFinalize ) { Py_Finalize(); }
    else {
        PyGC_Collect();
    }
}

ConfigGraph*
SSTPythonModelDefinition::createConfigGraph()
{
    output->verbose(CALL_INFO, 1, 0, "Creating config graph for SST using Python model...\n");

    FILE* fp = fopen(scriptName.c_str(), "r");
    if ( !fp ) { output->fatal(CALL_INFO, 1, "Unable to open python script %s\n", scriptName.c_str()); }
    int createReturn = PyRun_AnyFileEx(fp, scriptName.c_str(), 1);

    if ( nullptr != PyErr_Occurred() ) {
        // Print the Python error and then let SST exit as a fatal-stop.
        PyErr_Print();
        output->fatal(CALL_INFO, 1, "Error occurred executing the Python SST model script.\n");
    }
    if ( -1 == createReturn ) {
        output->fatal(CALL_INFO, 1, "Execution of model construction function failed\n%s", loadErrors.c_str());
    }

    output->verbose(CALL_INFO, 1, 0, "Construction of config graph with Python is complete.\n");

    if ( nullptr != PyErr_Occurred() ) {
        PyErr_Print();
        output->fatal(CALL_INFO, 1, "Error occured handling the creation of the component graph in Python.\n");
    }

    return graph;
}

void
SSTPythonModelDefinition::pushNamePrefix(const char* name)
{
    if ( nullptr == namePrefix ) {
        namePrefix    = (char*)calloc(128, 1);
        namePrefixLen = 128;
    }

    size_t origLen = strlen(namePrefix);
    size_t newLen  = strlen(name);

    // Verify space available
    while ( (origLen + 2 + newLen) >= namePrefixLen ) {
        namePrefix = (char*)realloc(namePrefix, 2 * namePrefixLen);
        namePrefixLen *= 2;
    }

    if ( origLen > 0 ) {
        namePrefix[origLen]     = '.';
        namePrefix[origLen + 1] = '\0';
    }
    strcat(namePrefix, name);
    nameStack.push_back(origLen);
}

void
SSTPythonModelDefinition::popNamePrefix(void)
{
    if ( nameStack.empty() ) return;
    size_t off = nameStack.back();
    nameStack.pop_back();
    namePrefix[off] = '\0';
}

char*
SSTPythonModelDefinition::addNamePrefix(const char* name) const
{
    if ( nameStack.empty() ) { return strdup(name); }
    size_t prefixLen = strlen(namePrefix);
    char*  buf       = (char*)malloc(prefixLen + 2 + strlen(name));

    strcpy(buf, namePrefix);
    buf[prefixLen]     = '.';
    buf[prefixLen + 1] = '\0';
    strcat(buf, name);

    return buf;
}

UnitAlgebra
SSTPythonModelDefinition::getElapsedExecutionTime() const
{
    double      current_time = sst_get_cpu_time();
    double      elapsed_time = current_time - start_time;
    UnitAlgebra ret("1s");
    ret *= elapsed_time;
    return ret;
}

UnitAlgebra
SSTPythonModelDefinition::getLocalMemoryUsage() const
{
    UnitAlgebra ret("1kB");
    ret *= localMemSize();
    return ret;
}

std::map<std::string, std::string>
SST::Core::generateStatisticParameters(PyObject* statParamDict)
{
    PyObject*  pykey = nullptr;
    PyObject*  pyval = nullptr;
    Py_ssize_t pos   = 0;

    std::map<std::string, std::string> p;

    // If the user did not include a dict for the parameters
    // the variable statParamDict will be NULL.
    if ( statParamDict ) {
        // Make sure it really is a Dict
        if ( PyDict_Check(statParamDict) ) {

            // Extract the Key and Value for each parameter and put them into the vectors
            while ( PyDict_Next(statParamDict, &pos, &pykey, &pyval) ) {
                PyObject* pyparam = PyObject_CallMethod(pykey, (char*)"__str__", nullptr);
                PyObject* pyvalue = PyObject_CallMethod(pyval, (char*)"__str__", nullptr);

                p[SST_ConvertToCppString(pyparam)] = SST_ConvertToCppString(pyvalue);

                Py_XDECREF(pyparam);
                Py_XDECREF(pyvalue);
            }
        }
    }
    return p;
}

SST::Params
SST::Core::pythonToCppParams(PyObject* statParamDict)
{
    auto        themap = generateStatisticParameters(statParamDict);
    SST::Params p;
    for ( auto& pair : themap ) {
        p.insert(pair.first, pair.second);
    }
    return p;
}

PyObject*
SST::Core::buildStatisticObject(StatisticId_t id)
{
    PyObject* argList = Py_BuildValue("(k)", id);
    PyObject* statObj = PyObject_CallObject((PyObject*)&PyModel_StatType, argList);
    Py_DECREF(argList);
    return statObj;
}

PyObject*
SST::Core::buildEnabledStatistic(
    ConfigComponent* cc, const char* statName, PyObject* statParamDict, bool apply_to_children)
{
    ConfigStatistic* cs = cc->enableStatistic(statName, pythonToCppParams(statParamDict), apply_to_children);
    return buildStatisticObject(cs->id);
}

PyObject*
SST::Core::buildEnabledStatistics(ConfigComponent* cc, PyObject* statList, PyObject* paramDict, bool apply_to_children)
{
    // Figure out how many stats there are
    Py_ssize_t numStats       = PyList_Size(statList);
    // Generate the Statistic Parameters
    auto       params         = pythonToCppParams(paramDict);
    PyObject*  statObjectList = PyList_New(numStats);
    // For each stat, enable on compoennt
    for ( uint32_t x = 0; x < numStats; x++ ) {
        PyObject*        pylistitem = PyList_GetItem(statList, x);
        PyObject*        pyname     = PyObject_CallMethod(pylistitem, (char*)"__str__", nullptr);
        std::string      name       = SST_ConvertToCppString(pyname);
        ConfigStatistic* cs         = cc->enableStatistic(name, params, apply_to_children);
        PyObject*        statObj    = buildStatisticObject(cs->id);
        PyList_SetItem(statList, x, statObj);
    }
    return statObjectList;
}

REENABLE_WARNING
