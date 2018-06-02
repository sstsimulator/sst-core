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

#ifndef _H_SST_CORE_STATISTICS_OUTPUT_JSON
#define _H_SST_CORE_STATISTICS_OUTPUT_JSON

#include "sst/core/sst_types.h"

#include <sst/core/statapi/statoutput.h>

namespace SST {
namespace Statistics {

/**
    \class StatisticOutputJSON

	The class for statistics output to a JSON formatted file
*/
class StatisticOutputJSON : public StatisticOutput
{
public:    
    /** Construct a StatOutputJSON
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputJSON(Params& outputParameters);

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

    /** Implementation function for the start of output.
     * This will be called by the Statistic Processing Engine to indicate that  
     * a Statistic is about to send data to the Statistic Output for processing.
     * @param statistic - Pointer to the statistic object than the output can 
     * retrieve data from.
     */
    void implStartOutputEntries(StatisticBase* statistic) override;
    
    /** Implementation function for the end of output.
     * This will be called by the Statistic Processing Engine to indicate that  
     * a Statistic is finished sending data to the Statistic Output for processing.
     * The Statistic Output can perform any output related functions here.
     */
    void implStopOutputEntries() override;

    /** Implementation functions for output.
     * These will be called by the statistic to provide Statistic defined 
     * data to be output.
     * @param fieldHandle - The handle to the registered statistic field.
     * @param data - The data related to the registered field to be output.
     */
    void implOutputField(fieldHandle_t fieldHandle, int32_t data) override;
    void implOutputField(fieldHandle_t fieldHandle, uint32_t data) override;
    void implOutputField(fieldHandle_t fieldHandle, int64_t data) override;
    void implOutputField(fieldHandle_t fieldHandle, uint64_t data) override;
    void implOutputField(fieldHandle_t fieldHandle, float data) override;
    void implOutputField(fieldHandle_t fieldHandle, double data) override;
    
    void printIndent();

protected:
    StatisticOutputJSON() {;} // For serialization

private:
    bool openFile();
    void closeFile();
    
private:
    FILE*                    m_hFile;
    std::string              m_FilePath;
    std::string              m_currentComponentName;
    std::string              m_currentStatisticName;
    std::string              m_currentStatisticSubId;
    std::string              m_currentStatisticType;
    bool                     m_outputSimTime;
    bool                     m_outputRank;
    bool					 m_firstEntry;
    bool					 m_firstField;
    bool 					 m_processedAnyStats;
    int						 m_curIndentLevel;

};

} //namespace Statistics
} //namespace SST

#endif
