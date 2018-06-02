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
#include <sst/core/configGraph.h>

using namespace SST;

namespace SST {
namespace Core {

class SSTPythonModelDefinition : public SSTModelDescription {

	public:
		SSTPythonModelDefinition(const std::string script_file, int verbosity, Config* config, int argc, char **argv);
		SSTPythonModelDefinition(const std::string script_file, int verbosity, Config* config);
		virtual ~SSTPythonModelDefinition();

		ConfigGraph* createConfigGraph() override;

	protected:
		void initModel(const std::string script_file, int verbosity, Config* config, int argc, char** argv);
		std::string scriptName;
		Output* output;
		Config* config;
		ConfigGraph *graph;
        char *namePrefix;
        size_t namePrefixLen;
        std::vector<size_t> nameStack;
        std::map<std::string, ComponentId_t> compNameMap;
        ComponentId_t nextComponentId;


	public:  /* Public, but private.  Called only from Python functions */
		Config* getConfig(void) const { return config; }
		ConfigGraph* getGraph(void) const { return graph; }
		Output* getOutput() const { return output; }
        ComponentId_t getNextComponentId() { return nextComponentId++; }
        ComponentId_t addComponent(const char *name, const char *type) {
            ComponentId_t id = getNextComponentId();
            graph->addComponent(id, name, type);
            compNameMap[std::string(name)] = id;
            return id;
        }
        ComponentId_t findComponentByName(const char *name) const {
            auto itr = compNameMap.find(name);
            return ( itr != compNameMap.end() ) ? itr->second : UNSET_COMPONENT_ID;
        }

        void addLink(ComponentId_t id, const char *link_name, const char *port, const char *latency, bool no_cut) const {graph->addLink(id, link_name, port, latency, no_cut); }
        void setLinkNoCut(const char *link_name) const {graph->setLinkNoCut(link_name); }

        void pushNamePrefix(const char *name);
        void popNamePrefix(void);
        char* addNamePrefix(const char *name) const;

        void setStatisticOutput(const char* Name) { graph->setStatisticOutput(Name); }
        void addStatisticOutputParameter(const std::string &param, const std::string &value) { graph->addStatisticOutputParameter(param, value); }
        void setStatisticLoadLevel(uint8_t loadLevel) { graph->setStatisticLoadLevel(loadLevel); }

        void enableStatisticForComponentName(const std::string &compname, const std::string &statname) const { graph->enableStatisticForComponentName(compname, statname); }
        void enableStatisticForComponentType(const std::string &comptype, const std::string &statname) const  { graph->enableStatisticForComponentType(comptype, statname); }

        void addStatisticParameterForComponentName(const std::string &compname, const std::string &statname, const std::string &param, const std::string &value) { graph->addStatisticParameterForComponentName(compname, statname, param, value); }
        void addStatisticParameterForComponentType(const std::string &comptype, const std::string &statname, const std::string &param, const std::string &value) { graph->addStatisticParameterForComponentType(comptype, statname, param, value); }
};

std::map<std::string,std::string> generateStatisticParameters(PyObject* statParamDict);

}
}

#endif

#endif
