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

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_H
#define SST_CORE_SERIALIZATION_SERIALIZE_H

#include "sst/core/from_string.h"
#include "sst/core/serialization/objectMap.h"
#include "sst/core/serialization/serializer.h"
#include "sst/core/warnmacros.h"

#include <atomic>
#include <iostream>
#include <type_traits>
#include <typeinfo>

namespace SST::Core::Serialization {

/**
   Base serialize class.

   This class also acts as the default case, which if hit will check
   to see if this is an otherwise unhandled non-polymorphic class. If
   it is, it will just attempt to call the serialize_order() function.
   All other instances are partial specializations of this class and
   do all the real serialization.
 */
template <typename T, typename = void>
class serialize_impl
{
    template <typename, typename = void>
    struct has_serialize_order : std::false_type
    {};

    template <typename U>
    struct has_serialize_order<
        U, std::void_t<decltype(std::declval<U>().serialize_order(std::declval<serializer&>()))>> : std::true_type
    {};

    static_assert(
        std::is_class_v<std::remove_pointer_t<T>> && !std::is_polymorphic_v<std::remove_pointer_t<T>>,
        "Trying to serialize an object that is not serializable.");

    static_assert(
        has_serialize_order<std::remove_pointer_t<T>>::value,
        "Serializable class does not have serialize_order() method");

public:
    void operator()(T& t, serializer& ser)
    {
        // tPtr is a reference to either t if it's a pointer, or to &t if it's not
        const auto& tPtr = [&]() -> decltype(auto) {
            if constexpr ( std::is_pointer_v<T> )
                return t;
            else
                return &t;
        }();

        const auto mode = ser.mode();
        if ( mode == serializer::MAP ) {
            if ( !tPtr ) return; // No need to map a nullptr
            auto* map = new ObjectMapClass(tPtr, typeid(T).name());
            ser.report_object_map(map);
            ser.mapper().map_hierarchy_start(ser.getMapName(), map);
        }
        else if constexpr ( std::is_pointer_v<T> ) {
            if ( mode == serializer::UNPACK ) {
                t = new std::remove_pointer_t<T>();
                ser.report_new_pointer(reinterpret_cast<uintptr_t>(t));
            }
        }

        // class needs to define a serialize_order() method
        tPtr->serialize_order(ser);

        if ( mode == serializer::MAP ) ser.mapper().map_hierarchy_end();
    }
};

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
    void operator()(T& t, serializer& ser) { return serialize_impl<T>()(t, ser); }

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
        case serializer::MAP:
            // Add your code here
            break;
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
        if ( !ser.is_pointer_tracking_enabled() ) {
            // Handle nullptr
            char null_char = (nullptr == t ? 0 : 1);
            switch ( ser.mode() ) {
            case serializer::SIZER:
                // We will always put in a char to tell whether or not
                // this is nullptr.
                ser.size(null_char);

                // If this is a nullptr, then we are done
                if ( null_char == 0 ) return;

                // Not nullptr, so we need to serialize the object
                serialize_impl<T*>()(t, ser);
                break;
            case serializer::PACK:
                // We will always put in a char to tell whether or not
                // this is nullptr.
                ser.pack(null_char);

                // If this is a nullptr, then we are done
                if ( null_char == 0 ) return;

                // Not nullptr, so we need to serialize the object
                serialize_impl<T*>()(t, ser);
                break;
            case serializer::UNPACK:
            {
                // Get the ptr and check to see if we've already deserialized
                ser.unpack(null_char);

                // Check to see if this was a nullptr
                if ( 0 == null_char ) {
                    t = nullptr;
                    return;
                }
                // Not nullptr, so deserialize
                serialize_impl<T*>()(t, ser);
            }
            case serializer::MAP:
                // If this version of serialize gets called in mapping
                // mode, there is nothing to do
                break;
            }
            return;
        }

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
        {
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
            break;
        }
        case serializer::MAP:
        {
            ObjectMap* map = ser.check_pointer_map(reinterpret_cast<uintptr_t>(t));
            if ( map != nullptr ) {
                // If we've already seen this object, just add the
                // existing ObjectMap to the parent.
                ser.mapper().map_existing_object(ser.getMapName(), map);
            }
            else {
                serialize_impl<T*>()(t, ser);
            }
            break;
        }
        }
    }
};


/**
   Version of serialize that works for fundamental types and enums.
 */

template <class T>
class serialize_impl<T, std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>>>
{
public:
    void operator()(T& t, serializer& ser)
    {
        if ( ser.mode() == serializer::MAP ) {
            auto* obj_map = new ObjectMapFundamental<T>(&t);
            ser.mapper().map_primitive(ser.getMapName(), obj_map);
        }
        else {
            ser.primitive(t);
        }
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
class serialize_impl<T*, std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>>>
{
    template <typename>
    friend class serialize;
    void operator()(T*& t, serializer& ser)
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
        case serializer::MAP:
            // Add your code here
            break;
        }
    }
};

// All calls to serialize objects need to go through one of the
// following functions, or through the serialize<T> template so that
// pointer tracking can be done.
template <class T>
void
sst_map_object(serializer& ser, T& t, std::string name = "")
{
    if ( ser.mode() == serializer::MAP ) {
        if ( !name.empty() ) { // Do nothing if name is empty
            ser.pushMapName(std::move(name));
            serialize<T>()(t, ser);
            ser.popMapName();
        }
    }
    else {
        serialize<T>()(t, ser);
    }
}

template <class T>
void
operator&(serializer& ser, T& obj)
{
    sst_map_object(ser, obj);
}

template <class T>
void
operator|(serializer& ser, T& obj)
{
    serialize<T>().serialize_and_track_pointer(obj, ser);
}

// Serialization macros for checkpoint/debug serialization
#define SST_SER(obj)        sst_map_object(ser, (obj), #obj);
#define SST_SER_AS_PTR(obj) (ser | (obj));

} // namespace SST::Core::Serialization

// These includes have guards to print warnings if they are included
// independent of this file.  Set the #define that will disable the
// warnings.
#define SST_INCLUDING_SERIALIZE_H
#include "sst/core/serialization/impl/serialize_adapter.h"
#include "sst/core/serialization/impl/serialize_array.h"
#include "sst/core/serialization/impl/serialize_atomic.h"
#include "sst/core/serialization/impl/serialize_insertable.h"
#include "sst/core/serialization/impl/serialize_string.h"
#include "sst/core/serialization/impl/serialize_tuple.h"

// Reenble warnings for including the above file independent of this
// file.
#undef SST_INCLUDING_SERIALIZE_H

#endif // SST_CORE_SERIALIZATION_SERIALIZE_H
