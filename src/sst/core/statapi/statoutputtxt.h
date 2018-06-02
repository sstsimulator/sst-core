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

#ifndef _H_SST_CORE_STATISTICS_OUTPUTTXT
#define _H_SST_CORE_STATISTICS_OUTPUTTXT

#include "sst/core/sst_types.h"

#include <sst/core/statapi/statoutput.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

namespace SST {
namespace Statistics {

/**
    \class StatisticOutputTxt

	The class for statistics output to a text file.  
*/
class StatisticOutputTxt : public StatisticOutput
{
public:    
    /** Construct a StatOutputTxt
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputTxt(Params& outputParameters, bool compressed);

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

protected:
    StatisticOutputTxt() {;} // For serialization

private:
    bool openFile();
    void closeFile();
    int print(const char* fmt, ...);

private:
#ifdef HAVE_LIBZ
    gzFile                   m_gzFile;
#endif
    FILE*                    m_hFile;
    std::string              m_outputBuffer;
    std::string              m_FilePath;
    bool                     m_outputTopHeader;
    bool                     m_outputInlineHeader;
    bool                     m_outputSimTime;
    bool                     m_outputRank;
    bool                     m_useCompression;

};

} //namespace Statistics
} //namespace SST

#endif
