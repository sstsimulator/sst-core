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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UNIQUE_PTR_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UNIQUE_PTR_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_unique_ptr.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace SST::Core::Serialization {

namespace pvt {

// std::unique_ptr wrapper class
// Default template has a reference to a std::unique_ptr and a deleter which might be defaulted
template <class PTR_TYPE, class DELETER = std::default_delete<PTR_TYPE>, class SIZE_T = void>
struct unique_ptr_wrapper
{
    std::unique_ptr<PTR_TYPE, DELETER>& ptr;
    DELETER                             del {};
};

// For unbounded array types, a reference to a size parameter is added
template <class ELEM_TYPE, class DELETER, class SIZE_T>
struct unique_ptr_wrapper<ELEM_TYPE[], DELETER, SIZE_T>
{
    std::unique_ptr<ELEM_TYPE[], DELETER>& ptr;
    SIZE_T&                                size;
    DELETER                                del {};
};

} // namespace pvt

// Serialize std::unique_ptr with a default deleter but not an unbounded array type
template <class PTR_TYPE>
class serialize_impl<std::unique_ptr<PTR_TYPE>, std::enable_if_t<!is_unbounded_array_v<PTR_TYPE>>>
{
    void operator()(std::unique_ptr<PTR_TYPE>& ptr, serializer& ser, ser_opt_t opt)
    {
        // Create a wrapper with a default deleter and serialize the wrapper with the generalized code below
        pvt::unique_ptr_wrapper<PTR_TYPE> wrapper { ptr };
        serialize_impl<pvt::unique_ptr_wrapper<PTR_TYPE>>()(wrapper, ser, opt);
    }
    SST_FRIEND_SERIALIZE();
};

// Serialize std::unique_ptr with a wrapper which handles unbounded arrays with a runtime size, and custom deleters
template <class PTR_TYPE, class DELETER, class SIZE_T>
class serialize_impl<pvt::unique_ptr_wrapper<PTR_TYPE, DELETER, SIZE_T>>
{
    // OWNER_TYPE is PTR_TYPE with cv-qualifiers removed so that it can be serialized
    using OWNER_TYPE = std::remove_cv_t<PTR_TYPE>;

    // ELEM_TYPE is the array element type, or OWNER_TYPE if OWNER_TYPE is not an array
    // "pointer" member of std::unique_ptr is used in case DELETER has its own pointer type
    using ELEM_TYPE = std::remove_cv_t<std::remove_pointer_t<typename std::unique_ptr<PTR_TYPE, DELETER>::pointer>>;

    void operator()(pvt::unique_ptr_wrapper<PTR_TYPE, DELETER, SIZE_T>& ptr, serializer& ser, ser_opt_t opt)
    {
        const auto opts = SerOption::is_set(opt, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;
        const auto mode = ser.mode();

        if ( mode == serializer::MAP ) {
            // TODO: Mapping std::unique_ptr
            return;
        }

        // Destroy the old std::unique_ptr
        if ( mode == serializer::UNPACK ) ptr.ptr.~unique_ptr();

        size_t size = 0;
        if constexpr ( is_unbounded_array_v<PTR_TYPE> ) {
            // If PTR_TYPE is an unbounded array and there is a size parameter

            // Get the array size, which is 0 if the pointer is null
            if ( mode != serializer::UNPACK )
                if ( ptr.ptr )
                    size = get_array_size(ptr.size,
                        "Serialization Error: Array size in SST::Core::Serialization::unique_ptr() cannot fit "
                        "inside size_t. size_t should be used for array sizes.\n");

            // Serialize the array size
            ser.primitive(size);
            if ( mode == serializer::UNPACK ) ptr.size = static_cast<SIZE_T>(size);
        }

        // Serialize the address stored in the std::unique_ptr
        uintptr_t ptr_stored = reinterpret_cast<uintptr_t>(ptr.ptr.get());
        ser.primitive(ptr_stored);

        uintptr_t real_ptr      = ptr_stored;
        bool      serialize_obj = false;

        switch ( mode ) {
        case serializer::SIZER:
            serialize_obj = ptr_stored && !ser.sizer().check_pointer_sizer(ptr_stored);
            break;

        case serializer::PACK:
            serialize_obj = ptr_stored && !ser.packer().check_pointer_pack(ptr_stored);
            break;

        case serializer::UNPACK:
            if ( ptr_stored ) {
                // Check if the pointer was seen before
                real_ptr = ser.unpacker().check_pointer_unpack(ptr_stored);

                // If pointer was not seen before, allocate the object, report its address and serialize the object
                if ( !real_ptr ) {
                    if constexpr ( is_unbounded_array_v<PTR_TYPE> )
                        real_ptr = reinterpret_cast<uintptr_t>(new ELEM_TYPE[size]());
                    else if constexpr ( std::is_array_v<PTR_TYPE> )
                        real_ptr = reinterpret_cast<uintptr_t>(new ELEM_TYPE[std::extent_v<PTR_TYPE>]());
                    else
                        real_ptr = reinterpret_cast<uintptr_t>(new OWNER_TYPE());
                    ser.unpacker().report_real_pointer(ptr_stored, real_ptr);
                    serialize_obj = true;
                }
            }
            // Create a std::unique_ptr owning the real pointer (or null pointer if ptr_stored == 0)
            new (&ptr.ptr) std::unique_ptr<PTR_TYPE, DELETER>(
                reinterpret_cast<ELEM_TYPE*>(real_ptr), std::forward<decltype(ptr.del)>(ptr.del));
            break;

        default:
            break;
        }

        // Serialize the object if the pointer is non-null and this is the first time it's been seen
        if ( serialize_obj ) {
            if constexpr ( !is_unbounded_array_v<PTR_TYPE> )
                SST_SER(*reinterpret_cast<OWNER_TYPE*>(real_ptr));
            else if constexpr ( is_trivially_serializable_v<ELEM_TYPE> )
                ser.raw(reinterpret_cast<ELEM_TYPE*>(real_ptr), size * sizeof(ELEM_TYPE));
            else
                pvt::serialize_array(
                    ser, reinterpret_cast<ELEM_TYPE*>(real_ptr), opts, size, pvt::serialize_array_element<ELEM_TYPE>);
        }
    }

    // For the serialize_impl specialization above
    friend serialize_impl<std::unique_ptr<PTR_TYPE>>;

    SST_FRIEND_SERIALIZE();
};

// Wrapper for a std::unique_ptr to an unbounded array with a runtime size
template <class ELEM_TYPE, class DELETER, class SIZE_T>
std::enable_if_t<std::is_integral_v<SIZE_T>, pvt::unique_ptr_wrapper<ELEM_TYPE[], DELETER, SIZE_T>>
unique_ptr(std::unique_ptr<ELEM_TYPE[], DELETER>& ptr, SIZE_T& size)
{
    return { ptr, size };
}

// Wrapper for a std::unique_ptr with a custom deleter but no unbounded array
template <class PTR_TYPE, class DELETER, class DEL>
std::enable_if_t<!is_unbounded_array_v<PTR_TYPE> && std::is_constructible_v<DELETER, DEL&&>,
    pvt::unique_ptr_wrapper<PTR_TYPE, DELETER>>
unique_ptr(std::unique_ptr<PTR_TYPE, DELETER>& ptr, DEL&& del)
{
    return { ptr, std::forward<DEL>(del) };
}

// Wrapper for a std::unique_ptr with a custom deleter and an unbounded array with a runtime size
template <class ELEM_TYPE, class DELETER, class SIZE_T, class DEL>
std::enable_if_t<std::is_integral_v<SIZE_T> && std::is_constructible_v<DELETER, DEL&&>,
    pvt::unique_ptr_wrapper<ELEM_TYPE[], DELETER, SIZE_T>>
unique_ptr(std::unique_ptr<ELEM_TYPE[], DELETER>& ptr, SIZE_T& size, DEL&& del)
{
    return { ptr, size, std::forward<DEL>(del) };
}

// NOP for consistency
template <class PTR_TYPE, class DELETER>
std::unique_ptr<PTR_TYPE, DELETER>&
unique_ptr(std::unique_ptr<PTR_TYPE, DELETER>& ptr)
{
    return ptr;
}

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UNIQUE_PTR_H
