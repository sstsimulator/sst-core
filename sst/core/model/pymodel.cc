// -*- c++ -*-

// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/sst_config.h>

#include <sst/core/model/pymodel.h>

#include <sst/core/component.h>

#ifdef HAVE_PYTHON

static SSTPythonModelDefinition *gModel = NULL;

using namespace SST;

extern "C" {



typedef struct {
    PyObject_HEAD
    ComponentId_t id;
} ComponentPy_t;


typedef struct {
    PyObject_HEAD
    char *name;
} LinkPy_t;


static int compInit(ComponentPy_t *self, PyObject *args, PyObject *kwds);
static PyObject* compAddParam(PyObject *self, PyObject *args);
static PyObject* compAddParams(PyObject *self, PyObject *args);
static PyObject* compSetRank(PyObject *self, PyObject *arg);
static PyObject* compSetWeight(PyObject *self, PyObject *arg);
static PyObject* compAddLink(PyObject *self, PyObject *args);


static int linkInit(LinkPy_t *self, PyObject *args, PyObject *kwds);
static PyObject* linkConnect(PyObject* self, PyObject *args);


static PyMethodDef componentMethods[] = {
    {   "addParam",
        compAddParam, METH_VARARGS,
        "Adds a parameter(name, value)"},
    {   "addParams",
        compAddParams, METH_O,
        "Adds Multiple Parameters from a dict"},
    {   "setRank",
        compSetRank, METH_O,
        "Sets which rank on which this component should sit"},
    {   "setWeight",
        compSetWeight, METH_O,
        "Sets the weight of the component"},
    {   "addLink",
        compAddLink, METH_VARARGS,
        "Connects this component to a Link"},
    {   NULL, NULL, 0, NULL }
};


static PyTypeObject ComponentType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.Component",           /* tp_name */
    sizeof(ComponentPy_t),     /* tp_basicsize */
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
    {   NULL, NULL, 0, NULL }
};


static PyTypeObject LinkType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.Link",                /* tp_name */
    sizeof(LinkPy_t),          /* tp_basicsize */
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



static int compInit(ComponentPy_t *self, PyObject *args, PyObject *kwds)
{
    char *name, *type;
    if ( !PyArg_ParseTuple(args, "ss", &name, &type) )
        return -1;

    self->id = gModel->getConfigGraph()->addComponent(name, type);

    return 0;
}


static PyObject* compAddParam(PyObject *self, PyObject *args)
{
    char *param, *value;
    if ( !PyArg_ParseTuple(args, "ss", &param, &value) )
        return NULL;

    ComponentId_t id = ((ComponentPy_t*)self)->id;
    gModel->getConfigGraph()->addParameter(id, param, value, true);

    return PyInt_FromLong(0);
}


static PyObject* compAddParams(PyObject *self, PyObject *args)
{
    ConfigGraph *graph = gModel->getConfigGraph();
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
        graph->addParameter(id, PyString_AsString(kstr), PyString_AsString(vstr));
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return PyInt_FromLong(count);
}


static PyObject* compSetRank(PyObject *self, PyObject *arg)
{
    ConfigGraph *graph = gModel->getConfigGraph();
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyErr_Clear();
    long rank = PyInt_AsLong(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    graph->setComponentRank(id, rank);

    return PyInt_FromLong(0);
}


static PyObject* compSetWeight(PyObject *self, PyObject *arg)
{
    ConfigGraph *graph = gModel->getConfigGraph();
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyErr_Clear();
    double weight = PyFloat_AsDouble(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    graph->setComponentWeight(id, weight);

    return PyInt_FromLong(0);
}


static PyObject* compAddLink(PyObject *self, PyObject *args)
{
    ConfigGraph *graph = gModel->getConfigGraph();
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyObject *plink;
    char *port, *lat;

    if ( !PyArg_ParseTuple(args, "O!ss", &LinkType, &plink, &port, &lat) )
        return NULL;

    LinkPy_t*link = (LinkPy_t*)plink;

    graph->addLink(id, link->name, port, lat);

    return PyInt_FromLong(0);
}




static int linkInit(LinkPy_t *self, PyObject *args, PyObject *kwds)
{
    char *name;
    if ( !PyArg_ParseTuple(args, "s", &name) ) return -1;

    self->name = name;

    return 0;
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
    char *lat0, *lat1;

    if ( !PyArg_ParseTuple(t0, "O!ss",
                &ComponentType, &c0, &port0, &lat0) )
        return NULL;
    if ( !PyArg_ParseTuple(t1, "O!ss",
                &ComponentType, &c1, &port1, &lat1) )
        return NULL;

    ConfigGraph *graph = gModel->getConfigGraph();
    graph->addLink(((ComponentPy_t*)c0)->id,
            ((LinkPy_t*)self)->name,
            port0, lat0);
    graph->addLink(((ComponentPy_t*)c1)->id,
            ((LinkPy_t*)self)->name,
            port1, lat1);


    return PyInt_FromLong(0);
}





/***** Module information *****/


static PyObject* setProgramOption(PyObject* self, PyObject* args)
{
    char *param, *value;
    PyErr_Clear();
    int argOK = PyArg_ParseTuple(args, "ss", &param, &value);
    if ( argOK ) {
        gModel->getParams()[param] = value;
    } else {
        return NULL;
    }
    return PyInt_FromLong(0);
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
        gModel->getParams()[PyString_AsString(key)] = PyString_AsString(val);
        count++;
    }
    return PyInt_FromLong(count);
}


static PyObject* getProgramOptions(PyObject*self, PyObject *args)
{
    // Load parameters already set.
    Config *cfg = gModel->getConfig();
    cfg->parseConfigFile(gModel->getConfigString());
    gModel->getParams().clear(); // No longer need anything set.

    PyObject* dict = PyDict_New();
    PyDict_SetItem(dict, PyString_FromString("debug-file"), PyString_FromString(cfg->debugFile.c_str()));
    PyDict_SetItem(dict, PyString_FromString("lib-path"), PyString_FromString(cfg->libpath.c_str()));
    PyDict_SetItem(dict, PyString_FromString("stop-at"), PyString_FromString(cfg->stopAtCycle.c_str()));
    PyDict_SetItem(dict, PyString_FromString("timebase"), PyString_FromString(cfg->timeBase.c_str()));
    PyDict_SetItem(dict, PyString_FromString("partitioner"), PyString_FromString(cfg->partitioner.c_str()));
    PyDict_SetItem(dict, PyString_FromString("verbose"), PyBool_FromLong(cfg->verbose));

    const char *runModeStr = "UNKNWON";
    switch (cfg->runMode) {
        case Config::INIT: runModeStr = "init"; break;
        case Config::RUN: runModeStr = "run"; break;
        case Config::BOTH: runModeStr = "both"; break;
        default: break;
    }
    PyDict_SetItem(dict, PyString_FromString("run-mode"), PyString_FromString(runModeStr));
    return dict;
}



static PyObject* exitsst(PyObject* self, PyObject* args)
{
    exit(-1);
    return NULL;
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
    {   "exit",
        exitsst, METH_NOARGS,
        "Exits SST - indicates the script wanted to exit." },
    {   NULL, NULL, 0, NULL }
};

}  /* extern C */






SSTPythonModelDefinition::SSTPythonModelDefinition(const std::string script_file, int verbosity,
    Config* configObj, int argc, char **argv) :
    SSTModelDescription(), scriptName(script_file), config(configObj)
{

    output = new Output("SSTPythonModel ", verbosity, 0, SST::Output::STDOUT);

    if ( gModel ) {
        output->fatal(CALL_INFO, -1, 0, 0, "A Python Config Model is already in progress.\n");
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
    if ( ( PyType_Ready(&ComponentType) < 0 ) ||
         ( PyType_Ready(&LinkType) < 0 ) ) {
        output->fatal(CALL_INFO, -1, 0, 0, "Error loading Python types.\n");
    }

    // Add our built in methods to the Python engine
    PyObject *module = Py_InitModule("sst", sstModuleMethods);

    Py_INCREF(&ComponentType);
    PyModule_AddObject(module, "Component", (PyObject*)&ComponentType);
    Py_INCREF(&LinkType);
    PyModule_AddObject(module, "Link", (PyObject*)&LinkType);

}


SSTPythonModelDefinition::~SSTPythonModelDefinition() {
    delete output;
    gModel = NULL;

    // Shut Python engine down, this consumes a fair amount of resources
    // according to some guides so we may need to do this earlier (after
    // model generation or be quick to free the model definition.
    Py_Finalize();
}


ConfigGraph* SSTPythonModelDefinition::createConfigGraph()
{
    output->verbose(CALL_INFO, 1, 0, "Creating config graph for SST using Python model...\n");

    FILE *fp = fopen(scriptName.c_str(), "r");
    if ( !fp ) {
        output->fatal(CALL_INFO, -1, 0, 0,
                "Unable to open python script %s", scriptName.c_str());
    }
    int createReturn = PyRun_AnyFileEx(fp, scriptName.c_str(), 1);

    if(NULL != PyErr_Occurred()) {
        // Print the Python error and then let SST exit as a fatal-stop.
        PyErr_Print();
        output->fatal(CALL_INFO, -1, 0, 0,
            "Error occurred executing the Python SST model script.\n");
    }
    if(-1 == createReturn) {
        output->fatal(CALL_INFO, -1, 0, 0,
            "Execution of model construction function failed.\n");
    }


    output->verbose(CALL_INFO, 1, 0, "Configuration will now parse parameters.\n");
    config->parseConfigFile(getConfigString());

    output->verbose(CALL_INFO, 1, 0, "Construction of config graph with Python is complete.\n");

    return graph;
}


std::string SSTPythonModelDefinition::getConfigString()
{
    std::stringstream ss;
    for ( std::map<std::string, std::string>::iterator i = cfgParams.begin(); i != cfgParams.end() ; ++i ) {
        ss << i->first << "=" << i->second << "\n";
    }
    return ss.str();
}

#endif
