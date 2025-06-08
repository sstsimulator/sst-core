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

#ifndef SST_CORE_SERIALIZATION_SERIALIZER_H
#define SST_CORE_SERIALIZATION_SERIALIZER_H

// These includes have guards to print warnings if they are included
// independent of this file.  Set the #define that will disable the warnings.
#define SST_INCLUDING_SERIALIZER_H
#include "sst/core/serialization/impl/mapper.h"
#include "sst/core/serialization/impl/packer.h"
#include "sst/core/serialization/impl/sizer.h"
#include "sst/core/serialization/impl/unpacker.h"
// Reenble warnings for including the above file independent of this file.
#undef SST_INCLUDING_SERIALIZER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

namespace SST::Core::Serialization {

class ObjectMap;
class ObjectMapContext;

/**
 * This class is basically a wrapper for objects to declare the order in
 * which their members should be ser/des
 */
class serializer
{
public:
    enum SERIALIZE_MODE { SIZER, PACK, UNPACK, MAP };

    pvt::ser_mapper&   mapper() { return std::get<MAP>(ser_); }
    pvt::ser_packer&   packer() { return std::get<PACK>(ser_); }
    pvt::ser_unpacker& unpacker() { return std::get<UNPACK>(ser_); }
    pvt::ser_sizer&    sizer() { return std::get<SIZER>(ser_); }

    template <class T>
    void size(T& t)
    {
        sizer().size(t);
    }

    template <class T>
    void pack(T& t)
    {
        packer().pack(t);
    }

    template <class T>
    void unpack(T& t)
    {
        unpacker().unpack(t);
    }

    virtual ~serializer() = default;

    SERIALIZE_MODE
    mode() const { return static_cast<SERIALIZE_MODE>(ser_.index()); }

    [[deprecated("SST::Core::Serialization::serializer::set_mode() is deprecated and will be removed. start_sizing(), "
                 "start_packing(), start_unpacking(), or start_mapping() should be used instead.")]]
    void set_mode(SERIALIZE_MODE mode);

    [[deprecated("SST::Core::Serialization::serializer::reset() is deprecated and will be removed.")]]
    void reset();

    template <typename T>
    void primitive(T& t)
    {
        switch ( mode() ) {
        case SIZER:
            sizer().size(t);
            break;
        case PACK:
            packer().pack(t);
            break;
        case UNPACK:
            unpacker().unpack(t);
            break;
        case MAP:
            break;
        }
    }

    void raw(void* data, size_t size);

    template <typename ELEM_T, typename SIZE_T>
    void binary(ELEM_T*& buffer, SIZE_T& size)
    {
        switch ( mode() ) {
        case SIZER:
            sizer().add(sizeof(SIZE_T));
            sizer().add(size * sizeof(ELEM_T));
            break;
        case PACK:
            if ( buffer ) {
                packer().pack(size);
                packer().pack_buffer(buffer, size * sizeof(ELEM_T));
            }
            else {
                SIZE_T nullsize = 0;
                packer().pack(nullsize);
            }
            break;
        case UNPACK:
            unpacker().unpack(size);
            buffer = nullptr;
            if ( size ) unpacker().unpack_buffer(&buffer, size * sizeof(ELEM_T));
            break;
        case MAP:
            break;
        }
    }

    // For void*, we get sizeof(), which errors.
    // Create a wrapper that casts to char* and uses above
    template <typename Int>
    void binary(void*& buffer, Int& size)
    {
        binary(reinterpret_cast<char*&>(buffer), size);
    }

    size_t     size();
    void       string(std::string& str);
    void       start_sizing() { ser_.emplace<SIZER>(); }
    void       start_packing(char* buffer, size_t size) { ser_.emplace<PACK>(buffer, size); }
    void       start_unpacking(char* buffer, size_t size) { ser_.emplace<UNPACK>(buffer, size); }
    void       start_mapping(ObjectMap* obj) { ser_.emplace<MAP>(obj); }
    bool       check_pointer_sizer(uintptr_t ptr) { return sizer().check_pointer_sizer(ptr); }
    bool       check_pointer_pack(uintptr_t ptr) { return packer().check_pointer_pack(ptr); }
    void       report_new_pointer(uintptr_t real_ptr) { unpacker().report_new_pointer(real_ptr); }
    void       report_real_pointer(uintptr_t ptr, uintptr_t real_ptr) { unpacker().report_real_pointer(ptr, real_ptr); }
    uintptr_t  check_pointer_unpack(uintptr_t ptr) { return unpacker().check_pointer_unpack(ptr); }
    ObjectMap* check_pointer_map(uintptr_t ptr) { return mapper().check_pointer_map(ptr); }
    void       report_object_map(ObjectMap* ptr) { mapper().report_object_map(ptr); }
    void       enable_pointer_tracking(bool value = true) { enable_ptr_tracking_ = value; }
    bool       is_pointer_tracking_enabled() { return enable_ptr_tracking_; }
    const char* getMapName() const;

private:
    std::variant<pvt::ser_sizer, pvt::ser_packer, pvt::ser_unpacker, pvt::ser_mapper> ser_;

    bool                    enable_ptr_tracking_ = false;
    const ObjectMapContext* mapContext           = nullptr;
    friend class ObjectMapContext;

    size_t shared_ptr_tag = 0;  // Unique tag for each std::shared_ptr owner
    std::map<std::weak_ptr<const void>, size_t, std::owner_less<>> shared_ptr_map;
    std::map<size_t, std::shared_ptr<void>> shared_ptr_reverse_map;
    friend class serialize_shared_ptr_impl;
}; // class serializer

/**
   ObjectMap context which is saved in a virtual stack when name or other information changes.
   When ObjectMapContext is destroyed, the serializer goes back to the previous ObjectMapContext.
 */

class ObjectMapContext
{
    serializer&                   ser;
    const ObjectMapContext* const prevContext;
    const char* const             name;

public:
    ObjectMapContext(serializer& ser, const char* name) :
        ser(ser),
        prevContext(ser.mapContext),
        name(name)
    {
        DISABLE_WARN_DANGLING_POINTER // GCC 13 bug causes spurious warning
            ser.mapContext = this;    // change the serializer's context to this new one
        REENABLE_WARNING
    }
    ~ObjectMapContext() { ser.mapContext = prevContext; } // restore the serializer's old context
    const char* getName() const { return name; }
}; // class ObjectMapContext

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_SERIALIZER_H
