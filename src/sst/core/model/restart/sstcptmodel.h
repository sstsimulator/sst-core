// -*- c++ -*-

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

#ifndef SST_CORE_MODEL_RESTART_SSTCPTMODEL_H
#define SST_CORE_MODEL_RESTART_SSTCPTMODEL_H

#include "sst/core/config.h"
#include "sst/core/configGraph.h"
#include "sst/core/model/sstmodel.h"
#include "sst/core/output.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

// #include "nlohmann/json.hpp"

#include <string>

// using json = nlohmann::json;

namespace SST::Core {

class SSTCPTModelDefinition : public SSTModelDescription
{
public:
    SST_ELI_REGISTER_MODEL_DESCRIPTION(
          SST::Core::SSTCPTModelDefinition,
          "sst",
          "model.sstcpt",
          SST_ELI_ELEMENT_VERSION(1,0,0),
          "JSON model for building SST simulation graphs",
          false)

    SST_ELI_DOCUMENT_MODEL_SUPPORTED_EXTENSIONS(".sstcpt")

    SSTCPTModelDefinition(const std::string& script_file, int verbosity, Config* config, double start_time);
    ~SSTCPTModelDefinition();

    ConfigGraph* createConfigGraph() override;

protected:
    std::string  manifest_;
    Output*      output_     = nullptr;
    Config*      config_     = nullptr;
    ConfigGraph* graph_      = nullptr;
    double       start_time_ = 0.0;
};

} // namespace SST::Core

#endif // SST_CORE_MODEL_RESTART_SSTCPTMODEL_H
