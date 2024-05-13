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

#ifndef SST_CORE_SERIALIZATION_SERIALIZER_H
#define SST_CORE_SERIALIZATION_SERIALIZER_H

#include "sst/core/serialization/serialize_packer.h"
#include "sst/core/serialization/serialize_sizer.h"
#include "sst/core/serialization/serialize_unpacker.h"

#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <set>
#include <typeinfo>
#include <vector>

namespace SST {
namespace Core {
namespace Serialization {

/**
 * This class is basically a wrapper for objects to declare the order in
 * which their members should be ser/des
 */
class serializer
{
public:
    typedef enum { SIZER, PACK, UNPACK } SERIALIZE_MODE;

public:
    serializer() : mode_(SIZER) // just sizing by default
    {}

    pvt::ser_packer& packer() { return packer_; }

    pvt::ser_unpacker& unpacker() { return unpacker_; }

    pvt::ser_sizer& sizer() { return sizer_; }

    template <class T>
    void size(T& t)
    {
        sizer_.size<T>(t);
    }

    template <class T>
    void pack(T& t)
    {
        packer_.pack<T>(t);
    }

    template <class T>
    void unpack(T& t)
    {
        unpacker_.unpack<T>(t);
    }

    virtual ~serializer() {}

    SERIALIZE_MODE
    mode() const { return mode_; }

    void set_mode(SERIALIZE_MODE mode) { mode_ = mode; }

    void reset()
    {
        sizer_.reset();
        packer_.reset();
        unpacker_.reset();
    }

    template <typename T>
    void primitive(T& t)
    {
        switch ( mode_ ) {
        case SIZER:
            sizer_.size(t);
            break;
        case PACK:
            packer_.pack(t);
            break;
        case UNPACK:
            unpacker_.unpack(t);
            break;
        }
    }

    template <class T, int N>
    void array(T arr[N])
    {
        switch ( mode_ ) {
        case SIZER:
        {
            sizer_.add(sizeof(T) * N);
            break;
        }
        case PACK:
        {
            char* charstr = packer_.next_str(N * sizeof(T));
            ::memcpy(charstr, arr, N * sizeof(T));
            break;
        }
        case UNPACK:
        {
            char* charstr = unpacker_.next_str(N * sizeof(T));
            ::memcpy(arr, charstr, N * sizeof(T));
            break;
        }
        }
    }

    template <typename T, typename Int>
    void binary(T*& buffer, Int& size)
    {
        switch ( mode_ ) {
        case SIZER:
        {
            sizer_.add(sizeof(Int));
            sizer_.add(size);
            break;
        }
        case PACK:
        {
            if ( buffer ) {
                packer_.pack(size);
                packer_.pack_buffer(buffer, size * sizeof(T));
            }
            else {
                Int nullsize = 0;
                packer_.pack(nullsize);
            }
            break;
        }
        case UNPACK:
        {
            unpacker_.unpack(size);
            if ( size != 0 ) { unpacker_.unpack_buffer(&buffer, size * sizeof(T)); }
            else {
                buffer = nullptr;
            }
            break;
        }
        }
    }

    // For void*, we get sizeof(void), which errors.
    // Create a wrapper that casts to char* and uses above
    template <typename Int>
    void binary(void*& buffer, Int& size)
    {
        char* tmp = (char*)buffer;
        binary<char>(tmp, size);
        buffer = tmp;
    }

    void string(std::string& str);

    void start_packing(char* buffer, size_t size)
    {
        packer_.init(buffer, size);
        mode_ = PACK;
        ser_pointer_set.clear();
        ser_pointer_map.clear();
    }

    void start_sizing()
    {
        sizer_.reset();
        mode_ = SIZER;
        ser_pointer_set.clear();
        ser_pointer_map.clear();
    }

    void start_unpacking(char* buffer, size_t size)
    {
        unpacker_.init(buffer, size);
        mode_ = UNPACK;
        ser_pointer_set.clear();
        ser_pointer_map.clear();
    }

    size_t size() const
    {
        switch ( mode_ ) {
        case SIZER:
            return sizer_.size();
        case PACK:
            return packer_.size();
        case UNPACK:
            return unpacker_.size();
        }
        return 0;
    }

    inline bool check_pointer_pack(uintptr_t ptr)
    {
        if ( ser_pointer_set.count(ptr) == 0 ) {
            ser_pointer_set.insert(ptr);
            return false;
        }
        return true;
    }

    inline uintptr_t check_pointer_unpack(uintptr_t ptr)
    {
        auto it = ser_pointer_map.find(ptr);
        if ( it != ser_pointer_map.end() ) { return it->second; }
        // Keep a copy of the ptr in case we have a split report
        split_key = ptr;
        return 0;
    }

    inline void report_new_pointer(uintptr_t real_ptr) { ser_pointer_map[split_key] = real_ptr; }

    inline void report_real_pointer(uintptr_t ptr, uintptr_t real_ptr) { ser_pointer_map[ptr] = real_ptr; }

    void enable_pointer_tracking(bool value = true) { enable_ptr_tracking_ = value; }

    inline bool is_pointer_tracking_enabled() { return enable_ptr_tracking_; }

protected:
    // only one of these is going to be valid for this serializer
    // not very good class design, but a little more convenient
    pvt::ser_packer   packer_;
    pvt::ser_unpacker unpacker_;
    pvt::ser_sizer    sizer_;
    SERIALIZE_MODE    mode_;
    bool              enable_ptr_tracking_ = false;

    std::set<uintptr_t>            ser_pointer_set;
    std::map<uintptr_t, uintptr_t> ser_pointer_map;
    uintptr_t                      split_key;
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_SERIALIZER_H
