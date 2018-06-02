// -*- c++ -*-

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include <sst/core/warnmacros.h>
#include <Python.h>


#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include <string.h>
#include <sstream>

#include <sst/core/sst_types.h>
#include <sst/core/model/pymodel.h>
#include <sst/core/model/pymodel_comp.h>
#include <sst/core/model/pymodel_link.h>
#include <sst/core/model/pymodel_statgroup.h>

#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/factory.h>
#include <sst/core/component.h>
#include <sst/core/configGraph.h>
DISABLE_WARN_STRICT_ALIASING

using namespace SST::Core;

SST::Core::SSTPythonModelDefinition *gModel = NULL;

extern "C" {


struct ModuleLoaderPy_t {
    PyObject_HEAD
};


static PyObject* setStatisticOutput(PyObject *self, PyObject *args);
static PyObject* setStatisticLoadLevel(PyObject *self, PyObject *args);

static PyObject* enableAllStatisticsForAllComponents(PyObject *self, PyObject *args);
static PyObject* enableAllStatisticsForComponentName(PyObject *self, PyObject *args);
static PyObject* enableAllStatisticsForComponentType(PyObject *self, PyObject *args);
static PyObject* enableStatisticForComponentName(PyObject *self, PyObject *args);
static PyObject* enableStatisticForComponentType(PyObject *self, PyObject *args);


static PyObject* mlFindModule(PyObject *self, PyObject *args);
static PyObject* mlLoadModule(PyObject *self, PyObject *args);




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
            // genPythonModuleFunction func = Factory::getFactory()->getPythonModule(modName);
            SSTElementPythonModule* pymod = Factory::getFactory()->getPythonModule(modName);
            if ( pymod ) {
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

static PyObject* mlLoadModule(PyObject *UNUSED(self), PyObject *args)
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
    // genPythonModuleFunction func = Factory::getFactory()->getPythonModule(modName);
    SSTElementPythonModule* pymod = Factory::getFactory()->getPythonModule(modName);
    PyObject* mod = NULL;
    if ( !pymod ) {
        mod = Py_InitModule(name, emptyModMethods);
    } else {
        mod = static_cast<PyObject*>(pymod->load());
    }

    return mod;
}



/***** Module information *****/

static PyObject* findComponentByName(PyObject* UNUSED(self), PyObject* args)
{
    if ( ! PyString_Check(args) ) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    char *name = PyString_AsString(args);
    ComponentId_t id = gModel->findComponentByName(name);
    if ( id != UNSET_COMPONENT_ID ) {
        PyObject *argList = Py_BuildValue("ssk", name, "irrelephant", id);
        PyObject *res = PyObject_CallObject((PyObject *) &PyModel_ComponentType, argList);
        Py_DECREF(argList);
        return res;
    }
    Py_INCREF(Py_None);
    return Py_None;
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
        return NULL;
    }
}


static PyObject* setProgramOptions(PyObject* UNUSED(self), PyObject* args)
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


static PyObject* getProgramOptions(PyObject*UNUSED(self), PyObject *UNUSED(args))
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


static PyObject* pushNamePrefix(PyObject* UNUSED(self), PyObject* arg)
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


static PyObject* popNamePrefix(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    gModel->popNamePrefix();
    return PyInt_FromLong(0);
}


static PyObject* exitsst(PyObject* UNUSED(self), PyObject* UNUSED(args))
{
    exit(-1);
    return NULL;
}

static PyObject* getSSTMPIWorldSize(PyObject* UNUSED(self), PyObject* UNUSED(args)) {
    int ranks = 1;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
#endif
    return PyInt_FromLong(ranks);
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
    PyObject*  outputParamDict = NULL;

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
        return NULL;
    }
    return PyInt_FromLong(0);
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
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* setStatisticOutputOptions(PyObject* UNUSED(self), PyObject* args)
{
    PyErr_Clear();

    if ( !PyDict_Check(args) ) {
        return NULL;
    }

    // Generate and Add the Statistic Output Parameters
    for ( auto p : generateStatisticParameters(args) ) {
        gModel->addStatisticOutputParameter(p.first, p.second);
    }
    return PyInt_FromLong(0);
}


static PyObject* setStatisticLoadLevel(PyObject* UNUSED(self), PyObject* arg)
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


static PyObject* enableAllStatisticsForAllComponents(PyObject* UNUSED(self), PyObject* args)
{
    int           argOK = 0;
    PyObject*     statParamDict = NULL;

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!", &PyDict_Type, &statParamDict);

    if (argOK) {
        gModel->enableStatisticForComponentName(STATALLFLAG, STATALLFLAG);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentName(STATALLFLAG, STATALLFLAG, p.first, p.second);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }

    return PyInt_FromLong(0);
}


static PyObject* enableAllStatisticsForComponentName(PyObject *UNUSED(self), PyObject *args)
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
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentName(compName, STATALLFLAG, p.first, p.second);
        }

    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* enableAllStatisticsForComponentType(PyObject *UNUSED(self), PyObject *args)
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
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentType(compType, STATALLFLAG, p.first, p.second);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* enableStatisticForComponentName(PyObject *UNUSED(self), PyObject *args)
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
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentName(compName, statName, p.first, p.second);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return NULL;
    }
    return PyInt_FromLong(0);
}


static PyObject* enableStatisticForComponentType(PyObject *UNUSED(self), PyObject *args)
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
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            gModel->addStatisticParameterForComponentType(compType, statName, p.first, p.second);
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
        "Enables all statistics on all components with output at end of simulation."},
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


void SSTPythonModelDefinition::initModel(const std::string script_file, int verbosity, Config* UNUSED(config), int argc, char** argv)
{
    output = new Output("SSTPythonModel ", verbosity, 0, SST::Output::STDOUT);

    if ( gModel ) {
        output->fatal(CALL_INFO, -1, "A Python Config Model is already in progress.\n");
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

    // Get the Python scripting engine started
    Py_Initialize();

    PySys_SetArgv(argc, argv);

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
        output->fatal(CALL_INFO, -1, "Error loading Python types.\n");
    }

    // Add our built in methods to the Python engine
    PyObject *module = Py_InitModule("sst", sstModuleMethods);

    Py_INCREF(&PyModel_ComponentType);
    Py_INCREF(&PyModel_SubComponentType);
    Py_INCREF(&PyModel_LinkType);
    Py_INCREF(&PyModel_StatGroupType);
    Py_INCREF(&PyModel_StatOutputType);
    Py_INCREF(&ModuleLoaderType);

    PyModule_AddObject(module, "Link", (PyObject*)&PyModel_LinkType);
    PyModule_AddObject(module, "Component", (PyObject*)&PyModel_ComponentType);
    PyModule_AddObject(module, "SubComponent", (PyObject*)&PyModel_SubComponentType);
    PyModule_AddObject(module, "StatisticGroup", (PyObject*)&PyModel_StatGroupType);
    PyModule_AddObject(module, "StatisticOutput", (PyObject*)&PyModel_StatOutputType);


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

/* Utilities */
std::map<std::string,std::string> SST::Core::generateStatisticParameters(PyObject* statParamDict)
{
    PyObject*     pykey = NULL;
    PyObject*     pyval = NULL;
    Py_ssize_t    pos = 0;

    std::map<std::string,std::string> p;

    // If the user did not include a dict for the parameters
    // the variable statParamDict will be NULL.
    if (NULL != statParamDict) {
        // Make sure it really is a Dict
        if (true == PyDict_Check(statParamDict)) {

            // Extract the Key and Value for each parameter and put them into the vectors 
            while ( PyDict_Next(statParamDict, &pos, &pykey, &pyval) ) {
                PyObject* pyparam = PyObject_CallMethod(pykey, (char*)"__str__", NULL);
                PyObject* pyvalue = PyObject_CallMethod(pyval, (char*)"__str__", NULL);

                p[PyString_AsString(pyparam)] = PyString_AsString(pyvalue);

                Py_XDECREF(pyparam);
                Py_XDECREF(pyvalue);
            }
        }
    }
    return p;
}

REENABLE_WARNING
