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

#include "sst/core/statapi/statintensity.h"

namespace SST {
namespace Statistics {

IntensityStatistic::IntensityStatistic(BaseComponent* comp, const std::string& statName,
                 const std::string& statSubId, Params& statParams) :
  MultiStatistic<uint64_t, double>(comp, statName, statSubId, statParams), stat_3d_viz_(statParams)
{
    this->setStatisticTypeName("IntensityStatistic");
}

void
IntensityStatistic::registerOutput(StatisticOutput * /*statOutput*/)
{

}

void
IntensityStatistic::registerOutputFields(StatisticFieldsOutput * /*statOutput*/)
{

}

void IntensityStatistic::outputStatisticFields(StatisticFieldsOutput* statOutput, bool UNUSED(EndOfSimFlag))
{

}

void IntensityStatistic::addData_impl(uint64_t time, double intensity) {
    // Create a new intensity_event with the a new traffic event
    intensity_event event(time, intensity);
    intensity_event_vector_.emplace_back(event);
}

const std::vector<intensity_event>& IntensityStatistic::getEvents() const {
    return intensity_event_vector_;
}

Stat3DViz IntensityStatistic::geStat3DViz() const {
    return stat_3d_viz_;
}

}
}
