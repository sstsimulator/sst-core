// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization.h>

#include <sst/core/simulation.h>
#include <sst/core/statapi/statoutputtxt.h>
#include <sst/core/stringize.h>

namespace SST {
namespace Statistics {

StatisticOutputTxt::StatisticOutputTxt(Params& outputParameters) 
    : StatisticOutput (outputParameters)
{
    // Announce this output object's name
    Output out = Simulation::getSimulation()->getSimulationOutput();
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
    getOutputParameters().find_string("help", "1", foundKey);
    if (true == foundKey) {
        return false;
    }
    m_FilePath = getOutputParameters().find_string("filepath", "./StatisticOutput.csv", foundKey);
    topHeaderFlag = getOutputParameters().find_string("outputtopheader", "0", foundKey);
    inlineHeaderFlag = getOutputParameters().find_string("outputinlineheader", "1", foundKey);
    simTimeFlag = getOutputParameters().find_string("outputsimtime", "1", foundKey);
    rankFlag = getOutputParameters().find_string("outputrank", "1", foundKey);
    m_outputTopHeader = ("1" == topHeaderFlag);
    m_outputInlineHeader = ("1" == inlineHeaderFlag);
    m_outputSimTime = ("1" == simTimeFlag);
    m_outputRank = ("1" == rankFlag);

    // Get the parameters
    m_FilePath = getOutputParameters().find_string("filepath", "./StatisticOutput.txt", foundKey);
    
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
    if (1 < Simulation::getSimulation()->getNumRanks()) {
        int rank = Simulation::getSimulation()->getRank();
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

void StatisticOutputTxt::implStartOutputEntries(const char* componentName, const char* statisticName) 
{
    char buffer[256];

    // Starting Output
    m_outputBuffer.clear();
    
    m_outputBuffer += componentName;
    m_outputBuffer += ".";
    m_outputBuffer += statisticName;
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
            sprintf(buffer, "Rank = %d", Simulation::getSimulation()->getRank());
        } else {
            sprintf(buffer, "%d", Simulation::getSimulation()->getRank());
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

void StatisticOutputTxt::implOutputField(fieldHandle_t fieldHandle, const char* data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if (true == m_outputInlineHeader) {
            sprintf(buffer, "%s.%s = %s", FieldInfo->getFieldName().c_str(), typeName, data);
        } else {
            sprintf(buffer, "%s", data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}
    
} //namespace Statistics
} //namespace SST

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Statistics::StatisticOutputTxt)

