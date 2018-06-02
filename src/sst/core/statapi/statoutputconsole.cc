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

#include <sst/core/simulation.h>
#include <sst/core/statapi/statoutputconsole.h>

namespace SST {
namespace Statistics {

StatisticOutputConsole::StatisticOutputConsole(Params& outputParameters) 
    : StatisticOutput (outputParameters)
{
    Output out = Simulation::getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputConsole enabled...\n");
    setStatisticOutputName("StatisticOutputConsole");
}

bool StatisticOutputConsole::checkOutputParameters()
{
    bool foundKey;
    
    // Look for Help Param
    getOutputParameters().find<std::string>("help", "1", foundKey);
    if (true == foundKey) {
        return false;
    }
    return true;
}

void StatisticOutputConsole::printUsage()
{
    Output out("", 0, 0, Output::STDOUT);
    out.output(" : Usage - Sends all statistic output to the Console.\n");
    out.output(" : Parameters:\n");
    out.output(" : help = Force Statistic Output to display usage\n");
}

void StatisticOutputConsole::startOfSimulation() 
{
}

void StatisticOutputConsole::endOfSimulation() 
{
}

void StatisticOutputConsole::implStartOutputEntries(StatisticBase* statistic) 
{
    // Starting Output
    m_OutputBuffer.clear();
    m_OutputBuffer += statistic->getFullStatName();
    m_OutputBuffer += " : ";
    m_OutputBuffer += statistic->getStatTypeName();
    m_OutputBuffer += " : ";
}

void StatisticOutputConsole::implStopOutputEntries() 
{
    // Done with Output
    printf(" %s\n", m_OutputBuffer.c_str());
}

void StatisticOutputConsole::implOutputField(fieldHandle_t fieldHandle, int32_t data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());
        sprintf(buffer, "%s.%s = %" PRId32, FieldInfo->getFieldName().c_str(), typeName, data);
        m_OutputBuffer += buffer;
        m_OutputBuffer += "; ";
    }
}

void StatisticOutputConsole::implOutputField(fieldHandle_t fieldHandle, uint32_t data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());
        sprintf(buffer, "%s.%s = %" PRIu32, FieldInfo->getFieldName().c_str(), typeName, data);
        m_OutputBuffer += buffer;
        m_OutputBuffer += "; ";
    }
}

void StatisticOutputConsole::implOutputField(fieldHandle_t fieldHandle, int64_t data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());
        sprintf(buffer, "%s.%s = %" PRId64, FieldInfo->getFieldName().c_str(), typeName, data);
        m_OutputBuffer += buffer;
        m_OutputBuffer += "; ";
    }
}

void StatisticOutputConsole::implOutputField(fieldHandle_t fieldHandle, uint64_t data) 
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());
        sprintf(buffer, "%s.%s = %" PRIu64, FieldInfo->getFieldName().c_str(), typeName, data);
        m_OutputBuffer += buffer;
        m_OutputBuffer += "; ";
    }
}

void StatisticOutputConsole::implOutputField(fieldHandle_t fieldHandle, float data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());
        sprintf(buffer, "%s.%s = %f", FieldInfo->getFieldName().c_str(), typeName, data);
        m_OutputBuffer += buffer;
        m_OutputBuffer += "; ";
    }
}

void StatisticOutputConsole::implOutputField(fieldHandle_t fieldHandle, double data)
{
    char buffer[256];
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if (NULL != FieldInfo) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());
        sprintf(buffer, "%s.%s = %f", FieldInfo->getFieldName().c_str(), typeName, data);
        m_OutputBuffer += buffer;
        m_OutputBuffer += "; ";
    }
}

} //namespace Statistics
} //namespace SST
