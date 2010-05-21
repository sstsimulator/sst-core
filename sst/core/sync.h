// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_SYNC_H
#define SST_SYNC_H

#include <sst/core/sst.h>
#include <sst/core/component.h>
#include <sst/core/syncEvent.h>
#include <boost/mpi.hpp>
#include <sst/core/compEvent.h>

namespace SST {

#define _SYNC_DBG( fmt, args...) __DBG( DBG_SYNC, Sync, fmt, ## args )

class Sync {
private:

    struct linkInfo {
        Link*   link; // local Link object
        int     rank; // rank of other end of link
    };

    // The Sync object communicates with a group of ranks.  "sendQ" holds
    // a group of CompEvents. "sendQ" is passed to a Link object. 
    // Link::Send() places CompEvents in sendQ.
    // When it's time to synchronize with other events the Sync object
    // sends sendQ to the far rank via mpi::isend. "recvQ" is used to
    // receive a queue of CompEvents from the far rank. Both mpi::isend 
    // and mpi::irecv are non-blocking and need corresponding request
    // objects  
    struct rankInfo {
        CompEventQueue_t* sendQ;
        CompEventQueue_t* recvQ;
        boost::mpi::request mpiRecvReq;
        boost::mpi::request mpiSendReq;
    };

public:
    Sync( TimeConverter* period );

    // linnkName - global link name
    // link      - pointer to local Link object
    // farRank   - rank of far link object
    // lat       - link latency 
    int registerLink( std::string linkName, Link* link,
                      int farRank, Cycle_t lat );

    int exchangeFunctors();

private:
    Sync( const Sync& c );
    Sync() {}

    bool handler( Event* event );

private:
    // functor which is called at the Sync frequency 
    EventHandler< Sync, bool, Event* >* m_functor;

    TimeConverter*      m_period;
    SyncEvent*          m_event;

    // map a link name to the local Link object and far rank 
    std::map< std::string, linkInfo >    m_linkMap;

    // map a rank to it's data
    std::map< int, rankInfo >            m_rankMap;                

    friend class boost::serialization::access;
    template<class Archive> 
    void serialize(Archive & ar, const unsigned int )
    {
        ar & BOOST_SERIALIZATION_NVP( m_functor );
        ar & BOOST_SERIALIZATION_NVP( m_period );
        ar & BOOST_SERIALIZATION_NVP( m_event );
        // BWB: FIXME: should this be serialized?
        // ar & BOOST_SERIALIZATION_NVP( m_linkMap );
        // ar & BOOST_SERIALIZATION_NVP( m_rankMap );
    }

    template<class Archive>
    void save_construct_data
    (Archive & ar, const Sync * t, const unsigned int )
    {
        TimeConverter* period    = t->m_period;
        ar << BOOST_SERIALIZATION_NVP( period );
    }

    template<class Archive>
    void load_construct_data
    (Archive & ar, Sync * t, const unsigned int )
    {
        TimeConverter* period;
        ar >> BOOST_SERIALIZATION_NVP( period );
        ::new(t)Sync(period);
    }
};    


} // namespace SST

#endif // SST_SYNC_H
