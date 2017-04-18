// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef _H_SST_CORE_CONFIG_OUTPUT_DOT
#define _H_SST_CORE_CONFIG_OUTPUT_DOT

#include <sst/core/configGraph.h>
#include <sst/core/configGraphOutput.h>

namespace SST {
namespace Core {

class DotConfigGraphOutput : public ConfigGraphOutput {
public:
        DotConfigGraphOutput(const char* path);
	virtual void generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException) override;
protected:
	void generateDot(const ConfigComponent& comp, const ConfigLinkMap_t& linkMap) const;
	void generateDot(const ConfigLink& link) const;
};

}
}

#endif
