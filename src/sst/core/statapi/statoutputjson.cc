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
#include <sst/core/statapi/statoutputcsv.h>
#include <sst/core/stringize.h>

#include <sst/core/statapi/statoutputjson.h>

namespace SST {
namespace Statistics {

StatisticOutputJSON::StatisticOutputJSON(Params& outputParameters)
    : StatisticOutput (outputParameters)
{
    // Announce this output object's name
    Output &out = Simulation::getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputJSON enabled...\n");
    setStatisticOutputName("StatisticOutputJSON");
    
    m_currentComponentName = "";
    m_firstEntry = false;
    m_processedAnyStats = false;
}

bool StatisticOutputJSON::checkOutputParameters()
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
    m_FilePath = getOutputParameters().find<std::string>("filepath", "./StatisticOutput.json");
    simTimeFlag = getOutputParameters().find<std::string>("outputsimtime", "1");
    rankFlag = getOutputParameters().find<std::string>("outputrank", "1");
    
    m_outputSimTime = ("1" == simTimeFlag);
    m_outputRank = ("1" == rankFlag);

    if (0 == m_FilePath.length()) { 
        // Filepath is zero length
        return false;
    }

    return true;
}

void StatisticOutputJSON::printUsage()
{
    // Display how to use this output object
    Output out("", 0, 0, Output::STDOUT);
    out.output(" : Usage - Sends all statistic output to a JSON File.\n");
    out.output(" : Parameters:\n");
    out.output(" : help = Force Statistic Output to display usage\n");
    out.output(" : filepath = <Path to .csv file> - Default is ./StatisticOutput.csv\n");
    out.output(" : outputsimtime = 0 | 1 - Output Simulation Time - Default is 1\n");
    out.output(" : outputrank = 0 | 1 - Output Rank - Default is 1\n");
}

void StatisticOutputJSON::startOfSimulation() 
{
    // Open the finalized filename
    if ( ! openFile() )
        return;
        
    fprintf(m_hFile, "{\n");
    m_curIndentLevel++;
    
    if (1 < Simulation::getSimulation()->getNumRanks().rank) {
        const int thisRank = Simulation::getSimulation()->getRank().rank;
        
        printIndent();
    	fprintf(m_hFile, "\"rank\" : %d\n\n", thisRank);
    }

    printIndent();
    fprintf(m_hFile, "\"components\" : [\n");
    m_curIndentLevel++;
}

void StatisticOutputJSON::endOfSimulation()
{
	if(m_processedAnyStats) {
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

void StatisticOutputJSON::implStartOutputEntries(StatisticBase* statistic)
{
	if( m_currentComponentName != statistic->getCompName() ) {
		if(m_currentComponentName != "") {
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
		fprintf(m_hFile, "\"statistics\" : [\n");

		m_curIndentLevel++;
		m_firstEntry = true;

	}

    // Save the current Component and Statistic Names for when we stop output and send to file
    m_currentComponentName = statistic->getCompName();
    m_currentStatisticName = statistic->getStatName();
    m_currentStatisticSubId = statistic->getStatSubId();
    m_currentStatisticType = statistic->getStatTypeName();

    if(m_firstEntry) {
    	m_firstEntry = false;
    } else {
    	fprintf(m_hFile, ",\n");
    }
    
    printIndent();
    fprintf(m_hFile, "{ \"stat\" : \"%s\", \"values\" : [ ", statistic->getStatName().c_str());

	m_processedAnyStats = true;
	m_firstField = true;
}

void StatisticOutputJSON::implStopOutputEntries() 
{
	fprintf(m_hFile, " ] }");
}

void StatisticOutputJSON::implOutputField(fieldHandle_t UNUSED(fieldHandle), int32_t data)
{
	if( ! m_firstField) {
		fprintf(m_hFile, ", ");
	}
	
	fprintf(m_hFile, "%" PRId32 , data);
	
	m_firstField = false;
}

void StatisticOutputJSON::implOutputField(fieldHandle_t UNUSED(fieldHandle), uint32_t data)
{
	if( ! m_firstField) {
		fprintf(m_hFile, ", ");
	}
	
	fprintf(m_hFile, "%" PRIu32 , data);
	
	m_firstField = false;
}

void StatisticOutputJSON::implOutputField(fieldHandle_t UNUSED(fieldHandle), int64_t data)
{
	if( ! m_firstField) {
		fprintf(m_hFile, ", ");
	}
	
	fprintf(m_hFile, "%" PRId64 , data);
	
	m_firstField = false;
}

void StatisticOutputJSON::implOutputField(fieldHandle_t UNUSED(fieldHandle), uint64_t data) 
{
	if( ! m_firstField) {
		fprintf(m_hFile, ", ");
	}
	
	fprintf(m_hFile, "%" PRIu64 , data);
	
	m_firstField = false;
}

void StatisticOutputJSON::implOutputField(fieldHandle_t UNUSED(fieldHandle), float data)
{
	if( ! m_firstField) {
		fprintf(m_hFile, ", ");
	}
	
	fprintf(m_hFile, "%f ", data);
	
	m_firstField = false;
}

void StatisticOutputJSON::implOutputField(fieldHandle_t UNUSED(fieldHandle), double data)
{
	if( ! m_firstField) {
		fprintf(m_hFile, ", ");
	}

	fprintf(m_hFile, "%f ", data);
	
	m_firstField = false;
}


bool StatisticOutputJSON::openFile(void)
{
	m_hFile = fopen(m_FilePath.c_str(), "w");
	
	if (NULL == m_hFile) {
		// We got an error of some sort
		Output out = Simulation::getSimulation()->getSimulationOutput();
		out.fatal(CALL_INFO, -1, " : StatisticOutputJSON - Problem opening File %s - %s\n", m_FilePath.c_str(), strerror(errno));
		return false;
	}
	
    return true;
}

void StatisticOutputJSON::closeFile(void)
{
	fclose(m_hFile);
}

void StatisticOutputJSON::printIndent() {
	for(int i = 0; i < m_curIndentLevel; ++i) {
		fprintf(m_hFile, "   ");
	}
}

} //namespace Statistics
} //namespace SST
