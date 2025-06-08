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

#include "sst_config.h"

#include "sst/core/serialization/serializer.h"

#include "sst/core/output.h"
#include "sst/core/serialization/serializable.h"

#include <cstring>
#include <stdexcept>
#include <string>

namespace SST::Core::Serialization {

namespace pvt {

void
ser_unpacker::unpack_buffer(void* buf, size_t size)
{
    if ( size == 0 ) {
        Output& output = Output::getDefaultObject();
        output.fatal(__LINE__, __FILE__, __func__, 1, "trying to unpack buffer of size 0");
    }
    *static_cast<void**>(buf) = memcpy(new char[size], buf_next(size), size);
}

void
ser_packer::pack_buffer(void* buf, size_t size)
{
    if ( buf == nullptr ) {
        Output& output = Output::getDefaultObject();
        output.fatal(__LINE__, __FILE__, __func__, 1, "trying to pack nullptr buffer");
    }
    memcpy(buf_next(size), buf, size);
}

void
ser_unpacker::unpack_string(std::string& str)
{
    size_t size;
    unpack(size);
    str.resize(size);
    memcpy(str.data(), buf_next(size), size);
}

void
ser_packer::pack_string(std::string& str)
{
    size_t size = str.size();
    pack(size);
    memcpy(buf_next(size), str.data(), size);
}

} // namespace pvt

void
serializer::set_mode(SERIALIZE_MODE mode)
{
    switch ( mode ) {
    case SIZER:
        ser_.emplace<SIZER>();
        break;
    case PACK:
        ser_.emplace<PACK>();
        break;
    case UNPACK:
        ser_.emplace<UNPACK>();
        break;
    case MAP:
        ser_.emplace<MAP>();
        break;
    }
}

void
serializer::reset()
{
    switch ( mode() ) {
    case SIZER:
        sizer().reset();
        break;
    case PACK:
        packer().reset();
        break;
    case UNPACK:
        unpacker().reset();
        break;
    case MAP:
        break;
    }
}

void
serializer::raw(void* data, size_t size)
{
    switch ( mode() ) {
    case SIZER:
        sizer().add(size);
        break;
    case PACK:
        memcpy(packer().buf_next(size), data, size);
        break;
    case UNPACK:
        memcpy(data, unpacker().buf_next(size), size);
        break;
    case MAP:
        break;
    }
}

size_t
serializer::size()
{
    switch ( mode() ) {
    case SIZER:
        return sizer().size();
    case PACK:
        return packer().size();
    case UNPACK:
        return unpacker().size();
    default:
        return 0;
    }
}

void
serializer::string(std::string& str)
{
    switch ( mode() ) {
    case SIZER:
        sizer().size_string(str);
        break;
    case PACK:
        packer().pack_string(str);
        break;
    case UNPACK:
        unpacker().unpack_string(str);
        break;
    case MAP:
        break;
    }
}

const char*
serializer::getMapName() const
{
    if ( !mapContext ) throw std::invalid_argument("Internal error: Empty map name when map serialization requires it");
    return mapContext->getName();
}

} // namespace SST::Core::Serialization
