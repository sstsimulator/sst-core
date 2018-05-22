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

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <sst/core/serialization/serializer.h>
#include <sst/core/warnmacros.h>
//#include <sprockit/debug.h>
//DeclareDebugSlot(serialize);

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
class serialize {
public:
    inline void operator()(T& UNUSED(t), serializer& UNUSED(ser)){
        // If the default gets called, then it's actually invalid
        // because we don't know how to serialize it.
        
        // This is a bit strange, but if I just do a
        // static_assert(false) it always triggers, but if I use
        // std::is_* then it seems to only trigger if something expands
        // to this version of the template.
        // static_assert(false,"Trying to serialize an object that is not serializable.");
        static_assert(std::is_fundamental<T>::value,"Trying to serialize an object that is not serializable.");
        static_assert(!std::is_fundamental<T>::value,"Trying to serialize an object that is not serializable.");
    }
};

/**
   Version of serialize that works for fundamental types and enums.
 */
template <class T>
class serialize <T, typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type> {
public:
    inline void operator()(T& t, serializer& ser){
        ser.primitive(t); 
    }
};

// template <class U, class V>
// class serialize<std::pair<U,V>,
//                 typename std::enable_if<(std::is_fundamental<U>::value || std::is_enum<U>::value) &&
//                                         (std::is_fundamental<V>::value || std::is_enum<V>::value)>::type> {
// public:
//     inline void operator()(std::pair<U,V>& t, serializer& ser){
//         ser.primitive(t); 
//     }
// };


/**
   Version of serialize that works for bool.
 */
template <>
class serialize<bool> {
public:
    void operator()(bool &t, serializer& ser){
        int bval = t;
        ser.primitive(bval);
        t = bool(bval);
    }
};

/**
   Version of serialize that works for pointers to fundamental types
   and enums. Note that there is no pointer tracking.  This only
   copies the value pointed to into the buffer.  If multiple objects
   point to the same location, they will each have an independent copy
   after deserialization.
 */
template <class T>
class serialize<T*, typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type> {
public:
    inline void operator()(T*& t, serializer& ser){
        switch (ser.mode()){
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
class serialize<std::pair<U,V> > {
public:
    inline void operator()(std::pair<U,V>& t, serializer& ser){
        serialize<U>()(t.first,ser);
        serialize<V>()(t.second,ser);        
    }
};

template <class T>
inline void
operator&(serializer& ser, T& t){
    serialize<T>()(t, ser);
}

}
}
}

#include <sst/core/serialization/serialize_array.h>
#include <sst/core/serialization/serialize_deque.h>
#include <sst/core/serialization/serialize_list.h>
#include <sst/core/serialization/serialize_map.h>
#include <sst/core/serialization/serialize_set.h>
#include <sst/core/serialization/serialize_vector.h>
#include <sst/core/serialization/serialize_string.h>

#endif // SERIALIZE_H
