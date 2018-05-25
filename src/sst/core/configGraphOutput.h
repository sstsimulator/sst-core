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

#ifndef _H_SST_CORE_CONFIG_OUTPUT
#define _H_SST_CORE_CONFIG_OUTPUT

#include <configGraph.h>

#include <exception>
#include <cstring>
#include <cstdio>

namespace SST {
class ConfigGraph;

namespace Core {

class ConfigGraphOutputException : public std::exception {
public:
	ConfigGraphOutputException(const char* msg) {
		exMsg = (char*) malloc( sizeof(char) * (strlen(msg) + 1) );
		std::strcpy(exMsg, msg);
	}

	virtual const char* what() const throw() override {
		return exMsg;
	}

protected:
	char* exMsg;
};

class ConfigGraphOutput {
public:
	ConfigGraphOutput(const char* path) {
		outputFile = fopen(path, "wt");
	}

	virtual ~ConfigGraphOutput() {
		fclose(outputFile);
	}

	virtual void generate(const Config* cfg,
		ConfigGraph* graph) throw(ConfigGraphOutputException) = 0;
protected:
	FILE* outputFile;

};

}
}

#endif
