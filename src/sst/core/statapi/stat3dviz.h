// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_CORE_STAT_3D_VIZ
#define _H_SST_CORE_STAT_3D_VIZ

#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"
#include "sst/core/simulation.h"

#include <string>

static const std::string cubeKey = "cube";
static const std::string lineKey = "line";

namespace SST {
namespace Statistics {

// use common base class for the topology/geometry
struct Shape3D {
   enum Shape {
     Box,
     Line
   };
   Shape shape;

   Shape3D(Shape s) :
     shape(s)
   {
   }
};
struct Box3D : Shape3D {
    Box3D(std::vector<double> origin, std::vector<double> size) :
      Shape3D(Shape::Box),
      origin_(origin), size_(size)
    {
    }

    std::vector<double> origin_;
    std::vector<double> size_;
};

struct Line3D : Shape3D {
    Line3D(std::vector<double> origin, std::vector<double> size) :
        Shape3D(Shape::Line),
        origin_(origin), size_(size)
    {
    }

    std::vector<double> origin_;
    std::vector<double> size_;
};

struct Stat3DViz {

    Stat3DViz(Params& params){
        std::vector<double> origin;
        if (params.contains("origin")){
            params.find_array("origin", origin);
        } else {
            Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot create a Shape3D with no origin");
        }

        std::vector<double> size;
        if (params.contains("size")){
            params.find_array("size", size);
        } else {
            Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot create a Shape3D with no size");
        }

        std::string shape = params.find<std::string>("shape", "");
        if (shape == cubeKey) {
            my_shape_ = new Box3D(origin, size);
        }
        else if (shape == lineKey) {
            my_shape_ = new Line3D(origin, size);
        }
        else {
            Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot create a Shape3D: "
                                                                                   "Unknown %s type detected\n", shape.c_str());
        }
    }

    void setId(uint64_t id) {
        id_ = id;
    }

    uint64_t id_;
    Shape3D* my_shape_;
};

}
}

#endif
