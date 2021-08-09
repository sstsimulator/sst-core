// -*- c++ -*-

// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYTHON_PYMODEL_H
#define SST_CORE_MODEL_PYTHON_PYMODEL_H

// This only works if we have Python defined from configure, otherwise this is
// a compile time error.
//#ifdef SST_CONFIG_HAVE_PYTHON

#include "sst/core/config.h"
#include "sst/core/configGraph.h"
#include "sst/core/model/sstmodel.h"
#include "sst/core/output.h"
#include "sst/core/rankInfo.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <map>
#include <string>
#include <vector>

using namespace SST;

namespace SST {
namespace Core {

class SSTPythonModelDefinition : public SSTModelDescription
{
public:
    SST_ELI_REGISTER_MODEL_DESCRIPTION(
        SST::Core::SSTPythonModelDefinition,
        "sst",
        "model.python",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Python model for building SST simulation graphs",
        true)

    SST_ELI_DOCUMENT_MODEL_SUPPORTED_EXTENSIONS(".py")


    // SSTPythonModelDefinition(
    //     const std::string& script_file, int verbosity, Config* config, double start_time, int argc, char** argv);
    SSTPythonModelDefinition(const std::string& script_file, int verbosity, Config* config, double start_time);
    virtual ~SSTPythonModelDefinition();

    ConfigGraph* createConfigGraph() override;

protected:
    void                initModel(const std::string& script_file, int verbosity, Config* config, int argc, char** argv);
    std::string         scriptName;
    Output*             output;
    Config*             config;
    ConfigGraph*        graph;
    char*               namePrefix;
    size_t              namePrefixLen;
    std::vector<size_t> nameStack;
    std::map<std::string, ComponentId_t> compNameMap;
    ComponentId_t                        nextComponentId;
    double                               start_time;

public: /* Public, but private.  Called only from Python functions */
    Config* getConfig(void) const { return config; }

    ConfigGraph* getGraph(void) const { return graph; }

    Output* getOutput() const { return output; }

    ComponentId_t getNextComponentId() { return nextComponentId++; }

    ComponentId_t addComponent(const char* name, const char* type)
    {
        auto id = graph->addComponent(name, type);
        return id;
    }

    ConfigComponent* findComponentByName(const char* name) const
    {
        return graph->findComponentByName(std::string(name));
    }

    ConfigComponentMap_t& components() { return graph->getComponentMap(); }

    void addLink(ComponentId_t id, const char* link_name, const char* port, const char* latency, bool no_cut) const
    {
        graph->addLink(id, link_name, port, latency, no_cut);
    }
    void setLinkNoCut(const char* link_name) const { graph->setLinkNoCut(link_name); }

    void  pushNamePrefix(const char* name);
    void  popNamePrefix(void);
    char* addNamePrefix(const char* name) const;

    void setStatisticOutput(const char* Name) { graph->setStatisticOutput(Name); }
    void addStatisticOutputParameter(const std::string& param, const std::string& value)
    {
        graph->addStatisticOutputParameter(param, value);
    }
    void setStatisticLoadLevel(uint8_t loadLevel) { graph->setStatisticLoadLevel(loadLevel); }

    void addGlobalParameter(const char* set, const char* key, const char* value, bool overwrite)
    {
        Params::insert_global(set, key, value, overwrite);
    }

    UnitAlgebra getElapsedExecutionTime() const;
    UnitAlgebra getLocalMemoryUsage() const;
};

// For xml inputs (.xml or .sdl), we just use a python script to parse
// the xml.  So, this model definition just uses the python model with
// a few tweaked inputs to the constructor.
class SSTXmlModelDefinition : public SSTModelDescription
{
public:
    SST_ELI_REGISTER_MODEL_DESCRIPTION(
        SST::Core::SSTXmlModelDefinition,
        "sst",
        "model.xml",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "XML model for building SST simulation graphs",
        false)

    SST_ELI_DOCUMENT_MODEL_SUPPORTED_EXTENSIONS(".xml",".sdl")


    SSTXmlModelDefinition(const std::string& script_file, int verbosity, Config* config, double start_time)
    {
        config->model_options = script_file;

        actual_model_
            = new SSTPythonModelDefinition(SST_INSTALL_PREFIX "/libexec/xmlToPython.py", verbosity, config, start_time);
    }

    ConfigGraph* createConfigGraph() override { return actual_model_->createConfigGraph(); }

    virtual ~SSTXmlModelDefinition() {}

private:
    SSTPythonModelDefinition* actual_model_;
};

std::map<std::string, std::string> generateStatisticParameters(PyObject* statParamDict);
SST::Params                        pythonToCppParams(PyObject* statParamDict);
PyObject*                          buildStatisticObject(StatisticId_t id);
PyObject*
buildEnabledStatistic(ConfigComponent* cc, const char* statName, PyObject* statParamDict, bool apply_to_children);
PyObject* buildEnabledStatistics(ConfigComponent* cc, PyObject* statList, PyObject* paramDict, bool apply_to_children);

} // namespace Core
} // namespace SST

#endif // SST_CORE_MODEL_PYTHON_PYMODEL_H
