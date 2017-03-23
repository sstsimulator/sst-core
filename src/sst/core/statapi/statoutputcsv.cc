// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <sst/core/simulation.h>
#include <sst/core/statapi/statoutputcsv.h>
#include <sst/core/stringize.h>

namespace SST {
namespace Statistics {

StatisticOutputCSV::StatisticOutputCSV(Params& outputParameters)
    : StatisticOutput (outputParameters)
{
    // Announce this output object's name
    Output &out = Simulation::getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputCSV enabled...\n");
    setStatisticOutputName("StatisticOutputCSV");
}

bool StatisticOutputCSV::checkOutputParameters()
{
    bool foundKey;
    std::string topHeaderFlag;
    std::string simTimeFlag;
    std::string rankFlag;

    // Review the output parameters and make sure they are correct, and 
    // also setup internal variables

    // Look for Help Param
    getOutputParameters().find<std::string>("help", "1", foundKey);
    if (true == foundKey) {
        return false;
    }

    // Get the parameters
    m_Separator = getOutputParameters().find<std::string>("separator", ", ");
    m_FilePath = getOutputParameters().find<std::string>("filepath", "./StatisticOutput.csv");
    topHeaderFlag = getOutputParameters().find<std::string>("outputtopheader", "1");
    simTimeFlag = getOutputParameters().find<std::string>("outputsimtime", "1");
    rankFlag = getOutputParameters().find<std::string>("outputrank", "1");
    m_outputTopHeader = ("1" == topHeaderFlag);
    m_outputSimTime = ("1" == simTimeFlag);
    m_outputRank = ("1" == rankFlag);

    // Perform some checking on the parameters
    if (0 == m_Separator.length()) { 
        // Separator is zero length
        return false;
    }
    if (0 == m_FilePath.length()) { 
        // Filepath is zero length
        return false;
    }

    return true;
}

void StatisticOutputCSV::printUsage()
{
    // Display how to use this output object
    Output out("", 0, 0, Output::STDOUT);
    out.output(" : Usage - Sends all statistic output to a CSV File.\n");
    out.output(" : Parameters:\n");
    out.output(" : help = Force Statistic Output to display usage\n");
    out.output(" : filepath = <Path to .csv file> - Default is ./StatisticOutput.csv\n");
    out.output(" : separator = <separator between fields> - Default is \", \"\n");
    out.output(" : outputtopheader = 0 | 1 - Output Header at top - Default is 1\n");
    out.output(" : outputsimtime = 0 | 1 - Output Simulation Time - Default is 1\n");
    out.output(" : outputrank = 0 | 1 - Output Rank - Default is 1\n");
}

void StatisticOutputCSV::startOfSimulation() 
{
    StatisticFieldInfo*        statField;
    std::string                outputBuffer;
    FieldInfoArray_t::iterator it_v;
    
    // Set Filename with Rank if Num Ranks > 1
    if (1 < Simulation::getSimulation()->getNumRanks().rank) {
        int rank = Simulation::getSimulation()->getRank().rank;
        std::string rankstr = "_" + SST::to_string(rank);

        // Search for any extension        
        size_t index = m_FilePath.find_last_of(".");
        if (std::string::npos != index) {
            // We found a . at the end of the file, insert the rank string
            m_FilePath.insert(index, rankstr);
        } else {
            // No . found, append the rank string
            m_FilePath += rankstr;
        }
    }

    // Open the finalized filename
    m_hFile = fopen(m_FilePath.c_str(), "w");
    if (NULL == m_hFile){
        // We got an error of some sort
        Output out = Simulation::getSimulation()->getSimulationOutput();
        out.fatal(CALL_INFO, -1, " : StatisticOutputCSV - Problem opening File %s - %s\n", m_FilePath.c_str(), strerror(errno));
        return;
    }

    // Initialize the OutputBufferArray with std::string objects
    for (FieldInfoArray_t::iterator it_v = getFieldInfoArray().begin(); it_v != getFieldInfoArray().end(); it_v++) {
        m_OutputBufferArray.push_back(std::string(""));
    }
    
    if (true == m_outputTopHeader) {
        // Add a Component Time Header to the front
        outputBuffer = "ComponentName";
        outputBuffer += m_Separator;
        fprintf(m_hFile, "%s", outputBuffer.c_str());

        outputBuffer = "StatisticName";
        outputBuffer += m_Separator;
        fprintf(m_hFile, "%s", outputBuffer.c_str());

        outputBuffer = "StatisticSubId";
        outputBuffer += m_Separator;
        fprintf(m_hFile, "%s", outputBuffer.c_str());

        outputBuffer = "StatisticType";
        outputBuffer += m_Separator;
        fprintf(m_hFile, "%s", outputBuffer.c_str());

        if (true == m_outputSimTime) {
            // Add a Simulation Time Header to the front
            outputBuffer = "SimTime";
            outputBuffer += m_Separator;
            fprintf(m_hFile, "%s", outputBuffer.c_str());
        }
    
        if (true == m_outputRank) {
            // Add a rank Header to the front
            outputBuffer = "Rank";
            outputBuffer += m_Separator;
            fprintf(m_hFile, "%s", outputBuffer.c_str());
        }

        // Output all Headers
        it_v = getFieldInfoArray().begin();
        
        while (it_v != getFieldInfoArray().end()) {
            statField = *it_v;
            outputBuffer = statField->getFieldName();
            outputBuffer += ".";
            outputBuffer += getFieldTypeShortName(statField->getFieldType());
            
            // Increment the iterator 
            it_v++;
            // If not the last field, tack on a separator             
            if (it_v != getFieldInfoArray().end()) {
                outputBuffer += m_Separator;
            }
    
            fprintf(m_hFile, "%s", outputBuffer.c_str());
        }
        fprintf(m_hFile, "\n");
    }
}

void StatisticOutputCSV::endOfSimulation() 
{
    // Close the file
    fclose(m_hFile);
}

void StatisticOutputCSV::implStartOutputEntries(StatisticBase* statistic) 
{
    // Save the current Component and Statistic Names for when we stop output and send to file
    m_currentComponentName = statistic->getCompName();
    m_currentStatisticName = statistic->getStatName();
    m_currentStatisticSubId = statistic->getStatSubId();
    m_currentStatisticType = statistic->getStatTypeName();
    
    // Starting Output, Initialize the Buffers 
    for (uint32_t x = 0; x < getFieldInfoArray().size(); x++) {
        // Initialize the Output Buffer Array strings
        m_OutputBufferArray[x] = "0";
    }
}

void StatisticOutputCSV::implStopOutputEntries() 
{
    uint32_t x;
    
    // Output the Component and Statistic names
    fprintf(m_hFile, "%s", m_currentComponentName.c_str());
    fprintf(m_hFile, "%s", m_Separator.c_str());
    fprintf(m_hFile, "%s", m_currentStatisticName.c_str());
    fprintf(m_hFile, "%s", m_Separator.c_str());
    fprintf(m_hFile, "%s", m_currentStatisticSubId.c_str());
    fprintf(m_hFile, "%s", m_Separator.c_str());
    fprintf(m_hFile, "%s", m_currentStatisticType.c_str());
    fprintf(m_hFile, "%s", m_Separator.c_str());
    
    // Done with Output, Send a line of data to the file
    if (true == m_outputSimTime) {
        // Add the Simulation Time to the front
        fprintf(m_hFile, "%" PRIu64, Simulation::getSimulation()->getCurrentSimCycle());
        fprintf(m_hFile, "%s", m_Separator.c_str());
    }

    // Done with Output, Send a line of data to the file
    if (true == m_outputRank) {
        // Add the Simulation Time to the front
        fprintf(m_hFile, "%d", Simulation::getSimulation()->getRank().rank);
        fprintf(m_hFile, "%s", m_Separator.c_str());
    }
    
    x = 0;
    while (x < m_OutputBufferArray.size()) {
        fprintf(m_hFile, "%s", m_OutputBufferArray[x].c_str());
        x++;
        if (x != m_OutputBufferArray.size()) {
            fprintf(m_hFile, "%s", m_Separator.c_str());
        }
    }
    fprintf(m_hFile, "\n");
}

void StatisticOutputCSV::implOutputField(fieldHandle_t fieldHandle, int32_t data)
{
    char buffer[256];
    sprintf(buffer, "%" PRId32, data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void StatisticOutputCSV::implOutputField(fieldHandle_t fieldHandle, uint32_t data)
{
    char buffer[256];
    sprintf(buffer, "%" PRIu32, data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void StatisticOutputCSV::implOutputField(fieldHandle_t fieldHandle, int64_t data)
{
    char buffer[256];
    sprintf(buffer, "%" PRId64, data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void StatisticOutputCSV::implOutputField(fieldHandle_t fieldHandle, uint64_t data) 
{
    char buffer[256];
    sprintf(buffer, "%" PRIu64, data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void StatisticOutputCSV::implOutputField(fieldHandle_t fieldHandle, float data)
{
    char buffer[256];
    sprintf(buffer, "%f", data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void StatisticOutputCSV::implOutputField(fieldHandle_t fieldHandle, double data)
{
    char buffer[256];
    sprintf(buffer, "%f", data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

} //namespace Statistics
} //namespace SST
