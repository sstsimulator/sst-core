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


#ifndef COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_TRIGGERED_H
#define COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_TRIGGERED_H

#include "algorithm.h"
#include "trig_cpu.h"
#include "portals.h"

class allreduce_tree_triggered :  public algorithm {
public:
    allreduce_tree_triggered(trig_cpu *cpu) : algorithm(cpu)
    {
        radix = cpu->getRadix();
        ptl = cpu->getPortalsHandle();

        // compute my root and children
        boost::tie(my_root, my_children) = buildBinomialTree(radix);
        num_children = my_children.size();
    }

    bool
    operator()(Event *ev)
    {
        ptl_md_t md;
        ptl_me_t me;

        switch (state) {
        case 0:
            /* Setup persistent ME to hold answer data */

            // setup md handles
            ptl->PtlCTAlloc(PTL_CT_OPERATION, up_tree_ct_h);
            me.start = NULL;
            me.length = 8;
            me.ignore_bits = ~0x0;
            me.ct_handle = up_tree_ct_h;
            ptl->PtlMEAppend(PT_UP, me, PTL_PRIORITY_LIST, NULL, up_tree_me_h);
	
            md.start = NULL;
            md.length = 8;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &up_tree_md_h);

            md.start = NULL;
            md.length = 8;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &zero_md_h);

            state = 1;
            break;

        case 1:
            /* setup algorithm.  This is the point we should reset to in the completion case. */

            // 200ns startup time
            start_time = cpu->getCurrentSimTimeNano();
            cpu->addBusyTime("200ns");

            // Create description of user buffer.  We can't possibly have
            // a result to need this information before we add our portion
            // to the result, so this doesn't need to be persistent.
            ptl->PtlCTAlloc(PTL_CT_OPERATION, user_ct_h);
            me.start = NULL;
            me.length = 8;
            me.ignore_bits = ~0x0;
            me.ct_handle = user_ct_h;
            ptl->PtlMEAppend(PT_DOWN, me, PTL_PRIORITY_LIST, NULL, user_me_h);

            md.start = NULL;
            md.length = 8;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &user_md_h);

            state = (num_children == 0) ? 2 : 3;
            break;

        case 2:
            // leaf node - push directly to the upper level's up tree
            ptl->PtlAtomic(user_md_h, 0, 8, 0, my_root, PT_UP, 0, 0, NULL, 0, PTL_SUM, PTL_DOUBLE);
            state = 8;
            break;

        case 3:
            // add our portion to the mix
            ptl->PtlAtomic(user_md_h, 0, 8, 0, my_id, PT_UP, 0, 0, NULL, 
                           0, PTL_SUM, PTL_DOUBLE);
            state = 4;
            break;

        case 4:
            if (0 == my_id) {
                // setup trigger to move data to right place, then send
                // data out of there down the tree
                ptl->PtlTriggeredPut(up_tree_md_h, 0, 8, 0, my_root, PT_DOWN, 0, 0, NULL, 
                                     0, up_tree_ct_h, num_children+1);
            } else {
                // setup trigger to move data up the tree when we get enough updates
                ptl->PtlTriggeredAtomic(up_tree_md_h, 0, 8, 0, my_root, PT_UP,
                                        0, 0, NULL, 0, PTL_SUM, PTL_DOUBLE,
                                        up_tree_ct_h, num_children+1);
            }
            state = 5;
            break;

        case 5:
            // and to clean up after ourselves
            ptl->PtlTriggeredPut(zero_md_h, 0, 8, 0, my_id, PT_UP, 0, 0, NULL, 
                                 0, up_tree_ct_h, num_children+1);
            state = 6;
            break;

        case 6:
            ptl->PtlTriggeredCTInc(up_tree_ct_h, -(num_children+2), up_tree_ct_h, num_children+2);

            loop_var = 0;
            state = 7;
            break;

        case 7:
            // setup trigger to move data down the tree when we get our down data
            if (loop_var < num_children) {
                ptl->PtlTriggeredPut(user_md_h, 0, 8, 0, my_children[num_children-1-loop_var], PT_DOWN,
                                     0, 0, NULL, 0, user_ct_h, 1);
                loop_var++;
                break;
            }
            state = 8;
            break;

        case 8:
            if (ptl->PtlCTWait(user_ct_h, 1))  state = 9;
            break;

        case 9:
            trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);
            // Unbind ME so that next iteration will work
            ptl->PtlMEUnlink(user_me_h);
            state = 1;
            return true;

        default:
            printf("triggered tree: unhandled state: %d\n", state);
            exit(1);
        }

        return false;
    }

private:
    allreduce_tree_triggered();
    allreduce_tree_triggered(const algorithm& a);
    void operator=(allreduce_tree_triggered const&);

    portals *ptl;

    SimTime_t start_time;
    int radix;
    int curr_radix;
    int level;
    int loop_var;

    ptl_handle_ct_t up_tree_ct_h;
    ptl_handle_me_t up_tree_me_h;
    ptl_handle_md_t up_tree_md_h;

    ptl_handle_ct_t user_ct_h;
    ptl_handle_me_t user_me_h;
    ptl_handle_md_t user_md_h;

    ptl_handle_md_t zero_md_h;

    int my_root;
    std::vector<int> my_children;
    int num_children;

    static const int PT_UP = 0;
    static const int PT_DOWN = 1;
};

#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_TRIGGERED_H
