// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_CORE_STRINGIZE
#define _H_SST_CORE_STRINGIZE

#include <sst_config.h>

#include <string>
#include <cinttypes>

namespace SST {

inline std::string to_string(double val) {
	char buffer[32];
    sprintf(buffer, "%f", val);
    
    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(float val) {
    char buffer[32];
    sprintf(buffer, "%f", val);

    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(int32_t val) {
    char buffer[32];
    sprintf(buffer, "%" PRId32, val);
    
    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(int64_t val) {
    char buffer[32];
    sprintf(buffer, "%" PRId64, val);
    
    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(uint32_t val) {
    char buffer[32];
    sprintf(buffer, "%" PRIu32, val);
    
    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(uint64_t val) {
    char buffer[32];
    sprintf(buffer, "%" PRIu64, val);
    
    std::string buffer_str(buffer);
    return buffer_str;
};

}

#endif
