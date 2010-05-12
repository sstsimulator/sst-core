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


#ifndef COMPONENTS_TRIG_CPU_BCAST_TREE_TRIGGERED_H
#define COMPONENTS_TRIG_CPU_BCAST_TREE_TRIGGERED_H

#include "algorithm.h"
#include "trig_cpu.h"
#include "portals.h"

class bcast_tree_triggered :  public algorithm {
public:
    bcast_tree_triggered(trig_cpu *cpu) : algorithm(cpu), init(false)
    {
        radix = cpu->getRadix();
        ptl = cpu->getPortalsHandle();

        msg_size = cpu->getMessageSize();
        chunk_size = cpu->getChunkSize();

        // compute my root and children
        boost::tie(my_root, my_children) = buildBinomialTree(radix);
        num_children = my_children.size();

        in_buf = (char*) malloc(msg_size);
        for (i = 0 ; i < msg_size ; ++i) {
            in_buf[i] = i % 255;
        }
        out_buf = (char*) malloc(msg_size);
        bounce_buf = (char*) malloc(chunk_size);
    }

    bool
    operator()(Event *ev)
    {
        ptl_md_t md;
        ptl_me_t me;

        crBegin();

        if (!init) {
            /* Setup persistent ME/MD/CT to hold bounce data */
            ptl->PtlCTAlloc(PTL_CT_OPERATION, bounce_ct_h);
            crReturn();
            me.start = bounce_buf;
            me.length = chunk_size;
            me.match_bits = 0x0;
            me.ignore_bits = 0x0;
            me.ct_handle = bounce_ct_h;
            ptl->PtlMEAppend(PT_BOUNCE, me, PTL_PRIORITY_LIST, NULL, bounce_me_h);
            crReturn();

            md.start = bounce_buf;
            md.length = chunk_size;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &bounce_md_h);
            crReturn();

            init = true;
        }

        /* Initialization case */
        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        // Create description of user buffer.
        ptl->PtlCTAlloc(PTL_CT_OPERATION, out_me_ct_h);
        crReturn();
        me.start = out_buf;
        me.length = msg_size;
        me.match_bits = 0x0;
        me.ignore_bits = 0x0;
        me.ct_handle = out_me_ct_h;
        ptl->PtlMEAppend(PT_OUT, me, PTL_PRIORITY_LIST, NULL, out_me_h);
        crReturn();

        ptl->PtlCTAlloc(PTL_CT_OPERATION, out_md_ct_h);
        crReturn();
        md.start = out_buf;
        md.length = msg_size;
        md.eq_handle = PTL_EQ_NONE;
        md.ct_handle = out_md_ct_h;
        ptl->PtlMDBind(md, &out_md_h);
        crReturn();

        /* long protocol only for now */
        if (my_id == my_root) {
            /* copy to self */
            memcpy(out_buf, in_buf, msg_size);
            /* send to children */
            for (j = 0 ; j < msg_size ; j += chunk_size) {
                for (i = 0 ; i < num_children ; ++i) {
                    ptl->PtlPut(bounce_md_h, 0, 0, 0, my_children[i],
                                PT_BOUNCE, 0x0, 0, NULL, 0);
                    crReturn();
                }
            }
            
        } else {
            for (j = 0 ; j < msg_size ; j += chunk_size) {
                /* when a chunk is ready, issue get. */
                comm_size = (msg_size - j > chunk_size) ? 
                    chunk_size : msg_size - j;
                ptl->PtlTriggeredGet(out_md_h, j, comm_size, my_root,
                                     PT_OUT, 0x0, NULL, j, bounce_ct_h,
                                     j / chunk_size);
                crReturn();

                /* then when the get is completed, send ready acks to children */
                for (i = 0 ; i < num_children ; ++i) {
                    ptl->PtlTriggeredPut(bounce_md_h, 0, 0, 0, my_children[i],
                                         PT_BOUNCE, 0x0, 0, NULL, 0, out_md_ct_h,
                                         j / chunk_size);
                    crReturn();
                }
            }

            /* reset 0-byte put received counter */
            count = (msg_size + chunk_size - 1) / chunk_size;
            ptl->PtlTriggeredCTInc(bounce_ct_h, -count, bounce_ct_h, count);
            crReturn();
        }

        if (num_children > 0) {
            /* wait for completion */
            count = num_children * ((msg_size + chunk_size - 1) / chunk_size);
            while (!ptl->PtlCTWait(out_me_ct_h, count)) { crReturn(); }
        } else {
            /* wait for local gets to complete */
            count = (msg_size + chunk_size - 1) / chunk_size;
            while (!ptl->PtlCTWait(out_md_ct_h, count)) { crReturn(); }
        }
        crReturn();

        ptl->PtlCTFree(out_me_ct_h);
        crReturn();
        ptl->PtlMEUnlink(out_me_h);
        crReturn();
        ptl->PtlCTFree(out_md_ct_h);
        crReturn();
        ptl->PtlMDRelease(out_md_h);
        crReturn();

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        crFinish();
        return true;
    }

private:
    bcast_tree_triggered();
    bcast_tree_triggered(const algorithm& a);
    void operator=(bcast_tree_triggered const&);

    bool init;
    portals *ptl;

    SimTime_t start_time;
    int radix;
    int i, j;

    int msg_size;
    int chunk_size;
    int comm_size;
    int count;

    char *in_buf;
    char *out_buf;
    char *bounce_buf;
    
    ptl_handle_ct_t bounce_ct_h; /* short (me), long (me) */
    ptl_handle_me_t bounce_me_h; /* short, long */
    ptl_handle_md_t bounce_md_h; /* short, long */

    ptl_handle_ct_t out_me_ct_h; /* short (me), long (me) */
    ptl_handle_me_t out_me_h; /* short, long */
    ptl_handle_ct_t out_md_ct_h; /* long (md) */
    ptl_handle_md_t out_md_h; /* short, long */

    ptl_handle_ct_t ack_ct_h; /* short (me) */
    ptl_handle_me_t ack_me_h; /* short */
    ptl_handle_md_t ack_md_h; /* short */

    int my_root;
    std::vector<int> my_children;
    int num_children;

    static const int PT_BOUNCE = 0;
    static const int PT_ACK    = 1;
    static const int PT_OUT    = 2;
};

#endif // COMPONENTS_TRIG_CPU_BCAST_TREE_TRIGGERED_H
