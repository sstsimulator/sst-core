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
//

#ifndef _H_SST_CORE_CONFIG_OUTPUT_PYTHON
#define _H_SST_CORE_CONFIG_OUTPUT_PYTHON

#include <sst/core/configGraph.h>
#include <sst/core/configGraphOutput.h>
#include <map>

namespace SST {
namespace Core {

class PythonConfigGraphOutput : public ConfigGraphOutput {
public:
    PythonConfigGraphOutput(const char* path);

    virtual void generate(const Config* cfg,
            ConfigGraph* graph) throw(ConfigGraphOutputException) override;

protected:
    ConfigGraph* getGraph() { return graph; }
    void generateParams( const Params &params );
    void generateCommonComponent( const char* objName, const ConfigComponent &comp);
    void generateSubComponent( const char* owner, const ConfigComponent &comp );
    void generateComponent( const ConfigComponent &comp );
    void generateStatGroup(const ConfigGraph* graph, const ConfigStatGroup &grp);

    const std::string& getLinkObject(LinkId_t id);

    char* makePythonSafeWithPrefix(const std::string name, const std::string prefix) const;
    void makeBufferPythonSafe(char* buffer) const;
    char* makeEscapeSafe(const std::string input) const;
    bool strncmp(const char* a, const char* b, const size_t n) const;
    bool isMultiLine(const char* check) const;
    bool isMultiLine(const std::string check) const;

private:
    ConfigGraph *graph;
    std::map<LinkId_t, std::string> linkMap;
};

}
}

#endif
