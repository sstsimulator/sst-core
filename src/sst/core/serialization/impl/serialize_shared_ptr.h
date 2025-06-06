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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_SHARED_PTR_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_SHARED_PTR_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_shared_ptr.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/output.h"
#include "sst/core/serialization/impl/serialize_utility.h"
#include "sst/core/serialization/serializer.h"

#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace SST::Core::Serialization {

namespace pvt {

// Unique tag for each std::shared_ptr owner
inline size_t shared_ptr_tag;

// Map of ownership from std::shared_ptr or std::weak_ptr to a numeric tag representing the owner.
// std::owner_less<> compares two std::shared_ptr or std::weak_ptr instances with .owner_before() to form an
// ordering which std:map uses to detect whether two std::shared_ptr or std::weak_ptr pointers have the same owner.
// The keys are of type std::weak_ptr<const void> which any std::shared_ptr or std::weak_ptr can be converted to
// and whose ownership can be compared with any other.
inline std::map<std::weak_ptr<const void>, size_t, std::owner_less<>> shared_ptr_map;

// Reverse map from numeric tag to an ownership std::shared_ptr<PARENT_TYPE>. When std::shared_ptr or std::weak_ptr
// is deserialized, numeric tags are used to represent owners. A std::shared_ptr<PARENT_TYPE> is allocated and
// PARENT_TYPE is deserialized for each new owner tag. The map keeps a copy of std::shared_ptr<PARENT_TYPE> around
// for every deserialization of std::shared_ptr or std::weak_ptr with the same ownership tag.
template <class PARENT_TYPE>
std::map<size_t, std::shared_ptr<PARENT_TYPE>> shared_ptr_reverse_map;

// Wrapper class which references a std::shared_ptr or std::weak_ptr and a std::shared_ptr which manages ownership
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE = PTR_TYPE, class = std::true_type>
struct shared_ptr_wrapper_t
{
    PTR_TEMPLATE<PTR_TYPE>&       ptr;
    std::shared_ptr<PARENT_TYPE>& parent;
};

// Specialization for parent std::shared_ptr to unbounded arrays which have reference to size
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE>
struct shared_ptr_wrapper_t<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE, is_unbounded_array<PARENT_TYPE>>
{
    PTR_TEMPLATE<PTR_TYPE>&       ptr;
    std::shared_ptr<PARENT_TYPE>& parent;
    size_t&                       size;
};

// Make sure that ptr and parent have the same owning control block or are both empty
// ptr.owner_before(parent) is true if ptr's owner is before parent's owner in a partial ordering
// parent.owner_before(ptr) is true if parent's owner is before ptr's owner in a partial ordering
// If neither ptr.owner_before(parent) nor parent.owner_before(ptr) is true, they have the same owner
// If one is empty and the other is non-empty, they will compare as having different owners
// If they are both empty, they will compare as having the same owner
template <class PTR, class PARENT_PTR>
void assert_same_owner(const PTR& ptr, const PARENT_PTR& parent)
{
    if ( ptr.owner_before(parent) || parent.owner_before(ptr) ) {
        SST::Output output { "", 5, SST::Output::PrintAll, SST::Output::STDERR };
        const char* ptr_type = is_same_type_template_v<PTR, std::weak_ptr> ? "weak_ptr" : "shared_ptr";
        output.fatal(__LINE__, __FILE__, __func__, 1,
            "ERROR: Serialized std::%s does not have the same owner as parent as indicated by "
            "SST::Core::Serialization::%s(std::%s<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)\n",
            ptr_type, ptr_type, ptr_type);
    }
}

// Serialize a std::shared_ptr or std::weak_ptr whose ownership is managed by a std::shared_ptr parent. The types
// of the pointees, PTR_TYPE and PARENT_TYPE, do not need to be the same, since a std::shared_ptr or std::weak_ptr
// can point to a sub-object of a managed object. PTR_TEMPLATE<PTR_TYPE> matches either a std::shared_ptr or
// std::weak_ptr being serialized.
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE>
void serialize_shared_ptr_impl(PTR_TEMPLATE<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, serializer& ser,
    ser_opt_t opt, size_t* size = nullptr)
{
    using elem_t           = std::remove_extent_t<PTR_TYPE>;
    using parent_elem_t    = std::remove_extent_t<PARENT_TYPE>;
    size_t    tag          = 0;
    bool      is_first_use = false;
    bool      nonnull      = false;
    ptrdiff_t offset       = 0;

    switch ( ser.mode() ) {
    case serializer::SIZER:
    case serializer::PACK:
    {
        // Make sure that ptr and parent have the same owning control block
        assert_same_owner(ptr, parent);

        // If the use_count() is 0, the std::shared_ptr or std::weak_ptr has no managed object and is empty, so a
        // tag of 0 is serialized
        if ( ptr.use_count() ) {
            // Look for the owner of the shared pointer in the map, creating a new entry if it doesn't exist
            decltype(shared_ptr_map)::iterator it;

#if defined(__GLIBCXX__) && __GLIBCXX__ < 20250425
            // Workaround for libstdc++ bug 120561 converting std::weak_ptr<T[]> to std::weak_ptr<void>
            std::tie(it, is_first_use) = shared_ptr_map.try_emplace(ptr.lock());
#else
            std::tie(it, is_first_use) = shared_ptr_map.try_emplace(ptr);
#endif

            // If this is the first use of the shared pointer owning control block, store a new non-zero tag
            if ( is_first_use ) it->second = ++shared_ptr_tag;

            // Tag for this shared pointer
            tag = it->second;

            // Address stored in the shared pointer
            // If ptr is a std::shared_ptr, it is returned by get()
            // if ptr is a std::weak_ptr, we have to lock() it to get() the stored address
            void* addr;
            if constexpr ( is_same_template_v<PTR_TEMPLATE, std::shared_ptr> )
                addr = ptr.get();
            else
                addr = ptr.lock().get();

            // Whether the stored address is not a nullptr
            nonnull = addr != nullptr;

            // Offset of the pointer's stored address to its parent's stored address
            if ( nonnull ) offset = (char*)addr - (char*)parent.get();
        }

        // Serialize the tag
        SST_SER(tag);

        // If the tag is not zero, serialize whether the pointer is null, the offset, and possibly the parent object
        if ( tag != 0 ) {
            SST_SER(nonnull);
            SST_SER(offset);

            // If this is the first use of the tag of the owning control block, serialize the parent object
            if ( is_first_use ) {
                if constexpr ( is_unbounded_array_v<PARENT_TYPE> ) {
                    // If the parent object is an unbounded array

                    // Serialize the size
                    SST_SER(*size);

                    // Serialize the array elements
                    if constexpr ( std::is_arithmetic_v<parent_elem_t> ||
                                   std::is_enum_v<parent_elem_t> /* is_trivially_serializable_v<parent_elem_t> */ )
                        ser.raw(parent.get(), *size * sizeof(parent_elem_t));
                    else
                        pvt::serialize_array(
                            ser, parent.get(), opt, *size, pvt::serialize_array_element<parent_elem_t>);
                }
                else {
                    // Serialize the parent object
                    // The cast is to make it work when PARENT_TYPE is a bounded array
                    SST_SER(*reinterpret_cast<PARENT_TYPE*>(parent.get()));
                }
            }
        }
        break;
    }

    case serializer::UNPACK:
    {
        // Reset the std::shared_ptr or std::weak_ptr so that it is empty
        ptr.reset();

        // Deserialize the tag
        SST_SER(tag);

        // If the tag is not 0, there is a shared pointer control block
        if ( tag != 0 ) {
            // Deserialize whether the pointer is null and the offset
            SST_SER(nonnull);
            SST_SER(offset);

            // Look for the shared pointer owner tag in the reverse map, creating a new entry if it doesn't exist
            typename decltype(shared_ptr_reverse_map<PARENT_TYPE>)::iterator it;
            std::tie(it, is_first_use) = shared_ptr_reverse_map<PARENT_TYPE>.try_emplace(tag);

            // Owning std::shared_ptr of the control block
            std::shared_ptr<PARENT_TYPE>& owner = it->second;

            // If this is the first use of the shared pointer owning control block, deserialize the parent object
            if ( is_first_use ) {
                // Create a std::shared_ptr for the owning parent object
                if constexpr ( is_unbounded_array_v<PARENT_TYPE> ) {
                    // If the parent type is an unbounded array

                    // Deserialize the size
                    SST_SER(*size);

                    // Allocate a std::shared_ptr to a default-initialized PARENT_TYPE array of size
                    // Prior to C++20, std::make_shared, which is more efficient, does not support arrays
                    if constexpr ( __cplusplus < 202002l )
                        owner = std::shared_ptr<PARENT_TYPE>(new parent_elem_t[*size]);
                    else
                        owner = std::make_shared<PARENT_TYPE>(*size);

                    // Deserialize the array elements
                    if constexpr ( std::is_arithmetic_v<parent_elem_t> ||
                                   std::is_enum_v<parent_elem_t> /* is_trivially_serializable_v<parent_elem_t> */ )
                        ser.raw(owner.get(), *size * sizeof(parent_elem_t));
                    else
                        pvt::serialize_array(ser, owner.get(), opt, *size, pvt::serialize_array_element<parent_elem_t>);
                }
                else {
                    // Prior to C++20, std::make_shared, which is more efficient, does not support arrays
                    if constexpr ( std::is_array_v<PARENT_TYPE> && __cplusplus < 202002l )
                        owner = std::shared_ptr<PARENT_TYPE>(new parent_elem_t[std::extent_v<PARENT_TYPE>]);
                    else
                        owner = std::make_shared<PARENT_TYPE>();

                    // Deserialize the parent object
                    // The cast is to make it work when PARENT_TYPE is an array
                    SST_SER(*reinterpret_cast<PARENT_TYPE*>(owner.get()));
                }

                // Set the parent to the owning std::shared_ptr
                parent = owner;
            }

            // Stored pointer is either null or an offset within the parent object
            elem_t* addr = nullptr;
            if ( nonnull )
                addr = const_cast<elem_t*>(
                    reinterpret_cast<const elem_t*>(reinterpret_cast<const char*>(owner.get()) + offset));

            // Set the std::shared_ptr or std::weak_ptr to the address within the parent std::shared_ptr object
            ptr = std::shared_ptr<PTR_TYPE>(owner, addr);
        }
        break;
    }
    case serializer::MAP:
    {
        // TODO: Mapping std::shared_ptr or std::weak_ptr
        break;
    }
    }
}

} // namespace pvt

// Serialize a std::shared_ptr, assuming that it points to the beginning of the managed object
// For std::shared_ptr to unbounded arrays, the wrapper class must be used
// std::shared_ptr to functions is not supported
template <class PTR_TYPE>
class serialize_impl<std::shared_ptr<PTR_TYPE>,
    std::enable_if_t<!is_unbounded_array_v<PTR_TYPE> && !std::is_function_v<PTR_TYPE>>>
{
    void operator()(std::shared_ptr<PTR_TYPE>& ptr, serializer& ser, ser_opt_t opt)
    {
        ser_opt_t elem_opt = SerOption::is_set(opt, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;

        // The std::shared_ptr is considered its own parent owning the managed storage
        pvt::serialize_shared_ptr_impl(ptr, ptr, ser, elem_opt);
    }
    SST_FRIEND_SERIALIZE();
};

// Serialize a std::weak_ptr, assuming that it points to the beginning of the managed object
// For std::weak_ptr to unbounded arrays, the wrapper class must be used
// std::weak_ptr to functions is not supported
template <class PTR_TYPE>
class serialize_impl<std::weak_ptr<PTR_TYPE>,
    std::enable_if_t<!is_unbounded_array_v<PTR_TYPE> && !std::is_function_v<PTR_TYPE>>>
{
    void operator()(std::weak_ptr<PTR_TYPE>& ptr, serializer& ser, ser_opt_t opt)
    {
        ser_opt_t elem_opt = SerOption::is_set(opt, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;

        // A temporary std::shared_ptr created from std::weak_ptr acts as the parent owning the shared object. This
        // std::shared_ptr will be destroyed at the end of this function, but it will be used for (de)serialization
        // of std::weak_ptr, and in the case of deserialization, a copy of the std::shared_ptr managed object will
        // be stored in a map until it is copied into one or more std::shared_ptr instances which are deserialized.
        // This allows a std::weak_ptr to be (de)serialized before its owning std::shared_ptr is (de)serialized.
        std::shared_ptr<PTR_TYPE> parent = ptr.lock();
        pvt::serialize_shared_ptr_impl(ptr, parent, ser, elem_opt);
    }
    SST_FRIEND_SERIALIZE();
};

// Serialize a std::shared_ptr or std::weak_ptr with another std::shared_ptr object which manages the shared object
// std::shared_ptr or std::weak_ptr to functions is not supported
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE>
class serialize_impl<pvt::shared_ptr_wrapper_t<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE>,
    std::enable_if_t<!std::is_function_v<PTR_TYPE>>>
{
    void operator()(pvt::shared_ptr_wrapper_t<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE>& ptr, serializer& ser, ser_opt_t opt)
    {
        ser_opt_t elem_opt = SerOption::is_set(opt, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;

        // Serialize std::shared_ptr or std::weak_ptr pointer and its std::shared_ptr parent
        // If PARENT_TYPE is an unbounded array, pass size parameter
        if constexpr ( is_unbounded_array_v<PARENT_TYPE> )
            pvt::serialize_shared_ptr_impl(ptr.ptr, ptr.parent, ser, elem_opt, &ptr.size);
        else
            pvt::serialize_shared_ptr_impl(ptr.ptr, ptr.parent, ser, elem_opt);
    }
    SST_FRIEND_SERIALIZE();
};

// std::shared_ptr wrappers

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, std::shared:ptr& ) ) serializes a std::shared_ptr
// with a parent std::shared_ptr managing the object
template <class PTR_TYPE, class PARENT_TYPE>
std::enable_if_t<!is_unbounded_array_v<PARENT_TYPE>, pvt::shared_ptr_wrapper_t<std::shared_ptr, PTR_TYPE, PARENT_TYPE>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)
{
    return { ptr, parent };
}

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, size_t& size ) ) serializes a
// std::shared_ptr managing the object when the pointer is to an unbounded array of size
template <class PTR_TYPE>
std::enable_if_t<is_unbounded_array_v<PTR_TYPE>, pvt::shared_ptr_wrapper_t<std::shared_ptr, PTR_TYPE>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, size_t& size)
{
    return { ptr, ptr, size };
}

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, std::shared:ptr&, size_t& size ) ) serializes a
// std::shared_ptr with a parent std::shared_ptr managing the object when the pointer is to an unbounded array of size
template <class PTR_TYPE, class PARENT_TYPE>
std::enable_if_t<is_unbounded_array_v<PARENT_TYPE>, pvt::shared_ptr_wrapper_t<std::shared_ptr, PTR_TYPE, PARENT_TYPE>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, size_t& size)
{
    return { ptr, parent, size };
}

// std::weak_ptr wrappers

// SST_SER( SST::Core::Serialization::weak_ptr( std::weak_ptr&, std::shared:ptr& ) ) serializes a std::weak_ptr
// with a parent std::shared_ptr managing the object
template <class PTR_TYPE, class PARENT_TYPE>
std::enable_if_t<!is_unbounded_array_v<PARENT_TYPE>, pvt::shared_ptr_wrapper_t<std::weak_ptr, PTR_TYPE, PARENT_TYPE>>
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)
{
    return { ptr, parent };
}

// SST_SER( SST::Core::Serialization::weak_ptr( std::weak_ptr&, std::shared:ptr&, size_t& size) ) serializes a
// std::weak_ptr with a parent std::shared_ptr managing the object when the pointer is to an unbounded array of size
template <class PTR_TYPE, class PARENT_TYPE>
std::enable_if_t<is_unbounded_array_v<PARENT_TYPE>, pvt::shared_ptr_wrapper_t<std::weak_ptr, PTR_TYPE, PARENT_TYPE>>
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, size_t& size)
{
    return { ptr, parent, size };
}

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_SHARED_PTR_H
