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

#ifndef SST_CORE_CONFIGGRAPH_OUTPUT_H
#define SST_CORE_CONFIGGRAPH_OUTPUT_H

#include "sst/core/configGraph.h"

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
};

} // namespace Core
} // namespace SST

#endif // SST_CORE_CONFIGGRAPH_OUTPUT_H
