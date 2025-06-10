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

#include <stdexcept>

namespace SST::Core::Serialization {

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
