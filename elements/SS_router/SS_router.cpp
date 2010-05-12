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

#include <sst_config.h>

#include <sstream>

#include "SS_router.h"
#include <paramUtil.h>
#include <sst/eventHandler1Arg.h>
#include "SS_routerInternals.cpp"
#include "SS_routerEvent.cpp"

using namespace std;

//: Router function to pass tokens back
// When tokens are returned, this might cause an output queue to become ready to accept data
void SS_router::updateToken_flits( int link, int vc, int flits )
{
    DBprintf ("%lld: link %d return %d flit token to rtr %d, vc %d\n",
              cycle(), link, flits, routerID, vc);

    int old_tokens = outLCB[link].vcTokens[vc];

    if ( link == ROUTER_HOST_PORT ) {
        vc =  NIC_2_RTR_VC( vc );
    }

    outLCB[link].vcTokens[vc] += flits;

    if (old_tokens < MaxPacketSize) {
        if (outLCB[link].vcTokens[vc] >= MaxPacketSize) {
            outLCB[link].ready_vc_count++;
            if (!outLCB[link].internal_busy)
                ready_oLCB = true;
        }
    }
}

void SS_router::returnToken_flits (int dir, int flits, int vc) {

    DBprintf("dir=%d flits=%d vc=%d\n",dir,flits,vc );
    RtrEvent* event = new RtrEvent;
    event->type  = RtrEvent::Credit;
    if ( dir == ROUTER_HOST_PORT ) {
        vc = RTR_2_NIC_VC( vc );
    }

    event->u.credit.num = flits;
    event->u.credit.vc = vc;

    linkV[dir]->Send( event );
}

//: Constructor
SS_router::SS_router( ComponentId_t id, Params_t& params ) :
        Component( id ),
        oLCB_maxSize_flits(512),
        m_cycle(0),
        dumpTables(false),
        overheadMultP(1.5),
        m_dbg( *new Log< SS_ROUTER_DBG >( "SS_router::", false ) ),
        m_log( *new Log<>( "INFO SS_router: ", false ) )
{
    int tmp;
    //printParams(params);
    if ( params.find( "info" ) != params.end() ) {
        if ( params[ "info" ].compare( "yes" ) == 0 ) {
            m_log.enable();
        }
    }

    if ( params.find( "debug" ) != params.end() ) {
        if ( params[ "debug" ].compare( "yes" ) == 0 ) {
            m_dbg.enable();
        }
    }

    if ( params.find( "id" ) == params.end() ) {
        _abort(SS_router,"couldn't find routerID\n" );
    }

    routerID = str2long( params[ "id" ] );

    std::ostringstream idStr;
    idStr << routerID << ":";
    m_dbg.prepend( idStr.str() );
    m_log.prepend( idStr.str() );

    DBprintf("this=%p id=%d\n",this,id);

    sprintf (LINK_DIR_STR[LINK_POS_X], "POSX");
    sprintf (LINK_DIR_STR[LINK_POS_Y], "POSY");
    sprintf (LINK_DIR_STR[LINK_POS_Z], "POSZ");
    sprintf (LINK_DIR_STR[LINK_NEG_X], "NEGX");
    sprintf (LINK_DIR_STR[LINK_NEG_Y], "NEGY");
    sprintf (LINK_DIR_STR[LINK_NEG_Z], "NEGZ");

    for (int ln = 0; ln < ROUTER_NUM_LINKS+1; ln++) {
        //No links are set up yet
        rx_netlinks[ln] = NULL;
        tx_netlinks[ln] = NULL;
    }

    if ( params.find( "iLCBLat" ) == params.end() ) {
        _abort(SS_router,"couldn't find iLCBLat\n" );
    }
    iLCBLat = str2long( params[ "iLCBLat" ] );

    if ( params.find( "oLCBLat" ) == params.end() ) {
        _abort(SS_router,"couldn't find oLCBLat\n" );
    }
    oLCBLat = str2long( params[ "oLCBLat" ] );

    if ( params.find( "routingLat" ) == params.end() ) {
        _abort(SS_router,"couldn't find routingLat\n" );
    }
    routingLat = str2long( params[ "routingLat" ] );

    if ( params.find( "iQLat" ) == params.end() ) {
        _abort(SS_router,"couldn't find iQLat\n" );
    }
    iQLat = str2long( params[ "iQLat" ] );

    if ( params.find( "OutputQSize_flits" ) == params.end() ) {
        _abort(SS_router,"couldn't find OutputQSize_flits\n" );
    }
    tmp = str2long( params[ "OutputQSize_flits" ] );

    m_log.write("OutputQSize_flits=%d\n",tmp);

    for ( int i = 0; i < ROUTER_NUM_LINKS; i++ ) {
        rtrOutput_maxQSize_flits[i] = tmp;
    }

    if ( params.find( "InputQSize_flits" ) == params.end() ) {
        _abort(SS_router,"couldn't find InputQSize_flits\n" );
    }
    tmp = str2long( params[ "InputQSize_flits" ] );

    m_log.write("InputQSize_flits=%d\n",tmp);

    for ( int i = 0; i < ROUTER_NUM_LINKS; i++ ) {
        rtrInput_maxQSize_flits[i] = tmp;
    }

    if ( params.find( "Router2NodeQSize_flits" ) == params.end() ) {
        _abort(SS_router,"couldn't find Router2NodeQSize_flits\n" );
    }
    tmp = str2long( params[ "Router2NodeQSize_flits" ] );

    m_log.write("Router2NodeQSize_flits=%d\n",tmp);

    rtrOutput_maxQSize_flits[ROUTER_HOST_PORT] = tmp;
    rtrInput_maxQSize_flits[ROUTER_HOST_PORT] = tmp;

    if ( params.find( "debugInterval" ) != params.end() ) {
        debug_interval = str2long( params[ "debugInterval" ] );
    }
    else debug_interval = 0;

    if ( params.find( "dumpTables" ) != params.end() ) {
        dumpTables = str2long( params[ "dumpTables" ] );
    }

    if ( params.find( "overheadMult" ) != params.end() ) {
        overheadMultP = str2long( params[ "overheadMult" ] );
    }

    m_log.write("overhead mult %f\n",overheadMultP);

    int ln, ch, iln;

    for (ln = 0; ln < ROUTER_NUM_OUTQS; ln++)
        for (iln = 0; iln < ROUTER_NUM_INQS; iln++) {
            for (ch = 0; ch < ROUTER_NUM_VCS; ch++) {
                outputQ[ln][iln].size_flits[ch] = 0;
                outputQ[ln][iln].vcQ[ch].clear();
                //-KDU
                //inputQ[iln].skipQs[ln][ch] = -1;
            }
        }


    for (iln = 0; iln < ROUTER_NUM_INQS; iln++) {
        //ready_inQs[iln] = NULL;

        inputQ[iln].head_busy = false;
        inputQ[iln].link = iln;
        inputQ[iln].vc_rr = 0;
        inputQ[iln].ready_vcQs = 0;

        for (ch = 0; ch < ROUTER_NUM_VCS; ch++) {
            //all input queues are empty
            inputQ[iln].size_flits[ch] = 0;
            inputQ[iln].vcQ[ch].clear();
            //inputQ[iln].vc_ready[ch] = false;
        }
    }


    for (int link = 0; link < ROUTER_NUM_OUTQS; link++) {

        //ready_oLCB[link] = NULL;
        //ready_iLCB[link] = NULL;

        //init packet counters
        txCount[link] = 0;
        rxCount[link] = 0;

        outLCB[link].dataQ.clear();
        outLCB[link].size_flits = 0;
        outLCB[link].external_busy = false;
        outLCB[link].internal_busy = false;
        outLCB[link].link = link;
        outLCB[link].ready_vc_count = 0;
        outLCB[link].vc_rr = 0;

        inLCB[link].dataQ.clear();
        inLCB[link].size_flits = 0;
        //inLCB[link].external_busy = false;
        inLCB[link].internal_busy = false;
        inLCB[link].link = link;

        for (ch = 0; ch < ROUTER_NUM_VCS; ch++) {
            //outLCB[link].front_rp_count[ch] = 0;
            outLCB[link].ilink_rr[ch] = 0;
            //outLCB[link].ready_vc[ch] = false;
            outLCB[link].stall4tokens[ch] = 0;
            outLCB[link].vcTokens[ch] = rtrInput_maxQSize_flits[link];
            outLCB[link].ready_outQ_count[ch] = 0;

            //for (iln = 0; iln < ROUTER_NUM_INQS; iln++)
            //outLCB[link].front_rp[ch][iln] = NULL;
        }
    }

    //ready_inQs_count = 0;
    //ready_oLCB_count = 0;
    //ready_iLCB_count = 0;

    ready_inQ = false;
    ready_oLCB = false;
    ready_iLCB = false;

    Params_t tmp_params; 

    m_datelineV.resize( 3 );
    for ( int i = 0; i < 3; i++ ) {
        m_datelineV[i] = false;
    }
    findParams( "network.", params, tmp_params );
    network = new Network( tmp_params ); 

    findParams( "routing.", params, tmp_params );
    setupRoutingTable ( tmp_params, network->size(), network->xDimSize(),
                        network->yDimSize(), network->zDimSize() );

    if (debug_interval > 0) {
        rtrEvent_t *event = eventPool.getEvent();
        event->cycle = cycle() + debug_interval;
        event->type = Debug;
        event->rp = NULL;
        rtrEventQ.push_back(event);
    }

    int x,y,z;

    z = routerID / (network->xDimSize() * network->yDimSize());
    y = (routerID / network->xDimSize()) % network->yDimSize();
    x = routerID % network->xDimSize();

    find_neighbors(network,x,y,z);


    linkV.resize( ROUTER_NUM_LINKS + 1 );
    for (int dir = 0; dir < ROUTER_NUM_LINKS + 1; dir++) {
        //set up this nodes router link to the neighbor router

        Event::Handler_t*   handler = new EventHandler1Arg< 
                            SS_router, bool, Event*, int >
                       ( this, &SS_router::handleParcel, dir );

        DBprintf("adding link %s\n", LinkNames[dir] );
        linkV[dir] = LinkAdd( LinkNames[dir], handler );

        txlinkTo( linkV[dir], dir );
    }

    ClockHandler_t*     clockHandler;
    clockHandler = new EventHandler< SS_router, bool, Cycle_t >
                                                ( this, &SS_router::clock );

    std::string frequency = "1GHz";
    if ( params.find("clock") != params.end() ) {
        frequency = params["clock"];
    }

    m_log.write("frequency=%s\n", frequency.c_str() );

    TimeConverter* tc = registerClock( frequency, clockHandler );
    if ( ! tc ) {
        _abort(XbarV2,"couldn't register clock handler");
    }
}

void SS_router::setup() {
}

//: Output statistics
void SS_router::finish () {
    DBprintf("\n");
    if ( m_print_info )
        dumpStats(stdout);

#if 0 // finish() dumpTables
    if ( m_print_info && dumpTables ) {
        string filename = configuration::getStrValue (":ed:datadir");
        char buf[256];
        sprintf(buf, "%s/routing/routing_table-%d", filename.c_str(), routerID);
        FILE *fp = fopen (buf, "w+");
        dumpTable(fp);
        fclose(fp);
    }
#endif
}

void SS_router::dumpStats (FILE* fp) {
    DBprintf("\n");
    int rxt, txt, myt;

    fprintf(fp, "Router %d\n", routerID);

    rxt = 0;
    txt = 0;
    myt = 0;

#if 0 // dumpStats
    int clockRate = configuration::getValue(":ed:MTA:clockRate");

    for (int i = 0; i < ROUTER_NUM_OUTQS; i++) {
        double sent = (double)txCount[i] / (double)cycle() * (double)clockRate;
        double recv = (double)rxCount[i] / (double)cycle() * (double)clockRate;
        fprintf(fp, "rtr %2d: %2d link: %9.3fM per sec sent %9.3fM per sec recv\n", routerID, i, sent, recv);
    }
    for (int i = 0; i < ROUTER_NUM_OUTQS; i++) {
        fprintf(fp, "rtr %2d: %2d link: %9d flits sent %9d flits recv\n", routerID, i, txCount[i], rxCount[i]);
    }

    fprintf(fp, "\t%9d total packets recv\n", rxt);
    fprintf(fp, "\t%9d total packets sent\n", txt);
    fprintf(fp, "\t%9d total packets for me\n", myt);
#endif

}

//: create a transmit network link
// and connect this and a neighboring router together over it
void SS_router::txlinkTo (Link* neighbor, int dir) {

    DBprintf("dir=%d link=%p\n",dir,neighbor);

    if (tx_netlinks[dir] != NULL) {
        printf ("Error: router %d cannot tx link to %d dir, already linked\n", routerID, dir);
        return;
    }

    //create the new link and add it to the tx links
    link_t ln = new netlink;
    tx_netlinks[dir] = ln;
    ln->link = neighbor;
    ln->dir = dir;

    DBprintf ("Router %d tx linked to router XXX in %d direction\n", routerID, dir);
}

//: Used to see if all the links have been initialized for this router
bool SS_router::checkLinks() {

    DBprintf("\n");
    for (int ln = 0; ln < ROUTER_NUM_LINKS; ln++)
        if ( (rx_netlinks[ln] == NULL) || (tx_netlinks[ln] == NULL) )
            return false;

    return true;
}


//: Dump the routing table for debugging
void SS_router::dumpTable (FILE *fp) {

    DBprintf("\n");
    pair<int, int> key, localdest;
    int nodes = network->size();

    for (int nd = 0; nd < nodes ; nd++) {
        key.first = nd;

        for (int vc = 0; vc < ROUTER_NUM_VCS; vc++) {
            key.second = vc;
            localdest = routingTable[key];
            fprintf (fp, "node %3d: key %3d:%1d, dest %1d:%1d\n", routerID, key.first, key.second,
                     localdest.first, localdest.second);
        }
    }

}


//: add a routing entry for each virtual channel
//
// Inputs are
// the node being routed to
// the direction which the route should take for the next hop
// the remote node's coordinate in this dimension
// the router's coordinate in this dimention
void SS_router::setVCRoutes ( int nd, int dir, bool crossDateline[ROUTER_NUM_LINKS] ) {
    printf("%d:  crossDateline[%d] = %d\n",nd,dir,crossDateline[dir]);
    printf("  crossDateline = %d %d %d %d %d %d\n",crossDateline[0],crossDateline[1],crossDateline[2],crossDateline[3],crossDateline[4],crossDateline[5]);
    DBprintf("nd=%d dir=%d\n",nd,dir);
    pair <int, int> key, localDest;
    int vc;

    //set up an error routing entry
    if (dir < 0) {
        for (vc = 0; vc < ROUTER_NUM_VCS; vc++) {
            key.first = nd;
            key.second = vc;
            localDest.first = dir;
            localDest.second = -1;
        }
        return;
    }

    for (vc = 0; vc < ROUTER_NUM_VCS; vc++) {
        key.first = nd;
        key.second = vc;
        localDest.first = dir;

        if ( dir == ROUTER_HOST_PORT ) {
            if ( vc < 2 ) {
                localDest.second = NIC_VC_0;
            } else {
                localDest.second = NIC_VC_1;
            }
        } else {
            if ( crossDateline[dir] ) {
                //crossing dateline, vc0->vc1, vc1->vc1, vc2->vc3, vc3->vc3
                //Changed to cross up on 01-06-2006 -KDU
                if ((vc == LINK_VC0) || (vc == LINK_VC1))
                    localDest.second = LINK_VC1;
                else
                    localDest.second = LINK_VC3;
            }

            else
                localDest.second = vc;
        }

        DBprintf("dir=%d vc=%d\n",dir,localDest.second);
        routingTable[key] = localDest;
    }
}

//: Build the routing table
void SS_router::setupRoutingTable( Params_t params, int nodes,
                                   int xDim, int yDim, int zDim) {

    DBprintf("\n");
    int dateline[ROUTER_NUM_LINKS];
    bool crossDateline[ROUTER_NUM_LINKS];

    if ( params.find( "xDateline" ) == params.end() ) {
        _abort(SS_router,"couldn't find xDateline\n" );
    }
    if ( str2long( params["xDateline"] ) == calcXPosition(routerID,xDim,yDim,zDim) ) {
        m_datelineV[dimension(LINK_POS_X)] = true; 
    }
#if 0
    dateline[LINK_POS_X] = str2long( params["xDateline"] );
    dateline[LINK_NEG_X] = dateline[LINK_POS_X];
#endif

    if ( params.find( "yDateline" ) == params.end() ) {
        _abort(SS_router,"couldn't find yDateline\n" );
    }
    if ( str2long( params["yDateline"] ) == calcYPosition(routerID,xDim,yDim,zDim) ) {
        m_datelineV[dimension(LINK_POS_Y)] = true; 
    }
#if 0
    dateline[LINK_POS_Y] = str2long( params["yDateline"] );
    dateline[LINK_NEG_Y] = dateline[LINK_POS_Y];
#endif

    if ( params.find( "zDateline" ) == params.end() ) {
        _abort(SS_router,"couldn't find zDateline\n" );
    }

    if ( str2long( params["xDateline"] ) == calcZPosition(routerID,xDim,yDim,zDim) ) {
        m_datelineV[dimension(LINK_POS_Z)] = true; 
    }
#if 0
    dateline[LINK_POS_Z] = str2long( params["zDateline"] );
    dateline[LINK_NEG_Z] = dateline[LINK_POS_Z];
#endif
    DBprintf("datelineX=%d datelineY=%d datelineZ=%d\n",
                    iAmDateline(dimension(LINK_POS_X)),
                    iAmDateline(dimension(LINK_POS_Y)),
                    iAmDateline(dimension(LINK_POS_Z)) );


    int my_xPos = calcXPosition( routerID, xDim, yDim, zDim );
    int my_yPos = calcYPosition( routerID, xDim, yDim, zDim );
    int my_zPos = calcZPosition( routerID, xDim, yDim, zDim );

//     printf("%d (%d,%d,%d): datelineX=%d datelineY=%d datelineZ=%d\n",routerID,
// 	   my_xPos,my_yPos,my_zPos,
// 	   iAmDateline(dimension(LINK_POS_X)),
//                     iAmDateline(dimension(LINK_POS_Y)),
//                     iAmDateline(dimension(LINK_POS_Z)) );

    m_routingTableV.resize(network->size());
    for ( int i = 0; i < network->size(); i++ ) {
        int dst_xPos = calcXPosition( i, xDim, yDim, zDim );
        int dst_yPos = calcYPosition( i, xDim, yDim, zDim );
        int dst_zPos = calcZPosition( i, xDim, yDim, zDim );

        if ( my_xPos != dst_xPos ) {
            int dir = calcDirection( my_xPos, dst_xPos, xDim );
            if ( dir > 0 ) {
                m_routingTableV[ i ] = LINK_POS_X; 
            } else {
                m_routingTableV[ i ] = LINK_NEG_X; 
            } 
        } else if ( my_yPos != dst_yPos ) {
            int dir = calcDirection( my_yPos, dst_yPos, yDim );
            if ( dir > 0 ) {
                m_routingTableV[ i ] = LINK_POS_Y; 
            } else {
                m_routingTableV[ i ] = LINK_NEG_Y;
            }
        } else if ( my_zPos != dst_zPos ) {
            int dir = calcDirection( my_zPos, dst_zPos, zDim );
            if ( dir > 0 ) {
                m_routingTableV[ i ] = LINK_POS_Z; 
            } else {
                m_routingTableV[ i ] = LINK_NEG_Z; 
            } 
        } else {
            m_routingTableV[ i ] = ROUTER_HOST_PORT;
        }
        DBprintf("dir=%d\n",m_routingTableV[i]);
// 	printf("(%d,%d,%d) to (%d,%d,%d) -> %d\n",my_xPos,my_yPos,my_zPos,dst_xPos,dst_yPos,dst_zPos,m_routingTableV[i]);
    } 

    return;

#if 0
    int my_yLoc = (routerID / xDim) % yDim;
    int my_xLoc = routerID % xDim;
    int my_zLoc = routerID / (xDim * yDim);

    int nd_xDim, nd_yDim, nd_zDim, nd, nd_Dim, my_Dim, max_Dim;
    my_Dim = -1;
    nd_Dim = -1;
    max_Dim = -1;

    for (int ln = 0; ln < ROUTER_NUM_LINKS; ln++) {
        nd = neighborID(ln);
        switch (ln) {
        case LINK_POS_X:
        case LINK_NEG_X:
            nd_Dim = nd % xDim;
            my_Dim = my_xDim;
            max_Dim = xDim;
            break;
        case LINK_POS_Y:
        case LINK_NEG_Y:
            nd_Dim = (nd / xDim) % yDim;
            my_Dim = my_yDim;
            max_Dim = yDim;
            break;
        case LINK_POS_Z:
        case LINK_NEG_Z:
            nd_Dim = nd / (xDim * yDim);
            my_Dim = my_zDim;
            max_Dim = zDim;
            break;
        }

        if ( (my_Dim == (dateline[ln])) &&
                ( (nd_Dim == ( (dateline[ln]+1) % max_Dim) ) ||
                  (nd_Dim == ( (dateline[ln] > 0) ? (dateline[ln] - 1) : max_Dim ) ) ) )
            crossDateline[ln] = true;
        else
            crossDateline[ln] = false;
    }

    for (nd = 0; nd < nodes ; nd++) {
        if (nd != routerID) {
            nd_xDim = nd % xDim;
            // two nodes lie in same x dimension
            if ( nd_xDim == my_xDim ) {
                nd_yDim = (nd / xDim) % yDim;
                // two nodes lie in same y dimension
                if ( nd_yDim == my_yDim ) {
                    // route along z axis
                    nd_zDim = nd / (xDim * yDim);
                    // node lies in negative Z direction
                    if ((DIST_NEG(my_zDim,nd_zDim,zDim)) < (DIST_POS(my_zDim,nd_zDim,zDim)))
                        setVCRoutes ( nd, LINK_NEG_Z, crossDateline);
                    // node lies in positive Z direction
                    else if ((DIST_NEG(my_zDim,nd_zDim,zDim)) > (DIST_POS(my_zDim,nd_zDim,zDim)))
                        setVCRoutes ( nd, LINK_POS_Z, crossDateline);
                    else if ((routerID % 2) == 1)
                        setVCRoutes ( nd, LINK_NEG_Z, crossDateline);
                    else
                        setVCRoutes ( nd, LINK_POS_Z, crossDateline);
                }
                // ( nd_yDim != my_yDim ) route along Y axis
                else {
                    if ((DIST_NEG(my_yDim,nd_yDim,yDim)) < (DIST_POS(my_yDim,nd_yDim,yDim)))
                        setVCRoutes ( nd, LINK_NEG_Y, crossDateline );
                    // node lies in pos Y dir
                    else if ((DIST_NEG(my_yDim,nd_yDim,yDim)) > (DIST_POS(my_yDim,nd_yDim,yDim)))
                        setVCRoutes ( nd, LINK_POS_Y, crossDateline );
                    else if ((routerID % 2) == 1)
                        setVCRoutes ( nd, LINK_NEG_Y, crossDateline);
                    else
                        setVCRoutes ( nd, LINK_POS_Y, crossDateline);
                }
            }
            // (nd_xDim != my_xDim) route along X axis
            else {
                if ((DIST_NEG(my_xDim,nd_xDim,xDim)) < (DIST_POS(my_xDim,nd_xDim,xDim)))
                    setVCRoutes ( nd, LINK_NEG_X, crossDateline );
                // node lies in pos x dir
                else if ((DIST_NEG(my_xDim,nd_xDim,xDim)) > (DIST_POS(my_xDim,nd_xDim,xDim)))
                    setVCRoutes ( nd, LINK_POS_X, crossDateline );
                else if ((routerID % 2) == 1)
                    setVCRoutes ( nd, LINK_NEG_X, crossDateline);
                else
                    setVCRoutes ( nd, LINK_POS_X, crossDateline);
            }
        }
        // routing entry for me is to the host port
        else
            setVCRoutes ( nd, ROUTER_HOST_PORT, crossDateline);
    }
#endif
}

//: receive a parcel, which should carry a packet
bool SS_router::handleParcel( Event* e, int dir )
{
    RtrEvent*  event = static_cast<RtrEvent*>(e);

    DBprintf("got event type %#ld on link %s\n", event->type, LinkNames[dir] );

    if ( event->type == RtrEvent::Credit ) {
        DBprintf("%s returned tokens vc=%d num=%ld\n", LinkNames[dir],
                 event->u.credit.vc, event->u.credit.num );
        updateToken_flits( dir, event->u.credit.vc, event->u.credit.num );
        delete event;
        return false;
    }

    int ilink, ivc, flits;
    networkPacket *np = &event->u.packet;
    flits = np->sizeInFlits();
    ivc = NIC_2_RTR_VC(np->vc());
    np->vc() = ivc;

    ilink = dir;

    rxCount[ilink] += flits;
    InLCB( event, ilink, ivc, flits);
    return false;
}

//: route a packet
bool SS_router::route(rtrP* rp)
{
    DBprintf("\n");
    networkPacket *packet =  &rp->event->u.packet;

    return findRoute( packet->destNum(), 
            rp->ivc, rp->ilink, rp->ovc, rp->olink );
}

#if 0
    pair< int, int > key, localDest;

    key.first = packet->destNum();
    key.second = rp->ivc;

    if (routingTable.count(key) <= 0)
        return false;

    localDest = routingTable[key];

    rp->olink = localDest.first;
    rp->ovc = localDest.second;

    bool switch_dim = false;
//Deleted on 01-06-2006 -KDU
//reinstated on 01-09-2006 -KDU.  We need to cross back down when
//crossing dimensions
    switch (rp->ilink) {
    case LINK_NEG_X:
        if (rp->olink != LINK_POS_X) switch_dim = true;
        break;
    case LINK_POS_X:
        if (rp->olink != LINK_NEG_X) switch_dim = true;
        break;
    case LINK_NEG_Y:
        if (rp->olink != LINK_POS_Y) switch_dim = true;
        break;
    case LINK_POS_Y:
        if (rp->olink != LINK_NEG_Y) switch_dim = true;
        break;
    case LINK_NEG_Z:
        if (rp->olink != LINK_POS_Z) switch_dim = true;
        break;
    case LINK_POS_Z:
        if (rp->olink != LINK_NEG_Z) switch_dim = true;
        break;
    }

    if (switch_dim && rp->olink != ROUTER_HOST_PORT) {
        switch (rp->ovc) {
        case LINK_VC0:
        case LINK_VC1:
            rp->ovc = LINK_VC0;
            break;
        case LINK_VC2:
        case LINK_VC3:
            rp->ovc = LINK_VC2;
            break;
        }
    }

    DBprintf ("%lld: router %d parcel final dest %d (inc vc %d) local dest :%d:%d:\n",
              cycle(), routerID, key.first, key.second, localDest.first,
              localDest.second);

    return true;
}
#endif
