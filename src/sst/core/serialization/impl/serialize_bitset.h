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

namespace SST::Core::Serialization {

// Serialize std::bitset<N>
template <size_t N>
class serialize_impl<std::bitset<N>>
{
    void operator()(std::bitset<N>& t, serializer& ser, ser_opt_t UNUSED(options))
    {
        switch ( ser.mode() ) {
        case serializer::MAP:
        {
            ser.mapper().map_hierarchy_start(ser.getMapName(), new ObjectMapContainer<std::bitset<N>>(&t));

            // Serialize reference wrappers to each bit
            for ( size_t i = 0; i < N; ++i )
                SST_SER_NAME(pvt::bit_reference_wrapper<std::bitset<N>>(t[i]), std::to_string(i).c_str());

            ser.mapper().map_hierarchy_end();
            break;
        }

        default:
            ser.primitive(t);
            break;
        }
    }
    SST_FRIEND_SERIALIZE();
};

// Serialize std::bitset<N>*
template <size_t N>
class serialize_impl<std::bitset<N>*>
{
    void operator()(std::bitset<N>*& t, serializer& ser, ser_opt_t UNUSED(options))
    {
        if ( ser.mode() == serializer::UNPACK ) t = new std::bitset<N>;
        SST_SER(*t);
    }
    SST_FRIEND_SERIALIZE();
};

// Serialize a std::bitset<N> bit using the pvt::bit_reference_wrapper<std::bitset<N>>
// This is only used in mapping mode
template <size_t N>
class serialize_impl<pvt::bit_reference_wrapper<std::bitset<N>>>
{
    void operator()(pvt::bit_reference_wrapper<std::bitset<N>>& t, serializer& ser, ser_opt_t UNUSED(options))
    {
        ser.mapper().map_hierarchy_start(
            ser.getMapName(), new ObjectMapFundamentalReference<bool, typename std::bitset<N>::reference>(t.ref));
        ser.mapper().map_hierarchy_end();
    }
    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_BITSET_H
