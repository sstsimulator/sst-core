// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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

namespace SST {
namespace Core {

class PythonConfigGraphOutput : public ConfigGraphOutput {
public:
        PythonConfigGraphOutput(const char* path);

        virtual void generate(const Config* cfg,
		ConfigGraph* graph) throw(ConfigGraphOutputException);

protected:
	char* makePythonSafeWithPrefix(const std::string name, const std::string prefix) const;
	void makeBufferPythonSafe(char* buffer) const;
	char* makeEscapeSafe(const std::string input) const;
	bool strncmp(const char* a, const char* b, const size_t n) const;
	bool isMultiLine(const char* check) const;
	bool isMultiLine(const std::string check) const;
};

}
}

#endif
