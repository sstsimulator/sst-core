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


#ifndef COMPONENTS_TRIG_CPU_ALLREDUCE_NARYTREE_H
#define COMPONENTS_TRIG_CPU_ALLREDUCE_NARYTREE_H

#include "algorithm.h"
#include "trig_cpu.h"

class allreduce_narytree :  public algorithm {
public:
    allreduce_narytree(trig_cpu *cpu) : algorithm(cpu)
    {
        radix = cpu->getRadix();
    }

    bool
    operator()(Event *ev)
    {
        int handle;

        switch (state) {
        case 0:
            /* Initialization case */
            // 200ns startup time
            start_time = cpu->getCurrentSimTimeNano();
            cpu->addBusyTime("200ns");

            my_num_children = 0;
            for (int i = 1 ; i <= radix ; ++i) {
                if (radix * my_id + i < num_nodes) my_num_children++;
            }

            loop_var = 1;
            state = (0 == my_num_children) ? 3 : 1;
            break;

        case 1:
            if (loop_var <= my_num_children) {
                if (cpu->recv(my_id * radix + loop_var, NULL, handle)) {
                    loop_var++;
                }
                break;
            }
            state = 2;
            break;

        case 2:
            if (! cpu->waitall()) break;
            // add compute time
            for (int i = 0 ; i <  (((my_num_children - 1) / 8) + 1) ; ++i) {
                cpu->addBusyTime("100ns");
            }
            loop_var = 1;
            state = (my_id == 0) ? 6 : 3;

        case 3:
            cpu->send((my_id - 1) / radix, 0);
            state = 4;
            break;

        case 4:
            if (cpu->recv((my_id - 1) / radix, NULL, handle)) {
                state = 5;
            }
            break;

        case 5:
            if (! cpu->waitall()) break;
            state = 6;
            break;

        case 6:
            if (loop_var <= my_num_children) {
                cpu->send(my_id * radix + loop_var, 0);
                loop_var++;
                break;
            }
            state = 7;
            break;

        case 7:
            trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano() - start_time);
            state = 0;
            return true;

        default:
            printf("tree: unhandled state: %d\n", state);
            exit(1);
        }

        return false;
    }

private:
    allreduce_narytree();
    allreduce_narytree(const algorithm& a);
    void operator=(allreduce_narytree const&);

    SimTime_t start_time;
    int radix;
    int loop_var;
    int my_num_children;
};

#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_NARYTREE_H
