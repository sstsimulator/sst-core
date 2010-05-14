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


/* Copyright 2007 Sandia Corporation. Under the terms
 of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
 Government retains certain rights in this software.

 Copyright (c) 2007, Sandia Corporation
 All rights reserved.

 This file is part of the SST software package. For license
 information, see the LICENSE file in the top level directory of the
 distribution. */


#ifndef ED_ROUTER_H
#define ED_ROUTER_H

#include <map>
#include <list>
#include <deque>
#include <string>
#include <vector>

#include <queue>
#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/log.h>
#include <sst/link.h>

#include "SS_network.h"

using namespace SST;

#define SS_ROUTER_DBG 1 
#ifndef SS_ROUTER_DBG
#define SS_ROUTER_DBG 0
#endif

#include "SS_network.h"

using namespace std;

#define DBprintf(fmt,args...) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

#define cycle() m_cycle

enum rtrEventType { iLCB_internalXferDone, InQ_tailXferDone, InQ_headXferDone,
                    OutQ_tailXferDone, OutQ_headXferDone, oLCB_internalXferDone,
                    oLCB_externalXferDone, Debug
                  };

typedef struct rtrEvent_s rtrEvent_t;

//: router parcel
// wrapper for a parcel while it lives in the router
//!SEC:EDSim
class rtrP
{
public:
    RtrEvent *event;
    int ilink;
    int olink;
    int ivc, ovc;
    int flits;
};

//: an internal router event
// wrapper for a parcel while it waits in a priority queue
//!SEC:EDSim
struct rtrEvent_s {
    long long unsigned cycle;
    rtrEventType type;
    rtrP *rp;
};

//: priority function to use for router Events
//!SEC:EDSim
inline bool rtrEvent_gt (rtrEvent_t* a, rtrEvent_t *b) {
    return (a->cycle > b->cycle);
}

//: Pool of router Events
// used to avoid many memory allocates
//!SEC:EDSim
class rtrEventPool {
private:
    vector<rtrEvent_t*> thePool;
    int count;
public:
    rtrEventPool() {
        rtrEvent_t *event;
        for (int i = 0; i < 10; i++) {
            event = new rtrEvent_t;
            thePool.push_back(event);
        }
        count = 10;
    }

    rtrEvent_t *getEvent() {
        rtrEvent_t *event;
        if (count > 0) {
            event = thePool.back();
            thePool.pop_back();
            count--;
        } else {
            event = new rtrEvent_t;
            thePool.push_back(event);
            event = new rtrEvent_t;
            count++;
        }
        return event;
    }

    void returnEvent(rtrEvent_t* e) {
        thePool.push_back(e);
        count++;
    }
};


//: Pool of router parcel wrappers
// used to avoid many memory allocates in the long run
//!SEC:EDSim
class rtrPPool {
private:
    vector<rtrP*> thePool;
    int count;
public:
    rtrPPool() {
        rtrP *rp;
        for (int i = 0; i < 10; i++) {
            rp = new rtrP;
            thePool.push_back(rp);
        }
        count = 10;
    }

    rtrP *getRp() {
        rtrP *rp;
        if (count > 0) {
            rp = thePool.back();
            thePool.pop_back();
            count--;
        } else {
            rp = new rtrP;
            thePool.push_back(rp);
            rp = new rtrP;
            count++;
        }
        return rp;
    }

    void returnRp(rtrP* rp) {
        thePool.push_back(rp);
        count++;
    }
};

//: 3D router
// models a 3D router in a toroid configuration with 4 virtual channels
//!SEC:EDSim
class SS_router : public Component
{
    bool findRoute( int destNid, int inputVC, int inputDir,
                    int& outVC, int& outDir );
    int findOutputDir( int destNid );
    int findOutputVC( int inVC, int inDir, int outDir );
    int dimension( int dir );
    bool iAmDateline( int dimension );
    int changeVC( int vc );
    int calcXPosition( int nodeNumber, int xSize, int ySize, int zSize );
    int calcYPosition( int nodeNumber, int xSize, int ySize, int zSize );
    int calcZPosition( int nodeNumber, int xSize, int ySize, int zSize );
    int calcDirection( int src, int dst, int size );

    vector<char> m_routingTableV; 
    vector<bool> m_datelineV;


    int neighbor[ROUTER_NUM_LINKS];
    int neighborID (int dir) {
        return neighbor[dir];
    }

    inline int NODE_ID(int x, int y, int z,Network *net) {
        return z * net->xDimSize() * net->yDimSize() + y * net->xDimSize() + x;
    }

    void find_neighbors(Network *net, int x, int y, int z) {

        DBprintf("located at (%d, %d, %d)\n", x, y, z);

        int posx,posy,posz,negx,negy,negz;
        posx = (x + 1) % net->xDimSize();
        posy = (y + 1) % net->yDimSize();
        posz = (z + 1) % net->zDimSize();
        negx = (x == 0) ? net->xDimSize() - 1 : (x - 1);
        negy = (y == 0) ? net->yDimSize() - 1 : (y - 1);
        negz = (z == 0) ? net->zDimSize() - 1 : (z - 1);

        neighbor[LINK_NEG_X] = NODE_ID(negx,y,z, net);
        neighbor[LINK_POS_X] = NODE_ID(posx,y,z, net);

        neighbor[LINK_NEG_Y] = NODE_ID(x,negy,z, net);
        neighbor[LINK_POS_Y] = NODE_ID(x,posy,z, net);

        neighbor[LINK_NEG_Z] = NODE_ID(x,y,negz, net);
        neighbor[LINK_POS_Z] = NODE_ID(x,y,posz, net);
        DBprintf("X %d %d\n", neighbor[LINK_NEG_X], neighbor[LINK_POS_X]);
        DBprintf("Y %d %d\n", neighbor[LINK_NEG_Y], neighbor[LINK_POS_Y]);
        DBprintf("Z %d %d\n", neighbor[LINK_NEG_Z], neighbor[LINK_POS_Z]);
    }

    friend class oLCB_t;

private:
    double overheadMultP;
protected:
    //: queue of internal router events
    vector<rtrEvent_t*> rtrEventQ;

    //: pool of unused allocated events to draw from
    rtrEventPool eventPool;
    //: pool of unused allocated router parcel wrappers
    rtrPPool rpPool;
    //: router ID, same as node ID
    int routerID;
    //: latency to get a packet
    int iLCBLat;
    //: latency to send a packet
    int oLCBLat;
    //: latency to route a packet
    int routingLat;
    //: queue access latency
    int iQLat;
    //: max output queue size in flits
    int rtrOutput_maxQSize_flits[ROUTER_NUM_LINKS + 1];
    //: max input queue size in flits
    int rtrInput_maxQSize_flits[ROUTER_NUM_LINKS + 1];
    //: number of flits which can fit into an LCB
    int oLCB_maxSize_flits;


    //: Routing table
    // It maps a destination node ID and incoming virtual channel
    // To an outgoing link and virtual channel
    // The virtual channel will always be the same unless
    // crossing a dateline
    typedef map< pair<int, int>, pair<int, int> > routingTable_t;
    //: routing table structure
    routingTable_t routingTable;

public:
    void updateToken_flits (int dir, int vc, int flits );
    void returnToken_flits (int dir, int vc, int flits );

protected:
    //: a link between routers, used to hold ID neighbor info and pass tokens
    struct netlink {
        Link *link;
        int   dir;
    };
    //: netlink pointer
    typedef netlink* link_t;

    //: RX Network links each of the six directions
    link_t rx_netlinks[ROUTER_NUM_LINKS];
    //: TX Network links each of the six directions
    link_t tx_netlinks[ROUTER_NUM_LINKS];
    //: pointers to neighbors who may send us packets
    // which is used when packets arrive from external sources, to check for a valid sender
    map <Link*, int> rxNeighbor;

    //: count packets sent from this node on each link
    int txCount[ROUTER_NUM_OUTQS];
    //: count packets received at this node on each link
    int rxCount[ROUTER_NUM_INQS];
    //: queue structure used for all input and output queues
    typedef deque<rtrP*> packetQ;

    //: input queue structure
    // contains a busy flag, 4 sub-queus for the virtual channels, a buffer for keeping
    // track of the queues that were skipped in a RR, and size of each queue in flits
    // which is used to reserve size to flow control between LCB and input queues
    struct inQ_t
    {
        bool head_busy;
        int link, vc_rr, ready_vcQs;
        queue<int> skipQs;
        int size_flits[ROUTER_NUM_VCS];
        packetQ vcQ[ROUTER_NUM_VCS];
        bool ready () {
            return ((ready_vcQs > 0) && (!head_busy));
        }
    };
    //: One input queue set per rx-link
    inQ_t inputQ[ROUTER_NUM_INQS];

    //: number of input queues which are ready to try and dequeue something
    //int ready_inQs_count;
    bool ready_inQ;

    //: ouput queue structure
    // contains 4 sub-queues for virtual channels, and size of each queue in flits
    // which is used to reserve size to flow control between input and output queues
    struct outQ_t
    {
        packetQ vcQ[ROUTER_NUM_VCS];
        int size_flits[ROUTER_NUM_VCS];
    };

    //: One output queue per tx-link per rx-link
    outQ_t outputQ[ROUTER_NUM_OUTQS][ROUTER_NUM_INQS];

    //: out LCB structure
    // contains round robin information as well as packet data
    struct oLCB_t
    {
        int size_flits;
        bool external_busy;
        bool internal_busy;
        int link;
        int vc_rr, ready_vc_count;
        queue<int> skipped_vcs;
        int ready_outQ_count[ROUTER_NUM_VCS];
        int ilink_rr[ROUTER_NUM_VCS];
        int vcTokens[ROUTER_NUM_VCS];
        int stall4tokens[ROUTER_NUM_VCS];
        packetQ dataQ;

        bool readyXfer() {
            return (!dataQ.empty() && !external_busy);
        }
        bool readyInternal() {
            return ((ready_vc_count > 0) && !internal_busy);
        }

    };

    //: in LCB structure
    // contains round robin information as well as packet data
    struct iLCB_t
    {
        int size_flits;
        bool internal_busy;
        int link;
        packetQ dataQ;
        bool readyInternal() {
            return (!dataQ.empty() && !internal_busy);
        }
    };

    //: output LCBs for each link plus one for the router port
    oLCB_t outLCB[ROUTER_NUM_OUTQS];
    //: input LCBs for each link plus one for the router port
    iLCB_t inLCB[ROUTER_NUM_INQS];

    //: number of out LCBs ready to move data
    //int ready_oLCB_count;
    bool ready_oLCB;
    //: number of in LCBs ready to move data
    //int ready_iLCB_count;
    bool ready_iLCB;

    Network *network;

    vector<Link*> linkV;

    bool handleNicParcel(Event*);
    bool handleParcel(Event*,int dir);
    bool route(rtrP*);

    void InLCB ( RtrEvent *e, int ilink, int ivc, int flits);

    bool LCBtoInQ_start(rtrP *rp);
    void LCBtoInQ_readyNext(rtrP *rp);
    void LCBtoInQ_done (rtrP *rp, int ivc, int ilink);
    void InQtoOutQ_start (rtrP *rp);
    void InQtoOutQ_readyNext (rtrP *rp);
    void InQtoOutQ_done (rtrP *rp, int ovc, int ilink, int olink);
    void OutQtoLCB_start (rtrP *rp);
    void OutQtoLCB_readyNext (rtrP *rp, int olink, int ilink, int ovc, int flits);
    void OutQtoLCB_done (rtrP *rp);
    void LCBxfer_start (int dir);
    void LCBxfer_readyNext (int dir);
    void LCBxfer_done (rtrP *rp, int olink, int flits);

    void arbitrateOutToLCB ();
    void arbitrateInToOut ();
    void iLCBtoIn ();

    void setup();
    void dumpStats(FILE *fp);
    void finish();

    void setupRoutingTable ( Params_t, int nodes, int xDim, int yDim, int zDim);
    void setVCRoutes ( int node, int dir, bool *crossDateline );

    void advanceEventQ();
    void DebugEvent ();
    //: if using the debug event, this is the number of cycles between successive events
    int debug_interval;
    bool dumpTables;

public:

    SS_router ( ComponentId_t id, Params_t& params );

private:
    SS_router();
    SS_router( const SS_router& );
    ~SS_router() {
        ;
    }

    //: simulate one pretic
    // advance the event queue, try to move data from in LCBs to input queues,
    // try to move data from input queues to output queues, and
    // try to move data from output queues to out LCBs
    bool clock( Cycle_t cycle )
    {
        //DBprintf("cycle=%ld\n",cycle);
        m_cycle = cycle;
        //if (!(cycle()%1000) && routerID == 0) printf ("cycle %lld\n", cycle());
        if (!rtrEventQ.empty())
            advanceEventQ();
        //if (ready_oLCB_count > 0)
        //if (ready_oLCBsleepchk())
        if (ready_oLCB)
            arbitrateOutToLCB ();
        //if (ready_inQs_count > 0)
        if (ready_inQ)
            arbitrateInToOut ();
        //if (ready_iLCB_count > 0)
        if (ready_iLCB)
            iLCBtoIn();
	return true; // KBW: hopefully, this is essentially meaningless
    }

    //: string values for link direction, only used for debugging and outputting link traffic data
    char LINK_DIR_STR[6][8];
    //: return router ID
    int getID() {
        return routerID;
    }

    void dumpTable( FILE* fp);
    void dumpState();

    void txlinkTo ( Link* neighbor, int dir);
    bool checkLinks ();

    Cycle_t                 m_cycle;
    bool                    m_print_info;
    Log< SS_ROUTER_DBG >&   m_dbg;
    Log<>&                  m_log;
};

#include "SS_router-inline.h"

#endif
