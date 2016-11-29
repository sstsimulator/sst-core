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
#ifdef SST_CONFIG_HAVE_PYTHON
#include <Python.h>

#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif

#include <string.h>
#include <sstream>

#include <sst/core/sst_types.h>
#include <sst/core/model/pymodel.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/factory.h>
#include <sst/core/component.h>

static SSTPythonModelDefinition *gModel = NULL;

using namespace SST;

extern "C" {


typedef struct {
    PyObject_HEAD
    ComponentId_t id;
	char *name;
} ComponentPy_t;


typedef struct {
    PyObject_HEAD
    char *name;
    bool no_cut;
    char *latency;
} LinkPy_t;


typedef struct {
    PyObject_HEAD
} ModuleLoaderPy_t;

static int compInit(ComponentPy_t *self, PyObject *args, PyObject *kwds);
static void compDealloc(ComponentPy_t *self);
static PyObject* compAddParam(PyObject *self, PyObject *args);
static PyObject* compAddParams(PyObject *self, PyObject *args);
static PyObject* compSetRank(PyObject *self, PyObject *arg);
static PyObject* compSetWeight(PyObject *self, PyObject *arg);
static PyObject* compAddLink(PyObject *self, PyObject *args);
static PyObject* compGetFullName(PyObject *self, PyObject *args);
static int compCompare(PyObject *obj0, PyObject *obj1);

static PyObject* compEnableAllStatistics(PyObject *self, PyObject *args);
static PyObject* compEnableStatistics(PyObject *self, PyObject *args);

static PyObject* setStatisticOutput(PyObject *self, PyObject *args);
static PyObject* setStatisticLoadLevel(PyObject *self, PyObject *args);

static PyObject* enableAllStatisticsForAllComponents(PyObject *self, PyObject *args);
static PyObject* enableAllStatisticsForComponentName(PyObject *self, PyObject *args);
static PyObject* enableAllStatisticsForComponentType(PyObject *self, PyObject *args);
static PyObject* enableStatisticForComponentName(PyObject *self, PyObject *args);
static PyObject* enableStatisticForComponentType(PyObject *self, PyObject *args);

static int linkInit(LinkPy_t *self, PyObject *args, PyObject *kwds);
static void linkDealloc(LinkPy_t *self);
static PyObject* linkConnect(PyObject* self, PyObject *args);
static PyObject* linkSetNoCut(PyObject* self, PyObject *args);

static PyObject* mlFindModule(PyObject *self, PyObject *args);
static PyObject* mlLoadModule(PyObject *self, PyObject *args);

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
    {   NULL, NULL, 0, NULL }
};


static PyTypeObject ComponentType = {
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

static PyMethodDef linkMethods[] = {
    {   "connect",
        linkConnect, METH_VARARGS,
        "Connects two components to a Link"},
    {   "setNoCut",
        linkSetNoCut, METH_NOARGS,
        "Specifies that this link should not be partitioned across"},
    {   NULL, NULL, 0, NULL }
};


static PyTypeObject LinkType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.Link",                /* tp_name */
    sizeof(LinkPy_t),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)linkDealloc,   /* tp_dealloc */
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
    "SST Link",                /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    linkMethods,               /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)linkInit,        /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};


static PyMethodDef mlMethods[] = {
    {   "find_module", mlFindModule, METH_VARARGS, "Finds an SST Element Module"},
    {   "load_module", mlLoadModule, METH_VARARGS, "Loads an SST Element Module"},
    {   NULL, NULL, 0, NULL }
};


static PyTypeObject ModuleLoaderType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "ModuleLoader",        /* tp_name */
    sizeof(ModuleLoaderPy_t),  /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
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
    "SST Module Loader",       /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    mlMethods,                 /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};



static int compInit(ComponentPy_t *self, PyObject *args, PyObject *kwds)
{
    char *name, *type;
    ComponentId_t useID = UNSET_COMPONENT_ID;
    if ( !PyArg_ParseTuple(args, "ss|k", &name, &type, &useID) )
        return -1;

    if ( useID == UNSET_COMPONENT_ID ) {
        self->name = gModel->addNamePrefix(name);
        self->id = gModel->addComponent(self->name, type);
        gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating component [%s] of type [%s]: id [%lu]\n", name, type, self->id);
    } else {
        self->name = name;
        self->id = useID;
    }

    return 0;
}


static void compDealloc(ComponentPy_t *self)
{
    if ( self->name ) free(self->name);
    self->ob_type->tp_free((PyObject*)self);
}



static PyObject* compAddParam(PyObject *self, PyObject *args)
{
    char *param = NULL;
    PyObject *value = NULL;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) )
        return NULL;

    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyObject *vstr = PyObject_CallMethod(value, (char*)"__str__", NULL);
    gModel->addParameter(id, param, PyString_AsString(vstr));
    Py_XDECREF(vstr);

    return PyInt_FromLong(0);
}


static PyObject* compAddParams(PyObject *self, PyObject *args)
{
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    if ( !PyDict_Check(args) ) {
        return NULL;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *val;
    long count = 0;

    while ( PyDict_Next(args, &pos, &key, &val) ) {
        PyObject *kstr = PyObject_CallMethod(key, (char*)"__str__", NULL);
        PyObject *vstr = PyObject_CallMethod(val, (char*)"__str__", NULL);
        gModel->addParameter(id, PyString_AsString(kstr), PyString_AsString(vstr));
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return PyInt_FromLong(count);
}


static PyObject* compSetRank(PyObject *self, PyObject *args)
{
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyErr_Clear();
    // long rank = PyInt_AsLong(arg);
    // if ( PyErr_Occurred() ) {
    //     PyErr_Print();
    //     exit(-1);
    // }

    unsigned long rank = (unsigned long)-1;
    unsigned long thread = (unsigned long)0;

    if ( !PyArg_ParseTuple(args, "k|k", &rank, &thread) ) {
        return NULL;
    }

    
    gModel->setComponentRank(id, rank, thread);

    return PyInt_FromLong(0);
}


static PyObject* compSetWeight(PyObject *self, PyObject *arg)
{
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyErr_Clear();
    double weight = PyFloat_AsDouble(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    gModel->setComponentWeight(id, weight);

    return PyInt_FromLong(0);
}


static PyObject* compAddLink(PyObject *self, PyObject *args)
{
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyObject *plink = NULL;
    char *port = NULL, *lat = NULL;


    if ( !PyArg_ParseTuple(args, "O!s|s", &LinkType, &plink, &port, &lat) ) {
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
    return PyString_FromString(((ComponentPy_t*)self)->name);
}

static int compCompare(PyObject *obj0, PyObject *obj1) {
    ComponentId_t id0 = ((ComponentPy_t*)obj0)->id;
    ComponentId_t id1 = ((ComponentPy_t*)obj1)->id;
    return (id0 < id1) ? -1 : (id0 > id1) ? 1 : 0;
}

void generateStatisticParameters(PyObject* statParamDict)
{
    PyObject*     pykey = NULL;
    PyObject*     pyval = NULL;
    Py_ssize_t    pos = 0;

    gModel->statParamKeyArray.clear();
    gModel->statParamValueArray.clear();
    
    // If the user did not include a dict for the parameters
    // the variable statParamDict will be NULL.       
    if (NULL != statParamDict) {
        // Make sure it really is a Dict
        if (true == PyDict_Check(statParamDict)) {

            // Extract the Key and Value for each parameter and put them into the vectors 
            while ( PyDict_Next(statParamDict, &pos, &pykey, &pyval) ) {
                PyObject* pyparam = PyObject_CallMethod(pykey, (char*)"__str__", NULL);
                PyObject* pyvalue = PyObject_CallMethod(pyval, (char*)"__str__", NULL);
                
                gModel->statParamKeyArray.push_back(PyString_AsString(pyparam));
                gModel->statParamValueArray.push_back(PyString_AsString(pyvalue));
                
                Py_XDECREF(pyparam);
                Py_XDECREF(pyvalue);
            }
        }
    }
}

static PyObject* compEnableAllStatistics(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    PyObject*     statParamDict = NULL;
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!", &PyDict_Type, &statParamDict);

    if (argOK) {
        gModel->enableComponentStatistic(id, STATALLFLAG);

        // Generate and Add the Statistic Parameters
        generateStatisticParameters(statParamDict);
        for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
            gModel->addComponentStatisticParameter(id, STATALLFLAG, gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
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
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyErr_Clear();
    
    // Parse the Python Args and get A List Object and the optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "O!|O!", &PyList_Type, &statList, &PyDict_Type, &statParamDict);

    if (argOK) {
        // Generate the Statistic Parameters
        generateStatisticParameters(statParamDict);
        
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
    
            gModel->enableComponentStatistic(id, PyString_AsString(pyname));
            
            // Add the parameters
            for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
                gModel->addComponentStatisticParameter(id, PyString_AsString(pyname), gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
            }

            Py_XDECREF(pyname);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}

static int linkInit(LinkPy_t *self, PyObject *args, PyObject *kwds)
{
    char *name = NULL, *lat = NULL;
    if ( !PyArg_ParseTuple(args, "s|s", &name, &lat) ) return -1;

    self->name = gModel->addNamePrefix(name);
    self->no_cut = false;
    self->latency = lat ? strdup(lat) : NULL;
	gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating Link %s\n", self->name);

    return 0;
}

static void linkDealloc(LinkPy_t *self)
{
    if ( self->name ) free(self->name);
    if ( self->latency ) free(self->latency);
    self->ob_type->tp_free((PyObject*)self);
}


static PyObject* linkConnect(PyObject* self, PyObject *args)
{
    PyObject *t0, *t1;
    if ( !PyArg_ParseTuple(args, "O!O!",
                &PyTuple_Type, &t0,
                &PyTuple_Type, &t1) ) {
        return NULL;
    }

    PyObject *c0, *c1;
    char *port0, *port1;
    char *lat0 = NULL, *lat1 = NULL;

    LinkPy_t *link = (LinkPy_t*)self;

    if ( !PyArg_ParseTuple(t0, "O!s|s",
                &ComponentType, &c0, &port0, &lat0) )
        return NULL;
    if ( NULL == lat0 )
        lat0 = link->latency;

    if ( !PyArg_ParseTuple(t1, "O!s|s",
                &ComponentType, &c1, &port1, &lat1) )
        return NULL;
    if ( NULL == lat1 )
        lat1 = link->latency;

    if ( NULL == lat0 || NULL == lat1 ) {
        gModel->getOutput()->fatal(CALL_INFO, 1, "No Latency specified for link %s\n", link->name);
        return NULL;
    }

	gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Connecting components %lu and %lu to Link %s (lat: %p %p)\n",
			((ComponentPy_t*)c0)->id, ((ComponentPy_t*)c1)->id, ((LinkPy_t*)self)->name, lat0, lat1);
    gModel->addLink(((ComponentPy_t*)c0)->id,
            link->name, port0, lat0, link->no_cut);
    gModel->addLink(((ComponentPy_t*)c1)->id,
            link->name, port1, lat1, link->no_cut);


    return PyInt_FromLong(0);
}


static PyObject* linkSetNoCut(PyObject* self, PyObject *args)
{
    LinkPy_t *link = (LinkPy_t*)self;
    bool prev = link->no_cut;
    link->no_cut = true;
    return PyBool_FromLong(prev ? 1 : 0);
}


static PyObject* mlFindModule(PyObject *self, PyObject *args)
{
    char *name;
    PyObject *path;
    if ( !PyArg_ParseTuple(args, "s|O", &name, &path) )
        return NULL;

    if ( !strncmp(name, "sst.", 4) ) {
        // We know how to handle only sst.<module>
        // Check for the existence of a library
        char *modName = name+4;
        if ( Factory::getFactory()->hasLibrary(modName) ) {
            genPythonModuleFunction func = Factory::getFactory()->getPythonModule(modName);
            if ( func ) {
                Py_INCREF(self);
                return self;
            }
        }
    }

    Py_RETURN_NONE;
}

static PyMethodDef emptyModMethods[] = {
    {NULL, NULL, 0, NULL }
};

static PyObject* mlLoadModule(PyObject *self, PyObject *args)
{
    char *name;
    if ( !PyArg_ParseTuple(args, "s", &name) )
        return NULL;

    if ( strncmp(name, "sst.", 4) ) {
        // We know how to handle only sst.<module>
        return NULL; // ERROR!
    }

    char *modName = name+4; // sst.<modName>

    //fprintf(stderr, "Loading SST module '%s' (from %s)\n", modName, name);
    genPythonModuleFunction func = Factory::getFactory()->getPythonModule(modName);
    PyObject* mod = NULL;
    if ( !func ) {
        mod = Py_InitModule(name, emptyModMethods);
    } else {
        mod = static_cast<PyObject*>((*func)());
    }

    return mod;
}



/***** Module information *****/

static PyObject* findComponentByName(PyObject* self, PyObject* args)
{
    if ( ! PyString_Check(args) ) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    char *name = PyString_AsString(args);
    ComponentId_t id = gModel->findComponentByName(name);
    if ( id != UNSET_COMPONENT_ID ) {
        PyObject *argList = Py_BuildValue("ssk", name, "irrelephant", id);
        PyObject *res = PyObject_CallObject((PyObject *) &ComponentType, argList);
        return res;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* setProgramOption(PyObject* self, PyObject* args)
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
        return NULL;
    }
}


static PyObject* setProgramOptions(PyObject* self, PyObject* args)
{
    if ( !PyDict_Check(args) ) {
        return NULL;
    }
    Py_ssize_t pos = 0;
    PyObject *key, *val;
    long count = 0;
    while ( PyDict_Next(args, &pos, &key, &val) ) {
        if ( gModel->getConfig()->setConfigEntryFromModel(PyString_AsString(key), PyString_AsString(val)) )
            count++;
    }
    return PyInt_FromLong(count);
}


static PyObject* getProgramOptions(PyObject*self, PyObject *args)
{
    // Load parameters already set.
    Config *cfg = gModel->getConfig();

    PyObject* dict = PyDict_New();
    PyDict_SetItem(dict, PyString_FromString("debug-file"), PyString_FromString(cfg->debugFile.c_str()));
    PyDict_SetItem(dict, PyString_FromString("stop-at"), PyString_FromString(cfg->stopAtCycle.c_str()));
    PyDict_SetItem(dict, PyString_FromString("heartbeat-period"), PyString_FromString(cfg->heartbeatPeriod.c_str()));
    PyDict_SetItem(dict, PyString_FromString("timebase"), PyString_FromString(cfg->timeBase.c_str()));
    PyDict_SetItem(dict, PyString_FromString("partitioner"), PyString_FromString(cfg->partitioner.c_str()));
    PyDict_SetItem(dict, PyString_FromString("verbose"), PyLong_FromLong(cfg->verbose));
    PyDict_SetItem(dict, PyString_FromString("output-partition"), PyString_FromString(cfg->dump_component_graph_file.c_str()));
    PyDict_SetItem(dict, PyString_FromString("output-config"), PyString_FromString(cfg->output_config_graph.c_str()));
    PyDict_SetItem(dict, PyString_FromString("output-dot"), PyString_FromString(cfg->output_dot.c_str()));
	PyDict_SetItem(dict, PyString_FromString("numRanks"), PyLong_FromLong(cfg->getNumRanks()));
	PyDict_SetItem(dict, PyString_FromString("numThreads"), PyLong_FromLong(cfg->getNumThreads()));

    const char *runModeStr = "UNKNOWN";
    switch (cfg->runMode) {
        case Simulation::INIT: runModeStr = "init"; break;
        case Simulation::RUN: runModeStr = "run"; break;
        case Simulation::BOTH: runModeStr = "both"; break;
        default: break;
    }
    PyDict_SetItem(dict, PyString_FromString("run-mode"), PyString_FromString(runModeStr));
    return dict;
}


static PyObject* pushNamePrefix(PyObject* self, PyObject* arg)
{
    char *name = NULL;
    PyErr_Clear();
    name = PyString_AsString(arg);

    if ( name != NULL ) {
        gModel->pushNamePrefix(name);
    } else {
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* popNamePrefix(PyObject* self, PyObject* args)
{
    gModel->popNamePrefix();
    return PyInt_FromLong(0);
}


static PyObject* exitsst(PyObject* self, PyObject* args)
{
    exit(-1);
    return NULL;
}

static PyObject* getSSTMPIWorldSize(PyObject* self, PyObject* args) {
    int ranks = 1;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
#endif
    return PyInt_FromLong(ranks);
}

static PyObject* getSSTThreadCount(PyObject* self, PyObject* args) {
    Config *cfg = gModel->getConfig();
    return PyLong_FromLong(cfg->getNumThreads());
}

static PyObject* setSSTThreadCount(PyObject* self, PyObject* args) {
    Config *cfg = gModel->getConfig();
    long oldNThr = cfg->getNumThreads();
    long nThr = PyLong_AsLong(args);
    if ( nThr > 0 && nThr <= oldNThr )
        cfg->setNumThreads(nThr);
    return PyLong_FromLong(oldNThr);
}


static PyObject* setStatisticOutput(PyObject* self, PyObject* args)
{
    char*      statOutputName; 
    int        argOK = 0;
    PyObject*  outputParamDict = NULL;

    PyErr_Clear();
    
    // Parse the Python Args and get the StatOutputName and optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!", &statOutputName, &PyDict_Type, &outputParamDict);

    if (argOK) {
        gModel->setStatisticOutput(statOutputName);
        
        // Generate and Add the Statistic Output Parameters
        generateStatisticParameters(outputParamDict);
        for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
            gModel->addStatisticOutputParameter(gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
        }
    } else {
        return NULL;
    }
    return PyInt_FromLong(0);
}

static PyObject* setStatisticOutputOption(PyObject* self, PyObject* args)
{
    char* param;
    char* value;
    int   argOK = 0;
    
    PyErr_Clear();
    
    argOK = PyArg_ParseTuple(args, "ss", &param, &value);

    if (argOK) {
        gModel->addStatisticOutputParameter(param, value);
    } else {
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* setStatisticOutputOptions(PyObject* self, PyObject* args)
{
    PyErr_Clear();
    
    if ( !PyDict_Check(args) ) {
        return NULL;
    }
    
    // Generate and Add the Statistic Output Parameters
    generateStatisticParameters(args);
    for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
        gModel->addStatisticOutputParameter(gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
    }
    return PyInt_FromLong(0);
}


static PyObject* setStatisticLoadLevel(PyObject*self, PyObject* arg)
{
    PyErr_Clear();
    
    uint8_t loadLevel = PyInt_AsLong(arg); 
    if (PyErr_Occurred()) {
        PyErr_Print();
        exit(-1);
    }

    gModel->setStatisticLoadLevel(loadLevel);

    return PyInt_FromLong(0);
}


static PyObject* enableAllStatisticsForAllComponents(PyObject* self, PyObject* args)
{
    int           argOK = 0;
    PyObject*     statParamDict = NULL;
                  
    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!", &PyDict_Type, &statParamDict);

    if (argOK) {
        gModel->enableStatisticForComponentName(STATALLFLAG, STATALLFLAG);

        // Generate and Add the Statistic Parameters
        generateStatisticParameters(statParamDict);
        for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
            gModel->addStatisticParameterForComponentName(STATALLFLAG, STATALLFLAG, gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    
    return PyInt_FromLong(0);
}


static PyObject* enableAllStatisticsForComponentName(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    char*         compName = NULL;
    PyObject*     statParamDict = NULL;
                  
    PyErr_Clear();

    // Parse the Python Args Component Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!", &compName, &PyDict_Type, &statParamDict);

    if (argOK) {
        gModel->enableStatisticForComponentName(compName, STATALLFLAG);

        // Generate and Add the Statistic Parameters
        generateStatisticParameters(statParamDict);
        for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
            gModel->addStatisticParameterForComponentName(compName, STATALLFLAG, gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
        }
                
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* enableAllStatisticsForComponentType(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    char*         compType = NULL;
    PyObject*     statParamDict = NULL;
                  
    PyErr_Clear();

    // Parse the Python Args Component Type and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "s|O!", &compType, &PyDict_Type, &statParamDict);

    if (argOK) {
        gModel->enableStatisticForComponentType(compType, STATALLFLAG);

        // Generate and Add the Statistic Parameters
        generateStatisticParameters(statParamDict);
        for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
            gModel->addStatisticParameterForComponentType(compType, STATALLFLAG, gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* enableStatisticForComponentName(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    char*         compName = NULL;
    char*         statName = NULL;
    PyObject*     statParamDict = NULL;
                  
    PyErr_Clear();

    // Parse the Python Args Component Name, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "ss|O!", &compName, &statName, &PyDict_Type, &statParamDict);

    if (argOK) {
        gModel->enableStatisticForComponentName(compName, statName);

        // Generate and Add the Statistic Parameters
        generateStatisticParameters(statParamDict);
        for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
            gModel->addStatisticParameterForComponentName(compName, statName, gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* enableStatisticForComponentType(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    char*         compType = NULL;
    char*         statName = NULL;
    PyObject*     statParamDict = NULL;
                  
    PyErr_Clear();

    // Parse the Python Args Component Type, Stat Name and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "ss|O!", &compType, &statName, &PyDict_Type, &statParamDict);

    if (argOK) {
        gModel->enableStatisticForComponentType(compType, statName);

        // Generate and Add the Statistic Parameters
        generateStatisticParameters(statParamDict);
        for (uint32_t x = 0; x < gModel->statParamKeyArray.size(); x++) {
            gModel->addStatisticParameterForComponentType(compType, statName, gModel->statParamKeyArray[x].c_str(), gModel->statParamValueArray[x].c_str());
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
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
        "Gets the number of MPI ranks currently being used to run SST" },
    {   "setThreadCount",
        setSSTThreadCount, METH_O,
        "Gets the number of MPI ranks currently being used to run SST" },
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
        "Enables all statistics on all components with output at end of simuation."},
    {   "enableAllStatisticsForComponentName",
        enableAllStatisticsForComponentName, METH_VARARGS,
        "Enables all statistics on a component with output occurring at defined rate."},
    {   "enableAllStatisticsForComponentType",
        enableAllStatisticsForComponentType, METH_VARARGS,
        "Enables all statistics on all components of component type with output occurring at defined rate."},
    {   "enableStatisticForComponentName",
        enableStatisticForComponentName, METH_VARARGS,
        "Enables a single statistic on a component with output occurring at defined rate."},
    {   "enableStatisticForComponentType",
        enableStatisticForComponentType, METH_VARARGS,
        "Enables a single statistic on all components of component type with output occurring at defined rate."},
    {   "findComponentByName",
        findComponentByName, METH_O,
        "Looks up to find a previously created component, based off of its name.  Returns None if none are to be found."},
    {   NULL, NULL, 0, NULL }
};

}  /* extern C */


void SSTPythonModelDefinition::initModel(const std::string script_file, int verbosity, Config* config, int argc, char** argv)
{
    output = new Output("SSTPythonModel ", verbosity, 0, SST::Output::STDOUT);

    if ( gModel ) {
        output->fatal(CALL_INFO, -1, "A Python Config Model is already in progress.\n");
    }
    gModel = this;

    graph = new ConfigGraph();

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

    // Get the Python scripting engine started
    Py_Initialize();

    PySys_SetArgv(argc, argv);

    // Initialize our types
    ComponentType.tp_new = PyType_GenericNew;
    LinkType.tp_new = PyType_GenericNew;
    ModuleLoaderType.tp_new = PyType_GenericNew;
    if ( ( PyType_Ready(&ComponentType) < 0 ) ||
         ( PyType_Ready(&LinkType) < 0 ) ||
         ( PyType_Ready(&ModuleLoaderType) < 0 ) ) {
        output->fatal(CALL_INFO, -1, "Error loading Python types.\n");
    }

    // Add our built in methods to the Python engine
    PyObject *module = Py_InitModule("sst", sstModuleMethods);

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    Py_INCREF(&ComponentType);
    Py_INCREF(&LinkType);
    Py_INCREF(&ModuleLoaderType);
#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif
    
    PyModule_AddObject(module, "Link", (PyObject*)&LinkType);
    PyModule_AddObject(module, "Component", (PyObject*)&ComponentType);


    // Add our custom loader
    PyObject *main_module = PyImport_ImportModule("__main__");
    PyModule_AddObject(main_module, "ModuleLoader", (PyObject*)&ModuleLoaderType);
    PyRun_SimpleString("def loadLoader():\n"
            "\timport sys\n"
            "\tsys.meta_path.append(ModuleLoader())\n"
            "\timport sst\n"
            "\tsst.__path__ = []\n"  // Must be here or else meta_path won't be questioned
            "loadLoader()\n");

}

SSTPythonModelDefinition::SSTPythonModelDefinition(const std::string script_file, int verbosity, Config* configObj) :
	SSTModelDescription(), scriptName(script_file), config(configObj), namePrefix(NULL), namePrefixLen(0)
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

SSTPythonModelDefinition::SSTPythonModelDefinition(const std::string script_file, int verbosity,
    Config* configObj, int argc, char **argv) :
    SSTModelDescription(), scriptName(script_file), config(configObj)
{
	initModel(script_file, verbosity, configObj, argc, argv);
}

SSTPythonModelDefinition::~SSTPythonModelDefinition() {
    delete output;
    gModel = NULL;

    if ( NULL != namePrefix ) free(namePrefix);
}


ConfigGraph* SSTPythonModelDefinition::createConfigGraph()
{
    output->verbose(CALL_INFO, 1, 0, "Creating config graph for SST using Python model...\n");

    FILE *fp = fopen(scriptName.c_str(), "r");
    if ( !fp ) {
        output->fatal(CALL_INFO, -1,
                "Unable to open python script %s\n", scriptName.c_str());
    }
    int createReturn = PyRun_AnyFileEx(fp, scriptName.c_str(), 1);

    if(NULL != PyErr_Occurred()) {
        // Print the Python error and then let SST exit as a fatal-stop.
        PyErr_Print();
        output->fatal(CALL_INFO, -1,
            "Error occurred executing the Python SST model script.\n");
    }
    if(-1 == createReturn) {
        output->fatal(CALL_INFO, -1,
            "Execution of model construction function failed.\n");
    }


    output->verbose(CALL_INFO, 1, 0, "Construction of config graph with Python is complete.\n");

    if(NULL != PyErr_Occurred()) {
        PyErr_Print();
	output->fatal(CALL_INFO, -1, "Error occured handling the creation of the component graph in Python.\n");
    }

    return graph;
}


void SSTPythonModelDefinition::pushNamePrefix(const char *name)
{
    if ( NULL == namePrefix ) {
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

#endif
