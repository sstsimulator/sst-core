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

#ifndef _H_SST_CORE_STATISTICS_OUTPUTCONSOLE
#define _H_SST_CORE_STATISTICS_OUTPUTCONSOLE

#include "sst/core/sst_types.h"
#include <sst/core/serialization.h>

#include <sst/core/statapi/statoutput.h>

namespace SST {
namespace Statistics {

/**
    \class StatisticOutputConsole

	The class for statistics output to the console.  This will be the
	default statistic output in SST.
*/
class StatisticOutputConsole : public StatisticOutput
{
public:
    /** Construct a StatOutputConsole
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputConsole(Params& outputParameters);

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
     * @param componentName - The name of the component performing the output
     * @param statisticName - The name of the statistic releated to the component performing the output
     */
    void implStartOutputEntries(const char* componentName, const char* statisticName); 
    
    /** Implementation function for the end of output.
     * This will be called by the Statistic Processing Engine to indicate that  
     * a Statistic is finished sendind data to the Statistic Output for processing.
     * The Statisic Output can perform any output related functions here.
     * @param componentName - The name of the component performing the output
     * @param statisticName - The name of the statistic releated to the component performing the output
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
    void implOutputField(fieldHandle_t fieldHandle, const char* data);

protected: 
    StatisticOutputConsole() {;} // For serialization
    
private:
    std::string m_OutputBuffer;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(StatisticOutput);
        ar & BOOST_SERIALIZATION_NVP(m_OutputBuffer);
    }
};

} //namespace Statistics
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Statistics::StatisticOutputConsole)

#endif
