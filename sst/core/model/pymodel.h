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


#ifndef SST_CORE_MODEL_PYTHON
#define SST_CORE_MODEL_PYTHON

// This only works if we have Python defined from configure, otherwise this is
// a compile time error.
#ifdef HAVE_PYTHON

#include <map>
#include <string>

#include <sst/core/model/sstmodel.h>
#include <sst/core/config.h>
#include <sst/core/output.h>
#include <Python.h>


using namespace SST;

namespace SST {

class SSTPythonModelDefinition : public SSTModelDescription {

	public:
		SSTPythonModelDefinition(const std::string script_file, int verbosity, Config* config, int argc, char **argv);
		SSTPythonModelDefinition(const std::string script_file, int verbosity, Config* config, const std::string modelParams);
		~SSTPythonModelDefinition();

		ConfigGraph* createConfigGraph();

	protected:
		void initModel(const std::string script_file, int verbosity, Config* config, int argc, char** argv);
		std::string scriptName;
		Output* output;
		Config* config;
		ConfigGraph *graph;
		std::map<std::string, std::string> cfgParams;

	public:  /* Public, but private.  Called only from Python functions */
		Config* getConfig(void) { return config; }
		std::map<std::string, std::string>& getParams(void) { return cfgParams; }
		ConfigGraph* getConfigGraph(void) { return graph; }
		std::string getConfigString(void);

};

}

#endif

#endif
