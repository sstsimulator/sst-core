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

#ifndef _H_SST_CORE_STATISTICS_OUTPUTHDF5
#define _H_SST_CORE_STATISTICS_OUTPUTHDF5

#include "sst/core/sst_types.h"

#include <sst/core/statapi/statoutput.h>

#include "H5Cpp.h"
#include <map>
#include <string>

namespace SST {
namespace Statistics {

/**
    \class StatisticOutputHDF5

	The class for statistics output to a comma separated file.
*/
class StatisticOutputHDF5 : public StatisticOutput
{
public:
    /** Construct a StatOutputHDF5
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputHDF5(Params& outputParameters);

protected:
    /** Perform a check of provided parameters
     * @return True if all required parameters and options are acceptable
     */
    bool checkOutputParameters();

    /** Print out usage for this Statistic Output */
    void printUsage();

    void implStartRegisterFields(StatisticBase *stat);
    void implRegisteredField(fieldHandle_t fieldHandle);
    void implStopRegisterFields();

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
    StatisticOutputHDF5() {;} // For serialization

private:

    typedef union {
        int32_t     i32;
        uint32_t    u32;
        int64_t     i64;
        uint64_t    u64;
        float       f;
        double      d;
    } StatData_u;

    class StatisticInfo {
        StatisticBase *statistic;
        std::vector<fieldHandle_t> indexMap;
        std::vector<StatData_u> currentData;
        std::vector<fieldType_t> typeList;
        std::vector<std::string> fieldNames;

        H5::DataSet *dataset;
        H5::CompType *memType;
        H5::H5File *file;

        hsize_t nEntries;

    public:
        StatisticInfo(StatisticBase *stat, H5::H5File *file) : statistic(stat), file(file), nEntries(0) { }
        ~StatisticInfo() {
            if ( dataset ) delete dataset;
            if ( memType ) delete memType;
        }
        void registerSimTime();
        void registerField(StatisticFieldInfo *fi);
        void finalizeRegistration();

        void startNewEntry();
        StatData_u& getFieldLoc(fieldHandle_t fieldHandle);
        void finishEntry();
    };


    H5::H5File*              m_hFile;
    StatisticInfo*           m_currentStatistic;
    std::map<StatisticBase*, StatisticInfo*> m_statistics;


    StatisticInfo*  initStatistic(StatisticBase* statistic);
    StatisticInfo*  getStatisticInfo(StatisticBase* statistic);
};

} //namespace Statistics
} //namespace SST

#endif
