// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_STATAPI_STATOUTPUTTXT_H
#define SST_CORE_STATAPI_STATOUTPUTTXT_H

#include "sst/core/sst_types.h"
#include "sst/core/statapi/statoutput.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

namespace SST {
namespace Statistics {


class StatisticOutputTextBase : public StatisticFieldsOutput
{
public:
    /** Construct a StatOutputTxt
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputTextBase(Params& outputParameters);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementVirtualSerializable(SST::Statistics::StatisticOutputTextBase) protected :
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
    void outputField(fieldHandle_t fieldHandle, int32_t data) override;
    void outputField(fieldHandle_t fieldHandle, uint32_t data) override;
    void outputField(fieldHandle_t fieldHandle, int64_t data) override;
    void outputField(fieldHandle_t fieldHandle, uint64_t data) override;
    void outputField(fieldHandle_t fieldHandle, float data) override;
    void outputField(fieldHandle_t fieldHandle, double data) override;

protected:
    StatisticOutputTextBase() { ; }

    bool m_outputTopHeader;
    bool m_outputInlineHeader;
    bool m_outputSimTime;
    bool m_outputRank;
    bool m_useCompression;

private:
    bool openFile();
    void closeFile();
    int  print(const char* fmt, ...);

private:
#ifdef HAVE_LIBZ
    gzFile m_gzFile;
#endif
    FILE*       m_hFile;
    std::string m_outputBuffer;
    std::string m_FilePath;

    /**
       Returns whether or not this outputter outputs to a file
    */
    virtual bool outputsToFile() = 0;

    /**
       Returns whether or not this outputter supports compression.
       Only checked if the class also writes to a file
    */
    virtual bool supportsCompression() = 0;

    /**
       Function that gets the end of the first line of help message
    */
    virtual std::string getUsageInfo() = 0;

    /**
       Returns a prefix that will start each new output entry
    */
    virtual std::string getStartOutputPrefix() = 0;

    /**
       These functions return the default value for the associated
       flags. These are implemented by the child class so that each
       final class can control how the defaults work.
     */
    virtual bool getOutputTopHeaderDefault()    = 0;
    virtual bool getOutputInlineHeaderDefault() = 0;
    virtual bool getOutputSimTimeDefault()      = 0;
    virtual bool getOutputRankDefault()         = 0;

    virtual std::string getDefaultFileName() { return ""; }

    /** True if this StatOutput can handle StatisticGroups */
    virtual bool acceptsGroups() const override { return true; }
};


/**
    \class StatisticOutputTxt

    The class for statistics output to a text file.
*/
class StatisticOutputTxt : public StatisticOutputTextBase
{
public:
    SST_ELI_REGISTER_DERIVED(
      StatisticOutput,
      StatisticOutputTxt,
      "sst",
      "statoutputtxt",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Output to text file"
   )

    /** Construct a StatOutputTxt
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputTxt(Params& outputParameters);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Statistics::StatisticOutputTxt)


protected:
    StatisticOutputTxt() { ; } // For serialization

private:
    /**
       Returns whether or not this outputter outputs to a file
    */
    bool outputsToFile() override { return true; }

    /**
       Returns whether or not this outputter supports compression.
       Only checked if the class also writes to a file
    */
    bool supportsCompression() override
    {
#ifdef HAVE_LIBZ
        return true;
#else
        return false;
#endif
    }

    /**
       Function that gets the end of the first line of help message
    */
    std::string getUsageInfo() override { return "a text file"; }

    /**
       Returns a prefix that will start each new output entry
    */
    std::string getStartOutputPrefix() override { return ""; }

    /**
       These functions return the default value for the associated
       flags. These are implemented by the child class so that each
       final class can control how the defaults work.
     */
    bool getOutputTopHeaderDefault() override { return false; }
    bool getOutputInlineHeaderDefault() override { return true; }
    bool getOutputSimTimeDefault() override { return true; }
    bool getOutputRankDefault() override { return true; }

    std::string getDefaultFileName() override { return "./StatisticOutput.txt"; }
};

/**
    \class StatisticOutputConsole

    The class for statistics output to the console.
*/
class StatisticOutputConsole : public StatisticOutputTextBase
{
public:
    SST_ELI_REGISTER_DERIVED(
      StatisticOutput,
      StatisticOutputConsole,
      "sst",
      "statoutputconsole",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Output to console"
   )

    /** Construct a StatOutputTxt
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputConsole(Params& outputParameters);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Statistics::StatisticOutputConsole)

protected:
    StatisticOutputConsole() { ; } // For serialization

private:
    /**
       Returns whether or not this outputter outputs to a file
    */
    bool outputsToFile() override { return false; }

    /**
       Returns whether or not this outputter supports compression.
       Only checked if the class also writes to a file
    */
    bool supportsCompression() override { return false; }

    /**
       Function that gets the end of the first line of help message
    */
    std::string getUsageInfo() override { return "the console"; }

    /**
       Returns a prefix that will start each new output entry
    */
    std::string getStartOutputPrefix() override { return " "; }

    /**
       These functions return the default value for the associated
       flags. These are implemented by the child class so that each
       final class can control how the defaults work.
     */
    bool getOutputTopHeaderDefault() override { return false; }
    bool getOutputInlineHeaderDefault() override { return true; }
    bool getOutputSimTimeDefault() override { return false; }
    bool getOutputRankDefault() override { return false; }
};

} // namespace Statistics
} // namespace SST

#endif // SST_CORE_STATAPI_STATOUTPUTTXT_H
