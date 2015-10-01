// -*- c++ -*-

// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_MODEL_PYTHON
#define SST_CORE_MODEL_PYTHON

// This only works if we have Python defined from configure, otherwise this is
// a compile time error.
#ifdef SST_CONFIG_HAVE_PYTHON

#include <map>
#include <string>
#include <sstream>
#include <vector>

#include <sst/core/model/sstmodel.h>
#include <sst/core/config.h>
#include <sst/core/rankInfo.h>
#include <sst/core/output.h>
#include <Python.h>


using namespace SST;

namespace SST {

class SSTPythonModelDefinition : public SSTModelDescription {

	public:
		SSTPythonModelDefinition(const std::string script_file, int verbosity, Config* config, int argc, char **argv);
		SSTPythonModelDefinition(const std::string script_file, int verbosity, Config* config);
		virtual ~SSTPythonModelDefinition();

		ConfigGraph* createConfigGraph();

	protected:
		void initModel(const std::string script_file, int verbosity, Config* config, int argc, char** argv);
		std::string scriptName;
		Output* output;
		Config* config;
		ConfigGraph *graph;
		std::map<std::string, std::string> cfgParams;
        char *namePrefix;
        size_t namePrefixLen;
        std::vector<size_t> nameStack;
    
    public:        
        std::vector<std::string> statParamKeyArray;
        std::vector<std::string> statParamValueArray;

	public:  /* Public, but private.  Called only from Python functions */
		Config* getConfig(void) const { return config; }
		std::map<std::string, std::string>& getParams(void) { return cfgParams; }
		//ConfigGraph* getConfigGraph(void) const { return graph; }
		std::string getConfigString(void) const;
		Output* getOutput() const { return output; }
        ComponentId_t addComponent(const char *name, const char *type) const { return graph->addComponent(name, type); }
        void addParameter(ComponentId_t id, const char *name, const char *value) const { graph->addParameter(id, name, value, true); }

        void setComponentRank(ComponentId_t id, uint32_t rank, uint32_t thread) const { graph->setComponentRank(id, RankInfo(rank, thread)); }
        void setComponentWeight(ComponentId_t id, float weight) const { graph->setComponentWeight(id, weight); }
        void addLink(ComponentId_t id, const char *name, const char *port, const char *latency, bool no_cut) const {graph->addLink(id, name, port, latency, no_cut); }

        void pushNamePrefix(const char *name);
        void popNamePrefix(void);
        char* addNamePrefix(const char *name) const;

        void setStatisticOutput(const char* Name) { graph->setStatisticOutput(Name); }
        void addStatisticOutputParameter(const char* param, const char* value) { graph->addStatisticOutputParameter(param, value); }
        void setStatisticLoadLevel(uint8_t loadLevel) { graph->setStatisticLoadLevel(loadLevel); }

        void enableComponentStatistic(ComponentId_t compid, const char* statname) const { graph->enableComponentStatistic(compid, statname); }
        void enableStatisticForComponentName(const char*  compname, const char*  statname) const { graph->enableStatisticForComponentName(compname, statname); }
        void enableStatisticForComponentType(const char*  comptype, const char*  statname) const  { graph->enableStatisticForComponentType(comptype, statname); }

        void addComponentStatisticParameter(ComponentId_t compid, const char* statname, const char* param, const char* value) { graph->addComponentStatisticParameter(compid, statname, param, value); }
        void addStatisticParameterForComponentName(const char*  compname, const char* statname, const char* param, const char* value) { graph->addStatisticParameterForComponentName(compname, statname, param, value); }
        void addStatisticParameterForComponentType(const char*  comptype, const char* statname, const char* param, const char* value) { graph->addStatisticParameterForComponentType(comptype, statname, param, value); }
};

}

#endif

#endif
