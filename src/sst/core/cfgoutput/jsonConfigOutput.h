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

#ifndef SST_CORE_JSON_CONFIG_OUTPUT_H
#define SST_CORE_JSON_CONFIG_OUTPUT_H

#include "sst/core/configGraph.h"
#include "sst/core/configGraphOutput.h"

namespace SST::Core {

class JSONConfigGraphOutput : public ConfigGraphOutput
{

public:
    explicit JSONConfigGraphOutput(const char* path);
    virtual void generate(const Config* cfg, ConfigGraph* graph) override;

private:
    std::map<SST::StatisticId_t, std::string> sharedStatMap;
};

} // namespace SST::Core

#endif // SST_CORE_JSON_CONFIG_OUTPUT_H
