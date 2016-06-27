// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <sst/core/simulation.h>
#include <sst/core/statapi/statoutputtxt.h>
#include <sst/core/stringize.h>

namespace SST {
namespace Statistics {

StatisticOutputTxt::StatisticOutputTxt(Params& outputParameters) 
    : StatisticOutput (outputParameters)
{
    // Announce this output object's name
    Output out = Simulation::getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputTxt enabled...\n");
    setStatisticOutputName("StatisticOutputTxt");
}

bool StatisticOutputTxt::checkOutputParameters()
{
    bool foundKey;
    std::string topHeaderFlag;
    std::string inlineHeaderFlag;
    std::string simTimeFlag;
    std::string rankFlag;

    // Review the output parameters and make sure they are correct, and 
    // also setup internal variables
    
    // Look for Help Param
    getOutputParameters().find<std::string>("help", "1", foundKey);
    if (true == foundKey) {
        return false;
    }
    m_FilePath = getOutputParameters().find<std::string>("filepath", "./StatisticOutput.txt");
    topHeaderFlag = getOutputParameters().find<std::string>("outputtopheader", "0");
    inlineHeaderFlag = getOutputParameters().find<std::string>("outputinlineheader", "1");
    simTimeFlag = getOutputParameters().find<std::string>("outputsimtime", "1");
    rankFlag = getOutputParameters().find<std::string>("outputrank", "1");
    m_outputTopHeader = ("1" == topHeaderFlag);
    m_outputInlineHeader = ("1" == inlineHeaderFlag);
    m_outputSimTime = ("1" == simTimeFlag);
    m_outputRank = ("1" == rankFlag);

    // Get the parameters
    m_FilePath = getOutputParameters().find<std::string>("filepath", "./StatisticOutput.txt");
    
    // Perform some checking on the parameters
    if (0 == m_FilePath.length()) { 
        // Filepath is zero length
        return false;
    }

    return true;
}

void StatisticOutputTxt::printUsage()
{
    // Display how to use this output object
    Output out("", 0, 0, Output::STDOUT);
    out.output(" : Usage - Sends all statistic output to a Text File.\n");
    out.output(" : Parameters:\n");
    out.output(" : help = Force Statistic Output to display usage\n");
    out.output(" : filePath = <Path to .txt file> - Default is ./StatisticOutput.txt\n");
    out.output(" : outputtopheader = <0|1> - Output Header at Top - Default is 0\n");
    out.output(" : outputinlineheader = <0|1>  - Output Header inline - Default is 1\n");
    out.output(" : outputsimtime = 0 | 1 - Output Simulation Time - Default is 1\n");
    out.output(" : outputrank = 0 | 1 - Output Rank - Default is 1\n");
}

void StatisticOutputTxt::startOfSimulation() 
{
    StatisticFieldInfo*        statField;
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
        out.fatal(CALL_INFO, -1, " : StatisticOutputTxt - Problem opening File %s - %s\n", m_FilePath.c_str(), strerror(errno));
        return;
    }

    // Output a Top Header is requested to do so
    if (true == m_outputTopHeader) {
        // Add a Simulation Component to the front
        m_outputBuffer = "Component.Statistic";
        fprintf(m_hFile, "%s; ", m_outputBuffer.c_str());
        
        if (true == m_outputSimTime) {
            // Add a Simulation Time Header to the front
            m_outputBuffer = "SimTime";
            fprintf(m_hFile, "%s; ", m_outputBuffer.c_str());
        }

        if (true == m_outputRank) {
            // Add a Rank Header to the front
            m_outputBuffer = "Rank";
            fprintf(m_hFile, "%s; ", m_outputBuffer.c_str());
        }
    
        // Output all Headers
        it_v = getFieldInfoArray().begin();
        
        while (it_v != getFieldInfoArray().end()) {
            statField = *it_v;
            m_outputBuffer += statField->getStatName();
            m_outputBuffer += ".";
            m_outputBuffer += statField->getFieldName();
            
            // Increment the iterator 
            it_v++;
    
            fprintf(m_hFile, "%s; ", m_outputBuffer.c_str());
        }
        fprintf(m_hFile, "\n");
    }
}

void StatisticOutputTxt::endOfSimulation() 
{
    // Close the file
    fclose(m_hFile);
}

void StatisticOutputTxt::implStartOutputEntries(StatisticBase* statistic) 
{
    char buffer[256];

    // Starting Output
    m_outputBuffer.clear();

    m_outputBuffer += statistic->getFullStatName();
    m_outputBuffer += " : ";
    m_outputBuffer += statistic->getStatTypeName();
    m_outputBuffer += " : ";
    if (true == m_outputSimTime) {
        // Add the Simulation Time to the front
        if (true == m_outputInlineHeader) {
            sprintf(buffer, "SimTime = %" PRIu64, Simulation::getSimulation()->getCurrentSimCycle());
        } else {
            sprintf(buffer, "%" PRIu64, Simulation::getSimulation()->getCurrentSimCycle());
        }

        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }

    if (true == m_outputRank) {
        // Add the Rank to the front
        if (true == m_outputInlineHeader) {
            sprintf(buffer, "Rank = %d", Simulation::getSimulation()->getRank().rank);
        } else {
            sprintf(buffer, "%d", Simulation::getSimulation()->getRank().rank);
        }

        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void StatisticOutputTxt::implStopOutputEntries() 
{
    // Done with Output
    fprintf(m_hFile, "%s\n", m_outputBuffer.c_str());
}

void StatisticOutputTxt::implOutputField(fieldHandle_t fieldHandle, int32_t data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);
    
    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if (true == m_outputInlineHeader) {
            sprintf(buffer, "%s.%s = %" PRId32, FieldInfo->getFieldName().c_str(), typeName, data);
        } else {
            sprintf(buffer, "%" PRId32, data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void StatisticOutputTxt::implOutputField(fieldHandle_t fieldHandle, uint32_t data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if (true == m_outputInlineHeader) {
            sprintf(buffer, "%s.%s = %" PRIu32, FieldInfo->getFieldName().c_str(), typeName, data);
        } else {
            sprintf(buffer, "%" PRIu32, data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void StatisticOutputTxt::implOutputField(fieldHandle_t fieldHandle, int64_t data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if (true == m_outputInlineHeader) {
            sprintf(buffer, "%s.%s = %" PRId64, FieldInfo->getFieldName().c_str(), typeName, data);
        } else {
            sprintf(buffer, "%" PRId64, data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void StatisticOutputTxt::implOutputField(fieldHandle_t fieldHandle, uint64_t data) 
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if (true == m_outputInlineHeader) {
            sprintf(buffer, "%s.%s = %" PRIu64, FieldInfo->getFieldName().c_str(), typeName, data);
        } else {
            sprintf(buffer, "%" PRIu64, data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void StatisticOutputTxt::implOutputField(fieldHandle_t fieldHandle, float data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if (true == m_outputInlineHeader) {
            sprintf(buffer, "%s.%s = %f", FieldInfo->getFieldName().c_str(), typeName, data);
        } else {
            sprintf(buffer, "%f", data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void StatisticOutputTxt::implOutputField(fieldHandle_t fieldHandle, double data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if (true == m_outputInlineHeader) {
            sprintf(buffer, "%s.%s = %f", FieldInfo->getFieldName().c_str(), typeName, data);
        } else {
            sprintf(buffer, "%f", data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

} //namespace Statistics
} //namespace SST
