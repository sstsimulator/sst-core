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


#ifndef COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_H
#define COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_H

#include "algorithm.h"
#include "trig_cpu.h"

class allreduce_recdbl :  public algorithm {
public:
    allreduce_recdbl(trig_cpu *cpu) : algorithm(cpu)
    {
    }

    bool
    operator()(Event *ev)
    {
        int adj, handle, remote;
        remote = my_id ^ level;

        switch (state) {
        case 0:
            /* Initialization case */
            for (adj = 0x1; adj <= num_nodes ; adj  <<= 1); adj = adj >> 1;
            if (adj != num_nodes) {
                printf("recursive_doubling requires power of 2 nodes (%d)\n",
                       num_nodes);
                exit(1);
            }

            // 200ns startup time
            start_time = cpu->getCurrentSimTimeNano();
            cpu->addBusyTime("200ns");

            level = 0x1;
            state = 1;
            break;

        case 1:
            /* start of round to distance "level" 
             * - Post non-blocking receive 
             */
            if (cpu->recv(remote, NULL, handle)) state = 2;
            break;

        case 2: 
            /* continuation of round to distance "level"
             * - Post non-blocking send
             */
            cpu->send(remote, 0);
            state = 3;
            break;

        case 3:
            /* finish of round to distance "level"
             * - wait for incoming communication
             * - apply operation
             * - increase level and go back to begining state
             */
            if (! cpu->waitall()) break;

            /* only one operand, so 100ns always to apply operation */
            cpu->addBusyTime("100ns");

            level <<= 1;
            state = (level < num_nodes) ? 1 : 4;
            break;

        case 4:
            trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);
            state = 0;
            return true;

        default:
            printf("recursive doubling: unhandled state: %d\n", state);
            exit(1);
        }

        return false;
    }

private:
    allreduce_recdbl();
    allreduce_recdbl(const algorithm& a);
    void operator=(allreduce_recdbl const&);

    SimTime_t start_time;
    int level;
};

#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_H
