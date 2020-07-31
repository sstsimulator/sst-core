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

#ifndef _H_SST_CORE_STATISTICS_OUTPUTEXODUS
#define _H_SST_CORE_STATISTICS_OUTPUTEXODUS

#include "sst/core/sst_types.h"

#include "sst/core/statapi/statoutput.h"
#include "sst/core/statapi/statintensity.h"

namespace SST {
namespace Statistics {

/**
    \class StatisticOutputEXODUS

  The class for statistics output to a EXODUS formatted file
*/
class StatisticOutputEXODUS : public StatisticOutput
{
public:
    /** Construct a StatOutputEXODUS
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputEXODUS(Params& outputParameters);

    void output(StatisticBase* statistic, bool endOfSimFlag) override;

    /** True if this StatOutput can handle StatisticGroups */
    virtual bool acceptsGroups() const { return true; }

protected:
    /** Perform a check of provided parameters
     * @return True if all required parameters and options are acceptable
     */
    bool checkOutputParameters() override;

    /** Print out usage for this Statistic Output */
    void printUsage() override;

    /** Indicate to Statistic Output that simulation started.
     *  Statistic output may perform any startup code here as necessary.
     */
    void startOfSimulation() override;

    /** Indicate to Statistic Output that simulation ended.
     *  Statistic output may perform any shutdown code here as necessary.
     */
    void endOfSimulation() override;

private:
    void registerStatistic(StatisticBase *stat) override;

    void startOutputGroup(StatisticGroup* group) override;
    void stopOutputGroup() override;

    void startRegisterGroup(StatisticGroup* group) override;
    void stopRegisterGroup();

    virtual void writeExodus() = 0;

protected:
    StatisticOutputEXODUS() {;} // For serialization

private:
    bool openFile();
    void closeFile();

protected:
    std::string              m_FilePath;
    std::multimap<uint64_t, sorted_intensity_event> m_traffic_progress_map;
    std::vector<Stat3DViz> m_stat_3d_viz_vector_;
    int cellId_;

};

} //namespace Statistics
} //namespace SST

#endif
