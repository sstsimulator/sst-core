
#ifndef SST_CORE_MODEL_PYTHON
#define SST_CORE_MODEL_PYTHON

#include <sst/sst_config.h>

// This only works if we have Python defined from configure, otherwise this is a
// compile time error.
#ifdef HAVE_PYTHON

#include <sst/core/model/sstmodel.h>
#include <sst/core/config.h>
#include <sst/core/output.h>
#include <Python.h>

using namespace std;
using namespace SST;

namespace SST {

class SSTPythonModelDefinition : public SSTModelDescription {

	public:
		SSTPythonModelDefinition(const string script_file, int verbosity, Config* config);
		~SSTPythonModelDefinition();

		ConfigGraph* createConfigGraph();
		string getStopAtString();
		string getTimeBaseString();

	protected:
		Output* output;
		PyObject* modelCreateFunc;
		int verboseLevel;
		Config* config;

};

}

#endif

#endif
