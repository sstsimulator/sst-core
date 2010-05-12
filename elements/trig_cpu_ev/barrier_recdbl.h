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


#ifndef COMPONENTS_TRIG_CPU_BARRIER_RECDBL_H
#define COMPONENTS_TRIG_CPU_BARRIER_RECDBL_H

#include "algorithm.h"
#include "trig_cpu.h"

class barrier_recdbl :  public algorithm {
public:
    barrier_recdbl(trig_cpu *cpu) : algorithm(cpu)
    {
        int adj;

        /* Initialization case */
        for (adj = 0x1; adj <= num_nodes ; adj  <<= 1); adj = adj >> 1;
        if (adj != num_nodes) {
            printf("recursive_doubling requires power of 2 nodes (%d)\n",
                   num_nodes);
            exit(1);
        }
    }

    bool
    operator()(Event *ev)
    {
        int handle;

        crBegin();

        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        for (level = 0x1 ; level < num_nodes ; level <<= 1) {
            remote = my_id ^ level;
            while (!cpu->irecv(remote, NULL, handle)) { crReturn(); }
            crReturn();
            cpu->isend(remote, NULL, 0);
            crReturn();

            while (!cpu->waitall()) { crReturn(); }
            crReturn();
        }

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        crFinish();
        return true;
    }

private:
    barrier_recdbl();
    barrier_recdbl(const algorithm& a);
    void operator=(barrier_recdbl const&);

    SimTime_t start_time;
    int level;
    int remote;
};

#endif // COMPONENTS_TRIG_CPU_BARRIER_RECDBL_H
