// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
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

#include "sst/core/serialization/chunked_buffer.h"
#include "sst/core/warnmacros.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
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

    // Current mode. Packing into and unpacking from a chunked_buffer are PACK and UNPACK modes, so serialization code
    // does not need to know which kind of buffer is being used.
    SERIALIZE_MODE
    mode() const { return mode_; }

    // Calling sizer(), packer(), unpacker() or mapper() when the corresponding mode is not active is flagged with a
    // std::bad_variant_access exception. packer() and unpacker() return the pointer tracking state of the active
    // packer or unpacker, which is the same whether it uses a single buffer or a chunked_buffer.
    pvt::ser_sizer&          sizer() { return std::get<SIZER>(ser_); }
    pvt::ser_pack_tracker&   packer();
    pvt::ser_unpack_tracker& unpacker();
    pvt::ser_mapper&         mapper() { return std::get<MAP>(ser_); }

    // Starting a new mode destroys the tracking object of the current mode and constructs a new tracking object
    void start_sizing() { start<SIZER, SIZER>(); }
    void start_packing(char* buffer, size_t size) { start<PACK, PACK>(buffer, size); }
    void start_unpacking(char* buffer, size_t size) { start<UNPACK, UNPACK>(buffer, size); }
    void start_mapping(ObjectMap* obj) { start<MAP, MAP>(obj); }

    // Packing into a chunked_buffer does not require a sizing pass: the chunked_buffer is cleared, and its chunks are
    // allocated and grown as needed to hold the data. Unpacking from a chunked_buffer reads the data in all of its
    // chunks. The chunked_buffer is owned by the caller and must remain valid until the serializer changes modes or is
    // finalized.
    void start_packing(chunked_buffer& buf) { start<PACK_GROWABLE, PACK>(buf); }
    void start_unpacking(const chunked_buffer& buf) { start<UNPACK_GROWABLE, UNPACK>(buf); }

    // finalize() destroys the tracking object of the current mode and switches to UNSTARTED mode. Functionally,
    // finalize() indicates the end of a serialization sequence, not just the end of the current mode. You do not
    // need to call finalize() if you are simply changing modes. finalize() can perform any final steps at the end
    // of a serialization sequence, such as destroying any std::shared_ptr tracking objects or generating a report.
    void finalize() { start<UNSTARTED, UNSTARTED>(); }

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
        if ( ser_.index() == PACK_GROWABLE )
            std::get<PACK_GROWABLE>(ser_).pack(t);
        else
            std::get<PACK>(ser_).pack(t);
    }

    template <typename T>
    void unpack(T& t)
    {
        if ( ser_.index() == UNPACK_GROWABLE )
            std::get<UNPACK_GROWABLE>(ser_).unpack(t);
        else
            std::get<UNPACK>(ser_).unpack(t);
    }

    template <typename T>
    void primitive(T& t)
    {
        static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>,
            "Error: ser.primitive() can only be called on trivially copyable, standard layout types.");
        switch ( mode() ) {
        case SIZER:
            active<SIZER>().size(t);
            break;
        case PACK:
            if ( ser_.index() == PACK_GROWABLE )
                active<PACK_GROWABLE>().pack(t);
            else
                active<PACK>().pack(t);
            break;
        case UNPACK:
            if ( ser_.index() == UNPACK_GROWABLE )
                active<UNPACK_GROWABLE>().unpack(t);
            else
                active<UNPACK>().unpack(t);
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
            active<SIZER>().size_buffer(buffer, size);
            break;
        case PACK:
            if ( ser_.index() == PACK_GROWABLE )
                active<PACK_GROWABLE>().pack_buffer(buffer, size);
            else
                active<PACK>().pack_buffer(buffer, size);
            break;
        case UNPACK:
            if ( ser_.index() == UNPACK_GROWABLE )
                active<UNPACK_GROWABLE>().unpack_buffer(buffer, size);
            else
                active<UNPACK>().unpack_buffer(buffer, size);
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
    // Variant indices of the packer and unpacker for a chunked_buffer, which are in PACK and UNPACK modes
    static constexpr size_t PACK_GROWABLE   = 5;
    static constexpr size_t UNPACK_GROWABLE = 6;

    // Default mode is UNSTARTED=0 with an empty std::monostate class. SIZER=1, PACK=2, UNPACK=3 and MAP=4 modes have
    // an associated tracking object which is constructed when starting the new mode. PACK_GROWABLE=5 and
    // UNPACK_GROWABLE=6 are the PACK and UNPACK modes using a chunked_buffer. finalize() destroys the current mode's
    // tracking object and goes back to UNSTARTED mode.
    std::variant<std::monostate, pvt::ser_sizer, pvt::ser_packer, pvt::ser_unpacker, pvt::ser_mapper,
        pvt::ser_packer_growable, pvt::ser_unpacker_growable>
        ser_;

    // Mode of the active ser_ alternative. It is stored separately since mode() is called very frequently.
    SERIALIZE_MODE mode_ = UNSTARTED;

    // Construct the tracking object at variant index INDEX, which is in mode MODE. mode_ stays UNSTARTED if the
    // construction throws, so mode_ always identifies an active alternative of ser_.
    template <size_t INDEX, SERIALIZE_MODE MODE, typename... ARGS>
    void start(ARGS&&... args)
    {
        mode_ = UNSTARTED;
        ser_.emplace<INDEX>(std::forward<ARGS>(args)...);
        mode_ = MODE;
    }

    // Unchecked access to the alternative at INDEX, which must be active. This is used after mode_ has been checked,
    // so that the variant index is not checked again in the most frequently used paths.
    template <size_t INDEX>
    auto& active()
    {
        if ( ser_.index() != INDEX ) __builtin_unreachable();
        return *std::get_if<INDEX>(&ser_);
    }

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
