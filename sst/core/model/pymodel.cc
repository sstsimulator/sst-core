
#include <sst/core/model/pymodel.h>

#ifdef HAVE_PYTHON

ConfigGraph* current_graph;
ComponentId_t current_component;
char* timeBaseString = NULL;
char* stopAtString = NULL;
bool isVerbose;

extern "C" {

static PyObject* isSimulationVerbose(PyObject* self, PyObject* args) {
	if(isVerbose) {
		return Py_True;
	} else {
		return Py_False;
	}
}

static PyObject* setTimeBase(PyObject* self, PyObject* args) {
	char* tmp_timebase;
	int timebase_size;

	int ok = PyArg_ParseTuple(args, "s#", &tmp_timebase, &timebase_size);

	if(ok) {
		timeBaseString = (char*) malloc(sizeof(char) * (timebase_size + 1));
		strcpy(timeBaseString, tmp_timebase);
		return PyInt_FromLong(0);
	} else {
		return PyInt_FromLong(1);
	}
}

static PyObject* setStopAt(PyObject* self, PyObject* args) {
	char* tmp_stopat;
	int stopat_size;

	int ok = PyArg_ParseTuple(args, "s#", &tmp_stopat, &stopat_size);

	if(ok) {
		stopAtString = (char*) malloc(sizeof(char) * (stopat_size + 1));
		strcpy(stopAtString, tmp_stopat);
		return PyInt_FromLong(0);
	} else {
		return PyInt_FromLong(1);
	}
}

static PyObject* exitsst(PyObject* self, PyObject* args) {
	std::cerr << "Exit called from SST Python model" << std::endl;
	exit(-1);
	return NULL;
}

static PyObject* createNewGraph(PyObject* self, PyObject* args) {
	current_graph = new ConfigGraph();
	return PyInt_FromLong(0);
}

static PyObject* createNewComponent(PyObject* self, PyObject* args) {
	if(current_graph == NULL) {
		return PyLong_FromUnsignedLong((unsigned long) 0);
	}

	char* comp_name;
	char* comp_type;

	int comp_name_size = 0;
	int comp_type_size = 0;

	int ok = PyArg_ParseTuple(args, "s#s#", &comp_name, &comp_name_size,
			&comp_type, &comp_type_size);

	if(ok) {
		string comp_name_str = comp_name;
		string comp_type_str = comp_type;

		ComponentId_t newCompID = current_graph->addComponent(comp_name_str, comp_type_str);
		current_graph->setComponentWeight((ComponentId_t) newCompID, 1.0);
		
		return PyLong_FromUnsignedLong((unsigned long) newCompID);
	} else {
		return PyLong_FromUnsignedLong((unsigned long) 0);
	}
}

static PyObject* setComponentRank(PyObject* self, PyObject* args) {
	if(current_graph == NULL) {
		return PyInt_FromLong(1);
	}

	int comp_rank;
	unsigned long compID;
	int ok = PyArg_ParseTuple(args, "ki", &compID, &comp_rank);

	if(ok) {
		current_graph->setComponentRank((ComponentId_t) compID, comp_rank);
		return PyInt_FromLong(0);
	} else {
		return PyInt_FromLong(1);
	}
}

static PyObject* setComponentWeight(PyObject* self, PyObject* args) {
	if(current_graph == NULL) {
		return PyInt_FromLong(1);
	}

	float comp_weight;
	unsigned long compID;
	int ok = PyArg_ParseTuple(args, "kf", &compID, &comp_weight);

	if(ok) {
		current_graph->setComponentWeight((ComponentId_t) compID, comp_weight);
		return PyInt_FromLong(0);
	} else {
		return PyInt_FromLong(1);
	}
}

static PyObject* addParameter(PyObject*self, PyObject* args) {
	if(current_graph == NULL) {
		return PyInt_FromLong(1);
	}

	char* param_key;
	char* param_value;
	int param_key_size = 0;
	int param_value_size = 0;
	unsigned long compID;

	int ok = PyArg_ParseTuple(args, "ks#s#",
			&compID,
			&param_key, &param_key_size,
			&param_value, &param_value_size);

	if(ok) {
		string param_key_str = param_key;
		string param_val_str = param_value;

		current_graph->addParameter((ComponentId_t) compID, param_key_str, param_val_str, true);
		return PyInt_FromLong(0);
	} else {
		return PyInt_FromLong(1);
	}
}

static PyObject* addLink(PyObject* self, PyObject* args) {
	if(current_graph == NULL) {
		return PyInt_FromLong(1);
	}

	char* link_name;
	char* port_name;
	char* link_lat;
	int link_name_size;
	int port_name_size;
	int link_lat_size;
	unsigned long compID;

	int ok = PyArg_ParseTuple(args, "ks#s#s#",
			&compID,
			&link_name, &link_name_size,
			&port_name, &port_name_size,
			&link_lat, &link_lat_size);

	if(ok) {
		string link_name_str = link_name;
		string port_name_str = port_name;
		string link_lat_str  = link_lat;

		current_graph->addLink((ComponentId_t) compID, link_name_str,
			port_name_str, link_lat_str);
		return PyInt_FromLong(0);
	} else {
		return PyInt_FromLong(1);
	}
}

static PyMethodDef sstPyMethods[] = {
	{ "creategraph", createNewGraph, METH_NOARGS, "Creates a new configGraph in SST." },
	{ "createcomponent", createNewComponent, METH_VARARGS, "Creates a new component in the configGraph." },
	{ "setcomprank", setComponentRank, METH_VARARGS, "Sets the rank of the current component." },
	{ "setcompweight", setComponentWeight, METH_VARARGS, "Sets the weight of the current component." },
	{ "addcompparam", addParameter, METH_VARARGS, "Adds a parameter value pair to the current component" },
	{ "addcomplink", addLink, METH_VARARGS, "Adds a link to the current component" },
	{ "setsimstopat", setStopAt, METH_O, "Sets when simulation should stop." },
	{ "setsimtimebase", setTimeBase, METH_O, "Sets the simulation timebase" },
	{ "verbose", isSimulationVerbose, METH_NOARGS, "Checks whether simulation is verbose" },
	{ "exit", exitsst, METH_NOARGS, "Exits SST - indicates the script wanted to exit." },
	{ NULL, NULL, 0, NULL }
};

}

SSTPythonModelDefinition::SSTPythonModelDefinition(const string script_file, int verbosity,
	Config* configObj) :
	SSTModelDescription() {

	output = new Output("SSTPythonModel ", verbosity, 0, SST::Output::STDOUT);
	verboseLevel = verbosity;
	config = configObj;

	// See if there is an existing PYTHONPATH the user is providing
	const char* existing_pypath = getenv("PYTHONPATH");
	if(NULL == existing_pypath) {
		existing_pypath = "";
	}

	char* current_directory = (char*) malloc(sizeof(char) * PATH_MAX);
	getcwd(current_directory, PATH_MAX);

	char* new_pypath = (char*) malloc(sizeof(char) * (strlen(existing_pypath) +
		strlen(current_directory) + 2));
	sprintf(new_pypath, "%s:%s", existing_pypath, current_directory);
	// Override the existing python path with the current directory
	setenv("PYTHONPATH", new_pypath, 1);

	string local_script_name;
	int substr_index = 0;

	for(substr_index = script_file.size() - 1; substr_index >= 0; --substr_index) {
		if(script_file.at(substr_index) == '/') {
			substr_index++;
			break;
		}
	}

	const string file_name_only = script_file.substr(max(0, substr_index));
	local_script_name = file_name_only.substr(0, file_name_only.size() - 3);

	output->verbose(CALL_INFO, 2, 0, "SST loading a Python model from script: %s / [%s]\n",
		script_file.c_str(), local_script_name.c_str());

	// Get the Python scripting engine started
	Py_Initialize();

	// Add our built in methods to the Python engine
	Py_InitModule("sst", sstPyMethods);

	// Load the Python script file into the engine
	PyObject* scriptName = PyString_FromString(local_script_name.c_str());
	PyObject* scriptModule = PyImport_Import(scriptName);

	// If we can't create the engine then we will have to fatal-stop.
	if(scriptModule == NULL) {
		PyErr_Print();
		output->fatal(CALL_INFO, -1, 
			"Unable to create a Python script engine in SST configGraph construction.\n");
	}

	// Request we get a reference to the sstcreatemodel function (which we will run)
	const char* run_function = "sstcreatemodel";
	PyObject * scriptDict = PyModule_GetDict(scriptModule);
	modelCreateFunc = PyDict_GetItemString(scriptDict, run_function);

	if( (NULL != modelCreateFunc) && (PyCallable_Check(modelCreateFunc)) ) {
		output->debug(CALL_INFO, 1, 0, "Python module loaded and SST model function detected, ready to be run.\n");
	} else {
		PyErr_Print();
		output->fatal(CALL_INFO, -1, 
			"Unable to find an executable function called sstcreatemodel in Python script: %s\n",
			script_file.c_str());
	}

	// Decrement counter on the script name, should be freed.
	Py_DECREF(scriptName);
}

SSTPythonModelDefinition::~SSTPythonModelDefinition() {
	delete output;

	// Shut Python engine down, this consumes a fair amount of resources
	// according to some guides so we may need to do this earlier (after
	// model generation or be quick to free the model definition.
	Py_Finalize();
}

std::string SSTPythonModelDefinition::getTimeBaseString() {
	string timebase_str = timeBaseString;
	return timebase_str;
}

std::string SSTPythonModelDefinition::getStopAtString() {
	string stop_str = stopAtString;
	return stop_str;
}

ConfigGraph* SSTPythonModelDefinition::createConfigGraph() {
	output->verbose(CALL_INFO, 1, 0, "Creating config graph for SST using Python model...\n");

	isVerbose = config->verbose;

	PyObject* args = PyTuple_New(0);
	PyObject* createReturn = PyObject_CallObject(modelCreateFunc, args);

	if(NULL != PyErr_Occurred()) {
		// Print the Python error and then let SST exit as a fatal-stop.
		PyErr_Print();
		output->fatal(CALL_INFO, -1, 
			"Error occurred executing the Python SST model script.\n");
	}

	if(NULL == createReturn) {
		output->fatal(CALL_INFO, -1, 
			"Execution of model construction function failed.\n");
	} else if(0 != PyInt_AsLong(createReturn)) {
		output->fatal(CALL_INFO, -1, 
			"Model construction script reported a non-zero return code, this indicates a script error.\n");
	}

	// Free up the arguments to be de-alloc'd
	Py_DECREF(args);
	Py_DECREF(createReturn);

	output->verbose(CALL_INFO, 1, 0, "Construction of config graph with Python is complete.\n");

	if(NULL != stopAtString) {
		std::string stopat_string = stopAtString;
		config->setStopAt(stopat_string);
	}

	if(NULL != timeBaseString) {
		std::string timebase_string = timeBaseString;
		config->setTimeBase(timebase_string);
	}

	return current_graph;
}

#endif
