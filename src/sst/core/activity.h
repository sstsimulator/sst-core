// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_ACTIVITY_H
#define SST_CORE_ACTIVITY_H

#include "sst/core/mempool.h"
#include "sst/core/output.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <cinttypes>
#include <cstring>
#include <errno.h>
#include <sstream>
#include <unordered_map>

// Default Priority Settings
#define THREADSYNCPRIORITY     20
#define SYNCPRIORITY           25
#define STOPACTIONPRIORITY     30
#define CLOCKPRIORITY          40
#define EVENTPRIORITY          50
#define MEMEVENTPRIORITY       50
#define BARRIERPRIORITY        75
#define ONESHOTPRIORITY        80
#define STATISTICCLOCKPRIORITY 85
#define FINALEVENTPRIORITY     98
#define EXITPRIORITY           99

extern int main(int argc, char** argv);

namespace SST {

/** Base class for all Activities in the SST Event Queue */
class Activity : public SST::Core::MemPoolItem
{
public:
    Activity() : delivery_time(0), priority_order(0), queue_order(0) {}
    virtual ~Activity() {}

    /**
       Class to use as the less than operator for STL functions or
       sorting algorithms.  If a template parameter is set to true,
       then that variable will be included in the comparison.  The
       parameters are: T - delivery time, P - priority and order tag,
       Q - queue order.
     */
    template <bool T, bool P, bool Q>
    class less
    {
    public:
        inline bool operator()(const Activity* lhs, const Activity* rhs) const
        {
            if ( T && lhs->delivery_time != rhs->delivery_time ) return lhs->delivery_time < rhs->delivery_time;
            if ( P && lhs->priority_order != rhs->priority_order ) return lhs->priority_order < rhs->priority_order;
            return Q && lhs->queue_order < rhs->queue_order;
        }

        // // Version without branching.  Still need to test to see if
        // // this is faster than the above implementation for "real"
        // // simulations.  For the sst-benchmark, the above is slightly
        // // faster.  Uses the bitwise operator because there are no
        // // early outs.  For bools, this is logically equivalent to the
        // // logical operators
        // inline bool operator()(const Activity* lhs, const Activity* rhs) const
        // {
        //     return ( T & lhs->delivery_time < rhs->delivery_time ) |
        //         (P & lhs->delivery_time == rhs->delivery_time & lhs->priority_order < rhs->priority_order) |
        //         (Q & lhs->delivery_time == rhs->delivery_time & lhs->priority_order == rhs->priority_order &
        //         lhs->queue_order < rhs->queue_order);
        // }

        // // Version without ifs, but with early outs in the logic.
        // inline bool operator()(const Activity* lhs, const Activity* rhs) const
        // {
        //     return ( T && lhs->delivery_time < rhs->delivery_time ) |
        //         (P && lhs->delivery_time == rhs->delivery_time && lhs->priority_order < rhs->priority_order) |
        //         (Q && lhs->delivery_time == rhs->delivery_time && lhs->priority_order == rhs->priority_order &&
        //         lhs->queue_order < rhs->queue_order);
        // }
    };

    /**
       Class to use as the greater than operator for STL functions or
       sorting algorithms (used if you want to sort opposite the
       natural soring order).  If a template parameter is set to true,
       then that variable will be included in the comparison.  The
       parameters are: T - delivery time, P - priority and order tag,
       Q - queue order.
     */
    template <bool T, bool P, bool Q>
    class greater
    {
    public:
        inline bool operator()(const Activity* lhs, const Activity* rhs) const
        {
            if ( T && lhs->delivery_time != rhs->delivery_time ) return lhs->delivery_time > rhs->delivery_time;
            if ( P && lhs->priority_order != rhs->priority_order ) return lhs->priority_order > rhs->priority_order;
            return Q && lhs->queue_order > rhs->queue_order;
        }

        // // Version without branching.  Still need to test to see if
        // // this is faster than the above implementation for "real"
        // // simulations.  For the sst-benchmark, the above is slightly
        // // faster.  Uses the bitwise operator because there are no
        // // early outs.  For bools, this is logically equivalent to the
        // // logical operators
        // inline bool operator()(const Activity* lhs, const Activity* rhs) const
        // {
        //     return ( T & lhs->delivery_time > rhs->delivery_time ) |
        //         (P & lhs->delivery_time == rhs->delivery_time & lhs->priority_order > rhs->priority_order) |
        //         (Q & lhs->delivery_time == rhs->delivery_time & lhs->priority_order == rhs->priority_order &
        //         lhs->queue_order > rhs->queue_order);
        // }

        // // Version without ifs, but with early outs in the logic.
        // inline bool operator()(const Activity* lhs, const Activity* rhs) const
        // {
        //     return ( T && lhs->delivery_time > rhs->delivery_time ) |
        //         (P && lhs->delivery_time == rhs->delivery_time && lhs->priority_order > rhs->priority_order) |
        //         (Q && lhs->delivery_time == rhs->delivery_time && lhs->priority_order == rhs->priority_order &&
        //         lhs->queue_order > rhs->queue_order);
        // }
    };


    /** Function which will be called when the time for this Activity comes to pass. */
    virtual void execute(void) = 0;

    /** Set the time for which this Activity should be delivered */
    inline void setDeliveryTime(SimTime_t time) { delivery_time = time; }

    /** Return the time at which this Activity will be delivered */
    inline SimTime_t getDeliveryTime() const { return delivery_time; }

    /** Return the Priority of this Activity */
    inline int getPriority() const { return (int)(priority_order >> 32); }

    /** Sets the order tag */
    inline void setOrderTag(uint32_t tag) { priority_order = (priority_order & 0xFFFFFFFF00000000ul) | (uint64_t)tag; }

    /** Return the order tag associated with this activity */
    inline uint32_t getOrderTag() const { return (uint32_t)(priority_order & 0xFFFFFFFFul); }

    /** Returns the queue order associated with this activity */
    inline uint64_t getQueueOrder() const { return queue_order; }

    /** Get a string represenation of the event.  The default version
     * will just use the name of the class, retrieved through the
     * cls_name() function inherited from the serialzable class, which
     * will return the name of the last class to call one of the
     * serialization macros (ImplementSerializable(),
     * ImplementVirtualSerializable(), or NotSerializable()).
     * Subclasses can override this function if they want to add
     * additional information.
     */
    std::string toString() const override
    {
        std::stringstream buf;

        buf << cls_name() << " to be delivered at " << getDeliveryTimeInfo();
        return buf.str();
    }

#ifdef __SST_DEBUG_EVENT_TRACKING__
    virtual void printTrackingInfo(const std::string& UNUSED(header), Output& UNUSED(out)) const {}
#endif

protected:
    /** Set the priority of the Activity */
    void setPriority(uint64_t priority) { priority_order = (priority_order & 0x00000000FFFFFFFFul) | (priority << 32); }

    /**
       Gets the delivery time info as a string.  To be used in
       inherited classes if they'd like to overwrite the default print
       or toString()
     */
    std::string getDeliveryTimeInfo() const
    {
        std::stringstream buf;
        buf << "time: " << delivery_time << ", priority: " << getPriority() << ", order tag: " << getOrderTag()
            << ", queue order: " << getQueueOrder();
        return buf.str();
    }

    // Function used by derived classes to serialize data members.
    // This class is not serializable, because not all class that
    // inherit from it need to be serializable.
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& delivery_time;
        ser& priority_order;
        ser& queue_order;
    }
    ImplementVirtualSerializable(SST::Activity)


        /** Set a new Queue order */
        void setQueueOrder(uint64_t order)
    {
        queue_order = order;
    }

private:
    // Data members
    SimTime_t delivery_time;
    // This will hold both the priority (high bits) and the link order
    // (low_bits)
    uint64_t  priority_order;
    // Used for TimeVortex implementations that don't naturally keep
    // the insertion order
    uint64_t  queue_order;
};

} // namespace SST

#endif // SST_CORE_ACTIVITY_H
