// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_H
#define SST_CORE_SERIALIZATION_SERIALIZE_H

#include "sst/core/serialization/serializer.h"
#include "sst/core/warnmacros.h"

#include <atomic>
#include <iostream>
#include <typeinfo>

namespace SST {
namespace Core {
namespace Serialization {

/**
   Base serialize class.  This is the default, which if hit will
   static_assert.  All other instances are partial specializations of
   this class and do all the real serialization.
 */
template <class T, class Enable = void>
class serialize_impl
{
public:
    // inline void operator()(T& UNUSED(t), serializer& UNUSED(ser))
    // {
    //     // If the default gets called, then it's actually invalid
    //     // because we don't know how to serialize it.

    //     // This is a bit strange, but if I just do a
    //     // static_assert(false) it always triggers, but if I use
    //     // std::is_* then it seems to only trigger if something expands
    //     // to this version of the template.
    //     // static_assert(false,"Trying to serialize an object that is not serializable.");
    //     static_assert(std::is_fundamental<T>::value, "Trying to serialize an object that is not serializable.");
    //     static_assert(!std::is_fundamental<T>::value, "Trying to serialize an object that is not serializable.");
    // }

    inline void operator()(T& t, serializer& ser)
    {
        // This is the fall through case.  Check to see if it's a pointer:
        if constexpr ( std::is_pointer_v<T> ) {
            // If it falls through to the default, let's check to see if it's
            // a non-polymorphic class and try to call serialize_order
            if constexpr ( std::is_class_v<std::remove_pointer<T>> && !std::is_polymorphic_v<std::remove_pointer<T>> ) {
                if ( ser.mode() == serializer::UNPACK ) {
                    t = new typename std::remove_pointer<T>::type();
                    ser.report_new_pointer(reinterpret_cast<uintptr_t>(t));
                }
                t->serialize_order(ser);
            }
            else {
                static_assert(std::is_fundamental<T>::value, "Trying to serialize an object that is not serializable.");
                static_assert(
                    !std::is_fundamental<T>::value, "Trying to serialize an object that is not serializable.");
            }
        }
        else {
            // If it falls through to the default, let's check to see if it's
            // a non-polymorphic class and try to call serialize_order
            if constexpr ( std::is_class_v<T> && !std::is_polymorphic_v<T> ) { t.serialize_order(ser); }
            else {
                static_assert(std::is_fundamental<T>::value, "Trying to serialize an object that is not serializable.");
                static_assert(
                    !std::is_fundamental<T>::value, "Trying to serialize an object that is not serializable.");
            }
        }
    }
};


// template <class T>
// class serialize_impl<T*,std::enable_if_t<std::is_base_of<SST::Core::Serialization::serializable, T>::value, bool> =
// true>
// {
// public:
//    inline void operator()(T& t, serializer& ser)
//    {
//        // If it falls through to the default, let's check to see if it's
//        // a non-polymorphic class and try to call serialize_order
//        if constexpr ( std::is_class_v<T> && !std::is_polymorphic_v<T> /*&& std::is_same_v<typename
//        MethodDetect<decltype( T::serialize_order() )>::type, void>*/)  {
//            t.serialize_order(ser);
//        }
//        else {
//            static_assert(std::is_fundamental<T>::value, "Trying to serialize an object that is not serializable.");
//            static_assert(!std::is_fundamental<T>::value, "Trying to serialize an object that is not serializable.");
//        }
//    }
// };


/**
   Serialization "gateway" object.  All serializations must come
   thorugh these template instances in order for pointer tracking to
   be controlled at one point. The actual serialization will happen in
   serialize_impl classes.
 */
template <class T>
class serialize
{
public:
    inline void operator()(T& t, serializer& ser) { return serialize_impl<T>()(t, ser); }

    /**
       This will track the pointer to the object if pointer tracking
       is turned on. Otherwise, it will just serialize normally.  This
       is only useful if you have a non-pointer struct/class where
       other pieces of code will have pointers to it (e.g. a set/map
       contains the actual struct and other places point to the data's
       location)
     */
    inline void serialize_and_track_pointer(T& t, serializer& ser)
    {
        if ( !ser.is_pointer_tracking_enabled() ) return serialize_impl<T>()(t, ser);

        // Get the pointer to the data that will be uesd for tracking.
        uintptr_t ptr = reinterpret_cast<uintptr_t>(&t);

        switch ( ser.mode() ) {
        case serializer::SIZER:
            // Check to see if the pointer has already been seen.  If
            // so, then we error since the non-pointer version of this
            // data needs to be serialized before any of the pointers
            // that point to it.
            if ( ser.check_pointer_pack(ptr) ) {
                // Error
            }

            // Always put the pointer in
            ser.size(ptr);
            // Count the data for the object
            serialize_impl<T>()(t, ser);
            break;
        case serializer::PACK:
            // Already checked serialization order during sizing, so
            // don't need to do it here
            ser.check_pointer_pack(ptr);
            // Always put the pointer in
            ser.pack(ptr);
            // Serialize the data for the object
            serialize_impl<T>()(t, ser);
            break;
        case serializer::UNPACK:
            // Checked order on serialization, so don't need to do it
            // here

            // Get the stored pointer to use as a tag for future
            // references to the data
            uintptr_t ptr_stored;
            ser.unpack(ptr_stored);

            // Now add this to the tracking data
            ser.report_real_pointer(ptr_stored, ptr);

            serialize_impl<T>()(t, ser);
        }
    }
};

template <class T>
class serialize<T*>
{
public:
    inline void operator()(T*& t, serializer& ser)
    {
        // We are a pointer, need to see if tracking is turned on
        if ( !ser.is_pointer_tracking_enabled() ) return serialize_impl<T*>()(t, ser);

        uintptr_t ptr = reinterpret_cast<uintptr_t>(t);
        if ( nullptr == t ) ptr = 0;

        switch ( ser.mode() ) {
        case serializer::SIZER:
            // Always put the pointer in
            ser.size(ptr);

            // If this is a nullptr, then we are done
            if ( 0 == ptr ) return;

            // If we haven't seen this yet, also need to serialize the
            // object
            if ( !ser.check_pointer_pack(ptr) ) { serialize_impl<T*>()(t, ser); }
            break;
        case serializer::PACK:
            // Always put the pointer in
            ser.pack(ptr);

            // Nothing else to do if this is nullptr
            if ( 0 == ptr ) return;

            if ( !ser.check_pointer_pack(ptr) ) { serialize_impl<T*>()(t, ser); }
            break;
        case serializer::UNPACK:
            // Get the ptr and check to see if we've already deserialized
            uintptr_t ptr_stored;
            ser.unpack(ptr_stored);

            // Check to see if this was a nullptr
            if ( 0 == ptr_stored ) {
                t = nullptr;
                return;
            }

            uintptr_t real_ptr = ser.check_pointer_unpack(ptr_stored);
            if ( real_ptr ) {
                // Already deserialized, so just return pointer
                t = reinterpret_cast<T*>(real_ptr);
            }
            else {
                serialize_impl<T*>()(t, ser);
                ser.report_real_pointer(ptr_stored, reinterpret_cast<uintptr_t>(t));
            }
        }
    }
};

/**
   Version of serialize that works for fundamental types and enums.
 */
template <class T>
class serialize_impl<T, typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type>
{
    template <class A>
    friend class serialize;
    inline void operator()(T& t, serializer& ser) { ser.primitive(t); }
};

/**
   Version of serialize that works for bool.
 */
template <>
class serialize_impl<bool>
{
    template <class A>
    friend class serialize;
    void operator()(bool& t, serializer& ser)
    {
        int bval = t;
        ser.primitive(bval);
        t = bool(bval);
    }
};

/**
   Version of serialize that works for pointers to fundamental types
   and enums. Note that the pointer tracking happens at a higher
   level, and only if it is turned on.  If it is not turned on, then
   this only copies the value pointed to into the buffer.  If multiple
   objects point to the same location, they will each have an
   independent copy after deserialization.
 */
template <class T>
class serialize_impl<T*, typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type>
{
    template <class A>
    friend class serialize;
    inline void operator()(T*& t, serializer& ser)
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
            ser.size(*t);
            break;
        case serializer::PACK:
            ser.primitive(*t);
            break;
        case serializer::UNPACK:
            t = new T();
            ser.primitive(*t);
            break;
        }
    }
};


/**
   Version of serialize that works for std::pair.
 */
template <class U, class V>
class serialize_impl<std::pair<U, V>>
{
    template <class A>
    friend class serialize;
    inline void operator()(std::pair<U, V>& t, serializer& ser)
    {
        serialize<U>()(t.first, ser);
        serialize<V>()(t.second, ser);
    }
};

// /**
//    Version of serialize that works for non-polymorphic types that have
//    a serialize_order function
//  */
// template <class T>
// class serialize_impl<T, typename std::enable_if<std::is_class<T>::value && !std::is_polymorphic<T>::value>::type>
// {
//     template <class A>
//     friend class serialize;
//     inline void operator()(T& t, serializer& ser)
//     {
//         t.serialize_order(ser);
//     }
// };

// All calls to serialize objects need to go through this function so
// that pointer tracking can be done
template <class T>
inline void
operator&(serializer& ser, T& t)
{
    serialize<T>()(t, ser);
}
template <class T>

inline void
operator|(serializer& ser, T& t)
{
    serialize<T>().serialize_and_track_pointer(t, ser);
}
} // namespace Serialization
} // namespace Core
} // namespace SST

#include "sst/core/serialization/serialize_array.h"
#include "sst/core/serialization/serialize_atomic.h"
#include "sst/core/serialization/serialize_deque.h"
#include "sst/core/serialization/serialize_list.h"
#include "sst/core/serialization/serialize_map.h"
#include "sst/core/serialization/serialize_priority_queue.h"
#include "sst/core/serialization/serialize_set.h"
#include "sst/core/serialization/serialize_string.h"
#include "sst/core/serialization/serialize_vector.h"

#endif // SST_CORE_SERIALIZATION_SERIALIZE_H
