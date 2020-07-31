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

#include "sst_config.h"
#include "sst/core/statapi/vtkstatoutputexodus.h"
#include "sst/core/statapi/vtkTrafficSource.h"

#include "sst/core/simulation.h"
#include "sst/core/stringize.h"

namespace SST {
namespace Statistics {

VTKStatisticOutputEXODUS::VTKStatisticOutputEXODUS(Params& outputParameters)
    : StatisticOutputEXODUS (outputParameters)
{
    // Announce this output object's name
    Output &out = Simulation::getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : VTKStatisticOutputEXODUS enabled...\n");
    setStatisticOutputName("VTKStatisticOutputEXODUS");

}

void VTKStatisticOutputEXODUS::writeExodus() {

    ::vtkTrafficSource::vtkOutputExodus(m_FilePath, std::move(m_traffic_progress_map),
                          std::move(m_stat_3d_viz_vector_)
                          );
}

} //namespace Statistics
} //namespace SST
