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

#ifndef SST_CORE_TIMEVORTEX_H
#define SST_CORE_TIMEVORTEX_H

#include "sst/core/activityQueue.h"
#include "sst/core/module.h"
#include "sst/core/serialization/serialize_impl_fwd.h"

namespace SST {

class Output;
class Simulation_impl;

/**
 * Primary Event Queue
 */
class TimeVortex : public ActivityQueue
{
public:
    SST_ELI_DECLARE_BASE(TimeVortex)
    SST_ELI_DECLARE_INFO_EXTERN(ELI::ProvidesParams)
    SST_ELI_DECLARE_CTOR_EXTERN(SST::Params&)

    TimeVortex();
    ~TimeVortex() {}

    // Inherited from ActivityQueue
    virtual bool      empty() override                    = 0;
    virtual int       size() override                     = 0;
    virtual void      insert(Activity* activity) override = 0;
    virtual Activity* pop() override                      = 0;
    virtual Activity* front() override                    = 0;

    /** Print the state of the TimeVortex */
    virtual void     print(Output& out) const = 0;
    virtual uint64_t getMaxDepth() const { return max_depth; }
    virtual uint64_t getCurrentDepth() const = 0;
    virtual void     dbg_print(Output& out) { print(out); }

    // Functions for checkpointing
    virtual void serialize_order(SST::Core::Serialization::serializer& ser) { ser& max_depth; }
    virtual void fixup_handlers() {}

protected:
    uint64_t max_depth;

    void fixup(Activity* act);

private:
    Simulation_impl* sim_ = nullptr;
};

namespace TV {
namespace pvt {

void pack_timevortex(TimeVortex*& s, SST::Core::Serialization::serializer& ser);
void unpack_timevortex(TimeVortex*& s, SST::Core::Serialization::serializer& ser);

} // namespace pvt
} // namespace TV

template <>
class SST::Core::Serialization::serialize_impl<TimeVortex*>
{

    template <class A>
    friend class serialize;
    void operator()(TimeVortex*& s, SST::Core::Serialization::serializer& ser)
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        case serializer::PACK:
            TV::pvt::pack_timevortex(s, ser);
            break;
        case serializer::UNPACK:
            TV::pvt::unpack_timevortex(s, ser);
            break;
        }
    }
};

} // namespace SST

#endif // SST_CORE_TIMEVORTEX_H
