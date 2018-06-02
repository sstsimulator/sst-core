// -*- c++ -*-

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

#ifndef SST_CORE_RANKINFO_H
#define SST_CORE_RANKINFO_H

#include <sst/core/serialization/serializable.h>

namespace SST {

class RankInfo : public SST::Core::Serialization::serializable {
public:
    static const uint32_t UNASSIGNED = (uint32_t)-1;
    uint32_t rank;
    uint32_t thread;

    RankInfo(uint32_t rank, uint32_t thread) :
        rank(rank), thread(thread)
    { }

    RankInfo() : rank(UNASSIGNED), thread(UNASSIGNED)
    { };

    bool isAssigned() const {
        return (rank != UNASSIGNED && thread != UNASSIGNED);
    }

    /**
     * @return true if other's rank and thread are less than ours
     */
    bool inRange(const RankInfo& other) const {
        return ((rank > other.rank) && (thread > other.thread));
    }

    bool operator==(const RankInfo& other) const {
        return (rank == other.rank) && (thread == other.thread);
    }

    bool operator!=(const RankInfo& other) const {
        return !(operator==(other));
    }

    bool operator<(const RankInfo& other) const {
        if ( rank == other.rank ) return thread < other.thread;
        return rank < other.rank;
    }

    bool operator<=(const RankInfo& other) const {
        if ( rank == other.rank ) return thread <= other.thread;
        return rank <= other.rank;
    }

    bool operator>(const RankInfo& other) const {
        if ( rank == other.rank ) return thread > other.thread;
        return rank > other.rank;
    }

    bool operator>=(const RankInfo& other) const {
        if ( rank == other.rank ) return thread >= other.thread;
        return rank >= other.rank;
    }

    void serialize_order(SST::Core::Serialization::serializer &ser) override
    {
        ser & rank;
        ser & thread;
    }

    
private:

    ImplementSerializable(SST::RankInfo)
};

}

#endif
