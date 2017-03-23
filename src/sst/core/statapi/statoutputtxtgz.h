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

#ifndef _H_SST_CORE_STATISTICS_OUTPUT_TXT_GZ
#define _H_SST_CORE_STATISTICS_OUTPUT_TXT_GZ

#include "sst/core/sst_types.h"

#include <sst/core/statapi/statoutput.h>

#ifdef HAVE_LIBZ

#include <zlib.h>

namespace SST {
namespace Statistics {

/**
    \class StatisticOutputCompressedTxt

	The class for statistics output to a text file.  
*/
class StatisticOutputCompressedTxt : public StatisticOutput
{
public:    
    /** Construct a StatOutputTxt
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputCompressedTxt(Params& outputParameters);

protected:
    /** Perform a check of provided parameters
     * @return True if all required parameters and options are acceptable 
     */
    bool checkOutputParameters();
    
    /** Print out usage for this Statistic Output */
    void printUsage();
    
    /** Indicate to Statistic Output that simulation started.
     *  Statistic output may perform any startup code here as necessary. 
     */
    void startOfSimulation(); 

    /** Indicate to Statistic Output that simulation ended.
     *  Statistic output may perform any shutdown code here as necessary. 
     */
    void endOfSimulation(); 

    /** Implementation function for the start of output.
     * This will be called by the Statistic Processing Engine to indicate that  
     * a Statistic is about to send data to the Statistic Output for processing.
     * @param statistic - Pointer to the statistic object than the output can 
     * retrieve data from.
     */
    void implStartOutputEntries(StatisticBase* statistic); 
    
    /** Implementation function for the end of output.
     * This will be called by the Statistic Processing Engine to indicate that  
     * a Statistic is finished sendind data to the Statistic Output for processing.
     * The Statisic Output can perform any output related functions here.
     */
    void implStopOutputEntries(); 

    /** Implementation functions for output.
     * These will be called by the statistic to provide Statistic defined 
     * data to be output.
     * @param fieldHandle - The handle to the registered statistic field.
     * @param data - The data related to the registered field to be output.
     */
    void implOutputField(fieldHandle_t fieldHandle, int32_t data);
    void implOutputField(fieldHandle_t fieldHandle, uint32_t data);
    void implOutputField(fieldHandle_t fieldHandle, int64_t data);
    void implOutputField(fieldHandle_t fieldHandle, uint64_t data); 
    void implOutputField(fieldHandle_t fieldHandle, float data);
    void implOutputField(fieldHandle_t fieldHandle, double data);

protected: 
    StatisticOutputCompressedTxt() {;} // For serialization
    
private:
    gzFile                   m_hFile;
    std::string              m_outputBuffer;
    std::string              m_FilePath;
    bool                     m_outputTopHeader;
    bool                     m_outputInlineHeader;
    bool                     m_outputSimTime;
    bool                     m_outputRank;

};

} //namespace Statistics
} //namespace SST

#endif
#endif
