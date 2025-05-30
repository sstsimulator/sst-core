// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef SST_CORE_DOT_CONFIG_OUTPUT_H
#define SST_CORE_DOT_CONFIG_OUTPUT_H

#include "sst/core/configGraph.h"
#include "sst/core/configGraphOutput.h"

#include <cstdint>

namespace SST::Core {

class DotConfigGraphOutput : public ConfigGraphOutput
{
public:
    explicit DotConfigGraphOutput(const char* path);
    virtual void generate(const Config* cfg, ConfigGraph* graph) override;

protected:
    void generateDot(const ConfigComponent* comp, const ConfigLinkMap_t& linkMap, const uint32_t dot_verbosity) const;
    void generateDot(const ConfigComponent* comp, const ConfigLinkMap_t& linkMap, const uint32_t dot_verbosity,
        const ConfigComponent* parent) const;
    void generateDot(const ConfigLink* link, const uint32_t dot_verbosity) const;
};

} // namespace SST::Core

#endif // SST_CORE_DOT_CONFIG_OUTPUT_H
