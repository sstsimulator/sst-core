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


#ifndef COMPONENTS_TRIG_CPU_BARRIER_TREE_TRIGGERED_H
#define COMPONENTS_TRIG_CPU_BARRIER_TREE_TRIGGERED_H

#include "algorithm.h"
#include "trig_cpu.h"
#include "portals.h"

class barrier_tree_triggered :  public algorithm {
public:
    barrier_tree_triggered(trig_cpu *cpu) : algorithm(cpu), init(false)
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

        crBegin();

        if (!init) {
            // setup md handles
            ptl->PtlCTAlloc(PTL_CT_OPERATION, up_tree_ct_h);
            crReturn();
            me.start = NULL;
            me.length = 0;
            me.match_bits = 0;
            me.ignore_bits = 0;
            me.ct_handle = up_tree_ct_h;
            ptl->PtlMEAppend(PT_UP, me, PTL_PRIORITY_LIST, NULL, up_tree_me_h);
            crReturn();
	
            ptl->PtlCTAlloc(PTL_CT_OPERATION, down_tree_ct_h);
            crReturn();
            me.start = NULL;
            me.length = 0;
            me.match_bits = 0;
            me.ignore_bits = 0;
            me.ct_handle = down_tree_ct_h;
            ptl->PtlMEAppend(PT_DOWN, me, PTL_PRIORITY_LIST, NULL, down_tree_me_h);
            crReturn();
	
            md.start = NULL;
            md.length = 0;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &my_md_h);
            crReturn();

            init = true;
        }

        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        if (num_children == 0) {
            ptl->PtlPut(my_md_h, 0, 0, 0, my_root, PT_UP, 0, 0, NULL, 0);
            crReturn();
        } else {
            if (my_id != my_root) {
                ptl->PtlTriggeredPut(my_md_h, 0, 0, 0, my_root, PT_UP, 0, 0, NULL, 0, 
                                     up_tree_ct_h, num_children);
                crReturn();
            } else {
                ptl->PtlTriggeredCTInc(down_tree_ct_h, 1, up_tree_ct_h, num_children);
                crReturn();
            }
            ptl->PtlTriggeredCTInc(up_tree_ct_h, -num_children, 
                                   up_tree_ct_h, num_children);
            crReturn();

            for (i = 0 ; i < num_children ; ++i) {
                ptl->PtlTriggeredPut(my_md_h, 0, 0, 0, my_children[i], PT_DOWN, 
                                     0, 0, NULL, 0, down_tree_ct_h, 1);
                crReturn();
            }
        }

        while (!ptl->PtlCTWait(down_tree_ct_h, 1)) { crReturn(); }
        crReturn(); 
        ptl->PtlTriggeredCTInc(down_tree_ct_h, -1, down_tree_ct_h, 1);
        crReturn(); 

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        crFinish();
        return true;
    }

private:
    barrier_tree_triggered();
    barrier_tree_triggered(const algorithm& a);
    void operator=(barrier_tree_triggered const&);

    portals *ptl;

    SimTime_t start_time;
    int radix;
    bool init;

    ptl_handle_ct_t up_tree_ct_h;
    ptl_handle_me_t up_tree_me_h;

    ptl_handle_ct_t down_tree_ct_h;
    ptl_handle_me_t down_tree_me_h;

    ptl_handle_md_t my_md_h;

    int i;
    int my_root;
    std::vector<int> my_children;
    int num_children;

    static const int PT_UP = 0;
    static const int PT_DOWN = 1;
};

#endif // COMPONENTS_TRIG_CPU_BARRIER_TREE_TRIGGERED_H
