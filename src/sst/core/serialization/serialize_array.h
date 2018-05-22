// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SERIALIZE_ARRAY_H
#define SERIALIZE_ARRAY_H

#include <sst/core/serialization/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

template <class TPtr, class IntType>
class ser_array_wrapper
{
public:
    TPtr*& bufptr;
    IntType& sizeptr;
    ser_array_wrapper(TPtr*& buf, IntType& size) :
        bufptr(buf), sizeptr(size) {}  
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
    raw_ptr_wrapper(TPtr*& ptr) :
        bufptr(ptr) {}
};

}
/** I have typedefing pointers, but no other way.
 *  T could be "void and TPtr void* */
template <class TPtr, class IntType>
inline pvt::ser_array_wrapper<TPtr,IntType>
array(TPtr*& buf, IntType& size)
{
    return pvt::ser_array_wrapper<TPtr,IntType>(buf, size);
}

template <class IntType>
inline pvt::ser_array_wrapper<void,IntType>
buffer(void*& buf, IntType& size)
{
    return pvt::ser_array_wrapper<void,IntType>(buf, size);
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
class serialize<T[N], typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type> {
public:
    void operator()(T arr[N], serializer& ser){
        ser.array<T,N>(arr);
    }
};

/**
   Version of serialize that works for statically allocated arrays of
   non base types.
 */
template <class T, int N>
class serialize<T[N], typename std::enable_if<!std::is_fundamental<T>::value && !std::is_enum<T>::value>::type> {
  public:
    void operator()(T arr[N], serializer& ser){
        for ( int i = 0; i < N; i++ ) {
            serialize<T>()(arr[i],ser);
        }
    }
};

/***  For dynamically allocated arrays ***/

/**
   Version of serialize that works for dynamically allocated arrays of
   fundamental types and enums.
 */
template <class T, class IntType>
class serialize<pvt::ser_array_wrapper<T,IntType>, typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type> {
public:
    void operator()(pvt::ser_array_wrapper<T,IntType> arr, serializer& ser) {
        ser.binary(arr.bufptr, arr.sizeptr);
    }
};

/**
   Version of serialize that works for dynamically allocated arrays of
   non base types.
 */
template <class T, class IntType>
class serialize<pvt::ser_array_wrapper<T,IntType>, typename std::enable_if<!std::is_fundamental<T>::value && !std::is_enum<T>::value>::type> {
public:
    void operator()(pvt::ser_array_wrapper<T,IntType> arr, serializer& ser) {
        ser.primitive(arr.sizeptr);
        for ( int i = 0; i < arr.sizeptr; i++ ) {
            serialize<T>()(arr[i],ser);
        }
    }
};

/**
   Version of serialize that works for statically allocated arrays of
   void*.
 */
template <class IntType>
class serialize<pvt::ser_array_wrapper<void,IntType> > {
public:
    void operator()(pvt::ser_array_wrapper<void,IntType> arr, serializer& ser) {
        ser.binary(arr.bufptr, arr.sizeptr);
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
class serialize<pvt::raw_ptr_wrapper<TPtr> > {
public:
    void operator()(pvt::raw_ptr_wrapper<TPtr> ptr, serializer& ser) {
        ser.primitive(ptr.bufptr);
    }
};




// Needed only because the default version in serialize.h can't get
// the template expansions quite right trying to look through several
// levels of expansion
template <class TPtr, class IntType>
inline void
operator&(serializer& ser, pvt::ser_array_wrapper<TPtr,IntType> arr){
    serialize<pvt::ser_array_wrapper<TPtr,IntType> >()(arr,ser);
}

// Needed only because the default version in serialize.h can't get
// the template expansions quite right trying to look through several
// levels of expansion
template <class TPtr>
inline void
operator&(serializer& ser, pvt::raw_ptr_wrapper<TPtr> ptr){
    serialize<pvt::raw_ptr_wrapper<TPtr> >()(ptr,ser);
}

}
}
}
#endif // SERIALIZE_ARRAY_H
