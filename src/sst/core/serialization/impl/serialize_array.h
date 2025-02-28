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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ARRAY_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ARRAY_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_array.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

namespace SST::Core::Serialization {
namespace pvt {

template <class TPtr, class IntType>
class ser_array_wrapper
{
public:
    TPtr*&   bufptr;
    IntType& sizeptr;
    ser_array_wrapper(TPtr*& buf, IntType& size) : bufptr(buf), sizeptr(size) {}
};

// template <class IntType>
// class ser_buffer_wrapper
// {
// public:
//     void*& bufptr;
//     IntType& sizeptr;
//     ser_buffer_wrapper(void*& buf, IntType& size) :
//         bufptr(buf), sizeptr(size) {}
// };

template <class TPtr>
class raw_ptr_wrapper
{
public:
    TPtr*& bufptr;
    raw_ptr_wrapper(TPtr*& ptr) : bufptr(ptr) {}
};

} // namespace pvt
/** I have typedefing pointers, but no other way.
 *  T could be "void and TPtr void* */
template <class TPtr, class IntType>
inline pvt::ser_array_wrapper<TPtr, IntType>
array(TPtr*& buf, IntType& size)
{
    return pvt::ser_array_wrapper<TPtr, IntType>(buf, size);
}

template <class IntType>
inline pvt::ser_array_wrapper<void, IntType>
buffer(void*& buf, IntType& size)
{
    return pvt::ser_array_wrapper<void, IntType>(buf, size);
}

template <class TPtr>
inline pvt::raw_ptr_wrapper<TPtr>
raw_ptr(TPtr*& ptr)
{
    return pvt::raw_ptr_wrapper<TPtr>(ptr);
}

/*****       Specializations of serialize class       *****/

/***    For statically allocated arrays ***/

/**
   Version of serialize that works for statically allocated arrays of
   fundamental types and enums.
 */
template <class T, int N>
class serialize_impl<T[N], std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>>>
{
    template <class A>
    friend class serialize;
    void operator()(T arr[N], serializer& ser) { ser.array<T, N>(arr); }

    void operator()(T UNUSED(arr[N]), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Implement mapping mode
    }
};

/**
   Version of serialize that works for statically allocated arrays of
   non base types.
 */
template <class T, int N>
class serialize_impl<T[N], std::enable_if_t<!std::is_fundamental_v<T> && !std::is_enum_v<T>>>
{
    template <class A>
    friend class serialize;
    void operator()(T arr[N], serializer& ser)
    {
        for ( int i = 0; i < N; i++ ) {
            ser& arr[i];
        }
    }

    void operator()(T UNUSED(arr[N]), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Implement mapping mode
    }
};

/***  For dynamically allocated arrays ***/

/**
   Version of serialize that works for dynamically allocated arrays of
   fundamental types and enums.
 */
template <class T, class IntType>
class serialize_impl<
    pvt::ser_array_wrapper<T, IntType>, std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>>>
{
    template <class A>
    friend class serialize;
    void operator()(pvt::ser_array_wrapper<T, IntType> arr, serializer& ser) { ser.binary(arr.bufptr, arr.sizeptr); }

    void operator()(pvt::ser_array_wrapper<T, IntType> UNUSED(arr), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Implement mapping mode
    }
};

/**
   Version of serialize that works for dynamically allocated arrays of
   non base types.
 */
template <class T, class IntType>
class serialize_impl<
    pvt::ser_array_wrapper<T, IntType>, std::enable_if_t<!std::is_fundamental_v<T> && !std::is_enum_v<T>>>
{
    template <class A>
    friend class serialize;
    void operator()(pvt::ser_array_wrapper<T, IntType> arr, serializer& ser)
    {
        ser.primitive(arr.sizeptr);
        for ( int i = 0; i < arr.sizeptr; i++ ) {
            ser& arr[i];
        }
    }

    void operator()(pvt::ser_array_wrapper<T, IntType> UNUSED(arr), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Implement mapping mode
    }
};

/**
   Version of serialize that works for statically allocated arrays of
   void*.
 */
template <class IntType>
class serialize_impl<pvt::ser_array_wrapper<void, IntType>>
{
    template <class A>
    friend class serialize;
    void operator()(pvt::ser_array_wrapper<void, IntType> arr, serializer& ser) { ser.binary(arr.bufptr, arr.sizeptr); }

    void
    operator()(pvt::ser_array_wrapper<void, IntType> UNUSED(arr), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Implement mapping mode
    }
};

/***   Other Specializations (raw_ptr and trivially_serializable)  ***/

/**
   Version of serialize that works for copying raw pointers (only
   copying the value of the pointer.  Note that this is only useful
   if the value is going to be sent back to the originator, since it
   won't be valid on the other rank.
 */
template <class TPtr>
class serialize_impl<pvt::raw_ptr_wrapper<TPtr>>
{
    template <class A>
    friend class serialize;
    void operator()(pvt::raw_ptr_wrapper<TPtr> ptr, serializer& ser) { ser.primitive(ptr.bufptr); }

    void operator()(pvt::raw_ptr_wrapper<TPtr> UNUSED(ptr), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Implement mapping mode
    }
};

// Needed only because the default version in serialize.h can't get
// the template expansions quite right trying to look through several
// levels of expansion
template <class TPtr, class IntType>
inline void
operator&(serializer& ser, pvt::ser_array_wrapper<TPtr, IntType> arr)
{
    operator&<pvt::ser_array_wrapper<TPtr, IntType>>(ser, arr);
    // serialize<pvt::ser_array_wrapper<TPtr, IntType>>()(arr, ser);
}

// Needed only because the default version in serialize.h can't get
// the template expansions quite right trying to look through several
// levels of expansion
template <class TPtr>
inline void
operator&(serializer& ser, pvt::raw_ptr_wrapper<TPtr> ptr)
{
    operator&<pvt::raw_ptr_wrapper<TPtr>>(ser, ptr);
    // serialize<pvt::raw_ptr_wrapper<TPtr>>()(ptr, ser);
}

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ARRAY_H
