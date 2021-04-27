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
#include "sst/core/statapi/statoutputexodus.h"

#include "sst/core/simulation.h"
#include "sst/core/stringize.h"

namespace SST {
namespace Statistics {

StatisticOutputEXODUS::StatisticOutputEXODUS(Params& outputParameters)
    : StatisticOutput (outputParameters), cellId_(0)
{
    // Announce this output object's name
    Output &out = Simulation::getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputEXODUS enabled...\n");
    setStatisticOutputName("StatisticOutputEXODUS");

}

void StatisticOutputEXODUS::output(StatisticBase* statistic, bool endOfSimFlag) {
    this->lock();

    if(endOfSimFlag) {
        IntensityStatistic* intensityStat = dynamic_cast<IntensityStatistic *>(statistic);
        if (intensityStat) {
            for (auto eventIte : intensityStat->getEvents()) {
                 // creation of the sorted event with the StatisticId
                sorted_intensity_event event(this->cellId_, eventIte);
                m_traffic_progress_map.emplace(eventIte.time_, event);
            }
            auto stat3dViz = intensityStat->geStat3DViz();
            stat3dViz.setId(this->cellId_);
            m_stat_3d_viz_vector_.emplace_back(stat3dViz);

            this->cellId_ = this->cellId_ + 1;
        }
        else {
            Output out = Simulation::getSimulation()->getSimulationOutput();
            out.fatal(CALL_INFO, 1, 0, " : StatisticOutputEXODUS - The ouput won't be produced : the statistic type is not of type  IntensityStatistic\n");
        }
    }

    this->unlock();
}

bool StatisticOutputEXODUS::checkOutputParameters()
{
    bool foundKey;
    // Review the output parameters and make sure they are correct, and
    // also setup internal variables

    // Look for Help Param
    getOutputParameters().find<std::string>("help", "1", foundKey);
    if (true == foundKey) {
        return false;
    }

    // Get the parameters
    m_FilePath = getOutputParameters().find<std::string>("filepath", "./statisticOutput.e");


    if (0 == m_FilePath.length()) {
        // Filepath is zero length
        return false;
    }

    return true;
}

void StatisticOutputEXODUS::printUsage()
{
    // Display how to use this output object
    Output out("", 0, 0, Output::STDOUT);
    out.output(" : Usage - Sends all statistic output to a Exodus File.\n");
    out.output(" : Parameters:\n");
    out.output(" : help = Force Statistic Output to display usage\n");
    out.output(" : filepath = <Path to .e file> - Default is ./StatisticOutput.e\n");
    out.output(" : outputsimtime = 0 | 1 - Output Simulation Time - Default is 1\n");
    out.output(" : outputrank = 0 | 1 - Output Rank - Default is 1\n");
}

void StatisticOutputEXODUS::startOfSimulation()
{
    // Open the finalized filename
    if ( ! openFile() )
        return;
}

void StatisticOutputEXODUS::endOfSimulation()
{
    this->writeExodus();

    // Close the file
    closeFile();
}

bool StatisticOutputEXODUS::openFile(void)
{
    return true;
}

void StatisticOutputEXODUS::closeFile(void)
{
}

void StatisticOutputEXODUS::registerStatistic(StatisticBase *stat)
{
  //
}

void StatisticOutputEXODUS::startOutputGroup(StatisticGroup *grp)
{
}

void StatisticOutputEXODUS::stopOutputGroup()
{
}

void StatisticOutputEXODUS::startRegisterGroup(StatisticGroup *grp)
{
}

void StatisticOutputEXODUS::stopRegisterGroup()
{
}

} //namespace Statistics
}
