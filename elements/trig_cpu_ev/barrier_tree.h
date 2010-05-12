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


#ifndef COMPONENTS_TRIG_CPU_BARRIER_TREE_H
#define COMPONENTS_TRIG_CPU_BARRIER_TREE_H

#include "algorithm.h"
#include "trig_cpu.h"

class barrier_tree :  public algorithm {
public:
    barrier_tree(trig_cpu *cpu) : algorithm(cpu)
    {
        radix = cpu->getRadix();

        // compute my root and children
        boost::tie(my_root, my_children) = buildBinomialTree(radix);
        num_children = my_children.size();
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

        for (i = 0 ; i < num_children ; ++i) { 
            while (!cpu->irecv(my_children[i] , NULL, handle)) { crReturn(); }
            crReturn();
        }
        if (num_children != 0) {
            while (!cpu->waitall()) { crReturn(); }
            crReturn();
        }
        if (my_root != my_id) {
            cpu->isend(my_root, NULL, 0);
            crReturn();
            while (!cpu->irecv(my_root, NULL, handle)) { crReturn(); }
            crReturn();
            while (!cpu->waitall()) { crReturn(); }
            crReturn();
        }
        for (i = 0 ; i < num_children ; ++i) { 
            cpu->isend(my_children[i], NULL, 0);
            crReturn();
        }

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        crFinish();
        return true;
    }

private:
    barrier_tree();
    barrier_tree(const algorithm& a);
    void operator=(barrier_tree const&);

    SimTime_t start_time;
    int radix;
    int i;

    int my_root;
    std::vector<int> my_children;
    int num_children;
};

#endif // COMPONENTS_TRIG_CPU_BARRIER_TREE_H
