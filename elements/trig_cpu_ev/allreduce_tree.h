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


#ifndef COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_H
#define COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_H

#include "algorithm.h"
#include "trig_cpu.h"

class allreduce_tree :  public algorithm {
public:
    allreduce_tree(trig_cpu *cpu) : algorithm(cpu)
    {
        radix = cpu->getRadix();
    }

    bool
    operator()(Event *ev)
    {
        int handle, my_root;
        if (0 != state) {
            my_root = (my_id / (curr_radix * level)) * (curr_radix * level);
        }

        switch (state) {
        case 0:
            /* Initialization case */
            // 200ns startup time
            start_time = cpu->getCurrentSimTimeNano();
            cpu->addBusyTime("200ns");

            level = 1;
            curr_radix = radix;
            state = 1;
            break;

        case 1:
            /* Start a level of sending up the tree */
            loop_var = 1;
            state = 2;

        case 2:
            /* Continue a level sending up the tree - either post receive or send message */
            if (my_id == my_root) {
                // Not a leaf.  Post receives for all of my leafs
                if ( loop_var < curr_radix ) {
                    if ( cpu->recv(my_id+(level*loop_var),NULL,handle) ) {
                        loop_var++;
                    }
                } else {
                    state = 3;
                }
            } else {
                // Leaf. Send to root and jump to waiting for data
                cpu->send(my_root, 0);
                state = 4;
            }
            break;

        case 3:
            /* Continue a level sending up the tree - wait for and process receives (roots only) */
            if (! cpu->waitall()) break;

            // Add time for reduction operation (100ns for every 8 entries we touch)
            for (int i = 0 ; i <  (((curr_radix - 1) / 8) + 1) ; ++i) {
                cpu->addBusyTime("100ns");
            }

            level *= curr_radix;
            if (level == num_nodes) {
                // Done working up tree, switch to working down tree
                state = 6;
            } else {
                // set the radix for the last step, which may be smaller than normal radix
                if (num_nodes / level < curr_radix) curr_radix = num_nodes / level;
                state = 1;
            } 
            break;

        case 4:
            /* Start a non-tree-root down level - post receive for data from my root */
            if (cpu->recv(my_root, NULL, handle) ) {
                state = 5;
                // done with top of the tree, so back to normal radix
                curr_radix = radix;
            }

            break;

        case 5:
            /* Continue a non-tree-root down level - wait for data from my
               root and either jump to preparing for sending down tree or
               to completion */
            if ( cpu->waitall() ) {
                state = (1 == level) ? 9 : 6;
            }
            break;

        case 6:
            /* prepare to send data down tree.  Entered either from top
               root out of up state or from local root during a down
               level */
            loop_var = 1;
            level /= curr_radix;
            state = 7;
            break;

        case 7:
            /* Start send data down a tree level */
            if ( loop_var < curr_radix ) {
                cpu->send(my_id + (loop_var*level),0);
                loop_var++;
            } else {
                state = 8;
            }
            break;

        case 8:
            /* Finish send data down a tree level.  If it was the last
               level, done.  Otherwise, send the next tree level down */
            curr_radix = radix;
            state = (1 == level) ? 9 : 6;
            break;

        case 9:
            trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);
            state = 0;
            return true;

        default:
            printf("tree: unhandled state: %d\n", state);
            exit(1);
        }

        return false;
    }

private:
    allreduce_tree();
    allreduce_tree(const algorithm& a);
    void operator=(allreduce_tree const&);

    SimTime_t start_time;
    int radix;
    int curr_radix;
    int level;
    int loop_var;
};

#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_H
