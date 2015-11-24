// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_CORE_STATISTICS_OUTPUT_CSV_GZ
#define _H_SST_CORE_STATISTICS_OUTPUT_CSV_GZ

#include "sst/core/sst_types.h"
#include <sst/core/serialization.h>

#include <sst/core/statapi/statoutput.h>

#include <zlib.h>

#ifdef HAVE_LIBZ

namespace SST {
namespace Statistics {

/**
    \class StatisticOutputCompressedCSV

	The class for statistics output to a comma separated file with compression.
*/
class StatisticOutputCompressedCSV : public StatisticOutput
{
public:    
    /** Construct a StatisticOutputCompressedCSV
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputCompressedCSV(Params& outputParameters);

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
    StatisticOutputCompressedCSV() {;} // For serialization
    
private:
    gzFile                   m_hFile;
    std::vector<std::string> m_OutputBufferArray;
    std::string              m_Separator;
    std::string              m_FilePath;
    std::string              m_currentComponentName;
    std::string              m_currentStatisticName;
    std::string              m_currentStatisticSubId;
    std::string              m_currentStatisticType;
    bool                     m_outputTopHeader;
    bool                     m_outputSimTime;
    bool                     m_outputRank;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(StatisticOutput);
        ar & BOOST_SERIALIZATION_NVP(m_OutputBufferArray);
        ar & BOOST_SERIALIZATION_NVP(m_Separator);
        ar & BOOST_SERIALIZATION_NVP(m_FilePath);
        ar & BOOST_SERIALIZATION_NVP(m_currentComponentName);
        ar & BOOST_SERIALIZATION_NVP(m_currentStatisticName);
        ar & BOOST_SERIALIZATION_NVP(m_currentStatisticSubId);
        ar & BOOST_SERIALIZATION_NVP(m_currentStatisticType);
        ar & BOOST_SERIALIZATION_NVP(m_outputTopHeader);
        ar & BOOST_SERIALIZATION_NVP(m_outputSimTime);
        ar & BOOST_SERIALIZATION_NVP(m_outputRank);
    }
};

} //namespace Statistics
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Statistics::StatisticOutputCompressedCSV)

#endif
#endif