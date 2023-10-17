// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CONFIGGRAPH_OUTPUT_H
#define SST_CORE_CONFIGGRAPH_OUTPUT_H

#include "sst/core/configGraph.h"
#include "sst/core/params.h"

#include <cstdio>
#include <exception>

namespace SST {
class ConfigGraph;

namespace Core {

class ConfigGraphOutputException : public std::exception
{
public:
    ConfigGraphOutputException(const char* msg)
    {
        exMsg = (char*)malloc(sizeof(char) * (strlen(msg) + 1));
        std::strcpy(exMsg, msg);
    }

    virtual const char* what() const noexcept override { return exMsg; }

protected:
    char* exMsg;
};

class ConfigGraphOutput
{
public:
    ConfigGraphOutput(const char* path) { outputFile = fopen(path, "wt"); }

    virtual ~ConfigGraphOutput() { fclose(outputFile); }

    virtual void generate(const Config* cfg, ConfigGraph* graph) = 0;

protected:
    FILE* outputFile;

    /**
     * Get a named global parameter set.
     *
     * @param name Name of the set to get
     *
     * @return returns a copy of the reqeusted global param set
     *
     */
    static std::map<std::string, std::string> getGlobalParamSet(const std::string& name)
    {
        return Params::getGlobalParamSet(name);
    }


    /**
     * Get a vector of the names of available global parameter sets.
     *
     * @return returns a vector of the names of available global param
     * sets
     *
     */
    static std::vector<std::string> getGlobalParamSetNames() { return Params::getGlobalParamSetNames(); }


    /**
     * Get a vector of the local keys
     *
     * @return returns a vector of the local keys in this Params
     * object
     *
     */
    std::vector<std::string> getParamsLocalKeys(const Params& params) const { return params.getLocalKeys(); }


    /**
     * Get a vector of the global param sets this Params object is
     * subscribed to
     *
     * @return returns a vector of the global param sets his Params
     * object is subscribed to
     *
     */
    std::vector<std::string> getSubscribedGlobalParamSets(const Params& params) const
    {
        return params.getSubscribedGlobalParamSets();
    }
};

} // namespace Core
} // namespace SST

#endif // SST_CORE_CONFIGGRAPH_OUTPUT_H
