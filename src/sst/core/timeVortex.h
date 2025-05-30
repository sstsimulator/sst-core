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

#ifndef SST_CORE_TIMEVORTEX_H
#define SST_CORE_TIMEVORTEX_H

#include "sst/core/activity.h"
#include "sst/core/activityQueue.h"
#include "sst/core/module.h"
#include "sst/core/serialization/serialize_impl_fwd.h"

#include <cstdint>
#include <vector>

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
    virtual void     print(Output& out) const;
    virtual uint64_t getMaxDepth() const { return max_depth; }
    virtual uint64_t getCurrentDepth() const = 0;
    virtual void     dbg_print(Output& out) const { print(out); }

    // Functions for checkpointing
    virtual void serialize_order(SST::Core::Serialization::serializer& ser) { SST_SER(max_depth); }

    /**
       Get a copy of the contents of the TimeVortex

       @return vector with a copy of the contents
     */
    virtual void getContents(std::vector<Activity*>& activities) const = 0;

protected:
    uint64_t max_depth;
};

namespace TV::pvt {

void pack_timevortex(TimeVortex*& s, SST::Core::Serialization::serializer& ser);
void unpack_timevortex(TimeVortex*& s, SST::Core::Serialization::serializer& ser);

} // namespace TV::pvt
} // namespace SST

#endif // SST_CORE_TIMEVORTEX_H
