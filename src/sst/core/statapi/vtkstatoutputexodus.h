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

#ifndef _H_SST_CORE_STATISTICS_VTKOUTPUTEXODUS
#define _H_SST_CORE_STATISTICS_VTKOUTPUTEXODUS

#include "sst/core/sst_types.h"

#include "sst/core/statapi/statoutputexodus.h"
#include "sst/core/statapi/statintensity.h"

namespace SST {
namespace Statistics {

/**
    \class VTKStatisticOutputEXODUS

  The class for statistics output to a EXODUS formatted file using VTK
*/
class VTKStatisticOutputEXODUS : public StatisticOutputEXODUS
{
public:
    SST_ELI_REGISTER_DERIVED(
      StatisticOutputEXODUS,
      VTKStatisticOutputEXODUS,
      "sst",
      "vtkstatisticoutputexodus",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "writes vtk exodus output");

    /** Construct a StatOutputEXODUS
     * @param outputParameters - Parameters used for this Statistic Output
     */
    VTKStatisticOutputEXODUS(Params& outputParameters);

    void writeExodus() override;

protected:
    VTKStatisticOutputEXODUS() {;} // For serialization

private:
    bool openFile();
    void closeFile();
};

} //namespace Statistics
} //namespace SST

#endif
