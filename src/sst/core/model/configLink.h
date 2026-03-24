// -*- c++ -*-

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

#ifndef SST_CORE_MODEL_CONFIGLINK_H
#define SST_CORE_MODEL_CONFIGLINK_H

#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace SST {
namespace Core::Serialization {
class serializer;
}

/** Represents the configuration of a generic Link */
class ConfigLink
{
    // Static data structures to map latency string to an index that
    // will hold the SimTime_t for the latency once the atomic
    // timebase has been set.
    static std::map<std::string, uint32_t> lat_to_index;

    static uint32_t               getIndexForLatency(const char* latency);
    static std::vector<SimTime_t> initializeLinkLatencyVector();
    SimTime_t                     getLatencyFromIndex(uint32_t index);

    /********* vv Member variables vv ***********/

public:
    /**
       Components that are connected to this link. They are filled in
       the order they are attached in.  If the link is marked as
       non-local, then component[1] holds the rank of the remote
       component.
    */
    ComponentId_t component_[2] = { UNSET_COMPONENT_ID, UNSET_COMPONENT_ID }; /*!< IDs of the connected components */

    /**
       This is a dual purpose data member.

       Graph construction - during graph construction, it holds the
       index into the LinkLatencyVector, which maps the stored index
       to the actual latency string.  This is done to reduce memory
       usage because we assume there will be a small number of
       different latencies specified.

       Post graph construction - after graph construction, the latency
       index is replaced with the SimTime_t value representing the
       specified latency that will be used by the link to add the
       specified latency to the event.

       In both cases, the indices match the indices found in the
       component array, and represent the latency of the link for
       events sent from the corresponding component.

       If the link is marked as non-local, then latency[1] holds the
       thread of the remote component.
    */
    SimTime_t latency_[2] = { 0, 0 };

    /**
       Name of the link.  This is used in two ways:

       Errors - if an error is found in the graph construction, the
       name is used to report the error to the user

       Parallel load - for parallel load, links that cross partition
       boundaries are connected by matching link names

       Link names are not carried over to the simulation objects
    */
    std::string name_;

    /**
       id of the link.  This is used primarily to find the link in ConfigGraph::links.  For parallel loads, the link ID
       is only unique on a given rank, not globally.

       The ID's are not carried over to the simulation objects
    */
    LinkId_t id_ = 0;

    /**
       Name of the ports the link is connected to.  The indices match
       the ones used in the component array
    */
    std::string port_[2];

    /**
       Whether or not this link is set to be no-cut
    */
    bool no_cut_ = false;

    /**
       Whether this link crosses the graph boundary and is connected
       on one end to a non-local component.  If set to true, there
       will only be one component connected (information in index 0
       for the arrays) and the rank for the remote component will be
       stored in component[1] and the thread in latency[1].
    */
    bool nonlocal_ = false;

    /**
       Set to true if this is a cross rank link
    */
    bool cross_rank_ = false;

    /**
       Set to true if this is a cross thread link on same rank
    */
    bool cross_thread_ = false;

    /********* ^^ Member variables ^^ ***********/


    /**
       Function used when storing ConfigLinks in a SparseVectorMap
    */
    inline LinkId_t key() const { return id_; }

    /**
       Return the minimum latency of this link (from both sides).  For non local links, it will return the local latency
    */
    SimTime_t getMinLatency() const
    {
        if ( nonlocal_ ) return latency_[0];
        if ( latency_[0] < latency_[1] ) return latency_[0];
        return latency_[1];
    }

    /**
       Gets the latency as a string from the id stored in latency[].  This is only allowed to be called during graph
       construction.  After post-graph construction checks, the latency[] array no longer hold indices to the strings.

       @see latency_
    */
    std::string latency_str(uint32_t index) const;

    /**
       Sets the link as a non-local link.  After the call, the local information will be held in index 0 of the various
       arrays, regardless of which component index holds the local information before the call.

       @param which_local specifies which index is for the local side of the link

       @param remote_rank_info Rank of the remote side of the link
    */
    void setAsNonLocal(int which_local, RankInfo remote_rank_info);

    /**
       Print the Link information
    */
    void print(std::ostream& os) const
    {
        os << "Link " << name_ << " (id = " << id_ << ")" << std::endl;
        os << "  nonlocal = " << nonlocal_ << std::endl;
        os << "  component[0] = " << component_[0] << std::endl;
        os << "  port[0] = " << port_[0] << std::endl;
        os << "  latency[0] = " << latency_[0] << std::endl;
        os << "  component[1] = " << component_[1] << std::endl;
        os << "  port[1] = " << port_[1] << std::endl;
        os << "  latency[1] = " << latency_[1] << std::endl;
    }

    /* Do not use.  For serialization only */
    ConfigLink() {}

    /**
       Serializes the ConfigLink
     */
    void serialize_order(SST::Core::Serialization::serializer& ser) /*override*/
    {
        SST_SER(id_);
        SST_SER(name_);
        SST_SER(component_);
        SST_SER(port_);
        SST_SER(latency_);
        SST_SER(nonlocal_);
        SST_SER(no_cut_);
        SST_SER(cross_rank_);
        SST_SER(cross_thread_);
    }

private:
    friend class ConfigGraph;
    explicit ConfigLink(LinkId_t id) :
        id_(id),
        no_cut_(false)
    {}

    ConfigLink(LinkId_t id, const std::string& n) :
        name_(n),
        id_(id),
        no_cut_(false)
    {}

    void updateLatencies();
};


class PartitionLink
{
public:
    LinkId_t      id_;
    ComponentId_t component_[2];
    SimTime_t     latency_[2];
    bool          no_cut_;

    PartitionLink(const ConfigLink& cl)
    {
        id_           = cl.id_;
        component_[0] = cl.component_[0];
        component_[1] = cl.component_[1];
        latency_[0]   = cl.latency_[0];
        latency_[1]   = cl.latency_[1];
        no_cut_       = cl.no_cut_;
    }

    inline LinkId_t key() const { return id_; }

    /** Return the minimum latency of this link (from both sides) */
    SimTime_t getMinLatency() const
    {
        if ( latency_[0] < latency_[1] ) return latency_[0];
        return latency_[1];
    }

    /** Print the Link information */
    void print(std::ostream& os) const
    {
        os << "    Link " << id_ << std::endl;
        os << "      component[0] = " << component_[0] << std::endl;
        os << "      latency[0] = " << latency_[0] << std::endl;
        os << "      component[1] = " << component_[1] << std::endl;
        os << "      latency[1] = " << latency_[1] << std::endl;
    }
};

} // namespace SST

#endif // SST_CORE_MODEL_CONFIGLINK_H
