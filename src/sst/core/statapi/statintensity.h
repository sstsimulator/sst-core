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

#ifndef _H_SST_CORE_INTENSITY_STATISTIC_
#define _H_SST_CORE_INTENSITY_STATISTIC_

#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/statapi/stat3dviz.h"

namespace SST {
namespace Statistics {

/**
 * @brief The intensity_event struct
 * An intensity_event event contains the collected data through the IntensityStatistic.
 * An intensity_event should only ever be pushed back on a vector or list.
 * It is not meant to be comparable and used in a map or set. It is also local to a single statistic.
 * time  = this is the time of the event.
 * intensity = this is the value (a double) that is written
 *         at a given timepoint. Depending on configuration,
 *         either intensity or level could be written as the color
*/
struct intensity_event {
    uint64_t time_; // progress time
    double intensity_;
    intensity_event(uint64_t t, double intensity) :
    time_(t), intensity_(intensity)
    {
    }
};

/**
 * @brief The sorted_intensity_event struct
 * A sorted_intensity_event event contains an intensity_event and its stat ID.
 * A sorted_intensity_event can be sorted by time.
 * It also contains a unique stat ID to distinguish events from different statistic collectors.
 * cellId_  = the unique stat ID.
 * ie_ = the intensity_event,
 *
 */
struct sorted_intensity_event {
  intensity_event ie_;
  uint64_t cellId_;
  sorted_intensity_event(uint64_t cellId, intensity_event event) :
  ie_(event), cellId_(cellId)
  {
  }
};

class IntensityStatistic : public MultiStatistic<uint64_t, double>
{

public:
  SST_ELI_REGISTER_MULTI_STATISTIC(
      IntensityStatistic,
      "sst",
      "IntensityStatistic",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Collect intensity at each time point for a component",
      uint64_t,double)

  IntensityStatistic(BaseComponent* comp, const std::string& statName,
          const std::string& statSubId, Params& statParams);

  void registerOutput(StatisticOutput* statOutput);

  void registerOutputFields(StatisticFieldsOutput* statOutput) override;
  void outputStatisticFields(StatisticFieldsOutput* statOutput, bool UNUSED(EndOfSimFlag)) override;

  void addData_impl(uint64_t time, double intensity) override;

  const std::vector<intensity_event>& getEvents() const;
  Stat3DViz geStat3DViz() const;


private:
  std::vector<intensity_event> intensity_event_vector_;
  Stat3DViz stat_3d_viz_;
};

}
}

#endif
