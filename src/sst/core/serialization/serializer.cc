// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/serialization/serializer.h"

#include "sst/core/output.h"
#include "sst/core/serialization/serializable.h"

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

void
ser_unpacker::unpack_buffer(void* buf, int size)
{
    if ( size == 0 ) {
        Output& output = Output::getDefaultObject();
        output.fatal(__LINE__, __FILE__, "ser_unpacker::unpack_bufffer", 1, "trying to unpack buffer of size 0");
    }
    // void* only for convenience... actually a void**
    void** bufptr = (void**)buf;
    *bufptr       = new char[size];
    char* charstr = next_str(size);
    ::memcpy(*bufptr, charstr, size);
}

void
ser_packer::pack_buffer(void* buf, int size)
{
    if ( buf == nullptr ) {
        Output& output = Output::getDefaultObject();
        output.fatal(__LINE__, __FILE__, "ser_packer::pack_bufffer", 1, "trying to pack nullptr buffer");
    }
    char* charstr = next_str(size);
    ::memcpy(charstr, buf, size);
}

void
ser_unpacker::unpack_string(std::string& str)
{
    int size;
    unpack(size);
    char* charstr = next_str(size);
    str.resize(size);
    str.assign(charstr, size);
}

void
ser_packer::pack_string(std::string& str)
{
    int size = str.size();
    pack(size);
    char* charstr = next_str(size);
    ::memcpy(charstr, str.data(), size);
}

void
ser_sizer::size_string(std::string& str)
{
    size_ += sizeof(int);
    size_ += str.size();
}

} // namespace pvt

void
serializer::string(std::string& str)
{
    switch ( mode_ ) {
    case SIZER:
    {
        sizer_.size_string(str);
        break;
    }
    case PACK:
    {
        packer_.pack_string(str);
        break;
    }
    case UNPACK:
    {
        unpacker_.unpack_string(str);
        break;
    }
    }
}

} // namespace Serialization
} // namespace Core
} // namespace SST
