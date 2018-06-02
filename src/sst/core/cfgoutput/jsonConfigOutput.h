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

#ifndef _SST_CORE_CONFIG_OUTPUT_JSON
#define _SST_CORE_CONFIG_OUTPUT_JSON

#include <sst/core/configGraph.h>
#include <sst/core/configGraphOutput.h>

namespace SST {
namespace Core {

class JSONConfigGraphOutput : public ConfigGraphOutput {
public:
        JSONConfigGraphOutput(const char* path);
	virtual void generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException) override;
protected:
	void generateJSON(const std::string indent, const ConfigComponent& comp,
                const ConfigLinkMap_t& linkMap) const;
};

}
}

#endif
