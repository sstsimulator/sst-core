// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "stringize.h"

#include <cstdarg>
#include <cstdio>

namespace SST {

std::string
vformat_string(size_t max_length, const char* format, va_list args)
{
    char* buf = new char[max_length];

    vsnprintf(buf, max_length, format, args);

    std::string ret(buf);
    delete[] buf;
    return ret;
}

std::string
vformat_string(const char* format, va_list args)
{
    char buf[256];

    va_list args_copy;
    va_copy(args_copy, args);
    size_t length = vsnprintf(buf, 256, format, args_copy);

    if ( length < 256 ) return std::string(buf);

    // Buffer was too small, create a bigger one and try again
    char* large_buf = new char[length + 1];
    vsnprintf(buf, length + 1, format, args);

    std::string ret(buf);
    delete[] large_buf;

    return ret;
}

std::string
format_string(size_t max_length, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    std::string ret = vformat_string(max_length, format, args);
    va_end(args);
    return ret;
}

std::string
format_string(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    std::string ret = vformat_string(format, args);
    va_end(args);
    return ret;
}

} // namespace SST
