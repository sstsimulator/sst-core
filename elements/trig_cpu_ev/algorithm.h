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


#ifndef COMPONENTS_TRIG_CPU_ALGORITHM_H
#define COMPONENTS_TRIG_CPU_ALGORITHM_H

#include <vector>
#include <utility>
#include <boost/tuple/tuple.hpp>

#include <sst/event.h>
#include "trig_cpu.h"

class algorithm {
public:
    virtual bool operator()(SST::Event *ev) = 0;

protected:
    algorithm(trig_cpu *cpu) : cpu(cpu), state(0)
    {
        my_id = cpu->getMyId();
        num_nodes = cpu->getNumNodes();
    }

    virtual ~algorithm() { }

    /**
     * Returns the floor form of binary logarithm for a 32 bit integer.
     * -1 is returned if n is 0.
     */
    uint32_t floorLog2(uint32_t n) {
        uint32_t pos = 0;
        if (n >= 1<<16) { n >>= 16; pos += 16; }
        if (n >= 1<< 8) { n >>=  8; pos +=  8; }
        if (n >= 1<< 4) { n >>=  4; pos +=  4; }
        if (n >= 1<< 2) { n >>=  2; pos +=  2; }
        if (n >= 1<< 1) {           pos +=  1; }
        return ((n == 0) ? (-1) : pos);
    }

    /**
     * Returns pair<int, vector<int>> containing root and children for
     * the current process and given radix
     */
    std::pair<int, std::vector<int> >
    buildBinomialTree(int radix)
    {
        std::vector<int> my_children;
        int my_root;

        for (int i = 1 ; i <= num_nodes ; i *= radix) {
            int tmp_radix = (num_nodes / i < radix) ? (num_nodes / i) : radix;
            my_root = (my_id / (tmp_radix * i)) * (tmp_radix * i);
            if (my_root != my_id) break;
            for (int j = 1 ; j < tmp_radix ; ++j) {
                my_children.push_back(my_id + i * j);
            }
        }
        std::reverse(my_children.begin(), my_children.end());

        return std::make_pair(my_root, my_children);
    }
    
    trig_cpu *cpu;
    int state;
    int my_id;
    int num_nodes;

private:
    algorithm(const algorithm& a);
    void operator=(algorithm const&);
};

/* Yeah for Duff's Device.  Google it. */
#define crBegin() switch(state) { case 0:
#define crReturn() do { state=__LINE__; return false; case __LINE__:; } while (0)
#define crFinish() state = 0 ;  }

#endif // COMPONENTS_TRIG_CPU_ALGORITHM_H
