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


#ifndef COMPONENTS_TRIG_CPU_BCAST_TREE_H
#define COMPONENTS_TRIG_CPU_BCAST_TREE_H

#include "algorithm.h"

class bcast_tree :  public algorithm {
public:
    bcast_tree(trig_cpu *cpu) : algorithm(cpu)
    {
        radix = cpu->getRadix();
        msg_size = cpu->getMessageSize();
        chunk_size = cpu->getChunkSize();

        in_buf = (char*) malloc(msg_size);
        for (i = 0 ; i < msg_size ; ++i) {
            in_buf[i] = i % 255;
        }
        out_buf = (char*) malloc(msg_size);

        // compute my root and children
        boost::tie(my_root, my_children) = buildBinomialTree(radix);
        num_children = my_children.size();
    }

    bool
    operator()(Event *ev)
    {
        int bad = 0;

        crBegin();

        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        if (my_root == my_id) memcpy(out_buf, in_buf, msg_size);

        for (i = 0 ; i < msg_size ; i += chunk_size) {
            comm_size = (msg_size - i > chunk_size) ? chunk_size : msg_size - i;
            if (my_root != my_id) {
                while (!cpu->irecv(my_root, out_buf + i, handle)) { crReturn(); }
                crReturn();
            }
            while (!cpu->waitall()) { crReturn(); }
            crReturn();
            for (j = 0 ; j < num_children ; ++j) {
                cpu->isend(my_children[j], in_buf + i, comm_size);
                crReturn();
            }
        }
        crReturn();
        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        for (i = 0 ; i < msg_size ; ++i) {
            if ((out_buf[i] & 0xff) != i % 255) bad++;
        }
        if (bad) printf("%5d: bad results: %d\n",my_id,bad);

        crFinish();
        return true;
    }

private:
    bcast_tree();
    bcast_tree(const algorithm& a);
    void operator=(bcast_tree const&);

    SimTime_t start_time;
    int radix;
    int i, j;
    int handle;

    int msg_size;
    int chunk_size;
    int comm_size;

    char *in_buf;
    char *out_buf;

    int my_root;
    std::vector<int> my_children;
    int num_children;
};

#endif // COMPONENTS_TRIG_CPU_BCAST_TREE_H
