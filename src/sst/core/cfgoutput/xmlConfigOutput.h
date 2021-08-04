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
//

#ifndef SST_CORE_XML_CONFIG_OUTPUT_H
#define SST_CORE_XML_CONFIG_OUTPUT_H

#include "sst/core/configGraph.h"
#include "sst/core/configGraphOutput.h"

namespace SST {
namespace Core {

class XMLConfigGraphOutput : public ConfigGraphOutput
{
public:
    XMLConfigGraphOutput(const char* path);
    virtual void generate(const Config* cfg, ConfigGraph* graph) override;

protected:
    void generateXML(const std::string& indent, const ConfigComponent* comp, const ConfigLinkMap_t& linkMap) const;
    void generateXML(const std::string& indent, const ConfigLink& link, const ConfigComponentMap_t& compMap) const;
};

} // namespace Core
} // namespace SST

#endif // SST_CORE_XML_CONFIG_OUTPUT_H
