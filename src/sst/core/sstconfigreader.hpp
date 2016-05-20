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

#ifndef _SST_CORE_CONFIG_READER
#define _SST_CORE_CONFIG_READER

#include <string>
#include <map>

namespace SST {
namespace Core {

void configReadLine(FILE* config, char* buffer, const size_t bufferLen);
void populateConfigMapFromFile(FILE* conf_file,
	std::map<std::string, std::string>& confMap);
void populateConfigMap(std::map<std::string, std::string>& confmap);

}
}

#endif
