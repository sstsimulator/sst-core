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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_BITSET_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_BITSET_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_bitset.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <bitset>
#include <cstddef>
#include <type_traits>

namespace SST::Core::Serialization {

// Serialize std::bitset
template <size_t N>
class serialize_impl<std::bitset<N>>
{
    using T = std::bitset<N>;
    void operator()(T& t, serializer& ser, ser_opt_t UNUSED(options))
    {
        switch ( ser.mode() ) {
        case serializer::MAP:
        {
            // TODO: Should this be mapped as an array? It will have the same problems as std::vector<bool>
            break;
        }

        default:
            static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
            ser.primitive(t);
            break;
        }
    }
    SST_FRIEND_SERIALIZE();
};

template <size_t N>
class serialize_impl<std::bitset<N>*>
{
    using T = std::bitset<N>;
    void operator()(T*& t, serializer& ser, ser_opt_t UNUSED(options))
    {
        if ( ser.mode() == serializer::UNPACK ) t = new T {};
        SST_SER(*t);
    }
    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_BITSET_H
