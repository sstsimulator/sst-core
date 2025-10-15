// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/statapi/statoutputjson.h"

#include "sst/core/statapi/statoutputcsv.h"
#include "sst/core/stringize.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>

namespace SST::Statistics {

StatisticOutputJSON::StatisticOutputJSON(Params& outputParameters) :
    StatisticFieldsOutput(outputParameters)
{
    // Announce this output object's name
    Output& out = getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputJSON enabled...\n");
    setStatisticOutputName("StatisticOutputJSON");

    m_currentComponentName = "";
    m_firstEntry           = false;
    m_processedAnyStats    = false;
    m_curIndentLevel       = 0;
}

bool
StatisticOutputJSON::checkOutputParameters()
{
    // Get the parameters
    m_FilePath      = getOutputParameters().find<std::string>("filepath", "StatisticOutput.json");
    m_outputSimTime = getOutputParameters().find<bool>("outputsimtime", true);
    m_outputRank    = getOutputParameters().find<bool>("outputrank", true);

    if ( 0 == m_FilePath.length() ) {
        // Filepath is zero length
        return false;
    }

    return true;
}

void
StatisticOutputJSON::startOfSimulation()
{
    // Open the finalized filename
    if ( !openFile() ) return;

    fprintf(m_hFile, "{\n");
    m_curIndentLevel++;

    if ( 1 < getNumRanks().rank ) {
        const int thisRank = getRank().rank;

        printIndent();
        fprintf(m_hFile, "\"rank\" : %d,\n\n", thisRank);
    }

    printIndent();
    fprintf(m_hFile, "\"components\" : [\n");
    m_curIndentLevel++;
}

void
StatisticOutputJSON::endOfSimulation()
{
    if ( m_processedAnyStats ) {
        fprintf(m_hFile, "\n");
        m_curIndentLevel--;
        printIndent();
        fprintf(m_hFile, "]\n");
        m_curIndentLevel--;
        printIndent();
        fprintf(m_hFile, "}\n");
        m_curIndentLevel--;
    }

    printIndent();
    fprintf(m_hFile, "]\n");
    fprintf(m_hFile, "}\n");

    // Close the file
    closeFile();
}

void
StatisticOutputJSON::implStartOutputEntries(StatisticBase* statistic)
{
    if ( m_currentComponentName != statistic->getCompName() ) {
        if ( m_currentComponentName != "" ) {
            m_curIndentLevel--;

            fprintf(m_hFile, "\n");
            printIndent();
            fprintf(m_hFile, "]\n");
            m_curIndentLevel--;
            printIndent();
            fprintf(m_hFile, "},\n");
        }

        printIndent();
        fprintf(m_hFile, "{\n");
        m_curIndentLevel++;
        printIndent();
        fprintf(m_hFile, "\"name\" : \"%s\",\n", statistic->getCompName().c_str());
        printIndent();
        if ( m_outputSimTime ) {
            fprintf(m_hFile, "\"simtime\" : %" PRIu64 ",\n", getCurrentSimCycle());
        }

        printIndent();
        fprintf(m_hFile, "\"statistics\" : [\n");

        m_curIndentLevel++;
        m_firstEntry = true;
    }

    // Save the current Component and Statistic Names for when we stop output and send to file
    m_currentComponentName  = statistic->getCompName();
    m_currentStatisticName  = statistic->getStatName();
    m_currentStatisticSubId = statistic->getStatSubId();
    m_currentStatisticType  = statistic->getStatTypeName();

    if ( m_firstEntry ) {
        m_firstEntry = false;
    }
    else {
        fprintf(m_hFile, ",\n");
    }

    printIndent();
    fprintf(m_hFile, "{ \"stat\" : \"%s\", \"values\" : [ ", statistic->getStatName().c_str());

    m_processedAnyStats = true;
    m_firstField        = true;
}

void
StatisticOutputJSON::implStopOutputEntries()
{
    fprintf(m_hFile, " ] }");
}

void
StatisticOutputJSON::outputField(fieldHandle_t UNUSED(fieldHandle), int32_t data)
{
    if ( !m_firstField ) {
        fprintf(m_hFile, ", ");
    }

    fprintf(m_hFile, "%" PRId32, data);

    m_firstField = false;
}

void
StatisticOutputJSON::outputField(fieldHandle_t UNUSED(fieldHandle), uint32_t data)
{
    if ( !m_firstField ) {
        fprintf(m_hFile, ", ");
    }

    fprintf(m_hFile, "%" PRIu32, data);

    m_firstField = false;
}

void
StatisticOutputJSON::outputField(fieldHandle_t UNUSED(fieldHandle), int64_t data)
{
    if ( !m_firstField ) {
        fprintf(m_hFile, ", ");
    }

    fprintf(m_hFile, "%" PRId64, data);

    m_firstField = false;
}

void
StatisticOutputJSON::outputField(fieldHandle_t UNUSED(fieldHandle), uint64_t data)
{
    if ( !m_firstField ) {
        fprintf(m_hFile, ", ");
    }

    fprintf(m_hFile, "%" PRIu64, data);

    m_firstField = false;
}

void
StatisticOutputJSON::outputField(fieldHandle_t UNUSED(fieldHandle), float data)
{
    if ( !m_firstField ) {
        fprintf(m_hFile, ", ");
    }

    fprintf(m_hFile, "%f ", data);

    m_firstField = false;
}

void
StatisticOutputJSON::outputField(fieldHandle_t UNUSED(fieldHandle), double data)
{
    if ( !m_firstField ) {
        fprintf(m_hFile, ", ");
    }

    fprintf(m_hFile, "%f ", data);

    m_firstField = false;
}

bool
StatisticOutputJSON::openFile()
{
    std::string filename = m_FilePath;
    // Set Filename with Rank if Num Ranks > 1
    if ( 1 < getNumRanks().rank ) {
        int         rank    = getRank().rank;
        std::string rankstr = "_" + std::to_string(rank);

        // Search for any extension
        size_t index = m_FilePath.find_last_of(".");
        if ( std::string::npos != index ) {
            // We found a . at the end of the file, insert the rank string
            filename.insert(index, rankstr);
        }
        else {
            // No . found, append the rank string
            filename += rankstr;
        }
    }

    // Get the absolute path for output file
    filename = getAbsolutePathForOutputFile(filename);

    m_hFile = fopen(filename.c_str(), "w");

    if ( nullptr == m_hFile ) {
        // We got an error of some sort
        Output out = getSimulationOutput();
        out.fatal(CALL_INFO, 1, " : StatisticOutputJSON - Problem opening File %s - %s\n", m_FilePath.c_str(),
            strerror(errno));
        return false;
    }

    return true;
}

void
StatisticOutputJSON::closeFile()
{
    fclose(m_hFile);
}

void
StatisticOutputJSON::printIndent()
{
    for ( int i = 0; i < m_curIndentLevel; ++i ) {
        fprintf(m_hFile, "   ");
    }
}

void
StatisticOutputJSON::serialize_order(SST::Core::Serialization::serializer& ser)
{
    StatisticFieldsOutput::serialize_order(ser);
    SST_SER(m_FilePath);
    SST_SER(m_outputSimTime);
    SST_SER(m_outputRank);
    SST_SER(m_firstEntry);
    SST_SER(m_firstField);
    SST_SER(m_processedAnyStats);
}

} // namespace SST::Statistics
