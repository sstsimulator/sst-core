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

#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>

namespace SST::Core::Serialization {

namespace pvt {

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Strings for error messages based on the types of shared pointers
template <template <typename> class PTR_TEMPLATE>
inline constexpr char ptr_string[] = "weak_ptr";

template <>
inline constexpr char ptr_string<std::shared_ptr>[] = "shared_ptr";

template <template <typename> class PTR_TEMPLATE, bool PARENT_IS_UNBOUNDED_ARRAY>
inline constexpr char wrapper_string[] =
    "SST::Core::Serialization::weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)";

template <>
inline constexpr char wrapper_string<std::weak_ptr, true>[] =
    "SST::Core::Serialization::weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, SIZE_T& "
    "size)";

template <>
inline constexpr char wrapper_string<std::shared_ptr, false>[] =
    "SST::Core::Serialization::shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)";

template <>
inline constexpr char wrapper_string<std::shared_ptr, true>[] =
    "SST::Core::Serialization::shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, "
    "SIZE_T& size)";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the tag for the owner of a std::shared_ptr or std::weak_ptr and whether it has been seen before
template <template <typename> class PTR_TEMPLATE, typename PTR_TYPE>
std::pair<size_t, bool>
get_shared_ptr_owner_tag(const PTR_TEMPLATE<PTR_TYPE>& ptr, serializer& ser)
{
// Workaround for libstdc++ bug 120561 which prevents converting std::weak_ptr<array> to std::weak_ptr<const void>
#if defined(__GLIBCXX__) && __GLIBCXX__ < 20250425
    if constexpr ( is_same_template_v<PTR_TEMPLATE, std::weak_ptr> && std::is_array_v<PTR_TYPE> ) {
        if ( ser.mode() == serializer::SIZER )
            return ser.sizer().get_shared_ptr_owner_tag(ptr.lock());
        else
            return ser.packer().get_shared_ptr_owner_tag(ptr.lock());
    }
    else
#endif

    {
        if ( ser.mode() == serializer::SIZER )
            return ser.sizer().get_shared_ptr_owner_tag(ptr);
        else
            return ser.packer().get_shared_ptr_owner_tag(ptr);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pack the address stored in a shared pointer. Returns true if successful, false if address is out of range.
//
// Although abitrary addresses are allowed to be stored in "aliasing" shared pointers, in order to serialize them, we
// assume that the stored addresses are a limited-range offset within a specified std::shared_ptr parent. To support
// arbitrary pointers stored in shared pointers, we would need another pointer tracking mechanism.
template <template <typename> class PTR_TEMPLATE, typename PTR_TYPE>
bool
pack_shared_ptr_address(const PTR_TEMPLATE<PTR_TYPE>& ptr, const void* parent_addr, size_t parent_size, serializer& ser)
{
    // If ptr is a std::shared_ptr, stored address is returned by get()
    // if ptr is a std::weak_ptr, we have to temporarily lock() it to get() the stored address
    const void* addr;
    if constexpr ( is_same_template_v<PTR_TEMPLATE, std::shared_ptr> )
        addr = ptr.get();
    else
        addr = ptr.lock().get();

    // Whether the stored address is not a nullptr
    bool nonnull = addr != nullptr;
    ser.primitive(nonnull);

    // If not null, serialize offset of the pointer's stored address to its parent's stored address
    if ( nonnull ) {
        // Offset of the shared pointer's stored address relative to the parent's address
        ptrdiff_t offset = static_cast<const char*>(addr) - static_cast<const char*>(parent_addr);

        // Make sure that the address offset is inside of the parent or one byte past the end
        if ( offset < 0 || static_cast<size_t>(offset) > parent_size ) return false;

        // Serialize the address offset
        ser.primitive(offset);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Unpack the address stored in a shared pointer
inline std::pair<bool, ptrdiff_t>
unpack_shared_ptr_address(serializer& ser)
{
    // Deserialize whether the pointer is null
    bool nonnull = false;
    ser.primitive(nonnull);

    // If it is not null, deserialize the offset
    ptrdiff_t offset = 0;
    if ( nonnull ) ser.primitive(offset);

    return { nonnull, offset };
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pack the parent owner of a shared pointer, which is done the first time an ownership tag is seen
template <class PARENT_TYPE>
void
pack_shared_ptr_parent(const std::shared_ptr<PARENT_TYPE>& parent, size_t* size, serializer& ser, ser_opt_t opt)
{
    // OWNER_TYPE is PARENT_TYPE with cv-qualifiers removed so that it can be serialized
    using OWNER_TYPE = std::remove_cv_t<PARENT_TYPE>;

    // ELEM_TYPE is the element type if OWNER_TYPE is an array
    using ELEM_TYPE = std::remove_extent_t<OWNER_TYPE>;

    if constexpr ( is_unbounded_array_v<OWNER_TYPE> ) {
        // If the parent object is an unbounded array

        // Serialize the array size
        ser.primitive(*size);

        // Serialize the array elements
        if constexpr ( is_trivially_serializable_v<ELEM_TYPE> )
            ser.raw(parent.get(), *size * sizeof(ELEM_TYPE));
        else
            pvt::serialize_array(ser, parent.get(), opt, *size, pvt::serialize_array_element<ELEM_TYPE>);
    }
    else {
        // If the parent object is not an unbounded array

        // Serialize the parent owner object
        SST_SER(*reinterpret_cast<OWNER_TYPE*>(const_cast<ELEM_TYPE*>(parent.get())), opt);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Unpack a shared pointer owner, finding or creating a std::shared_ptr owner with a particular tag
template <class PARENT_TYPE>
const std::shared_ptr<void>&
unpack_shared_ptr_owner(size_t tag, std::shared_ptr<PARENT_TYPE>& parent, size_t* size, serializer& ser, ser_opt_t opt)
{
    // OWNER_TYPE is PARENT_TYPE with cv-qualifiers removed so that it can be deserialized
    using OWNER_TYPE = std::remove_cv_t<PARENT_TYPE>;

    // ELEM_TYPE is the element type if OWNER_TYPE is an array
    using ELEM_TYPE = std::remove_extent_t<OWNER_TYPE>;

    // Look for the std::shared_ptr owner of the tag, creating a new empty one if the tag has not been seen before.
    // auto&& acts like a universal reference for each member.
    auto&& [owner, is_new_tag] = ser.unpacker().get_shared_ptr_owner(tag);

    // Important: owner must be a lvalue reference to a std::shared_ptr<void> so that the code below modifies the
    // std::shared_ptr owner stored in a table indexed by tag. The control block of the owner created here is used
    // now and in all future std::shared_ptr or std::weak_ptr deserializations of the same ownership tag.
    static_assert(std::is_same_v<decltype(owner), std::shared_ptr<void>&>);

    // If the tag has not been seen before, deserialize the parent object.
    if ( is_new_tag ) {
        if constexpr ( is_unbounded_array_v<OWNER_TYPE> ) {
            // If the parent type is an unbounded array

            // Deserialize the size
            ser.primitive(*size);

            // Allocate a std::shared_ptr for the parent. std::make_shared supports arrays only on C++20.
            if constexpr ( __cplusplus < 202002l )
                owner = std::shared_ptr<OWNER_TYPE>(new ELEM_TYPE[*size] {});
            else
                owner = std::make_shared<OWNER_TYPE>(*size);

            // Deserialize the array elements in the parent object
            if constexpr ( is_trivially_serializable_v<ELEM_TYPE> )
                ser.raw(owner.get(), *size * sizeof(ELEM_TYPE));
            else
                pvt::serialize_array(ser, owner.get(), opt, *size, pvt::serialize_array_element<ELEM_TYPE>);
        }
        else {
            // If the parent type is not an unbounded array

            // Allocate a std::shared_ptr for the parent. std::make_shared supports arrays only on C++20.
            if constexpr ( std::is_array_v<OWNER_TYPE> && __cplusplus < 202002l )
                owner = std::shared_ptr<OWNER_TYPE>(new ELEM_TYPE[std::extent_v<OWNER_TYPE>] {});
            else
                owner = std::make_shared<OWNER_TYPE>();

            // Deserialize the parent owner object
            SST_SER(*static_cast<OWNER_TYPE*>(owner.get()), opt);
        }

        // Set the parent to the owning std::shared_ptr, analogous to casting from void* to PARENT_TYPE*
        parent = std::static_pointer_cast<PARENT_TYPE>(owner);
    }
    return owner;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class serialize_shared_ptr_impl
{
    // Pointer to unbounded array size
    size_t* const size;

    // Size of the parent
    template <class PARENT_TYPE>
    size_t parent_size() const
    {
        if constexpr ( is_unbounded_array_v<PARENT_TYPE> )
            return *size * sizeof(std::remove_extent_t<PARENT_TYPE>);
        else
            return sizeof(PARENT_TYPE);
    }

public:
    // Optional size argument is pointer to unbounded array size
    explicit serialize_shared_ptr_impl(size_t* size = nullptr) :
        size(size)
    {}

    // Serialize a std::shared_ptr or std::weak_ptr whose ownership is managed by a std::shared_ptr parent. The types of
    // the pointees, PTR_TYPE and PARENT_TYPE, do not need to be the same, since a std::shared_ptr or std::weak_ptr can
    // point to a sub-object of a managed object. PTR_TEMPLATE<PTR_TYPE> matches either the std::shared_ptr or the
    // std::weak_ptr being serialized.
    template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE>
    void operator()(PTR_TEMPLATE<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, serializer& ser, ser_opt_t opt)
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        case serializer::PACK:
        {
            // Make sure that ptr and parent have the same owning control block or are both empty.
            if ( ptr.use_count() ? ptr.owner_before(parent) || parent.owner_before(ptr) : parent.use_count() != 0 ) {
                Output::getDefaultObject().fatal(__LINE__, __FILE__, __func__, 1,
                    "ERROR: Serialized std::%s does not have the same owner as parent specified by %s.\n",
                    ptr_string<PTR_TEMPLATE>, wrapper_string<PTR_TEMPLATE, is_unbounded_array_v<PARENT_TYPE>>);
            }

            // Get the tag of this shared pointer and whether it is the first time this tag has been seen
            auto [tag, is_new_tag] = get_shared_ptr_owner_tag(ptr, ser);

            // Serialize the ownership tag of this shared pointer
            ser.primitive(tag);

            // A tag of 0 indicates an empty shared pointer
            // If the tag is not 0, there is a shared pointer control block
            if ( tag != 0 ) {
                // Serialize the address stored in this shared pointer
                if ( !pack_shared_ptr_address(ptr, parent.get(), parent_size<PARENT_TYPE>(), ser) ) {
                    Output::getDefaultObject().fatal(__LINE__, __FILE__, __func__, 1,
                        "ERROR: Serialized std::%s has a stored pointer outside of the bounds of the parent specified "
                        "by %s.\nThere is no way to know how this raw pointer should be serialized.\n",
                        ptr_string<PTR_TEMPLATE>, wrapper_string<PTR_TEMPLATE, is_unbounded_array_v<PARENT_TYPE>>);
                }
                // If this is the first use of the tag of the owning control block, serialize the parent object
                if ( is_new_tag ) pack_shared_ptr_parent(parent, size, ser, opt);
            }
            break;
        }

        case serializer::UNPACK:
        {
            // Reset the std::shared_ptr or std::weak_ptr so that it is empty
            ptr.reset();

            // Deserialize the tag
            size_t tag = 0;
            ser.primitive(tag);

            // A tag of 0 indicates an empty shared pointer
            // If the tag is not 0, there is a shared pointer control block
            if ( tag != 0 ) {
                // Deserialize the address of the shared pointer
                auto [nonnull, offset] = unpack_shared_ptr_address(ser);

                // Find the owner of this pointer's control block based on tag, deserializing the parent if it is new
                const std::shared_ptr<void>& owner = unpack_shared_ptr_owner(tag, parent, size, ser, opt);

                // At this point, "owner" references a std::shared_ptr<void> which owns the control block for "tag",
                // whether it was newly allocated, or whether it was retrieved from the table holding it for the tag.

                // The pointer stored inside of "ptr" is either nullptr, or an offset within the owner std::shared_ptr.
                std::remove_extent_t<PTR_TYPE>* addr = nullptr;
                if ( nonnull )
                    addr = reinterpret_cast<std::remove_extent_t<PTR_TYPE>*>(static_cast<char*>(owner.get()) + offset);

                // Set the std::shared_ptr or std::weak_ptr to either nullptr, or to an offset within the owner.
                // What is important, is that "ptr" must have the same control block as "owner".
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

// Serialize a std::shared_ptr, assuming that it points to the beginning of the managed object.
// For std::shared_ptr to unbounded arrays with runtime size, the wrapper function must be used.
// std::shared_ptr to functions is not supported.
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

// Serialize a std::weak_ptr, assuming that it points to the beginning of the managed object.
// For std::weak_ptr to unbounded arrays with runtime size, the wrapper function must be used.
// std::weak_ptr to functions is not supported.
template <class PTR_TYPE>
class serialize_impl<std::weak_ptr<PTR_TYPE>,
    std::enable_if_t<!is_unbounded_array_v<PTR_TYPE> && !std::is_function_v<PTR_TYPE>>>
{
    void operator()(std::weak_ptr<PTR_TYPE>& ptr, serializer& ser, ser_opt_t opt)
    {
        ser_opt_t elem_opt = SerOption::is_set(opt, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;

        // A temporary std::shared_ptr created from a std::weak_ptr acts as the parent owning the shared object. This
        // std::shared_ptr will be destroyed at the end of this function, but it will be passed by reference for the
        // (de)serialization of std::weak_ptr, and in the case of deserialization, a copy of the std::shared_ptr to
        // the managed object will be saved in a table for use during the rest of the deserialization, where it can
        // be copied into one or more std::shared_ptr or std::weak_ptr instances which are deserialized. This allows
        // a std::weak_ptr to be (de)serialized before its owning std::shared_ptr is (de)serialized.
        std::shared_ptr<PTR_TYPE> parent = ptr.lock();
        pvt::serialize_shared_ptr_impl()(ptr, parent, ser, elem_opt);
    }
    SST_FRIEND_SERIALIZE();
};

namespace pvt {

// Wrapper class which references a std::shared_ptr or std::weak_ptr and a std::shared_ptr which manages ownership
//
// PTR_TEMPLATE: std::shared_ptr or std::weak_ptr, the template of the type being serialized
//
// PTR_TYPE: The pointee type of the std::shared_ptr or std::weak_ptr pointer. PTR_TEMPLATE<PTR_TYPE> is the type
// of the object being serialized.
//
// PARENT_TYPE: The pointee type of the std::shared_ptr<PARENT_TYPE> owning object. Every std::shared_ptr or
// std::weak_ptr is associated with a std::shared_ptr owner which is not necessarily the same pointee type.
//
// SIZE_T: An integral type used to hold the size of arrays when PARENT_TYPE is an unbounded array type T[].
//
// OWNER_TYPE: Either std::shared_ptr<PARENT_TYPE>& or std::shared_ptr<PARENT_TYPE>, the type of the owning parent
// used by this wrapper. When OWNER_TYPE is an lvalue reference, OWNER_TYPE&& parent collapses into an lvalue
// reference which must be bound to an lvalue owner. When OWNER_TYPE is not an lvalue reference, OWNER_TYPE&&
// parent is an rvalue reference which must be bound to an rvalue owner such as a temporary returned by
// std::weak_ptr::lock() when the std::weak_ptr has an unspecified owner.
//
// PARENT_IS_UNBOUNDED_ARRAY: Whether the parent object is an unbounded array type T[].

template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE = PTR_TYPE, class SIZE_T = void,
    class OWNER_TYPE               = std::shared_ptr<PARENT_TYPE>&,
    bool PARENT_IS_UNBOUNDED_ARRAY = is_unbounded_array_v<PARENT_TYPE>>
struct shared_ptr_wrapper
{
    PTR_TEMPLATE<PTR_TYPE>& ptr;
    OWNER_TYPE&&            parent;
};

// Specialization for unbounded arrays which has a reference to a size
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE, class SIZE_T, class OWNER_TYPE>
struct shared_ptr_wrapper<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE, SIZE_T, OWNER_TYPE, true>
{
    PTR_TEMPLATE<PTR_TYPE>& ptr;
    OWNER_TYPE&&            parent;
    SIZE_T&                 size;
};

} // namespace pvt

// Serialize a std::shared_ptr or std::weak_ptr with another std::shared_ptr object which manages the shared object
// std::shared_ptr or std::weak_ptr to functions is not supported
template <template <class> class PTR_TEMPLATE, class PTR_TYPE, class PARENT_TYPE, class SIZE_T, class OWNER_TYPE,
    bool PARENT_IS_UNBOUNDED_ARRAY>
class serialize_impl<
    pvt::shared_ptr_wrapper<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE, SIZE_T, OWNER_TYPE, PARENT_IS_UNBOUNDED_ARRAY>,
    std::enable_if_t<!std::is_function_v<PTR_TYPE>>>
{
    void operator()(
        pvt::shared_ptr_wrapper<PTR_TEMPLATE, PTR_TYPE, PARENT_TYPE, SIZE_T, OWNER_TYPE, PARENT_IS_UNBOUNDED_ARRAY>&
                    ptr,
        serializer& ser, ser_opt_t opt)
    {
        ser_opt_t elem_opt = SerOption::is_set(opt, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;
        if constexpr ( !PARENT_IS_UNBOUNDED_ARRAY ) {
            // If PARENT_TYPE is not an unbounded array
            pvt::serialize_shared_ptr_impl {}(ptr.ptr, ptr.parent, ser, elem_opt);
        }
        else if constexpr ( std::is_same_v<SIZE_T, size_t> ) {
            // If PARENT_TYPE is an unbounded array, pass address of size_t size parameter
            pvt::serialize_shared_ptr_impl { &ptr.size }(ptr.ptr, ptr.parent, ser, elem_opt);
        }
        else {
            // If PARENT_TYPE is an unbounded array, handle non-size_t size parameter
            const auto mode = ser.mode();
            size_t     size = 0;
            if ( mode == serializer::SIZER || mode == serializer::PACK ) {
                if ( (ptr.size < 0) || (ptr.size > SIZE_MAX) ) {
                    Output::getDefaultObject().fatal(__LINE__, __FILE__, __func__, 1,
                        "Serialization error: Array size in SST::Core::Serialization::%s() "
                        "cannot fit inside size_t. size_t should be used for array sizes.\n",
                        pvt::ptr_string<PTR_TEMPLATE>);
                }
                size = static_cast<size_t>(ptr.size);
            }
            pvt::serialize_shared_ptr_impl { &size }(ptr.ptr, ptr.parent, ser, elem_opt);
            if ( mode == serializer::UNPACK ) ptr.size = static_cast<SIZE_T>(size);
        }
    }
    SST_FRIEND_SERIALIZE();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// std::shared_ptr wrappers

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, std::shared:ptr& ) ) serializes a std::shared_ptr
// with a parent std::shared_ptr managing the owned object.
template <class PTR_TYPE, class PARENT_TYPE>
std::enable_if_t<!is_unbounded_array_v<PARENT_TYPE>, pvt::shared_ptr_wrapper<std::shared_ptr, PTR_TYPE, PARENT_TYPE>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)
{
    return { ptr, parent };
}

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, SIZE_T& size ) ) serializes a std::shared_ptr
// managing the owned object when the pointer is to an unbounded array of size "size".
template <class PTR_TYPE, class SIZE_T>
std::enable_if_t<is_unbounded_array_v<PTR_TYPE>, pvt::shared_ptr_wrapper<std::shared_ptr, PTR_TYPE, PTR_TYPE, SIZE_T>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, SIZE_T& size)
{
    return { ptr, ptr, size };
}

// SST_SER( SST::Core::Serialization::shared_ptr( std::shared_ptr&, std::shared:ptr&, SIZE_T& size ) ) serializes a
// std::shared_ptr with a parent std::shared_ptr managing the owned object when the pointer is to an unbounded array of
// size "size".
template <class PTR_TYPE, class PARENT_TYPE, class SIZE_T>
std::enable_if_t<is_unbounded_array_v<PARENT_TYPE>,
    pvt::shared_ptr_wrapper<std::shared_ptr, PTR_TYPE, PARENT_TYPE, SIZE_T>>
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, SIZE_T& size)
{
    return { ptr, parent, size };
}

// Identity operation for consistency -- SST_SER( SST::Core::Serialization::shared_ptr(ptr) ) is same as SST_SER(ptr).
template <class PTR_TYPE>
std::shared_ptr<PTR_TYPE>&
shared_ptr(std::shared_ptr<PTR_TYPE>& ptr)
{
    return ptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// std::weak_ptr wrappers

// SST_SER( SST::Core::Serialization::weak_ptr( std::weak_ptr&, std::shared:ptr& ) ) serializes a std::weak_ptr with a
// parent std::shared_ptr managing the owned object.
template <class PTR_TYPE, class PARENT_TYPE>
std::enable_if_t<!is_unbounded_array_v<PARENT_TYPE>, pvt::shared_ptr_wrapper<std::weak_ptr, PTR_TYPE, PARENT_TYPE>>
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent)
{
    return { ptr, parent };
}

// SST_SER( SST::Core::Serialization::weak_ptr( std::weak_ptr&, SIZE_T& size& ) ) serializes a std::weak_ptr with an
// unspecified std::shared_ptr managing the owned object when the pointer is to an unbounded array of size "size". See
// comments above about ptr.lock() being used to get a temporary std::shared_ptr to use as a parent.
template <class PTR_TYPE, class SIZE_T>
std::enable_if_t<is_unbounded_array_v<PTR_TYPE>,
    pvt::shared_ptr_wrapper<std::weak_ptr, PTR_TYPE, PTR_TYPE, SIZE_T, std::shared_ptr<PTR_TYPE>>>
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, SIZE_T& size)
{
    return { ptr, ptr.lock(), size };
}

// SST_SER( SST::Core::Serialization::weak_ptr( std::weak_ptr&, std::shared:ptr&, SIZE_T& size) ) serializes a
// std::weak_ptr with a parent std::shared_ptr managing the owned object when the pointer is to an unbounded array of
// size "size".
template <class PTR_TYPE, class PARENT_TYPE, class SIZE_T>
std::enable_if_t<is_unbounded_array_v<PARENT_TYPE>,
    pvt::shared_ptr_wrapper<std::weak_ptr, PTR_TYPE, PARENT_TYPE, SIZE_T>>
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr, std::shared_ptr<PARENT_TYPE>& parent, SIZE_T& size)
{
    return { ptr, parent, size };
}

// Identity operation for consistency -- SST_SER( SST::Core::Serialization::weak_ptr(ptr) ) is same as SST_SER(ptr).
template <class PTR_TYPE>
std::weak_ptr<PTR_TYPE>&
weak_ptr(std::weak_ptr<PTR_TYPE>& ptr)
{
    return ptr;
}

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_SHARED_PTR_H
