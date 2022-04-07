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

#ifndef SST_CORE_OBJECTSERIALIZATION_H
#define SST_CORE_OBJECTSERIALIZATION_H

#include "sst/core/serialization/serializer.h"

#include <vector>

namespace SST {

namespace Comms {

template <typename dataType>
std::vector<char>
serialize(dataType& data)
{
    SST::Core::Serialization::serializer ser;

    ser.start_sizing();
    ser& data;

    size_t size = ser.size();

    std::vector<char> buffer;
    buffer.resize(size);

    ser.start_packing(buffer.data(), size);
    ser& data;

    return buffer;
}


template <typename dataType>
dataType*
deserialize(std::vector<char>& buffer)
{
    dataType* tgt = nullptr;

    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer.data(), buffer.size());
    ser& tgt;

    return tgt;
}

template <typename dataType>
void
deserialize(std::vector<char>& buffer, dataType& tgt)
{
    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer.data(), buffer.size());
    ser& tgt;
}

template <typename dataType>
void
deserialize(char* buffer, int blen, dataType& tgt)
{
    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer, blen);
    ser& tgt;
}

} // namespace Comms

} // namespace SST

#endif // SST_CORE_OBJECTSERIALIZATION_H
