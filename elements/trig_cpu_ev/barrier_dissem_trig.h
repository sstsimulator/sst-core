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


#ifndef COMPONENTS_TRIG_CPU_BARRIER_DISSEMINATION_TRIGGERED_H
#define COMPONENTS_TRIG_CPU_BARRIER_DISSEMINATION_TRIGGERED_H

#include "algorithm.h"
#include "trig_cpu.h"
#include "portals.h"

class barrier_dissemination_triggered :  public algorithm {
public:
    barrier_dissemination_triggered(trig_cpu *cpu) : algorithm(cpu), init(false)
    {
        radix = cpu->getRadix();
        ptl = cpu->getPortalsHandle();

        shiftval = floorLog2(radix);
        my_levels = 1;
        for (level = 0x2 ; level < num_nodes ; level <<= shiftval) { my_levels++; }

        my_level_ct_hs.resize(my_levels + 1);
        my_level_me_hs.resize(my_levels + 1);
    }

    bool
    operator()(Event *ev)
    {
        ptl_md_t md;
        ptl_me_t me;

        crBegin();

        if (!init) {
            md.start = NULL;
            md.length = 0;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &my_md_h);
            crReturn();

            for (i = 0 ; i <= my_levels ; ++i) {
                ptl->PtlCTAlloc(PTL_CT_OPERATION, my_level_ct_hs[i]);
                crReturn();
                me.start = NULL;
                me.length = 0;
                me.match_bits = i;
                me.ignore_bits = 0;
                me.ct_handle = my_level_ct_hs[i];
                ptl->PtlMEAppend(0, me, PTL_PRIORITY_LIST, NULL, 
                                 my_level_me_hs[i]);
                crReturn();
            }

            init = true;
        }
        
        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        for (j = 1 ; j < radix ; ++j) {
            ptl->PtlPut(my_md_h, 0, 0, 0, (my_id + j) % num_nodes, 0, 0, 0, NULL, 0);
            crReturn();
        }

        for (i = 1, level = 0x2 ; level < num_nodes ; level <<= shiftval, ++i) {
            for (j = 0 ; j < (radix - 1) ;++j) {
                remote = (my_id + level + i) % num_nodes;
                ptl->PtlTriggeredPut(my_md_h, 0, 0, 0, remote, 0, i, 0, NULL, 
                                     0, my_level_ct_hs[i - 1], radix - 1);
                crReturn();
            }

            ptl->PtlTriggeredCTInc(my_level_ct_hs[i - 1], -(radix - 1), 
                                   my_level_ct_hs[i - 1], (radix - 1));
            crReturn();
        }

        // wait for completion
        while (!ptl->PtlCTWait(my_level_ct_hs[my_levels - 1], (radix - 1))) {
            crReturn(); 
        }
        crReturn();
        ptl->PtlTriggeredCTInc(my_level_ct_hs[my_levels - 1], -(radix - 1), 
                               my_level_ct_hs[my_levels - 1], (radix - 1));
        crReturn();

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        crFinish();
        return true;
    }

private:
    barrier_dissemination_triggered();
    barrier_dissemination_triggered(const algorithm& a);
    void operator=(barrier_dissemination_triggered const&);

    portals *ptl;
    SimTime_t start_time;
    int my_levels;
    int shiftval;
    int radix;
    bool init;

    int i, j;
    int level;
    int remote;

    std::vector<ptl_handle_ct_t> my_level_ct_hs;
    std::vector<ptl_handle_me_t> my_level_me_hs;
    ptl_handle_md_t my_md_h;
};

#endif // COMPONENTS_TRIG_CPU_BARRIER_DISSEMINATION_TRIGGERED_H
