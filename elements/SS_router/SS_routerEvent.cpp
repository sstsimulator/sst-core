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
//: Advance the event queue
// The event queue is advanced while there are events ready for this cycles
void SS_router::advanceEventQ () {
    rtrEvent_t *event;
    rtrP *rp;

    //Handle events from inputDelay
    DBprintf ("%lld: router %d event Q size %ld\n", cycle(), routerID, rtrEventQ.size());

    while ( (!rtrEventQ.empty()) && (rtrEventQ[0]->cycle <= cycle()))  {
        event = rtrEventQ[0];
        pop_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);
        rtrEventQ.pop_back();

        rp = event->rp;

        DBprintf ("%lld: router %d event parcel - type %d, cycle %lld\n",  cycle(), routerID,
                  event->type, event->cycle);

        switch (event->type) {

        case iLCB_internalXferDone:
            LCBtoInQ_readyNext(rp);
            eventPool.returnEvent(event);
            break;

        case InQ_tailXferDone:
            LCBtoInQ_done (rp, rp->ivc, rp->ilink);
            eventPool.returnEvent(event);
            break;

        case InQ_headXferDone:
            InQtoOutQ_readyNext (rp);
            eventPool.returnEvent(event);
            break;

        case OutQ_tailXferDone:
            InQtoOutQ_done (rp, rp->ovc, rp->ilink, rp->olink);
            eventPool.returnEvent(event);
            break;

        case OutQ_headXferDone:
            OutQtoLCB_readyNext(rp, rp->olink, rp->ilink, rp->ovc, rp->flits);
            eventPool.returnEvent(event);
            break;

        case oLCB_internalXferDone:
            OutQtoLCB_done(rp);
            eventPool.returnEvent(event);
            break;

        case oLCB_externalXferDone:
            LCBxfer_done(rp, rp->olink, rp->flits);
            eventPool.returnEvent(event);
            break;

        case Debug:
            DebugEvent();
            event->cycle = cycle() + debug_interval;
            rtrEventQ.push_back(event);
            push_heap(rtrEventQ.begin(), rtrEventQ.end(), rtrEvent_gt);
            break;

        default:
            printf ("%ld: router %d Error: unknown event type\n", cycle(), routerID);
        }
    }
}

//: Try to move data from an input LCB to input queue
// Data should always move freely from the ILCB to a queue as long as the iLCB is ready
// to send data
void SS_router::iLCBtoIn () {

    iLCB_t *iLCB;

    DBprintf("\n");
    for (int i = 0; i < ROUTER_NUM_INQS; i++) {
        if (inLCB[i].readyInternal()) {
            iLCB = &(inLCB[i]);

            LCBtoInQ_start (iLCB->dataQ.front());
            DBprintf ("%lld: router %d iLCB %d internal busy\n", cycle(), routerID, iLCB->link);
            iLCB->internal_busy = true;
            iLCB->dataQ.pop_front();
        }
    }

    //ready_iLCB_count = 0;
    ready_iLCB = false;
}

//: Try to move data from an input to output queue
// Each input queue RRs around each of its vcs each cycle, attempting to
// move a packet to an output queue, which can happen if there are enough tokens
// available. If a virtual channel has some data that is skipped due to lack of tokens,
// it is stored in the skip buffer, so that the next time that the output queue it was trying for
// it ready, it can transfer data. This prevents a starvation situation.
void SS_router::arbitrateInToOut () {

    DBprintf ("%lld: router %d arbitrate In to Out\n", cycle(), routerID);

    int ilink, ivc;
    inQ_t *inQ;
    packetQ *theQ;
    rtrP *rp;
    int max_count;
    int rcount = 0;

    for (int i = 0; i < ROUTER_NUM_INQS; i++) {
        int queue_selected = 0;
        int in_queue = 0;
        if (inputQ[i].ready()) {
            inQ = &(inputQ[i]);
            ilink = inQ->link;

            DBprintf ("%lld: router %d has input Q %d ready, %d ready rr Qs\n",
                      cycle(), routerID, ilink, inQ->ready_vcQs);

//#define DBprintf if (((routerID==446) || (routerID==447) || (routerID==439) || (routerID==431) || (routerID==423) || (routerID==415) || (routerID==407) || (routerID==399)) && cycle() >= 42000) printf
            //Find a VC that is ready and has room to flow.  Start with the skipped queues.
            max_count = ROUTER_NUM_VCS + inQ->skipQs.size();
            //#define DBprintf if (routerID == 357 && cycle() > 30000) printf
            DBprintf ("%lld: router %d has %d ready rr Qs, %ld skipped queues on iQ %d\n", cycle(), routerID,
                      inQ->ready_vcQs, inQ->skipQs.size(), i);
            while (!queue_selected && max_count > 0) {
                if (max_count > ROUTER_NUM_VCS) {
                    ivc = inQ->skipQs.front();
                    inQ->skipQs.pop();
                    theQ = &(inQ->vcQ[ivc]);
                    rp = theQ->front();
                    if (outputQ[rp->olink][ilink].size_flits[rp->ovc] + rp->flits <= rtrOutput_maxQSize_flits[rp->olink]) {
                        queue_selected = 1;
                        DBprintf ("%lld: router %d skipped VC %d unblocked and selected for output %d:%d:%d\n", cycle(), routerID,
                                  ivc, rp->olink, rp->ovc, ilink);
                    }
                    else {
                        inQ->skipQs.push(ivc);
                        in_queue |= 1 << ivc;
                        DBprintf ("%lld: router %d skipped VC %d blocked and requeued with %d in output queue %d:%d:%d\n",
                                  cycle(), routerID, ivc, outputQ[rp->olink][ilink].size_flits[rp->ovc], rp->olink, rp->ovc,
                                  ilink);
                    }
                }
                else {
                    ivc = inQ->vc_rr;
                    inQ->vc_rr = (inQ->vc_rr + 1) % ROUTER_NUM_VCS;
                    theQ = &(inQ->vcQ[ivc]);
                    rp = theQ->front();
                    //if (cycle() == 824 && routerID == 357) printf ("checking %d ", ivc);

                    if (!theQ->empty()) {
                        //if (cycle() == 824 && routerID == 357) printf ("and it has data\n");
                        if (outputQ[rp->olink][ilink].size_flits[rp->ovc] + rp->flits <= rtrOutput_maxQSize_flits[rp->olink]) {
                            queue_selected = 1;
                            DBprintf ("%lld: router %d regular VC %d unblocked and selected for output %d:%d:%d\n", cycle(), routerID,
                                      ivc, rp->olink, rp->ovc, ilink);
                        }
                        else {
                            //if (cycle() == 824 && routerID == 357) printf ("but it should be blocked\n");

                            if (!(in_queue & (1 << ivc))) {
                                inQ->skipQs.push(ivc);
                                in_queue |= (1 << ivc);
                                DBprintf ("%lld: router %d regular VC %d blocked and queued with %d in output queue %d:%d:%d\n",
                                          cycle(), routerID, ivc, outputQ[rp->olink][ilink].size_flits[rp->ovc], rp->olink,
                                          rp->ovc, ilink);
                            }
                        }
                    }
                    //else
                    //if (cycle() == 824 && routerID == 357) printf ("and it is empty\n");
                }
                max_count--;
            }

            if (!queue_selected) {
                if ( inQ->skipQs.size() == 0) {
                    printf ("%ld: Error router %d inQ %d has vc ready count %d, no queues ready\n",
                            cycle(), routerID, i, inQ->ready_vcQs);
                }
                continue;
            }

            DBprintf ("%ld: router %d theQ size %ld rp %p\n", cycle(), routerID, theQ->size(), rp);

            DBprintf ("%ld: router %d trying parcel from inQ %d:%d to %d:%d (size %d)\n",
                      cycle(), routerID, ilink, ivc, rp->olink, rp->ovc,
                      (outputQ[rp->olink][ilink].size_flits[rp->ovc]));
            theQ->pop_front();
            InQtoOutQ_start (rp);
            if (theQ->empty()) {
                inQ->ready_vcQs--;
                DBprintf ("%lld: router %d inQ %d down to %d ready vcs\n", cycle(), routerID, ilink, inQ->ready_vcQs);
            }
        }

        if (!(inputQ[i].ready()))
            rcount++;
    }

    if (rcount == ROUTER_NUM_INQS)
        ready_inQ = false;
}

//: Try to move data from output queues to output LCB
// The output LCB round robins between each of the four virtual channels. When a
// virtual channel is up in the rotation, it uses a separate round robin for the
// six output queues corresponding to that virtual channel. This inner round robin
// will not advance until the queue that it points to can move data. This prevents
// a starvation situation
void SS_router::arbitrateOutToLCB () {

    int ovc, ilink;
    rtrP *rp;
    oLCB_t *oLCB;
    int max_count;
    int rcount = 0;
    //bool again;
    //deque<rtrP*> *theQ;

//if (!(cycle() % 500) && routerID == 0) printf ("cycle %lld\n", cycle());
//#define DBprintf if (((routerID==446) || (routerID==447) || (routerID==439) || (routerID==431) || (routerID==423) || (routerID==415) || (routerID==407) || (routerID==399)) && cycle() >= 42000) printf
    //Operate on all of the output Qs
//#define DBprintf if (cycle() >= 4600 && routerID==345) printf
//#define DBprintf printf
    for (int i = 0; i < ROUTER_NUM_OUTQS; i++) {
        oLCB = 0;
        int in_queue = 0;
        DBprintf ("%lld: router %d is checking outq %d\n", cycle(), routerID, i);
        //Make sure to push LCB data when possible
        if (outLCB[i].readyXfer()) {
            DBprintf ("%lld: router %d oLCB %d ready to send Data\n", cycle(), routerID, outLCB[i].link);
            LCBxfer_start (i);
        }

        //If the LCB can take internal data
        if (outLCB[i].readyInternal()) {
//#if 0
            oLCB = &(outLCB[i]);
            int vc_selected = 0;
            int curr_vc;
            DBprintf ("%lld: router %d oLCB %d is ready\n", cycle(), routerID, i);
            //Pick a VC.  VCs that cannot handle a MaxPacketSize can be skipped.
            //Mark the ones that are skipped.
            max_count = ROUTER_NUM_VCS + oLCB->skipped_vcs.size();
            DBprintf ("%lld: router %d oLCB %d max_count %d\n", cycle(), routerID, i, max_count);
            while (!vc_selected && max_count > 0) {
                if (max_count > ROUTER_NUM_VCS) {
                    curr_vc = oLCB->skipped_vcs.front();
                    oLCB->skipped_vcs.pop();
                    //If it doen't have data, it wouldn't be on the skipped list
                    if ( oLCB->vcTokens[curr_vc] < MaxPacketSize) {

                        oLCB->skipped_vcs.push(curr_vc);
                        in_queue |= 1 << curr_vc;
                        if (oLCB->ready_outQ_count[curr_vc]<=0)
                            printf ("%ld: router %d oLCB %d skipped VC %d skipped with %d tokens and %d ready\n",
                                    cycle(), routerID, i, curr_vc, oLCB->vcTokens[curr_vc],
                                    oLCB->ready_outQ_count[curr_vc]);

                        DBprintf ("%lld: router %d oLCB %d skipped VC %d skipped with %d tokens and %d ready\n",
                                  cycle(), routerID, i, curr_vc, oLCB->vcTokens[curr_vc], oLCB->ready_outQ_count[curr_vc]);
                    } else {
                        DBprintf ("%ld: router %d oLCB %d skipped VC %d selected\n",
                                  cycle(), routerID, i, curr_vc);
                        if (oLCB->ready_outQ_count[curr_vc]<=0)
                            printf ("%ld: router %d oLCB %d skipped VC %d selected with %d tokens and %d ready\n",
                                    cycle(), routerID, i, curr_vc, oLCB->vcTokens[curr_vc],
                                    oLCB->ready_outQ_count[curr_vc]);

                        vc_selected = 1;
                        ovc = curr_vc;
                    }
                } else {
                    curr_vc = oLCB->vc_rr;
                    oLCB->vc_rr = (oLCB->vc_rr + 1) % ROUTER_NUM_VCS;

                    // No tokens
                    if ((oLCB->vcTokens[curr_vc] < MaxPacketSize)
                            && (oLCB->ready_outQ_count[curr_vc] > 0)) {

                        if (!(in_queue & (1 << curr_vc))) {
                            oLCB->skipped_vcs.push(curr_vc);
                            in_queue |= 1 << curr_vc;
                            if (oLCB->ready_outQ_count[curr_vc]<=0)
                                printf ("%ld: router %d oLCB %d regular VC %d skipped with %d tokens and %d ready\n",
                                        cycle(), routerID, i, curr_vc, oLCB->vcTokens[curr_vc],
                                        oLCB->ready_outQ_count[curr_vc]);

                            DBprintf ("%lld: router %d oLCB %d regular VC %d skipped with %d tokens and %d ready\n",
                                      cycle(), routerID, i, curr_vc, oLCB->vcTokens[curr_vc],
                                      oLCB->ready_outQ_count[curr_vc]);
                        }
                    } else if (oLCB->ready_outQ_count[curr_vc] > 0) {
                        DBprintf ("%lld: router %d oLCB %d regular VC %d selected\n",
                                  cycle(), routerID, i, curr_vc);
                        if (in_queue & (1 << curr_vc))
                            printf("%ld: router %d oLCB %d selecting VC %d in queue\n",
                                   cycle(), routerID, i, curr_vc);

                        vc_selected = 1;
                        ovc = curr_vc;
                    }
                }
                max_count--;
                DBprintf ("%lld: router %d oLCB %d max_count decremented to %d\n", cycle(), routerID, i, max_count);
            }

            if (vc_selected == 0) {
                DBprintf ("%lld: router %d oLCB %d no VCs ready\n",
                          cycle(), routerID, i);
                continue;
            }

            //VC is ready.  Choose an output queue.
            DBprintf ("%lld: router %d vc %d ready\n", cycle(), routerID, ovc);

            int *rr = &(oLCB->ilink_rr[ovc]);
            oLCB->ilink_rr[ovc] = (oLCB->ilink_rr[ovc] + 1) % ROUTER_NUM_INQS;
            outQ_t *oQs = outputQ[oLCB->link];
            max_count = ROUTER_NUM_INQS;
            //If the output queue has data, it MUST be able to flow.  We have
            //MaxPacketSize flits available
            while ((oQs[*rr].vcQ[ovc].empty()) && (max_count > 0)) {
                DBprintf ("%lld: router %d oLCB %d VC %d input %d empty\n",
                          cycle(), routerID, oLCB->link, ovc, *rr);
                *rr = (*rr + 1) % ROUTER_NUM_INQS;
                max_count--;
            }

            if (max_count == 0) {
                printf ("%ld: Error: router %d LCB %d had vc %d ready count %d vc_selected %d, but no ready output queues\n",
                        cycle(), routerID, oLCB->link, ovc, oLCB->ready_outQ_count[ovc], vc_selected);
                continue;
            }

            ilink = *rr;
            deque<rtrP*> *theQ = &(oQs[ilink].vcQ[ovc]);
            DBprintf ("%lld: router %d has output Q %d:%d:%d ready, size %d\n",
                      cycle(), routerID, oLCB->link, ilink, ovc, (int)theQ->size());

            rp = theQ->front();
            DBprintf ("%lld: router %d out Q %d:%d:%d trying to send parcel to oLCB\n", cycle(),
                      routerID, oLCB->link, ilink, ovc );

            if (oLCB->size_flits + rp->flits >= oLCB_maxSize_flits)
                printf("%ld: router %d out Q %d:%d:%d  Error! not enough space in oLCB!\n",
                       cycle(), routerID, oLCB->link, ilink, ovc);
            if (oLCB->vcTokens[ovc] < rp->flits)
                printf("%ld: router %d out Q %d:%d:%d  Error! not enough tokens in VC!\n",
                       cycle(), routerID, oLCB->link, ilink, ovc);

            oLCB->vc_rr = (oLCB->vc_rr + 1) % ROUTER_NUM_VCS;
            theQ->pop_front();
            if (theQ->empty()) {
                oLCB->ready_outQ_count[ovc]--;
                if (oLCB->ready_outQ_count[ovc] <= 0) {
                    oLCB->ready_vc_count--;
                }
            }

            // We have a packet, move it along.
            OutQtoLCB_start(rp);
//#endif
#if 0

            again = true;;
            while (again) {
                again = false;

                DBprintf ("%lld: router %d oLCB %d ready to move internal Data\n",
                          cycle(), routerID, outLCB[i].link);

                oLCB = &(outLCB[i]);
                max_count = ROUTER_NUM_VCS;
                //while ((oLCB->ready_outQ_count[oLCB->vc_rr] <= 0) && (max_count > 0)) {
                while (( (oLCB->ready_outQ_count[oLCB->vc_rr] <= 0) ||
                         (oLCB->stall4tokens[oLCB->vc_rr] > 0)  ||
                         (oLCB->vcTokens[oLCB->vc_rr] < ed_NIC::MaxPacketSize))
                        && (max_count > 0)) {
                    DBprintf ("%lld: router %d oLCB %d VC %d stalled: outQ count %d, stall 4 %d\n",
                              cycle(), routerID, oLCB->link, oLCB->vc_rr, oLCB->ready_outQ_count[oLCB->vc_rr],
                              oLCB->stall4tokens[oLCB->vc_rr]);
                    oLCB->vc_rr = (oLCB->vc_rr + 1) % ROUTER_NUM_VCS;
                    max_count--;
                }

                if (max_count == 0) {
                    break;
                    printf ("%lld: Error: router %d LCB %d had %d ready vcs, but no ready output queues\n",
                            cycle(), routerID, oLCB->link, oLCB->ready_vc_count);
                }

                ovc = oLCB->vc_rr;
                deque<rtrP*> *theQ;

                int *rr = &(oLCB->ilink_rr[ovc]);
                outQ_t *oQs = outputQ[oLCB->link];
                max_count = ROUTER_NUM_INQS;

                //TODO: right now, a cycle can be wasted by picking a vcQ that doesn't
                // have enough tokens, then get all the way down to the bottom and
                // do nothing, when another queue may be ok to send
                while ((oQs[*rr].vcQ[ovc].empty()) && (max_count > 0)) {
                    DBprintf ("%lld: router %d oLCB %d VC %d input %d empty\n",
                              cycle(), routerID, oLCB->link, ovc, *rr);
                    *rr = (*rr + 1) % ROUTER_NUM_INQS;
                    max_count--;
                }

                if (max_count == 0) {
                    printf ("%lld: Error: router %d LCB %d had vc %d ready count %d, but no ready output queues\n",
                            cycle(), routerID, oLCB->link, ovc, oLCB->ready_outQ_count[ovc]);
                }

                ilink = *rr;
                theQ = &(oQs[ilink].vcQ[ovc]);

                //#ifdef ERRCHK
                DBprintf ("%lld: router %d has output Q %d:%d:%d ready, size %d\n",
                          cycle(), routerID, oLCB->link, ilink, ovc, (int)theQ->size());

                rp = theQ->front();
                DBprintf ("%lld: router %d out Q %d:%d:%d trying to send %p to oLCB\n", cycle(),
                          routerID, oLCB->link, ilink, ovc, rp->p);

                if (oLCB->size_flits + rp->flits <= oLCB_maxSize_flits) {
                    //oLCB->vc_rr = (oLCB->vc_rr + 1) % ROUTER_NUM_VCS;

                    //if (oLCB->vcTokens[ovc] >= rp->flits) {
                    if (oLCB->vcTokens[ovc] < rp->flits) {
                        printf ("%lld: Error router %d OVC not enough flits\n");
                        oLCB->stall4tokens[ovc] = rp->flits - oLCB->vcTokens[ovc];
                        oLCB->ready_vc_count--;
                        if (oLCB->ready_vc_count > 0)
                            again = true;
                    } else {
                        oLCB->vc_rr = (oLCB->vc_rr + 1) % ROUTER_NUM_VCS;
                        DBprintf ("%lld: router %d oQ %d:%d:%d ok to send %p, %d flits, %d tokens\n",
                                  cycle(), routerID, oLCB->link, ilink, ovc, rp->p,
                                  rp->flits, oLCB->vcTokens[ovc]);

                        oLCB->ilink_rr[ovc] = (oLCB->ilink_rr[ovc] + 1) % ROUTER_NUM_INQS;
                        theQ->pop_front();

                        if (theQ->empty()) {
                            oLCB->ready_outQ_count[ovc]--;
                            if (oLCB->ready_outQ_count[ovc] <= 0) {
                                oLCB->ready_vc_count--;
                            }
                        }
                        OutQtoLCB_start(rp);
                    }
                }
            }
#endif
        }

        if ( !outLCB[i].readyXfer() && !outLCB[i].readyInternal())
            rcount++;
    }
    if (rcount == ROUTER_NUM_OUTQS)
        ready_oLCB = false;
}


void SS_router::DebugEvent() {

    vector<rtrEvent_t*> tmp;
    rtrEvent_t *event;

    for (unsigned int i = 0; i < rtrEventQ.size(); i++) {
        tmp.push_back(rtrEventQ[i]);
    }

    printf ("DebugEvent: %ld: router %d event q size %ld\n", cycle(), routerID, (long int)rtrEventQ.size());

    while (!tmp.empty()) {
        event = tmp[0];
        pop_heap(tmp.begin(), tmp.end(), rtrEvent_gt);
        tmp.pop_back();

        printf ("Debug: %ld: router %d event %d, cycle %lld\n", cycle(), routerID, event->type, event->cycle);
    }

}

//: This event can be used for debugging purposes
/*
void SS_router::DebugEvent() {
  int olink, ilink, ovc, ivc;
  int sum1, sum2;

  ivc = 0;

  for (olink = 0; olink < ROUTER_NUM_OUTQS; olink++)
    for (ovc = 0; ovc < ROUTER_NUM_VCS; ovc++) {
      if (outLCB[olink].vcTokens[ovc] > rtrInput_maxQSize_flits)
	printf ("%lld: DEBUG rtr %d too many tokens (%d) on %d:%d\n",
		cycle(), routerID, outLCB[olink].vcTokens[ovc], olink, ovc);

    }

  sum1 = 0;
  sum2 = 0;

  for (ilink = 0; ilink < ROUTER_NUM_INQS; ilink++) {
    if (ready_inQs[ilink] != NULL)
      sum1++;
    if (ready_iLCB[ilink] != NULL)
      sum2++;
  }

  if (sum1 != ready_inQs_count)
    printf ("%lld: DEBUG rtr %d has %d ready inQs count = %d\n", cycle(),
	    routerID, sum1, ready_inQs_count);

  if (sum2 != ready_iLCB_count)
    printf ("%lld: DEBUG rtr %d has %d ready iLCB count = %d\n", cycle(),
	    routerID, sum2, ready_iLCB_count);


  sum1 = 0;
  for (olink = 0; olink < ROUTER_NUM_OUTQS; olink++) {
     if (ready_oLCB[olink] != NULL)
      sum1++;
     else if (!outLCB[olink].sleeping())
       printf ("%lld: DEBUG router %d oLCB %d not sleeping not ready\n", cycle(),
	       routerID, olink);
  }

  if (sum1 != ready_oLCB_count)
    printf ("%lld: DEBUG rtr %d has %d ready oLCB count = %d\n", cycle(),
	    routerID, sum2, ready_oLCB_count);

}

*/
