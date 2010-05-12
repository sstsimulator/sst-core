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

#include "SS_router.h"
//: Parcel data to LCB
// A parcel has arrived at the router via handle parcel function. Now
// it is put into the in LCB for the appropriate link
void SS_router::InLCB ( RtrEvent *e, int ilink, int ivc, int flits) {

    rtrP* rp = rpPool.getRp(); //new rtrP;
    DBprintf("rp=%p\n",rp);
    //create a delay packet for use in internal model
    rp->event = e;
    rp->ilink=ilink;
    rp->ivc = ivc;
    rp->flits = flits;

    if (!route(rp)) {
        printf ("%ld: Error: router %d could not route\n", cycle(), routerID);
        rpPool.returnRp(rp);
        return;
    }

    inLCB[ilink].size_flits += flits;

    if (!inLCB[ilink].internal_busy) {
        LCBtoInQ_start(rp);
        inLCB[ilink].internal_busy = true;
    } else {
        inLCB[ilink].dataQ.push_back(rp);
    }

    DBprintf ("%lld: router %d put parcel in iLCB %d, size %d, internal_busy? %d\n",
              cycle(), routerID, ilink, inLCB[ilink].size_flits,
              inLCB[ilink].internal_busy);
}

//: Move packets into the input queue
// when the packet moves to the front of the queue in the LCB, this function will send
// it to the input queue. There should always be room in the input queue when flow control
// works properly. An event is created for the time when the next data transfer can begin from the LCB
bool SS_router::LCBtoInQ_start (rtrP *rp)
{
    DBprintf("\n");
    //Event for when the iLCB can start the next transfer
    rtrEvent_t *event = eventPool.getEvent();
    event->cycle = cycle() + rp->flits;
    event->type = iLCB_internalXferDone;
    event->rp = rp;
    rtrEventQ.push_back(event);
    push_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);

    //Event for when the input queue gets the first flit
    event = eventPool.getEvent();
    event->cycle = cycle() + iQLat;
    event->type = InQ_tailXferDone;
    event->rp = rp;
    rtrEventQ.push_back(event);
    push_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);

    //reserve space in the input queue for this link / vc
    inLCB[rp->ilink].size_flits -= rp->flits;
    inputQ[rp->ilink].size_flits[rp->ivc] += rp->flits;

    DBprintf ("%lld: router %d, %d flits to inputQ %d:%d on cycle %lld size is %d\n",
              cycle(), routerID, rp->flits, rp->ilink, rp->ivc,
              event->cycle, inputQ[rp->ilink].size_flits[rp->ivc]);

    if ( inputQ[rp->ilink].size_flits[rp->ivc] >
            rtrInput_maxQSize_flits[rp->ilink]) {
        printf ("%ld: Error: rp %p to router %d, inputQ:%d:%d: size = %d, max size = %d, size %d\n",
                cycle(), rp, routerID, rp->ilink, rp->ivc, inputQ[rp->ilink].size_flits[rp->ivc],
                rtrInput_maxQSize_flits[rp->ilink], rp->flits);
        return false;
    }

    return true;
}

//: input LCB is no longer busy
// The input LCB is now free to start sending the next data to an input queue
void SS_router::LCBtoInQ_readyNext (rtrP *rp) {
    DBprintf("\n");
    inLCB[rp->ilink].internal_busy = false;

    DBprintf ("%lld: router %d iLCB %d ready next, parcel done\n",
              cycle(), routerID, rp->ilink );

    if (inLCB[rp->ilink].readyInternal()) {
        DBprintf ("%lld: router %d iLCB %d back in ready iLCB list\n", cycle(), routerID, rp->ilink);
        //ready_iLCB_count++;
        ready_iLCB = true;
    }
}

//: Data arrives at the Input Queue
// The input queue now has the packet and can send it onto an output queue
void SS_router::LCBtoInQ_done (rtrP *rp, int ivc, int ilink) {
    DBprintf("\n");
    inQ_t *inQ = &(inputQ[ilink]);
    //bool wasReady = inQ->ready();

    if (inQ->vcQ[ivc].empty())
        inQ->ready_vcQs++;
    //inQ->ready_vc = true;

    //put the actual packet in the input Q
    inQ->vcQ[ivc].push_back(rp);

    //If this input Q was previously empty, put it in the ready pile
    DBprintf ("%lld: router %d add inQ %d to ready list, vc %d -- %d ready vcQs\n",
              cycle(), routerID, rp->ilink, rp->ivc, inQ->ready_vcQs);

    if (inQ->ready())
        ready_inQ = true;

    //if ( !wasReady && (inQ->ready()) ) {
    //ready_inQs_count++;
    //}

    DBprintf ("%lld: router %d finish move from iLCB to inQ %d:%d\n",
              cycle(), routerID, rp->ilink, rp->ivc);
}

//: Start sending data from an input queue to an output queue
void SS_router::InQtoOutQ_start (rtrP *rp) {

    DBprintf("\n");
    //event for when the input queue can start another transfer
    rtrEvent_t *event = eventPool.getEvent();
    event->cycle = cycle() + rp->flits;
    event->type = InQ_headXferDone;
    event->rp = rp;
    rtrEventQ.push_back(event);
    push_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);

    //event for when the output queue gets the first flit
    event = eventPool.getEvent();
    event->cycle = cycle() + routingLat;
    event->type = OutQ_tailXferDone;
    event->rp = rp;
    rtrEventQ.push_back(event);
    push_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);

    //update the output queue size
    inputQ[rp->ilink].size_flits[rp->ivc] -= rp->flits;
    outputQ[rp->olink][rp->ilink].size_flits[rp->ovc] += rp->flits;
    inputQ[rp->ilink].head_busy = true;

    DBprintf ("%lld: router %d starting move from inQ %d:%d to oQ %d:%d, arrive time %lld\n",
              cycle(), routerID, rp->ilink, rp->ivc, rp->olink, rp->ovc, event->cycle);
}


//: Data has left the input queue
// The input queue can now send the next data to an output queue when enough
// tokens have arrived
void SS_router::InQtoOutQ_readyNext (rtrP *rp) {
    DBprintf("\n");
    inputQ[rp->ilink].head_busy = false;

    returnToken_flits( rp->ilink, rp->flits, rp->event->u.packet.vc() );

    //if (inputQ[rp->ilink].ready_vcQs > 0) {
    if (inputQ[rp->ilink].ready()) {
        DBprintf ("%lld: router %d inQ %d has %d ready vcQs, adding to ready list after move parcel\n",
                  cycle(), routerID, rp->ilink, inputQ[rp->ilink].ready_vcQs );
        //ready_inQs_count++;
        ready_inQ = true;
    }
}

//: Data tranferred to output queue
// Now the packet is fully in the output queue and the output queue can send
// it onto the LCB
void SS_router::InQtoOutQ_done (rtrP *rp, int ovc, int ilink, int olink) {
    oLCB_t *oLCB = &(outLCB[olink]);
    packetQ *theQ = &(outputQ[olink][ilink].vcQ[ovc]);
    //bool wasReady = (oLCB->readyInternal() || oLCB->readyXfer());

    //if (oLCB->front_rp[ovc][ilink] == NULL) {
    if (theQ->empty()) {
        oLCB->ready_outQ_count[ovc]++;
        if (oLCB->ready_outQ_count[ovc] == 1) {
            oLCB->ready_vc_count++;
        }
    }
    //oLCB->ready_vc = true;
    //oLCB->ready_outQ[ovc] = true;

    //put the actual packet in the ouput queue
    theQ->push_back(rp);

    if (!oLCB->internal_busy)
        ready_oLCB = true;
    //If the oLCB is not on the ready list, put it there
    //if (!wasReady && oLCB->readyInternal()) {
    //ready_oLCB_count++;
    //}

    DBprintf ("%lld: router %d moved parcel from InQ :%d:%d to oQ %d:%d, oLCB rdy\n",
              cycle(), routerID, ilink, rp->ivc, rp->olink, rp->ovc);
}

//: Start transfer data from output queue to ouput LCB
// The data will arrive at the output LCB after some delay
void SS_router::OutQtoLCB_start (rtrP *rp) {

    oLCB_t *oLCB;

    outLCB[rp->olink].vcTokens[rp->ovc] -= rp->flits;
    //No longer ready when MaxPacketSize not met
    if (outLCB[rp->olink].vcTokens[rp->ovc] < MaxPacketSize)
        outLCB[rp->olink].ready_vc_count--;
    oLCB = &(outLCB[rp->olink]);

    //update LCB size
    outputQ[rp->olink][rp->ilink].size_flits[rp->ovc] -= rp->flits;
    oLCB->size_flits += rp->flits;
    oLCB->internal_busy = true;

//#define DBprintf if (routerID == 446 && rp->ovc == 0 && cycle() >=42000) printf
    DBprintf ("%lld: router %d starting move parcel from oQ %d:%d (size %d) to oLCB, outq rdy on %lld, tokens at %d\n",
              cycle(), routerID, rp->olink, rp->ovc,
              outputQ[rp->olink][rp->ilink].size_flits[rp->ovc], cycle() + rp->flits, outLCB[rp->olink].vcTokens[rp->ovc]);

    //event for when the output queue can start the next transfer
    rtrEvent_t *event = eventPool.getEvent();
    event->cycle = cycle() + rp->flits;
    event->type = OutQ_headXferDone;
    event->rp = rp;
    rtrEventQ.push_back(event);
    push_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);

    //event for when the outLCB gets the first flit
    event = eventPool.getEvent();
    event->cycle = cycle() + oLCBLat;
    event->type = oLCB_internalXferDone;
    event->rp = rp;
    rtrEventQ.push_back(event);
    push_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);


    DBprintf ("%lld: router %d (%d:%d) tokens %d after send parcel\n",
              cycle(), routerID, rp->olink, rp->ovc,
              outLCB[rp->olink].vcTokens[rp->ovc] );
}

//: Data has left the output queue
// The output queue can send the next data to the LCB when the LCB is ready for it
void SS_router::OutQtoLCB_readyNext (rtrP *rp, int olink, int ilink, int ovc, int flits) {
    //outQ_t *oQ = &(outputQ[olink][ilink]);
    oLCB_t *oLCB = &(outLCB[olink]);
    //bool wasReady = (oLCB->readyInternal() || oLCB->readyXfer());

    oLCB->internal_busy = false;

    DBprintf ("%lld: router %d OutQ %d:%d finished moving parcel, will be at LCB on %lld\n",
              cycle(), routerID, olink, ovc, cycle() + oLCBLat);

    //if (!wasReady && oLCB->readyInternal()) {
    if (oLCB->ready_vc_count > 0) {
        DBprintf ("%lld: router %d put LCB %d back in ready list\n", cycle(), routerID, olink);
        //ready_oLCB_count++;
        ready_oLCB = true;
    }
}

//: Data arrives at LCB
// The output LCB can send the data on the link when the link is ready
void SS_router::OutQtoLCB_done (rtrP *rp) {
    oLCB_t *oLCB = &(outLCB[rp->olink]) ;
    //bool wasReady = (oLCB->readyInternal() || oLCB->readyXfer());

    //put the actual packet in the oLCB
    oLCB->dataQ.push_back(rp);

    if (!oLCB->external_busy)
        ready_oLCB = true;
    //If the oLCB is not on the ready list, put it there
    //if (!wasReady && oLCB->readyXfer()) {
    //ready_oLCB_count++;
    //}

    DBprintf ("%lld: router %d finish move parcel from oQ :%d:%d:%d to oLCB size %d, %d datum, %d vc_rr, oLCB rdy\n",
              cycle(), routerID, rp->olink, rp->ilink, rp->ovc, oLCB->size_flits,
              (int)oLCB->dataQ.size(), (int)oLCB->ready_vc_count);
}

//: Data sent out on the link
// The data will arrive at the next router after some delay
void SS_router::LCBxfer_start (int dir) {
    oLCB_t *oLCB = NULL;
    Link *dest = NULL;
    int dest_ilink;
    double flits_w_overhead;

    DBprintf("dir=%d link=%s\n",dir, LinkNames[dir] );
    oLCB = &(outLCB[dir]);

    if (dir == ROUTER_HOST_PORT) {
        dest = linkV[ROUTER_HOST_PORT];
        dest_ilink = -1;
    } else {
        dest = tx_netlinks[dir]->link;
        dest_ilink = tx_netlinks[dir]->dir;
    }

    rtrP *rp = oLCB->dataQ.front();
    networkPacket *np = &rp->event->u.packet;

    np->vc() = rp->ovc;
    np->link() = dest_ilink;

    DBprintf("dest_ilink=%d ovc=%d link %s\n",
                            dest_ilink, rp->ovc, LinkNames[dir] );

    //send event to the next router
    dest->Send( iLCBLat, rp->event );    

    DBprintf ("%lld: router %d start xfer parcel (from %d to %d), oLCB %d size %d, %d datum, %d vc_rr, %d tokens\n",
              cycle(), routerID, np->srcNum(), np->destNum(), rp->olink, oLCB->size_flits,
              (int)oLCB->dataQ.size(), (int)oLCB->ready_vc_count, oLCB->vcTokens[rp->ovc]);

    //make an event for xfer complete
    rtrEvent_t *event = eventPool.getEvent();
    flits_w_overhead = (double)(rp->flits) * overheadMultP;
    //flits_w_overhead = (double)(rp->flits);
    event->cycle = cycle() + (int)flits_w_overhead;
    event->type = oLCB_externalXferDone;
    event->rp = rp;
    rtrEventQ.push_back(event);
    push_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);

    oLCB->external_busy = true;
}

//: Data has left LCB
// The LCB can start the next data transfer when the link is ready
void SS_router::LCBxfer_done (rtrP *rp, int olink, int flits) {

    oLCB_t *oLCB = &(outLCB[olink]);
    //bool wasReady = (oLCB->readyInternal() || oLCB->readyXfer());

#ifdef ERRCHK
    if (oLCB->dataQ.front() != rp)
        printf ("%lld: Error: router %d got oLCB complete parcel %p but %p is front of data q\n",
                cycle(), routerID, rp->p, oLCB->dataQ.front());
#endif

    rpPool.returnRp(rp);
    //pop the parcel off the front of the deque
    oLCB->dataQ.pop_front();

    //update the size
    oLCB->size_flits -= flits;
    oLCB->external_busy = false;

    DBprintf ("%lld: router %d finish xfer parcel (size %d), oLCB %d size %d, %d datum, %d vc_rr\n",
              cycle(), routerID, flits, rp->olink, oLCB->size_flits,
              (int)oLCB->dataQ.size(), (int)oLCB->ready_vc_count);

    if (!oLCB->dataQ.empty())
        ready_oLCB = true;
    //If the LCB is not empty or has waiting out Qs, put it in the ready list
    //if ( !oLCB->sleeping() && (ready_oLCB[olink] == NULL) ) {
    //if (!wasReady && oLCB->readyXfer()) {
    //ready_oLCB_count++;
    //}

    txCount[olink]+= flits;
}

