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

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include <string.h>

#include "sst/core/sst_types.h"
#include "sst/core/model/python3/pymodel.h"
#include "sst/core/model/python3/pymodel_comp.h"
#include "sst/core/model/python3/pymodel_link.h"
#include "sst/core/model/python3/pymodel_statgroup.h"
#include "sst/core/model/element_python.h"

#include "sst/core/simulation.h"
#include "sst/core/factory.h"
#include "sst/core/component.h"
#include "sst/core/configGraph.h"
DISABLE_WARN_STRICT_ALIASING

using namespace SST::Core;

SST::Core::SSTPythonModelDefinition *gModel = nullptr;

extern "C" {


struct ModuleLoaderPy_t {
    PyObject_HEAD
};


static PyObject* setStatisticOutput(PyObject *self, PyObject *args);
static PyObject* setStatisticLoadLevel(PyObject *self, PyObject *args);

static PyObject* enableAllStatisticsForAllComponents(PyObject *self, PyObject *args);

static PyObject* enableAllStatisticsForComponentName(PyObject *self, PyObject *args);
static PyObject* enableStatisticsForComponentName(PyObject *self, PyObject *args);
static PyObject* enableStatisticForComponentName(PyObject *self, PyObject *args);

static PyObject* enableAllStatisticsForComponentType(PyObject *self, PyObject *args);
static PyObject* enableStatisticsForComponentType(PyObject *self, PyObject *args);
static PyObject* enableStatisticForComponentType(PyObject *self, PyObject *args);

static PyObject* setStatisticLoadLevelForComponentName(PyObject *self, PyObject *args);
static PyObject* setStatisticLoadLevelForComponentType(PyObject *self, PyObject *args);


static PyObject* PyInit_sst(void);

static PyObject* mlFindModule(PyObject *self, PyObject *args);
static PyObject* mlLoadModule(PyObject *self, PyObject *args);




static PyMethodDef mlMethods[] = {
    {   "find_module", mlFindModule, METH_VARARGS, "Finds an SST Element Module"},
    {   "load_module", mlLoadModule, METH_VARARGS, "Loads an SST Element Module"},
    {   nullptr, nullptr, 0, nullptr }
};


static PyTypeObject ModuleLoaderType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "ModuleLoader",            /* tp_name */
    sizeof(ModuleLoaderPy_t),  /* tp_basicsize */
    0,                         /* tp_itemsize */
    nullptr,                   /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    nullptr,                   /* tp_getattr */
    nullptr,                   /* tp_setattr */
    nullptr,                   /* tp_as_async */
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
    "SST Module Loader",       /* tp_doc */
    nullptr,                   /* tp_traverse */
    nullptr,                   /* tp_clear */
    nullptr,                   /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    nullptr,                   /* tp_iter */
    nullptr,                   /* tp_iternext */
    mlMethods,                 /* tp_methods */
    nullptr,                   /* tp_members */
    nullptr,                   /* tp_getset */
    nullptr,                   /* tp_base */
    nullptr,                   /* tp_dict */
    nullptr,                   /* tp_descr_get */
    nullptr,                   /* tp_descr_set */
    0,                         /* tp_dictoffset */
    nullptr,                   /* tp_init */
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
    nullptr,                   /* tp_finalize */
};


// I hate having to do this through a global variable
// but there's really no other way to communicate errors from the importer
// to the parent simulator process
static std::string loadErrors;

static PyObject* mlFindModule(PyObject *self, PyObject *args)
{
    char *name;
    PyObject *path;
    if ( !PyArg_ParseTuple(args, "s|O", &name, &path) )
        return nullptr;

    //reset any previous load errors, they apparently didn't matter
    loadErrors = "";

    std::stringstream err_sstr;
    if ( !strncmp(name, "sst.", 4) ) {
        // We know how to handle only sst.<module>
        // Check for the existence of a library
        char *modName = name+4;
        if ( Factory::getFactory()->hasLibrary(modName, err_sstr) ) {
            // genPythonModuleFunction func = Factory::getFactory()->getPythonModule(modName);
            SSTElementPythonModule* pymod = Factory::getFactory()->getPythonModule(modName);
            if ( pymod ) {
                Py_INCREF(self);
                return self;
            } else {
                loadErrors += std::string("Succeeded in loading library for ") + (const char*) modName
                       + " but library does not contain a Python module\n";
                loadErrors += err_sstr.str();
            }
        } else {
          loadErrors += std::string("No component or Python model registered for ")
                   + (const char*) modName + "\n";
          loadErrors += err_sstr.str();
        }
    }
    Py_RETURN_NONE;
}

static PyMethodDef emptyModMethods[] = {
    {nullptr, nullptr, 0, nullptr }
};

/* This defines an empty module used if modName is not found during mlLoadModule() */
static struct PyModuleDef emptyModuleDef {
    PyModuleDef_HEAD_INIT,  /* m_base */
    "sstempty",             /* m_name */
    nullptr,                /* m_doc */
    -1,                     /* m_size */
    emptyModMethods,        /* m_methods */
    nullptr,                /* m_slots */
    nullptr,                /* m_traverse */
    nullptr,                /* m_clear */
    nullptr,                /* m_free */
};


static PyObject* mlLoadModule(PyObject *UNUSED(self), PyObject *args)
{
    char *name;
    if ( !PyArg_ParseTuple(args, "s", &name) )
        return nullptr;

    if ( strncmp(name, "sst.", 4) ) {
        // We know how to handle only sst.<module>
        return nullptr; // ERROR!
    }

    char *modName = name+4; // sst.<modName>

    //fprintf(stderr, "Loading SST module '%s' (from %s)\n", modName, name);
    // genPythonModuleFunction func = Factory::getFactory()->getPythonModule(modName);
    SSTElementPythonModule* pymod = Factory::getFactory()->getPythonModule(modName);
    PyObject* mod = nullptr;
    if ( !pymod ) {
        mod = PyModule_Create(&emptyModuleDef);
    } else {
        mod = static_cast<PyObject*>(pymod->load());
    }

    return mod;
}



/***** Module information *****/

static PyObject* findComponentByName(PyObject* UNUSED(self), PyObject* args)
{
    char *name = nullptr;
    int argOK = PyArg_ParseTuple(args, "s", &name);

    if ( argOK ) {
        ComponentId_t id = gModel->findComponentByName(name);

        if ( id == UNSET_COMPONENT_ID ) {
            Py_INCREF(Py_None);
	    return Py_None;
	}


        if ( SUBCOMPONENT_ID_MASK(id) == 0 ) {
            // Component
            PyObject *argList = Py_BuildValue("ssk", name, "irrelephant", id);
	    PyObject *res = PyObject_CallObject((PyObject *) &PyModel_ComponentType, argList);
	    Py_DECREF(argList);
	    return res;
	}
	else {
            // SubComponent
            PyObject *argList = Py_BuildValue("Ok", Py_None, id);
	    PyObject *res = PyObject_CallObject((PyObject *) &PyModel_SubComponentType, argList);
	    Py_DECREF(argList);
	    return res;
	}
    }
    else {
        return nullptr;
    }

    return PyLong_FromLong(0);
}

static PyObject* setProgramOption(PyObject* UNUSED(self), PyObject* args)
{
    char *param, *value;
    PyErr_Clear();
    int argOK = PyArg_ParseTuple(args, "ss", &param, &value);

    if ( argOK ) {
        if ( gModel->getConfig()->setConfigEntryFromModel(param, value) )
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    } else {
        return nullptr;
    }
}


static PyObject* setProgramOptions(PyObject* UNUSED(self), PyObject* args)
{
    if ( !PyDict_Check(args) ) {
        return nullptr;
    }
    Py_ssize_t pos = 0;
    PyObject *key, *val;
    long count = 0;
    while ( PyDict_Next(args, &pos, &key, &val) ) {
        if ( gModel->getConfig()->setConfigEntryFromModel(PyUnicode_AsUTF8(key), PyUnicode_AsUTF8(val)) )
            count++;
    }
    return PyLong_FromLong(count);
}


static PyObject* getProgramOptions(PyObject*UNUSED(self), PyObject *UNUSED(args))
{
    // Load parameters already set.
    Config *cfg = gModel->getConfig();

    PyObject* dict = PyDict_New();
    PyDict_SetItem(dict, PyBytes_FromString("debug-file"), PyBytes_FromString(cfg->debugFile.c_str()));
    PyDict_SetItem(dict, PyBytes_FromString("stop-at"), PyBytes_FromString(cfg->stopAtCycle.c_str()));
    PyDict_SetItem(dict, PyBytes_FromString("heartbeat-period"), PyBytes_FromString(cfg->heartbeatPeriod.c_str()));
    PyDict_SetItem(dict, PyBytes_FromString("timebase"), PyBytes_FromString(cfg->timeBase.c_str()));
    PyDict_SetItem(dict, PyBytes_FromString("partitioner"), PyBytes_FromString(cfg->partitioner.c_str()));
    PyDict_SetItem(dict, PyBytes_FromString("verbose"), PyLong_FromLong(cfg->verbose));
    PyDict_SetItem(dict, PyBytes_FromString("output-partition"), PyBytes_FromString(cfg->dump_component_graph_file.c_str()));
    PyDict_SetItem(dict, PyBytes_FromString("output-config"), PyBytes_FromString(cfg->output_config_graph.c_str()));
    PyDict_SetItem(dict, PyBytes_FromString("output-dot"), PyBytes_FromString(cfg->output_dot.c_str()));
    PyDict_SetItem(dict, PyBytes_FromString("numRanks"), PyLong_FromLong(cfg->getNumRanks()));
    PyDict_SetItem(dict, PyBytes_FromString("numThreads"), PyLong_FromLong(cfg->getNumThreads()));

    const char *runModeStr = "UNKNOWN";
    switch (cfg->runMode) {
        case Simulation::INIT: runModeStr = "init"; break;
        case Simulation::RUN: runModeStr = "run"; break;
        case Simulation::BOTH: runModeStr = "both"; break;
        default: break;
    }
    PyDict_SetItem(dict, PyBytes_FromString("run-mode"), PyBytes_FromString(runModeStr));
    return dict;
}


static PyObject* pushNamePrefix(PyObject* UNUSED(self), PyObject* arg)
{
    const char* prefix = PyUnicode_AsUTF8(arg);
    
    if ( prefix != nullptr ) {
        gModel->pushNamePrefix(prefix);
    } else {
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyObject* popNamePrefix(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    gModel->popNamePrefix();
    return PyLong_FromLong(0);
}


static PyObject* exitsst(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    exit(-1);
    return nullptr;
}


static PyObject* getSSTMPIWorldSize(PyObject* UNUSED(self), PyObject* UNUSED(args)) {
    int ranks = 1;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
#endif
    return PyLong_FromLong(ranks);
}


static PyObject* getSSTThreadCount(PyObject* UNUSED(self), PyObject* UNUSED(args)) {
    Config *cfg = gModel->getConfig();
    return PyLong_FromLong(cfg->getNumThreads());
}


static PyObject* setSSTThreadCount(PyObject* UNUSED(self), PyObject* args) {
    Config *cfg = gModel->getConfig();
    long oldNThr = cfg->getNumThreads();
    long nThr = PyLong_AsLong(args);
    if ( nThr > 0 && nThr <= oldNThr )
        cfg->setNumThreads(nThr);
    return PyLong_FromLong(oldNThr);
}


static PyObject* setStatisticOutput(PyObject* UNUSED(self), PyObject* args)
{
    char*      statOutputName;
    int        argOK = 0;
    PyObject*  outputParamDict = nullptr;

    PyErr_Clear();

    // Parse the Python Args and get the StatOutputName and optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!", &statOutputName, &PyDict_Type, &outputParamDict);

    if (argOK) {
        gModel->setStatisticOutput(statOutputName);

        // Generate and Add the Statistic Output Parameters
        for ( auto p : generateStatisticParameters(outputParamDict) ) {
            gModel->addStatisticOutputParameter(p.first, p.second);
        }
    } else {
        return nullptr;
    }
    return PyLong_FromLong(0);
}

static PyObject* setStatisticOutputOption(PyObject* UNUSED(self), PyObject* args)
{
    char* param;
    char* value;
    int   argOK = 0;

    PyErr_Clear();

    argOK = PyArg_ParseTuple(args, "ss", &param, &value);

    if (argOK) {
        gModel->addStatisticOutputParameter(param, value);
    } else {
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyObject* setStatisticOutputOptions(PyObject* UNUSED(self), PyObject* args)
{
    PyErr_Clear();

    if ( !PyDict_Check(args) ) {
        return nullptr;
    }

    // Generate and Add the Statistic Output Parameters
    for ( auto p : generateStatisticParameters(args) ) {
        gModel->addStatisticOutputParameter(p.first, p.second);
    }
    return PyLong_FromLong(0);
}


static PyObject* setStatisticLoadLevel(PyObject* UNUSED(self), PyObject* arg)
{
    PyErr_Clear();

    uint8_t loadLevel = PyLong_AsLong(arg);
    if (PyErr_Occurred()) {
        PyErr_Print();
        exit(-1);
    }

    gModel->setStatisticLoadLevel(loadLevel);

    return PyLong_FromLong(0);
}


static PyObject* enableAllStatisticsForAllComponents(PyObject* UNUSED(self), PyObject* args)
{
    int           argOK = 0;
    PyObject*     statParamDict = nullptr;

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!", &PyDict_Type, &statParamDict);

    if (argOK) {
        gModel->enableStatisticForComponentName(STATALLFLAG, STATALLFLAG, true);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentName(STATALLFLAG, STATALLFLAG, p.first, p.second, true);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }

    return PyLong_FromLong(0);
}


static PyObject* enableAllStatisticsForComponentName(PyObject *UNUSED(self), PyObject *args)
{
    int           argOK = 0;
    char*         compName = nullptr;
    PyObject*     statParamDict = nullptr;
    int           apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!i", &compName, &PyDict_Type, &statParamDict, &apply_to_children);

    if (argOK) {
        gModel->enableStatisticForComponentName(compName, STATALLFLAG, apply_to_children);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentName(compName, STATALLFLAG, p.first, p.second, apply_to_children);
        }

    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}

static PyObject* enableStatisticForComponentName(PyObject *UNUSED(self), PyObject *args)
{
    int           argOK = 0;
    char*         compName = nullptr;
    char*         statName = nullptr;
    PyObject*     statParamDict = nullptr;
    int           apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Name, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "ss|O!i", &compName, &statName, &PyDict_Type, &statParamDict, &apply_to_children);

    if (argOK) {
        gModel->enableStatisticForComponentName(compName, statName, apply_to_children);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentName(compName, statName, p.first, p.second, apply_to_children);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}

static PyObject* enableStatisticsForComponentName(PyObject *UNUSED(self), PyObject *args)
{
    int           argOK = 0;
    char*         compName = nullptr;
    PyObject*     statList = nullptr;
    char*         stat_str = nullptr;
    PyObject*     statParamDict = nullptr;
    Py_ssize_t    numStats = 0;
    int           apply_to_children = 0;

    PyErr_Clear();

    // Can either have a single string, or a list of strings.  Try single string first
    argOK = PyArg_ParseTuple(args,"ss|O!i", &compName, &stat_str, &PyDict_Type, &statParamDict, &apply_to_children);
    if ( argOK ) {
        statList = PyList_New(1);
        PyList_SetItem(statList,0,PyUnicode_FromString(stat_str));
    }
    else  {
        PyErr_Clear();
        apply_to_children = 0;
        // Try list version
        argOK = PyArg_ParseTuple(args, "sO!|O!i", &compName, &PyList_Type, &statList, &PyDict_Type, &statParamDict, &apply_to_children);
        if ( argOK )  Py_INCREF(statList);
    }

    
    if (argOK) {
        // Generate the Statistic Parameters
        auto params = generateStatisticParameters(statParamDict);

        // Get the component
        ComponentId_t id = gModel->findComponentByName(compName);
        if ( UNSET_COMPONENT_ID == id ) {
            gModel->getOutput()->fatal(CALL_INFO,1,"component name not found in call to enableStatisticsForComponentName(): %s\n",compName);
        }

        ConfigComponent* c = gModel->getGraph()->findComponent(id);
        
        // Figure out how many stats there are
        numStats = PyList_Size(statList);

        // For each stat, enable on compoennt
        for (uint32_t x = 0; x < numStats; x++) {
            PyObject* pylistitem = PyList_GetItem(statList, x);
            const char *name = PyUnicode_AsUTF8(pylistitem);
            c->enableStatistic(name,apply_to_children);

            // Add the parameters
            for ( auto p : params ) {
                c->addStatisticParameter(name, p.first, p.second, apply_to_children);
            }

        }        
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyObject* enableAllStatisticsForComponentType(PyObject *UNUSED(self), PyObject *args)
{
    int           argOK = 0;
    char*         compType = nullptr;
    PyObject*     statParamDict = nullptr;
    int           apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Type and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!i", &compType, &PyDict_Type, &statParamDict, &apply_to_children);

    if (argOK) {
        gModel->enableStatisticForComponentType(compType, STATALLFLAG, apply_to_children);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentType(compType, STATALLFLAG, p.first, p.second, apply_to_children);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}

static PyObject* enableStatisticForComponentType(PyObject *UNUSED(self), PyObject *args)
{
    int           argOK = 0;
    char*         compType = nullptr;
    char*         statName = nullptr;
    PyObject*     statParamDict = nullptr;
    int           apply_to_children = 0;
    
    PyErr_Clear();

    // Parse the Python Args Component Type, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "ss|O!i", &compType, &statName, &PyDict_Type, &statParamDict, &apply_to_children);

    if (argOK) {
        gModel->enableStatisticForComponentType(compType, statName, apply_to_children);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentType(compType, statName, p.first, p.second, apply_to_children);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}

static PyObject* enableStatisticsForComponentType(PyObject *UNUSED(self), PyObject *args)
{
    int           argOK = 0;
    char*         compType = nullptr;
    PyObject*     statList = nullptr;
    char*         stat_str = nullptr;
    PyObject*     statParamDict = nullptr;
    Py_ssize_t    numStats = 0;
    int           apply_to_children = 0;

    PyErr_Clear();

    // Can either have a single string, or a list of strings.  Try single string first
    argOK = PyArg_ParseTuple(args,"ss|O!i", &compType, &stat_str, &PyDict_Type, &statParamDict, &apply_to_children);
    if ( argOK ) {
        statList = PyList_New(1);
        PyList_SetItem(statList,0,PyUnicode_FromString(stat_str));
    }
    else  {
        PyErr_Clear();
        apply_to_children = 0;
        // Try list version
        argOK = PyArg_ParseTuple(args, "sO!|O!i", &compType, &PyList_Type, &statList, &PyDict_Type, &statParamDict, &apply_to_children);
        if ( argOK )  Py_INCREF(statList);
    }


    if (argOK) {
        // Figure out how many stats there are
        numStats = PyList_Size(statList);
        for (uint32_t x = 0; x < numStats; x++) {
            PyObject* pylistitem = PyList_GetItem(statList, x);
            const char *statName = PyUnicode_AsUTF8(pylistitem);
            
            gModel->enableStatisticForComponentType(compType, statName, apply_to_children);

            // Generate and Add the Statistic Parameters
            for ( auto p : generateStatisticParameters(statParamDict) ) {
                gModel->addStatisticParameterForComponentType(compType, statName, p.first, p.second, apply_to_children);
            }
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyObject* setStatisticLoadLevelForComponentName(PyObject *UNUSED(self), PyObject *args)
{
    int           argOK = 0;
    char*         compName = nullptr;
    int           level = STATISTICLOADLEVELUNINITIALIZED;
    int           apply_to_children = 0;
    
    PyErr_Clear();

    // Parse the Python Args Component Type, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "sH|i", &compName, &level, &apply_to_children);

    if (argOK) {
        // Get the component
        ComponentId_t id = gModel->findComponentByName(compName);
        if ( UNSET_COMPONENT_ID == id ) {
            gModel->getOutput()->fatal(CALL_INFO,1,"component name not found in call to setStatisticLoadLevelForComponentName(): %s\n",compName);
        }

        ConfigComponent* c = gModel->getGraph()->findComponent(id);

        c->setStatisticLoadLevel(level,apply_to_children);
    }
    else {
        return nullptr;
    }
    return PyLong_FromLong(0);
}

static PyObject* setStatisticLoadLevelForComponentType(PyObject *UNUSED(self), PyObject *args)
{
    int           argOK = 0;
    char*         compType = nullptr;
    uint8_t       level = STATISTICLOADLEVELUNINITIALIZED;
    int           apply_to_children = 0;

    PyErr_Clear();

    // Parse the Python Args Component Type, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "sH|i", &compType, &level, &apply_to_children);

    if (argOK) {
        gModel->getGraph()->setStatisticLoadLevelForComponentType(compType, level, apply_to_children);
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyMethodDef sstModuleMethods[] = {
    {   "setProgramOption",
        setProgramOption, METH_VARARGS,
        "Sets a single program configuration option (form:  setProgramOption(name, value))"},
    {   "setProgramOptions",
        setProgramOptions, METH_O,
        "Sets multiple program configuration option from a dict."},
    {   "getProgramOptions",
        getProgramOptions, METH_NOARGS,
        "Returns a dict of the current program options."},
    {   "pushNamePrefix",
        pushNamePrefix, METH_O,
        "Pushes a string onto the prefix of new component and link names"},
    {   "popNamePrefix",
        popNamePrefix, METH_NOARGS,
        "Removes the most recent addition to the prefix of new component and link names"},
    {   "exit",
        exitsst, METH_NOARGS,
        "Exits SST - indicates the script wanted to exit." },
    {   "getMPIRankCount",
        getSSTMPIWorldSize, METH_NOARGS,
        "Gets the number of MPI ranks currently being used to run SST" },
    {   "getThreadCount",
        getSSTThreadCount, METH_NOARGS,
        "Gets the number of threads currently being used to run SST" },
    {   "setThreadCount",
        setSSTThreadCount, METH_O,
        "Sets the number of threads being used to run SST" },
    {   "setStatisticOutput",
        setStatisticOutput, METH_VARARGS,
        "Sets the Statistic Output - default is console output." },
    {   "setStatisticLoadLevel",
        setStatisticLoadLevel, METH_O,
        "Sets the Statistic Load Level (0 - 10) - default is 0 (disabled)." },
    {   "setStatisticOutputOption",
        setStatisticOutputOption, METH_VARARGS,
        "Sets a single Statistic output option (form: setStatisticOutputOption(name, value))"},
    {   "setStatisticOutputOptions",
        setStatisticOutputOptions, METH_O,
        "Sets multiple Statistic output options from a dict."},
    {   "enableAllStatisticsForAllComponents",
        enableAllStatisticsForAllComponents, METH_VARARGS,
        "Enables all statistics on all components with output at end of simulation."},
    {   "enableAllStatisticsForComponentName",
        enableAllStatisticsForComponentName, METH_VARARGS,
        "Enables all statistics on a component with output occurring at defined rate."},
    {   "enableStatisticForComponentName",
        enableStatisticForComponentName, METH_VARARGS,
        "Enables a single statistic on a component with output occurring at defined rate."},
    {   "enableStatisticsForComponentName",
        enableStatisticsForComponentName, METH_VARARGS,
        "Enables a mulitple statistics on a component with output occurring at defined rate."},
    {   "enableAllStatisticsForComponentType",
        enableAllStatisticsForComponentType, METH_VARARGS,
        "Enables all statistics on all components of component type with output occurring at defined rate."},
    {   "enableStatisticForComponentType",
        enableStatisticForComponentType, METH_VARARGS,
        "Enables a single statistic on all components of component type with output occurring at defined rate."},
    {   "enableStatisticsForComponentType",
        enableStatisticsForComponentType, METH_VARARGS,
        "Enables a list of statistics on all components of component type with output occurring at defined rate."},
    {   "setStatisticLoadLevelForComponentName",
        setStatisticLoadLevelForComponentName, METH_VARARGS,
        "Sets the statistic load level for the specified component name."},
    {   "setStatisticLoadLevelForComponentType",
        setStatisticLoadLevelForComponentType, METH_VARARGS,
        "Sets the statistic load level for all components of the specified type."},
    {   "findComponentByName",
        findComponentByName, METH_VARARGS,
        "Looks up to find a previously created component, based off of its name.  Returns None if none are to be found."},
    {   nullptr, nullptr, 0, nullptr }
};


static struct PyModuleDef sstModuleDef {
    PyModuleDef_HEAD_INIT,  /* m_base */
    "sst",                  /* m_name */
    nullptr,                /* m_doc */
    -1,                     /* m_size */
    sstModuleMethods,       /* m_methods */
    nullptr,                /* m_slots */
    nullptr,                /* m_traverse */
    nullptr,                /* m_clear */
    nullptr,                /* m_free */
};


static PyObject* PyInit_sst(void)
{
    // Initialize our types
    PyModel_ComponentType.tp_new = PyType_GenericNew;
    PyModel_SubComponentType.tp_new = PyType_GenericNew;
    PyModel_LinkType.tp_new = PyType_GenericNew;
    PyModel_StatGroupType.tp_new = PyType_GenericNew;
    PyModel_StatOutputType.tp_new = PyType_GenericNew;
    ModuleLoaderType.tp_new = PyType_GenericNew;
    if ( ( PyType_Ready(&PyModel_ComponentType) < 0 ) ||
         ( PyType_Ready(&PyModel_SubComponentType) < 0 ) ||
         ( PyType_Ready(&PyModel_LinkType) < 0 ) ||
         ( PyType_Ready(&PyModel_StatGroupType) < 0 ) ||
         ( PyType_Ready(&PyModel_StatOutputType) < 0 ) ||
         ( PyType_Ready(&ModuleLoaderType) < 0 ) ) {
        return nullptr; // TODO better error message
    }

    // Create the module
    PyObject *module = PyModule_Create(&sstModuleDef);
    if (!module)
        return nullptr;

    Py_INCREF(&PyModel_ComponentType);
    Py_INCREF(&PyModel_SubComponentType);
    Py_INCREF(&PyModel_LinkType);
    Py_INCREF(&PyModel_StatGroupType);
    Py_INCREF(&PyModel_StatOutputType);
    Py_INCREF(&ModuleLoaderType);

    // Add the types
    PyModule_AddObject(module, "Link", (PyObject*)&PyModel_LinkType);
    PyModule_AddObject(module, "Component", (PyObject*)&PyModel_ComponentType);
    PyModule_AddObject(module, "SubComponent", (PyObject*)&PyModel_SubComponentType);
    PyModule_AddObject(module, "StatisticGroup", (PyObject*)&PyModel_StatGroupType);
    PyModule_AddObject(module, "StatisticOutput", (PyObject*)&PyModel_StatOutputType);
    PyModule_AddObject(module, "ModuleLoader", (PyObject*)&ModuleLoaderType);
    PyModule_AddObject(module, "__path__", Py_BuildValue("()"));

    return module;
}

}  /* extern C */


void SSTPythonModelDefinition::initModel(const std::string& script_file, int verbosity, Config* UNUSED(config), int argc, char** argv)
{
    output = new Output("SSTPythonModel ", verbosity, 0, SST::Output::STDOUT);

    if ( gModel ) {
        output->fatal(CALL_INFO, 1, "A Python Config Model is already in progress.\n");
    }
    gModel = this;

    graph = new ConfigGraph();
    nextComponentId = 0;

    std::string local_script_name;
    int substr_index = 0;

    for(substr_index = script_file.size() - 1; substr_index >= 0; --substr_index) {
        if(script_file.at(substr_index) == '/') {
            substr_index++;
            break;
        }
    }

    const std::string file_name_only = script_file.substr(std::max(0, substr_index));
    local_script_name = file_name_only.substr(0, file_name_only.size() - 3);

    output->verbose(CALL_INFO, 2, 0, "SST loading a Python model from script: %s / [%s]\n",
        script_file.c_str(), local_script_name.c_str());

    // Add sst module to the Python interpreter as a built in
    PyImport_AppendInittab("sst", &PyInit_sst);

    // Get the Python scripting engine started
    Py_Initialize();

    // Set arguments; Python3 takes wchar_t* arg instead of char*
    wchar_t** wargv = (wchar_t**)PyMem_Malloc(sizeof(wchar_t*)*argc);
    for (int i = 0; i < argc; i++)
        wargv[i] = Py_DecodeLocale(argv[i], nullptr);
    PySys_SetArgv(argc, wargv);
    PyRun_SimpleString("import sys\n"
                       "import sst\n"
                       "sys.meta_path.append(sst.ModuleLoader())\n");
}

SSTPythonModelDefinition::SSTPythonModelDefinition(const std::string& script_file, int verbosity, Config* configObj) :
    SSTModelDescription(), scriptName(script_file), config(configObj), namePrefix(nullptr), namePrefixLen(0)
{
    std::vector<std::string> argv_vector;
    argv_vector.push_back("sstsim.x");

    const int input_len = configObj->model_options.length();
    std::string temp = "";
    bool in_string = false;

    for(int i = 0; i < input_len; ++i) {
        if(configObj->model_options.substr(i, 1) == "\"") {
            if(in_string) {
                // We are ending a string
                if(! (temp == "")) {
                    argv_vector.push_back(temp);
                    temp = "";
                }

                in_string = false;
            } else {
                // We are starting a string
                in_string = true;
            }
        } else if(configObj->model_options.substr(i, 1) == " ") {
            if(in_string) {
                temp += " ";
            } else {
                if(! (temp == "")) {
                    argv_vector.push_back(temp);
                }

                temp = "";
            }
        } else {
            temp += configObj->model_options.substr(i, 1);
        }
    }

    // need to handle the last argument with a special case
    if(temp != "") {
        if(in_string) {
            // this maybe en error?
        } else {
            if(! (temp == "") ) {
                argv_vector.push_back(temp);
            }
        }
    }

    // generate C main style inputs to the Python program based on our processing above.
    char** argv = (char**) malloc(sizeof(char*) * argv_vector.size());
    const int argc = argv_vector.size();

    for(int i = 0; i < argc; ++i) {
        argv[i] = (char*) malloc(sizeof(char) * argv_vector[i].size() + 1);
        sprintf(argv[i], "%s", argv_vector[i].c_str());
    }

    // Init the model
    initModel(script_file, verbosity, configObj, argc, argv);

    // Free the vector
    free(argv);
}

SSTPythonModelDefinition::SSTPythonModelDefinition(const std::string& script_file, int verbosity,
    Config* configObj, int argc, char **argv) :
    SSTModelDescription(), scriptName(script_file), config(configObj)
{
    initModel(script_file, verbosity, configObj, argc, argv);
}

SSTPythonModelDefinition::~SSTPythonModelDefinition() {
    delete output;
    gModel = nullptr;

    if ( nullptr != namePrefix ) free(namePrefix);
}


ConfigGraph* SSTPythonModelDefinition::createConfigGraph()
{
    output->verbose(CALL_INFO, 1, 0, "Creating config graph for SST using Python model...\n");

    FILE *fp = fopen(scriptName.c_str(), "r");
    if ( !fp ) {
        output->fatal(CALL_INFO, 1,
                "Unable to open python script %s\n", scriptName.c_str());
    }
    int createReturn = PyRun_AnyFileEx(fp, scriptName.c_str(), 1);

    if(nullptr != PyErr_Occurred()) {
        // Print the Python error and then let SST exit as a fatal-stop.
        PyErr_Print();
        output->fatal(CALL_INFO, 1,
            "Error occurred executing the Python SST model script.\n");
    }
    if(-1 == createReturn) {
        output->fatal(CALL_INFO, 1,
            "Execution of model construction function failed\n%s",
             loadErrors.c_str());
    }


    output->verbose(CALL_INFO, 1, 0, "Construction of config graph with Python is complete.\n");

    if(nullptr != PyErr_Occurred()) {
        PyErr_Print();
        output->fatal(CALL_INFO, 1, "Error occured handling the creation of the component graph in Python.\n");
    }

    return graph;
}


void SSTPythonModelDefinition::pushNamePrefix(const char *name)
{
    if ( nullptr == namePrefix ) {
        namePrefix = (char*)calloc(128, 1);
        namePrefixLen = 128;
    }

    size_t origLen = strlen(namePrefix);
    size_t newLen = strlen(name);

    // Verify space available
    while ( (origLen + 2 + newLen) >= namePrefixLen ) {
        namePrefix = (char*)realloc(namePrefix, 2*namePrefixLen);
        namePrefixLen *= 2;
    }

    if ( origLen > 0 ) {
        namePrefix[origLen] = '.';
        namePrefix[origLen+1] = '\0';
    }
    strcat(namePrefix, name);
    nameStack.push_back(origLen);
}


void SSTPythonModelDefinition::popNamePrefix(void)
{
    if ( nameStack.empty() ) return;
    size_t off = nameStack.back();
    nameStack.pop_back();
    namePrefix[off] = '\0';
}


char* SSTPythonModelDefinition::addNamePrefix(const char *name) const
{
    if ( nameStack.empty() ) {
        return strdup(name);
    }
    size_t prefixLen = strlen(namePrefix);
    char *buf = (char*)malloc(prefixLen + 2 + strlen(name));

    strcpy(buf, namePrefix);
    buf[prefixLen] = '.';
    buf[prefixLen+1] = '\0';
    strcat(buf, name);

    return buf;
}

/* Utilities */
std::map<std::string,std::string> SST::Core::generateStatisticParameters(PyObject* statParamDict)
{
    PyObject*     pykey = nullptr;
    PyObject*     pyval = nullptr;
    Py_ssize_t    pos = 0;

    std::map<std::string,std::string> p;

    // If the user did not include a dict for the parameters
    // the variable statParamDict will be NULL.
    if (nullptr != statParamDict) {
        // Make sure it really is a Dict
        if (true == PyDict_Check(statParamDict)) {

            // Extract the Key and Value for each parameter and put them into the vectors
            while ( PyDict_Next(statParamDict, &pos, &pykey, &pyval) ) {
                PyObject* pyparam = PyObject_Str(pykey);
                PyObject* pyvalue = PyObject_Str(pyval);

                p[PyUnicode_AsUTF8(pyparam)] = PyUnicode_AsUTF8(pyvalue);

                Py_XDECREF(pyparam);
                Py_XDECREF(pyvalue);
            }
        }
    }
    return p;
}

REENABLE_WARNING
