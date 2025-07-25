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

#ifndef SST_CORE_STATAPI_STATOUTPUTJSON_H
#define SST_CORE_STATAPI_STATOUTPUTJSON_H

#include "sst/core/sst_types.h"
#include "sst/core/statapi/statoutput.h"

#include <cstdio>
#include <string>

namespace SST::Statistics {

/**
    \class StatisticOutputJSON

    The class for statistics output to a JSON formatted file
*/
class StatisticOutputJSON : public StatisticFieldsOutput
{
public:
    SST_ELI_REGISTER_DERIVED(
        StatisticOutput,
        StatisticOutputJSON,
        "sst",
        "statoutputjson",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Output to a JSON file")

    SST_ELI_DOCUMENT_PARAMS(
        { "filepath", "Filepath for the output file", "./StatisticOutput.json"},
        { "outputsimtime", "Whether to print the simulation time in the output", "True" },
        { "outputrank", "Whether to print the rank in the output", "True" }
    )

    /** Construct a StatOutputJSON
     * @param outputParameters - Parameters used for this Statistic Output
     */
    explicit StatisticOutputJSON(Params& outputParameters);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Statistics::StatisticOutputJSON)

protected:
    /** Perform a check of provided parameters
     * @return True if all required parameters and options are acceptable
     */
    bool checkOutputParameters() override;

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
    void outputField(fieldHandle_t fieldHandle, int32_t data) override;
    void outputField(fieldHandle_t fieldHandle, uint32_t data) override;
    void outputField(fieldHandle_t fieldHandle, int64_t data) override;
    void outputField(fieldHandle_t fieldHandle, uint64_t data) override;
    void outputField(fieldHandle_t fieldHandle, float data) override;
    void outputField(fieldHandle_t fieldHandle, double data) override;

    void printIndent();

protected:
    StatisticOutputJSON() :
        m_currentComponentName(""),
        m_firstEntry(false),
        m_processedAnyStats(false),
        m_curIndentLevel(0)
    {
        ;
    } // For serialization

private:
    bool openFile();
    void closeFile();

private:
    FILE*       m_hFile;
    std::string m_FilePath;
    std::string m_currentComponentName;
    std::string m_currentStatisticName;
    std::string m_currentStatisticSubId;
    std::string m_currentStatisticType;
    bool        m_outputSimTime;
    bool        m_outputRank;
    bool        m_firstEntry;
    bool        m_firstField;
    bool        m_processedAnyStats;
    int         m_curIndentLevel;
};

} // namespace SST::Statistics

#endif // SST_CORE_STATAPI_STATOUTPUTJSON_H
