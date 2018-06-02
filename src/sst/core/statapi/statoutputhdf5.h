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

#ifndef _H_SST_CORE_STATISTICS_OUTPUTHDF5
#define _H_SST_CORE_STATISTICS_OUTPUTHDF5

#include "sst/core/sst_types.h"

#include <sst/core/statapi/statoutput.h>
#include <sst/core/warnmacros.h>

DISABLE_WARN_MISSING_OVERRIDE
#include "H5Cpp.h"
REENABLE_WARNING

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

    bool acceptsGroups() const override { return true; }
protected:
    /** Perform a check of provided parameters
     * @return True if all required parameters and options are acceptable
     */
    bool checkOutputParameters() override;

    /** Print out usage for this Statistic Output */
    void printUsage() override;

    void implStartRegisterFields(StatisticBase *stat) override;
    void implRegisteredField(fieldHandle_t fieldHandle) override;
    void implStopRegisterFields() override;

    void implStartRegisterGroup(StatisticGroup* group ) override;
    void implStopRegisterGroup() override;

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

    void implStartOutputGroup(StatisticGroup* group) override;
    void implStopOutputGroup() override;

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


    class DataSet {
    public:
        DataSet(H5::H5File *file) : file(file) { }
        virtual ~DataSet() { }
        H5::H5File* getFile() { return file; }
        virtual bool isGroup() const = 0;

        virtual void setCurrentStatistic(StatisticBase *UNUSED(stat)) { }
        virtual void registerField(StatisticFieldInfo *fi) = 0;
        virtual void finalizeCurrentStatistic() = 0;

        virtual void beginGroupRegistration(StatisticGroup *UNUSED(group)) { }
        virtual void finalizeGroupRegistration() { }


        virtual void startNewGroupEntry() {}
        virtual void finishGroupEntry() {}

        virtual void startNewEntry(StatisticBase *stat) = 0;
        virtual StatData_u& getFieldLoc(fieldHandle_t fieldHandle) = 0;
        virtual void finishEntry() = 0;

    protected:
        H5::H5File *file;
    };

    class StatisticInfo : public DataSet {
        StatisticBase *statistic;
        std::vector<fieldHandle_t> indexMap;
        std::vector<StatData_u> currentData;
        std::vector<fieldType_t> typeList;
        std::vector<std::string> fieldNames;

        H5::DataSet *dataset;
        H5::CompType *memType;

        hsize_t nEntries;

    public:
        StatisticInfo(StatisticBase *stat, H5::H5File *file) :
            DataSet(file), statistic(stat), nEntries(0)
        {
            typeList.push_back(StatisticFieldInfo::UINT64);
            indexMap.push_back(-1);
            fieldNames.push_back("SimTime");
        }
        ~StatisticInfo() {
            if ( dataset ) delete dataset;
            if ( memType ) delete memType;
        }
        void registerField(StatisticFieldInfo *fi) override;
        void finalizeCurrentStatistic() override;

        bool isGroup() const override { return false; }
        void startNewEntry(StatisticBase *stat) override;
        StatData_u& getFieldLoc(fieldHandle_t fieldHandle) override;
        void finishEntry() override;
    };

    class GroupInfo : public DataSet {
        struct GroupStat {
            GroupInfo *gi;
            std::string statPath;

            H5::DataSet *dataset;
            H5::CompType *memType;

            hsize_t nEntries;

            std::vector<std::string> registeredFields; /* fi->uniqueName */
            std::vector<fieldType_t> typeList;
            std::map<fieldHandle_t, size_t> handleIndexMap;

            std::vector<StatData_u> currentData;
            size_t currentCompOffset;


            GroupStat(GroupInfo *group, StatisticBase *stat);
            void finalizeRegistration();
            static std::string getStatName(StatisticBase* stat);

            void startNewGroupEntry();

            void startNewEntry(size_t componentIndex, StatisticBase *stat);
            StatData_u& getFieldLoc(fieldHandle_t fieldHandle);
            void finishEntry();

            void finishGroupEntry();
        };



        hsize_t nEntries;
        std::map<std::string, GroupStat> m_statGroups;
        GroupStat *m_currentStat;
        StatisticGroup *m_statGroup;
        std::vector<BaseComponent*> m_components;
        H5::DataSet *timeDataSet;

    public:
        GroupInfo(StatisticGroup *group, H5::H5File *file);
        void beginGroupRegistration(StatisticGroup *UNUSED(group)) override { }
        void setCurrentStatistic(StatisticBase *stat) override;
        void registerField(StatisticFieldInfo *fi) override;
        void finalizeCurrentStatistic() override;
        void finalizeGroupRegistration() override;

        bool isGroup() const override { return true; }
        void startNewEntry(StatisticBase *stat) override;
        StatData_u& getFieldLoc(fieldHandle_t fieldHandle) override { return m_currentStat->getFieldLoc(fieldHandle); }
        void finishEntry() override;

        void startNewGroupEntry() override;
        void finishGroupEntry() override;
        size_t getNumComponents() const { return m_components.size(); }

        const std::string& getName() const { return m_statGroup->name; }
    };


    H5::H5File*              m_hFile;
    DataSet*                 m_currentDataSet;
    std::map<StatisticBase*, StatisticInfo*> m_statistics;
    std::map<std::string, GroupInfo> m_statGroups;


    StatisticInfo*  initStatistic(StatisticBase* statistic);
    StatisticInfo*  getStatisticInfo(StatisticBase* statistic);
};

} //namespace Statistics
} //namespace SST

#endif
