// -*- c++ -*-

// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/sst_config.h>
#ifdef HAVE_PYTHON
#include <Python.h>

#include <string.h>
#include <sstream>

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


static int linkInit(LinkPy_t *self, PyObject *args, PyObject *kwds);
static void linkDealloc(LinkPy_t *self);
static PyObject* linkConnect(PyObject* self, PyObject *args);


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
        compSetRank, METH_O,
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
    if ( !PyArg_ParseTuple(args, "ss", &name, &type) )
        return -1;

    self->name = gModel->addNamePrefix(name);
    self->id = gModel->addComponent(self->name, type);
	gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating component [%s] of type [%s]: id [%lu]\n", name, type, self->id);

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


static PyObject* compSetRank(PyObject *self, PyObject *arg)
{
    ComponentId_t id = ((ComponentPy_t*)self)->id;

    PyErr_Clear();
    long rank = PyInt_AsLong(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    gModel->setComponentRank(id, rank);

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

    PyObject *plink;
    char *port, *lat;

    if ( !PyArg_ParseTuple(args, "O!ss", &LinkType, &plink, &port, &lat) )
        return NULL;

    LinkPy_t* link = (LinkPy_t*)plink;

	gModel->getOutput()->verbose(CALL_INFO, 4, 0, "Connecting component %lu to Link %s\n", id, link->name);
    gModel->addLink(id, link->name, port, lat);

    return PyInt_FromLong(0);
}


static PyObject* compGetFullName(PyObject *self, PyObject *args)
{
    return PyString_FromString(((ComponentPy_t*)self)->name);
}



static int linkInit(LinkPy_t *self, PyObject *args, PyObject *kwds)
{
    char *name;
    if ( !PyArg_ParseTuple(args, "s", &name) ) return -1;

    self->name = gModel->addNamePrefix(name);
	gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating Link %s\n", self->name);

    return 0;
}

static void linkDealloc(LinkPy_t *self)
{
    if ( self->name ) free(self->name);
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
    char *lat0, *lat1;

    if ( !PyArg_ParseTuple(t0, "O!ss",
                &ComponentType, &c0, &port0, &lat0) )
        return NULL;
    if ( !PyArg_ParseTuple(t1, "O!ss",
                &ComponentType, &c1, &port1, &lat1) )
        return NULL;

    gModel->addLink(((ComponentPy_t*)c0)->id,
            ((LinkPy_t*)self)->name,
            port0, lat0);
    gModel->addLink(((ComponentPy_t*)c1)->id,
            ((LinkPy_t*)self)->name,
            port1, lat1);

	gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Connecting components %lu and %lu to Link %s\n",
			((ComponentPy_t*)c0)->id, ((ComponentPy_t*)c1)->id, ((LinkPy_t*)self)->name);

    return PyInt_FromLong(0);
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
        if ( Simulation::getSimulation()->getFactory()->hasLibrary(modName) ) {
            genPythonModuleFunction func = Simulation::getSimulation()->getFactory()->getPythonModule(modName);
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
    genPythonModuleFunction func = Simulation::getSimulation()->getFactory()->getPythonModule(modName);
    PyObject* mod = NULL;
    if ( !func ) {
        mod = Py_InitModule(name, emptyModMethods);
    } else {
        mod = static_cast<PyObject*>((*func)());
    }

    return mod;
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
    PyDict_SetItem(dict, PyString_FromString("stop-at"), PyString_FromString(cfg->stopAtCycle.c_str()));
    PyDict_SetItem(dict, PyString_FromString("heartbeat-period"), PyString_FromString(cfg->heartbeatPeriod.c_str()));
    PyDict_SetItem(dict, PyString_FromString("timebase"), PyString_FromString(cfg->timeBase.c_str()));
    PyDict_SetItem(dict, PyString_FromString("partitioner"), PyString_FromString(cfg->partitioner.c_str()));
    PyDict_SetItem(dict, PyString_FromString("verbose"), PyLong_FromLong(cfg->verbose));
    PyDict_SetItem(dict, PyString_FromString("output-partition"), PyString_FromString(cfg->dump_component_graph_file.c_str()));
    PyDict_SetItem(dict, PyString_FromString("output-config"), PyString_FromString(cfg->dump_config_graph.c_str()));
    PyDict_SetItem(dict, PyString_FromString("output-dot"), PyString_FromString(cfg->output_dot.c_str()));
	PyDict_SetItem(dict, PyString_FromString("numRanks"), PyLong_FromLong(cfg->getNumRanks()));

    const char *runModeStr = "UNKNOWN";
    switch (cfg->runMode) {
        case Config::INIT: runModeStr = "init"; break;
        case Config::RUN: runModeStr = "run"; break;
        case Config::BOTH: runModeStr = "both"; break;
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

    Py_INCREF(&ComponentType);
    Py_INCREF(&LinkType);
    Py_INCREF(&ModuleLoaderType);
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
				argv_vector.push_back(temp);
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
			argv_vector.push_back(temp);
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
        output->fatal(CALL_INFO, -1,
                "Unable to open python script %s", scriptName.c_str());
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


    output->verbose(CALL_INFO, 1, 0, "Configuration will now parse parameters.\n");
    config->parseConfigFile(getConfigString());

    output->verbose(CALL_INFO, 1, 0, "Construction of config graph with Python is complete.\n");

    return graph;
}


std::string SSTPythonModelDefinition::getConfigString() const
{
    std::stringstream ss;
    for ( std::map<std::string, std::string>::const_iterator i = cfgParams.begin(); i != cfgParams.end() ; ++i ) {
        ss << i->first << "=" << i->second << "\n";
    }
    return ss.str();
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
