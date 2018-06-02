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

#ifndef _H_SST_CORE_STATISTICS_GROUP
#define _H_SST_CORE_STATISTICS_GROUP

#include <sst/core/sst_types.h>
#include <sst/core/unitAlgebra.h>

#include <string>
#include <vector>

namespace SST {
class ConfigStatGroup;
namespace Statistics {
class StatisticBase;
class StatisticOutput;



class StatisticGroup {
public:
    StatisticGroup() : isDefault(true), name("default") { };
    StatisticGroup(const ConfigStatGroup &csg);

    bool containsStatistic(const StatisticBase *stat) const;
    bool claimsStatistic(const StatisticBase *stat) const;
    void addStatistic(StatisticBase *stat);

    bool isDefault;
    std::string name;
    StatisticOutput *output;
    UnitAlgebra outputFreq;

    std::vector<ComponentId_t> components;
    std::vector<std::string> statNames;
    std::vector<StatisticBase*> stats;
};

} //namespace Statistics
} //namespace SST

#endif
