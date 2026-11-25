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

#ifndef SST_CORE_OBJECTSERIALIZATION_H
#define SST_CORE_OBJECTSERIALIZATION_H

#include "sst/core/serialization/chunked_buffer.h"
#include "sst/core/serialization/serializer.h"

#include <vector>

namespace SST::Comms {

template <typename dataType>
std::vector<char>
serialize(dataType& data)
{
    SST::Core::Serialization::serializer ser;

    ser.start_sizing();
    SST_SER(data);

    size_t size = ser.size();

    std::vector<char> buffer(size);

    ser.start_packing(buffer.data(), size);
    SST_SER(data);

    return buffer;
}

// Serialize data into a chunked_buffer, without a sizing pass
template <typename dataType>
void
serialize(dataType& data, SST::Core::Serialization::chunked_buffer& buffer)
{
    SST::Core::Serialization::serializer ser;

    ser.start_packing(buffer);
    SST_SER(data);
}


template <typename dataType>
dataType*
deserialize(std::vector<char>& buffer)
{
    dataType* tgt = nullptr;

    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer.data(), buffer.size());
    SST_SER(tgt);

    return tgt;
}

template <typename dataType>
void
deserialize(std::vector<char>& buffer, dataType& tgt)
{
    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer.data(), buffer.size());
    SST_SER(tgt);
}

template <typename dataType>
void
deserialize(const SST::Core::Serialization::chunked_buffer& buffer, dataType& tgt)
{
    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer);
    SST_SER(tgt);
}

template <typename dataType>
void
deserialize(char* buffer, int blen, dataType& tgt)
{
    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer, blen);
    SST_SER(tgt);
}

} // namespace SST::Comms

#endif // SST_CORE_OBJECTSERIALIZATION_H
