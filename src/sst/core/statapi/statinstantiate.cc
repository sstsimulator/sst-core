// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <sst/core/statapi/statbase.h>
#include <sst/core/statapi/statfieldinfo.h>
#include <sst/core/statapi/stataccumulator.h>
#include <sst/core/statapi/statuniquecount.h>
#include <sst/core/statapi/statnull.h>
#include <sst/core/statapi/stathistogram.h>

namespace SST {
namespace Statistics {

SST_ELI_INSTANTIATE_STATISTIC(AccumulatorStatistic,sst,any_numeric_type)
SST_ELI_INSTANTIATE_STATISTIC(UniqueCountStatistic,sst,any_integer_type)
SST_ELI_INSTANTIATE_STATISTIC(HistogramStatistic,sst,any_numeric_type)
SST_ELI_INSTANTIATE_STATISTIC(NullStatistic,sst,any_numeric_type)

SST_REGISTER_STATISTIC_FIELD(int32_t, i32);
SST_REGISTER_STATISTIC_FIELD(uint32_t, u32);
SST_REGISTER_STATISTIC_FIELD(int64_t, i64);
SST_REGISTER_STATISTIC_FIELD(uint64_t, u64);
SST_REGISTER_STATISTIC_FIELD(float, f);
SST_REGISTER_STATISTIC_FIELD(double, d);

struct CompositeStat {
  //make this not copy or move constructible to ensure reference interface is used
  CompositeStat(CompositeStat&&) = delete;
  CompositeStat(const CompositeStat&) = delete;
};

#pragma GCC diagnostic ignored "-Wunused-parameter"
struct CompositeStatTester : public Statistic<CompositeStat> {
  SST_ELI_REGISTER_STATISTIC(
      CompositeStatTester, CompositeStat,
      "sst",
      "statCompositeTester",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Test instantiation",
      "SST::StatisticOutput")

 CompositeStatTester(BaseComponent* comp, const std::string& statName,
                     const std::string& statSubId, Params& statParams) :
   Statistic<CompositeStat>(comp, statName, statSubId, statParams)
 {
 }

 void addData_impl(CompositeStat&& stat) override {}
 void addData_impl(const CompositeStat& stat) override {}
 void registerOutputFields(StatisticOutput* statOutput) override {}
 void outputStatisticData(StatisticOutput* statOutput, bool EndOfSimFlag) override {}
};

SST_REGISTER_STATISTIC_FIELD(CompositeStat,comp);

#pragma GCC diagnostic ignored "-Wunused-function"
static void compositeStatTest(Statistic<CompositeStat>* stat, CompositeStat&& result){
  stat->addData(std::move(result));
}

}
}

