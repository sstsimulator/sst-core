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

#include <cstddef>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace SST::Core::Serialization {

namespace pvt {

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
        const char* ptr_type = is_same_type_template_v<PTR, std::weak_ptr> ? "weak_ptr" : "shared_ptr";
        Output::getDefaultObject().fatal(__LINE__, __FILE__, __func__, 1,
            "ERROR: Serialized std::%s does not have the same owner as parent as indicated by "
            "SST::Core::Serialization::%s(std::%s<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)\n",
            ptr_type, ptr_type, ptr_type);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Workaround for libstdc++ bug 120561 which prevents converting std::weak_ptr<array> to std::weak_ptr<const void>
#if defined(__GLIBCXX__) && __GLIBCXX__ < 20250425

template <class T>
std::enable_if_t<std::is_array_v<T>, std::weak_ptr<const void>> get_weak_ptr(const std::weak_ptr<T>& ptr)
{
    return ptr.lock();
}

template <class T>
std::weak_ptr<const void> get_weak_ptr(const T& ptr)
{
    return ptr;
}

#define GET_WEAK_PTR(ptr) get_weak_ptr(ptr)

#else

#define GET_WEAK_PTR(ptr) ptr

#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class serialize_shared_ptr_impl
{
    // Pointer to unbounded array size
    size_t* const size;

public:
    explicit serialize_shared_ptr_impl(size_t* size = nullptr) :
        size(size)
    {}

    // Serialize a std::shared_ptr or std::weak_ptr whose ownership is managed by a std::shared_ptr parent. The types
    // of the pointees, PTR_TYPE and PARENT_TYPE, do not need to be the same, since a std::shared_ptr or std::weak_ptr
    // can point to a sub-object of a managed object. PTR_TEMPLATE<PTR_TYPE> matches either a std::shared_ptr or
    // std::weak_ptr being serialized.
    template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE>
    void operator()(PTR_TEMPLATE<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, serializer& ser, ser_opt_t opt)
    {
        using elem_t        = std::remove_extent_t<PTR_TYPE>;
        using parent_elem_t = std::remove_extent_t<PARENT_TYPE>;
        const auto mode     = ser.mode();
        size_t     tag;
        bool       nonnull;
        ptrdiff_t  offset;

        switch ( mode ) {
        case serializer::SIZER:
        case serializer::PACK:
        {
            // Make sure that ptr and parent have the same owning control block
            assert_same_owner(ptr, parent);

            // If the use_count() is 0, the std::shared_ptr or std::weak_ptr has no managed object and is empty, so
            // a tag of 0 is serialized
            if ( !ptr.use_count() ) {
                tag = 0;
                SST_SER(tag);
            }
            else {
                bool is_new_tag;

                // Get the tag for the std::shared_ptr owner of this std::ptr or std::weak_ptr
                if ( mode == serializer::SIZER )
                    std::tie(tag, is_new_tag) = ser.sizer().get_shared_ptr_tag(GET_WEAK_PTR(ptr));
                else
                    std::tie(tag, is_new_tag) = ser.packer().get_shared_ptr_tag(GET_WEAK_PTR(ptr));

                // Serialize the tag
                SST_SER(tag);

                // Address stored in the shared pointer
                // If ptr is a std::shared_ptr, it is returned by get()
                // if ptr is a std::weak_ptr, we have to lock() it to get() the stored address
                const void* addr;
                if constexpr ( is_same_template_v<PTR_TEMPLATE, std::shared_ptr> )
                    addr = ptr.get();
                else
                    addr = ptr.lock().get();

                // Whether the stored address is not a nullptr
                nonnull = addr != nullptr;
                SST_SER(nonnull);

                // If not null, serialize offset of the pointer's stored address to its parent's stored address
                if ( nonnull ) {
                    offset = static_cast<const char*>(addr) - reinterpret_cast<const char*>(parent.get());
                    SST_SER(offset);
                }

                // If this is the first use of the tag of the owning control block, serialize the parent object
                if ( is_new_tag ) {
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
                        // If the parent object is not an unbounded array
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
                // Deserialize whether the pointer is null
                SST_SER(nonnull);

                // If it is not null, deserialize the offset
                if ( nonnull ) SST_SER(offset);

                // Look for the shared pointer owner of the tag
                auto&& [owner, is_new_tag] = ser.unpacker().get_shared_ptr_owner(tag);

                // If this is the first use of the shared pointer owning the control block, deserialize the parent
                // object
                if ( is_new_tag ) {
                    if constexpr ( is_unbounded_array_v<PARENT_TYPE> ) {
                        // If the parent type is an unbounded array

                        // Deserialize the size
                        SST_SER(*size);

                        // Allocate a std::shared_ptr for the parent. std::make_shared only supports arrays on C++20.
                        if constexpr ( __cplusplus < 202002l )
                            owner = std::shared_ptr<PARENT_TYPE>(new parent_elem_t[*size]);
                        else
                            owner = std::make_shared<PARENT_TYPE>(*size);

                        // Deserialize the array elements in the parent object
                        if constexpr ( std::is_arithmetic_v<parent_elem_t> ||
                                       std::is_enum_v<parent_elem_t> /* is_trivially_serializable_v<parent_elem_t> */ )
                            ser.raw(owner.get(), *size * sizeof(parent_elem_t));
                        else
                            pvt::serialize_array(
                                ser, owner.get(), opt, *size, pvt::serialize_array_element<parent_elem_t>);
                    }
                    else {
                        // If the parent type is not an unbounded array

                        // Allocated a std::shared_ptr for the parent. std::make_shared only supports arrays on C++20.
                        if constexpr ( std::is_array_v<PARENT_TYPE> && __cplusplus < 202002l )
                            owner = std::shared_ptr<PARENT_TYPE>(new parent_elem_t[std::extent_v<PARENT_TYPE>]);
                        else
                            owner = std::make_shared<PARENT_TYPE>();

                        // Deserialize the parent object
                        SST_SER(*static_cast<PARENT_TYPE*>(owner.get()));
                    }

                    // Set the parent to the owning std::shared_ptr
                    parent = std::static_pointer_cast<PARENT_TYPE>(owner);
                }

                // Stored pointer is either null or an offset within the parent object
                elem_t* addr = nullptr;
                if ( nonnull ) addr = reinterpret_cast<elem_t*>(reinterpret_cast<char*>(owner.get()) + offset);

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
}; // class serialize_shared_ptr_impl

} // namespace pvt

// Serialize a std::shared_ptr, assuming that it points to the beginning of the managed object
// For std::shared_ptr to unbounded arrays with runtime size, the wrapper class must be used
// std::shared_ptr to functions is not supported
template <class PTR_TYPE>
class serialize_impl<std::shared_ptr<PTR_TYPE>,
    std::enable_if_t<!is_unbounded_array_v<PTR_TYPE> && !std::is_function_v<PTR_TYPE>>>
{
    void operator()(std::shared_ptr<PTR_TYPE>& ptr, serializer& ser, ser_opt_t opt)
    {
        ser_opt_t elem_opt = SerOption::is_set(opt, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;

        // The std::shared_ptr is considered its own parent owning the managed storage
        pvt::serialize_shared_ptr_impl()(ptr, ptr, ser, elem_opt);
    }
    SST_FRIEND_SERIALIZE();
};

// Serialize a std::weak_ptr, assuming that it points to the beginning of the managed object
// For std::weak_ptr to unbounded arrays with runtime size, the wrapper class must be used
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
        pvt::serialize_shared_ptr_impl()(ptr, parent, ser, elem_opt);
    }
    SST_FRIEND_SERIALIZE();
};

namespace pvt {
// Wrapper class which references a std::shared_ptr or std::weak_ptr and a std::shared_ptr which manages ownership
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE = PTR_TYPE, class SIZE_T = void,
    class OWNER_TYPE = std::shared_ptr<PARENT_TYPE>&, bool = is_unbounded_array_v<PARENT_TYPE>>
struct shared_ptr_wrapper_t
{
    PTR_TEMPLATE<PTR_TYPE>& ptr;
    OWNER_TYPE              parent;
};

// Specialization for parent std::shared_ptr to unbounded arrays which have reference to size
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE, class SIZE_T, class OWNER_TYPE>
struct shared_ptr_wrapper_t<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE, SIZE_T, OWNER_TYPE, true>
{
    PTR_TEMPLATE<PTR_TYPE>& ptr;
    OWNER_TYPE              parent;
    SIZE_T&                 size;
};

} // namespace pvt

// Serialize a std::shared_ptr or std::weak_ptr with another std::shared_ptr object which manages the shared object
// std::shared_ptr or std::weak_ptr to functions is not supported
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE, class SIZE_T, class OWNER_TYPE>
class serialize_impl<pvt::shared_ptr_wrapper_t<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE, SIZE_T, OWNER_TYPE>,
    std::enable_if_t<!std::is_function_v<PTR_TYPE>>>
{
    void operator()(
        pvt::shared_ptr_wrapper_t<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE, SIZE_T>& ptr, serializer& ser, ser_opt_t opt)
    {
        ser_opt_t elem_opt = SerOption::is_set(opt, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;

        // Serialize std::shared_ptr or std::weak_ptr pointer and its std::shared_ptr parent
        // If PARENT_TYPE is an unbounded array, pass size parameter
        if constexpr ( is_unbounded_array_v<PARENT_TYPE> ) {
            if constexpr ( std::is_same_v<SIZE_T, size_t> ) {
                pvt::serialize_shared_ptr_impl { &ptr.size }(ptr.ptr, ptr.parent, ser, elem_opt);
            }
            else {
                size_t     size;
                const auto mode = ser.mode();
                if ( mode == serializer::SIZER || mode == serializer::PACK ) {
                    if ( (ptr.size < 0) || (ptr.size > SIZE_MAX) )
                        Output::getDefaultObject().fatal(__LINE__, __FILE__, __func__, 1,
                            "Error: Array size is cannot fit inside size_t. size_t should be used for array sizes.\n");
                    size = static_cast<size_t>(ptr.size);
                }
                pvt::serialize_shared_ptr_impl { &size }(ptr.ptr, ptr.parent, ser, elem_opt);
                if ( mode == serializer::UNPACK ) ptr.size = static_cast<size_t>(size);
            }
        }
        else {
            pvt::serialize_shared_ptr_impl {}(ptr.ptr, ptr.parent, ser, elem_opt);
        }
    }
    SST_FRIEND_SERIALIZE();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// std::shared_ptr wrappers

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, std::shared:ptr& ) ) serializes a
// std::shared_ptr with a parent std::shared_ptr managing the object
template <class PTR_TYPE, class PARENT_TYPE>
std::enable_if_t<!is_unbounded_array_v<PARENT_TYPE>, pvt::shared_ptr_wrapper_t<std::shared_ptr, PTR_TYPE, PARENT_TYPE>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)
{
    return { ptr, parent };
}

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, SIZE_T& size ) ) serializes a
// std::shared_ptr managing the object when the pointer is to an unbounded array of size
template <class PTR_TYPE, class SIZE_T>
std::enable_if_t<is_unbounded_array_v<PTR_TYPE>, pvt::shared_ptr_wrapper_t<std::shared_ptr, PTR_TYPE, PTR_TYPE, SIZE_T>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, SIZE_T& size)
{
    return { ptr, ptr, size };
}

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, std::shared:ptr&, SIZE_T& size ) ) serializes a
// std::shared_ptr with a parent std::shared_ptr managing the object when the pointer is to an unbounded array of
// size
template <class PTR_TYPE, class PARENT_TYPE, class SIZE_T>
std::enable_if_t<is_unbounded_array_v<PARENT_TYPE>,
    pvt::shared_ptr_wrapper_t<std::shared_ptr, PTR_TYPE, PARENT_TYPE, SIZE_T>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, SIZE_T& size)
{
    return { ptr, parent, size };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// std::weak_ptr wrappers

// SST_SER( SST::Core::Serialization::weak_ptr( std::weak_ptr&, std::shared:ptr& ) ) serializes a std::weak_ptr
// with a parent std::shared_ptr managing the object
template <class PTR_TYPE, class PARENT_TYPE>
std::enable_if_t<!is_unbounded_array_v<PARENT_TYPE>, pvt::shared_ptr_wrapper_t<std::weak_ptr, PTR_TYPE, PARENT_TYPE>>
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)
{
    return { ptr, parent };
}

// SST_SER( SST::Core::Serialization::weak_ptr( std::weak_ptr&, SIZE_T& size& ) ) serializes a std::weak_ptr
// with an unspecified std::shared_ptr managing the object when the pointer is to an unbounded array of size
// See comments above about ptr.lock() being used to get a temporary std::shared_ptr
template <class PTR_TYPE, class SIZE_T>
std::enable_if_t<is_unbounded_array_v<PTR_TYPE>,
    pvt::shared_ptr_wrapper_t<std::weak_ptr, PTR_TYPE, PTR_TYPE, SIZE_T, std::shared_ptr<PTR_TYPE>>>
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, SIZE_T& size)
{
    return { ptr, ptr.lock(), size };
}

// SST_SER( SST::Core::Serialization::weak_ptr( std::weak_ptr&, std::shared:ptr&, SIZE_T& size) ) serializes a
// std::weak_ptr with a parent std::shared_ptr managing the object when the pointer is to an unbounded array of size
template <class PTR_TYPE, class PARENT_TYPE, class SIZE_T>
std::enable_if_t<is_unbounded_array_v<PARENT_TYPE>,
    pvt::shared_ptr_wrapper_t<std::weak_ptr, PTR_TYPE, PARENT_TYPE, SIZE_T>>
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, SIZE_T& size)
{
    return { ptr, parent, size };
}

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_SHARED_PTR_H
