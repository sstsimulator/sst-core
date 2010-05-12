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


#ifndef COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_TRIGGERED_H
#define COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_TRIGGERED_H

#include "algorithm.h"
#include "trig_cpu.h"
#include "portals.h"

class allreduce_recdbl_triggered :  public algorithm {
public:
    allreduce_recdbl_triggered(trig_cpu *cpu) : algorithm(cpu)
    {
        ptl = cpu->getPortalsHandle();
    }

    bool
    operator()(Event *ev)
    {
        int adj;
        ptl_md_t md;
        ptl_me_t me;
        int next_level;
        int remote;

        switch (state) {
        case 0:
            /* Initialization case */
            my_levels = -1;
            for (adj = 0x1; adj <= num_nodes ; adj  <<= 1) { my_levels++; } adj = adj >> 1;
            if (adj != num_nodes) {
                printf("recursive_doubling requires power of 2 nodes (%d)\n",
                       num_nodes);
                exit(1);
            }

            my_level_steps.resize(my_levels);
            my_level_ct_hs.resize(my_levels);
            my_level_me_hs.resize(my_levels);
            my_level_md_hs.resize(my_levels);

            for (int i = 0 ; i < my_levels ; ++i) {
                my_level_steps[i] = 0;
                ptl->PtlCTAlloc(PTL_CT_OPERATION, my_level_ct_hs[i]);

                me.start = &my_level_steps[i];
                me.length = 8;
                me.match_bits = i;
                me.ignore_bits = 0;
                me.ct_handle = my_level_ct_hs[i];
                ptl->PtlMEAppend(0, me, PTL_PRIORITY_LIST, NULL, 
                                 my_level_me_hs[i]);

                md.start = &my_level_steps[i];
                me.length = 8;
                md.eq_handle = PTL_EQ_NONE;
                md.ct_handle = PTL_CT_NONE;
                ptl->PtlMDBind(md, &my_level_md_hs[i]);
            }
            state = 1;
            break;

        case 1:
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
            ptl->PtlMEAppend(1, me, PTL_PRIORITY_LIST, NULL, user_me_h);

            md.start = NULL;
            md.length = 8;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &user_md_h);

            state = 2;
            break;

        case 2:
            // start the trip
            ptl->PtlAtomic(user_md_h, 0, 8, 0, my_id, 0, 0, 0, NULL, 0, PTL_SUM, PTL_DOUBLE);
            state = 3;
            break;

        case 3:
            ptl->PtlAtomic(user_md_h, 0, 8, 0, my_id ^ 0x1, 0, 0, 0, NULL, 0, PTL_SUM, PTL_DOUBLE);

            loop_var = 1;
            state = (loop_var < my_levels) ? 4 : 8;
            break;

        case 4:
            next_level = 0x1 << loop_var;
            remote = my_id ^ next_level;
            ptl->PtlTriggeredAtomic(my_level_md_hs[loop_var - 1], 0, 8, 0, my_id, 0,
                                    loop_var, 0, NULL, 0, PTL_SUM, PTL_DOUBLE,
                                    my_level_ct_hs[loop_var - 1], 2);
            state = 5;
            break;

        case 5:
            next_level = 0x1 << loop_var;
            remote = my_id ^ next_level;
            ptl->PtlTriggeredAtomic(my_level_md_hs[loop_var - 1], 0, 8, 0, remote, 0,
                                    loop_var, 0, NULL, 0, PTL_SUM, PTL_DOUBLE,
                                    my_level_ct_hs[loop_var - 1], 2);
            state = 6;
            break;

        case 6:
            next_level = 0x1 << loop_var;
            remote = my_id ^ next_level;
            ptl->PtlTriggeredPut(zero_md_h, 0, 8, 0, my_id, 0, 
                                 loop_var - 1, 0, NULL, 0, my_level_ct_hs[loop_var - 1], 2);
            state = 7;
            break;
	
        case 7:
            ptl->PtlTriggeredCTInc(my_level_ct_hs[loop_var - 1], -3, 
                                   my_level_ct_hs[loop_var - 1], 3);
            loop_var++;
            state = (loop_var < my_levels) ? 4 : 8;
            break;

        case 8:
            // copy into user buffer
            ptl->PtlTriggeredPut(my_level_md_hs[my_levels - 1], 0, 8, 0, my_id, 1,
                                 0, 0, NULL, 0, my_level_ct_hs[my_levels - 1], 2);
            state = 9;
            break;

        case 9:
            ptl->PtlTriggeredPut(zero_md_h, 0, 8, 0, my_id, 0, 
                                 my_levels - 1, 0, NULL, 0, my_level_ct_hs[my_levels - 1], 2);
            ptl->PtlTriggeredCTInc(my_level_ct_hs[my_levels - 1], -3, 
                                   my_level_ct_hs[my_levels - 1], 3);
            state = 10;
            break;

        case 10:
            if (ptl->PtlCTWait(user_ct_h, 1)) state = 11;
            break;
        
        case 11:
            ptl->PtlMEUnlink(user_me_h);
            trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);
            ptl->PtlMEUnlink(user_me_h);
            state = 1;
            return true;

        default:
            printf("triggered recursive doubling: unhandled state: %d\n", state);
            exit(1);
        }

        return false;
    }

private:
    allreduce_recdbl_triggered();
    allreduce_recdbl_triggered(const algorithm& a);
    void operator=(allreduce_recdbl_triggered const&);

    portals *ptl;
    SimTime_t start_time;
    int loop_var;
    int my_levels;

    std::vector<double> my_level_steps;
    std::vector<ptl_handle_ct_t> my_level_ct_hs;
    std::vector<ptl_handle_me_t> my_level_me_hs;
    std::vector<ptl_handle_md_t> my_level_md_hs;

    ptl_handle_ct_t user_ct_h;
    ptl_handle_me_t user_me_h;
    ptl_handle_md_t user_md_h;

    ptl_handle_md_t zero_md_h;
};

#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_TRIGGERED_H
