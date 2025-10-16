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

#include "sst/core/warnmacros.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
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
    enum SERIALIZE_MODE : size_t { SIZER = 1, PACK, UNPACK, MAP };
    static constexpr SERIALIZE_MODE UNSTARTED { 0 }; // Defined outside of enum to avoid warnings about switch cases

    // Current mode is the index of the ser_ variant
    SERIALIZE_MODE
    mode() const { return SERIALIZE_MODE(ser_.index()); }

    // Calling sizer(), packer(), unpacker() or mapper() when the corresponding mode is not active is flagged with a
    // std::bad_variant_access exception.
    pvt::ser_sizer&    sizer() { return std::get<SIZER>(ser_); }
    pvt::ser_packer&   packer() { return std::get<PACK>(ser_); }
    pvt::ser_unpacker& unpacker() { return std::get<UNPACK>(ser_); }
    pvt::ser_mapper&   mapper() { return std::get<MAP>(ser_); }

    // Starting a new mode destroys the tracking object of the current mode and constructs a new tracking object
    void start_sizing() { ser_.emplace<SIZER>(); }
    void start_packing(char* buffer, size_t size) { ser_.emplace<PACK>(buffer, size); }
    void start_unpacking(char* buffer, size_t size) { ser_.emplace<UNPACK>(buffer, size); }
    void start_mapping(ObjectMap* obj) { ser_.emplace<MAP>(obj); }

    // finalize() destroys the tracking object of the current mode and switches to UNSTARTED mode. Functionally,
    // finalize() indicates the end of a serialization sequence, not just the end of the current mode. You do not
    // need to call finalize() if you are simply changing modes. finalize() can perform any final steps at the end
    // of a serialization sequence, such as destroying any std::shared_ptr tracking objects or generating a report.
    void finalize() { ser_.emplace<UNSTARTED>(); }

    // Standard operators are defaulted or deleted
    serializer()                             = default;
    serializer(const serializer&)            = delete;
    serializer& operator=(const serializer&) = delete;
    ~serializer()                            = default;

    template <typename T>
    void size(T& t)
    {
        sizer().size(t);
    }

    template <typename T>
    void pack(T& t)
    {
        packer().pack(t);
    }

    template <typename T>
    void unpack(T& t)
    {
        unpacker().unpack(t);
    }

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

    template <typename ELEM_T, typename SIZE_T>
    void binary(ELEM_T*& buffer, SIZE_T& size)
    {
        switch ( mode() ) {
        case SIZER:
            sizer().size_buffer(buffer, size);
            break;
        case PACK:
            packer().pack_buffer(buffer, size);
            break;
        case UNPACK:
            unpacker().unpack_buffer(buffer, size);
            break;
        case MAP:
            break;
        }
    }

    void   raw(void* data, size_t size);
    size_t size();
    void   string(std::string& str);

    void        enable_pointer_tracking(bool value = true) { enable_ptr_tracking_ = value; }
    bool        is_pointer_tracking_enabled() const { return enable_ptr_tracking_; }
    const char* getMapName() const;

private:
    // Default mode is UNSTARTED=0 with an empty std::monostate class. SIZER=1, PACK=2, UNPACK=3 and MAP=4 modes have
    // an associated tracking object which is constructed when starting the new mode. finalize() destroys the current
    // mode's tracking object and goes back to UNSTARTED mode.
    std::variant<std::monostate, pvt::ser_sizer, pvt::ser_packer, pvt::ser_unpacker, pvt::ser_mapper> ser_;

    bool                    enable_ptr_tracking_ = false;
    const ObjectMapContext* mapContext           = nullptr;
    friend class ObjectMapContext;
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
