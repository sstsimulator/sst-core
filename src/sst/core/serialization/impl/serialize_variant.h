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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VARIANT_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VARIANT_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_variant.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <utility>
#include <variant>

namespace SST::Core::Serialization {

// Serialize std::variant
template <typename... Types>
class serialize_impl<std::variant<Types...>>
{
    void operator()(std::variant<Types...>& obj, serializer& ser, ser_opt_t UNUSED(options))
    {
        size_t index = std::variant_npos;
        switch ( ser.mode() ) {
        case serializer::SIZER:
            index = obj.index();
            ser.size(index);
            break;

        case serializer::PACK:
            index = obj.index();
            ser.pack(index);
            break;

        case serializer::UNPACK:
            ser.unpack(index);

            // Set the active variant to index. The variant must be default-constructible.
            // We cannot portably restore valueless_by_exception but we do nothing in that case.
            if ( index != std::variant_npos ) set_index<std::index_sequence_for<Types...>>[index](obj);
            break;

        case serializer::MAP:
        {
            // TODO -- how to handle mapping of std::variant ?
            return;
        }
        }

        // Serialize the active variant if it's not valueless_by_exception
        if ( index != std::variant_npos ) std::visit([&](auto& x) { SST_SER(x); }, obj);
    }

    // Table of functions to set the active variant
    // Primary definition is std::nullptr_t to cause error if no specialization matches
    template <typename>
    static constexpr std::nullptr_t set_index {};

    SST_FRIEND_SERIALIZE();
};

// Table of functions to set the active variant
// This is defined outside of serialize_impl to work around GCC Bug 71954
template <typename... Types>
template <size_t... INDEX>
constexpr void (*serialize_impl<std::variant<Types...>>::set_index<std::index_sequence<INDEX...>>[])(
    std::variant<Types...>&) { [](std::variant<Types...>& obj) { obj.template emplace<INDEX>(); }... };

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VARIANT_H
