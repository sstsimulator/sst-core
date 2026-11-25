// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/serialization/serializer.h"

#include <stdexcept>

namespace SST::Core::Serialization {

pvt::ser_pack_tracker&
serializer::packer()
{
    if ( ser_.index() == PACK_GROWABLE ) return std::get<PACK_GROWABLE>(ser_);
    return std::get<PACK>(ser_);
}

pvt::ser_unpack_tracker&
serializer::unpacker()
{
    if ( ser_.index() == UNPACK_GROWABLE ) return std::get<UNPACK_GROWABLE>(ser_);
    return std::get<UNPACK>(ser_);
}

void
serializer::raw(void* data, size_t size)
{
    switch ( mode() ) {
    case SIZER:
        active<SIZER>().add(size);
        break;
    case PACK:
        if ( ser_.index() == PACK_GROWABLE )
            active<PACK_GROWABLE>().write(data, size);
        else
            active<PACK>().write(data, size);
        break;
    case UNPACK:
        if ( ser_.index() == UNPACK_GROWABLE )
            active<UNPACK_GROWABLE>().read(data, size);
        else
            active<UNPACK>().read(data, size);
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
        return active<SIZER>().size();
    case PACK:
        return ser_.index() == PACK_GROWABLE ? active<PACK_GROWABLE>().size() : active<PACK>().size();
    case UNPACK:
        return ser_.index() == UNPACK_GROWABLE ? active<UNPACK_GROWABLE>().size() : active<UNPACK>().size();
    default:
        return 0;
    }
}

void
serializer::string(std::string& str)
{
    switch ( mode() ) {
    case SIZER:
        active<SIZER>().size_string(str);
        break;
    case PACK:
        if ( ser_.index() == PACK_GROWABLE )
            active<PACK_GROWABLE>().pack_string(str);
        else
            active<PACK>().pack_string(str);
        break;
    case UNPACK:
        if ( ser_.index() == UNPACK_GROWABLE )
            active<UNPACK_GROWABLE>().unpack_string(str);
        else
            active<UNPACK>().unpack_string(str);
        break;
    case MAP:
        break;
    }
}

void
serializer::modeMismatch()
{
    throw std::logic_error("Internal error: serializer mode does not match its active tracking object");
}

const char*
serializer::getMapName() const
{
    if ( !mapContext ) throw std::invalid_argument("Internal error: Empty map name when map serialization requires it");
    return mapContext->getName();
}

} // namespace SST::Core::Serialization
