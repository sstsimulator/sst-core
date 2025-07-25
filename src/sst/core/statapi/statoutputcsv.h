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

#ifndef SST_CORE_STATAPI_STATOUTPUTCSV_H
#define SST_CORE_STATAPI_STATOUTPUTCSV_H

#include "sst/core/sst_types.h"
#include "sst/core/statapi/statoutput.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include <cstdio>
#include <string>
#include <vector>

namespace SST::Statistics {

/**
    \class StatisticOutputCSV

    The class for statistics output to a comma separated file.
*/
class StatisticOutputCSV : public StatisticFieldsOutput
{
public:
    SST_ELI_REGISTER_DERIVED(
      StatisticOutput,
      StatisticOutputCSV,
      "sst",
      "statoutputcsv",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Output directly to console screen"
   )

    SST_ELI_DOCUMENT_PARAMS(
        { "separator", "Field separator", ", "},
        { "filepath", "Filepath for the output file", "./StatisticOutput.csv"},
        { "outputtopheader", "Whether to print a header at the top of the CSV output", "True" },
        { "outputsimtime", "Whether to print the simulation time in the output", "True" },
        { "outputrank", "Whether to print the rank in the output", "True" }
    )

    /** Construct a StatOutputCSV
     * @param outputParameters - Parameters used for this Statistic Output
     */
    explicit StatisticOutputCSV(Params& outputParameters);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Statistics::StatisticOutputCSV)

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

    /** True if this StatOutput can handle StatisticGroups */
    virtual bool acceptsGroups() const override { return true; }

protected:
    StatisticOutputCSV() { ; } // For serialization

private:
    bool openFile();
    void closeFile();
    int  print(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

private:
#ifdef HAVE_LIBZ
    gzFile m_gzFile;
#endif
    FILE*                    m_hFile;
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
    bool                     m_useCompression;
};

} // namespace SST::Statistics

#endif // SST_CORE_STATAPI_STATOUTPUTCSV_H
