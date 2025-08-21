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

#ifndef SST_CORE_STATAPI_STATLGHISTOGRAM_H
#define SST_CORE_STATAPI_STATLGHISTOGRAM_H

#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/warnmacros.h"

namespace SST::Statistics {

/**
    \class HistogramStatistic
    Holder of data grouped into pre-determined width bins.
    \tparam BinDataType is the type of the data held in each bin (i.e. what data type described the width of the bin)
*/
using CountType   = uint64_t;
using NumBinsType = uint32_t;

template <class BinDataType>
class LogBinHistogramStatistic : public Statistic<BinDataType>
{

public:
    SST_ELI_DECLARE_STATISTIC_TEMPLATE_DERIVED(
        LogBinHistogramStatistic,
        BinDataType,
        "sst",
        "LogBinHistogramStatistic",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Track distribution of statistic across bins",
        "SST::Statistic<T>")

    SST_ELI_DOCUMENT_PARAMS(
        {"minvalue", "The minimum data value to include in the histogram.", "0"},
        {"numbins",  "The number of histogram bins.", "100"},
        {"dumpbinsonoutput", "Whether to output the data range of each bin as well as its value.", "true"},
        {"includeoutofbounds", "Whether to keep track of data that falls below or above the histogram bins in separate out-of-bounds bins.", "true"})


    LogBinHistogramStatistic(
        BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        Statistic<BinDataType>(comp, statName, statSubId, statParams)
    {
        // Identify what keys are Allowed in the parameters
        std::vector<std::string> allowedKeySet = { "minvalue", "numbins", "dumpbinsonoutput", "includeoutofbounds" };
        statParams.pushAllowedKeys(allowedKeySet);

        // Process the Parameters
        m_minValue           = statParams.find<BinDataType>("minvalue", 0);
        m_numBins            = statParams.find<NumBinsType>("numbins", 100);
        m_dumpBinsOnOutput   = statParams.find<bool>("dumpbinsonoutput", true);
        m_includeOutOfBounds = statParams.find<bool>("includeoutofbounds", true);

        // Initialize other properties
        m_totalSummed      = 0;
        m_totalSummedSqr   = 0;
        m_OOBMinCount      = 0;
        m_OOBMaxCount      = 0;
        m_itemsBinnedCount = 0;
        this->setCollectionCount(0);

        for ( std::size_t i = 0; i < m_numBins; ++i ) {
            m_binsMap.insert(std::pair<BinDataType, CountType>(i, static_cast<CountType>(0)));
        }
    }

    ~LogBinHistogramStatistic() {}

    LogBinHistogramStatistic() :
        Statistic<BinDataType>()
    {} // For serialization ONLY

    virtual const std::string& getStatTypeName() const override { return stat_type_; }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::Statistics::Statistic<BinDataType>::serialize_order(ser);
        SST_SER(m_minValue);
        SST_SER(m_binWidth);
        SST_SER(m_numBins);
        SST_SER(m_OOBMinCount);
        SST_SER(m_OOBMaxCount);
        SST_SER(m_itemsBinnedCount);
        SST_SER(m_totalSummed);
        SST_SER(m_totalSummedSqr);
        SST_SER(m_binsMap);
        SST_SER(m_dumpBinsOnOutput);
        SST_SER(m_includeOutOfBounds);
        // SST_SER(m_Fields); // Rebuilt by stat output object
    }

protected:
    /**
        Adds a new value to the histogram. The correct bin is identified and then incremented. If no bin can be found
        to hold the value then a new bin is created.
    */
    void addData_impl_Ntimes(uint64_t const N, BinDataType const value) override
    {
        // Check to see if the value is above or below the min/max values
        if ( value < getBinsMinValue() ) {
            m_OOBMinCount += N;
            return;
        }
        if ( value > getBinsMaxValue() ) {
            m_OOBMaxCount += N;
            return;
        }

        // This value is to be binned...
        // Add the "in limits" value to the total summation's
        m_totalSummed += static_cast<BinDataType>(N) * value;
        m_totalSummedSqr += static_cast<BinDataType>(N) * (value * value);

        // Increment the Binned count (note this <= to the Statistics added Item Count)
        ++m_itemsBinnedCount;

        // Figure out what the starting bin is and find it in the map
        // To support signed and unsigned values along with floating point types,
        // the calculation to find the bin_start value must be done in floating point
        // then converted to BinDataType
        double const      log2_value      = std::log2(static_cast<double>(value));
        double const      bin_floor_value = std::floor(log2_value); // Find the floor of the value
        BinDataType const bin_start       = static_cast<BinDataType>(bin_floor_value);

        m_binsMap[bin_start] += static_cast<CountType>(N);
    }

    void addData_impl(BinDataType const value) override { addData_impl_Ntimes(1, value); }

private:
    /** Count how many bins are active in this histogram */
    NumBinsType getActiveBinCount() const { return m_binsMap.size(); }

    /** Count how many bins are available */
    NumBinsType getNumBins() const { return m_numBins; }

    /** Get the width of a bin in this histogram */
    NumBinsType getBinWidth() const { return m_binWidth; }

    /**
        Get the count of items in the bin by the start value (e.g. give me the count of items in the bin which begins at
       value X). \return The count of items in the bin else 0.
    */
    CountType getBinCountByBinStart(BinDataType const binStartValue)
    {
        double const      log2_value      = std::log2(static_cast<double>(binStartValue));
        double const      bin_floor_value = std::floor(log2_value); // Find the floor of the value
        BinDataType const bin_start       = static_cast<BinDataType>(bin_floor_value);

        // Find the Bin Start Value in the Bin Map
        HistoMapItr_t bin_itr = m_binsMap.find(bin_start);

        // Check to see if the Start Value was found
        if ( bin_itr == m_binsMap.end() ) {
            // No, return no count for this bin
            return static_cast<CountType>(0);
        }

        // Yes, return the bin count
        return m_binsMap[binStartValue];
    }

    /**
        Get the smallest start value of a bin in this histogram (i.e. the minimum value possibly represented by this
       histogram)
    */
    BinDataType getBinsMinValue() { return 0; }

    /**
        Get the largest possible value represented by this histogram (i.e. the highest value in any of items bins
       rounded above to the size of the bin)
    */
    BinDataType getBinsMaxValue()
    {
        // Compute the max value; the histogram is indexed by the log2 floor of the bin
        // return pow2 of 1 + the number of total bins

        if constexpr ( std::is_floating_point<BinDataType>::value ) {
            return ldexp(2.0, m_numBins + 1);
        }
        else {
            return BinDataType { 2 } << (m_numBins + 1);
        }
    }

    /**
        Get the total number of items collected by the statistic
        \return The number of items that have been added to the statistic
    */
    uint64_t getStatCollectionCount()
    {
        // Get the number of items added (but not necessarily binned) to this statistic
        return this->getCollectionCount();
    }

    /**
        Get the total number of items contained in all bins
        \return The number of items contained in all bins
    */
    CountType getItemsBinnedCount() const { return m_itemsBinnedCount; }

    /**
        Sum up every item presented for storage in the histogram
        \return The sum of all values added into the histogram
    */
    BinDataType getValuesSummed() const { return m_totalSummed; }

    /**
    Sum up every squared value entered into the Histogram.
    \return The sum of all values added after squaring into the Histogram
    */
    BinDataType getValuesSquaredSummed() const { return m_totalSummedSqr; }

    void clearStatisticData() override
    {
        m_totalSummed      = 0;
        m_totalSummedSqr   = 0;
        m_OOBMinCount      = 0;
        m_OOBMaxCount      = 0;
        m_itemsBinnedCount = 0;
        m_binsMap.clear();
        this->setCollectionCount(0);
    }

    void registerOutputFields(StatisticFieldsOutput* statOutput) override
    {
        // Check to see if we have registered the Startup Fields
        m_Fields.push_back(statOutput->registerField<BinDataType>("BinsMinValue"));
        m_Fields.push_back(statOutput->registerField<BinDataType>("BinsMaxValue"));
        m_Fields.push_back(statOutput->registerField<NumBinsType>("BinWidth"));
        m_Fields.push_back(statOutput->registerField<NumBinsType>("TotalNumBins"));
        m_Fields.push_back(statOutput->registerField<BinDataType>("Sum"));
        m_Fields.push_back(statOutput->registerField<BinDataType>("SumSQ"));
        m_Fields.push_back(statOutput->registerField<NumBinsType>("NumActiveBins"));
        m_Fields.push_back(statOutput->registerField<CountType>("NumItemsCollected"));
        m_Fields.push_back(statOutput->registerField<CountType>("NumItemsBinned"));

        if ( true == m_includeOutOfBounds ) {
            m_Fields.push_back(statOutput->registerField<CountType>("NumOutOfBounds-MinValue"));
            m_Fields.push_back(statOutput->registerField<CountType>("NumOutOfBounds-MaxValue"));
        }

        // Do we also need to dump the bin counts on output
        if ( true == m_dumpBinsOnOutput ) {
            NumBinsType binLL;
            NumBinsType binUL;

            NumBinsType const nbt = getNumBins();

            std::vector<std::stringstream> streams;
            streams.reserve(nbt);

            std::vector<std::stringstream>::iterator sitr = streams.begin();

            for ( NumBinsType i = 0; i < nbt; ++i ) {
                // Figure out the upper and lower values for this bin
                binLL = i << 2; 
                binUL = ((i + 1) << 2) - 1;
                // Build the string name for this bin and add it as a field
                (*sitr) << "Bin" << i << ":" << binLL << "-" << binUL;
                m_Fields.push_back(statOutput->registerField<CountType>(sitr->str().c_str()));
            }
        }
    }

    void outputStatisticFields(StatisticFieldsOutput* statOutput, bool UNUSED(EndOfSimFlag)) override
    {
        StatisticOutput::fieldHandle_t x = 0;
        statOutput->outputField(m_Fields[x++], getBinsMinValue());
        statOutput->outputField(m_Fields[x++], getBinsMaxValue());
        statOutput->outputField(m_Fields[x++], getBinWidth());
        statOutput->outputField(m_Fields[x++], getNumBins());
        statOutput->outputField(m_Fields[x++], getValuesSummed());
        statOutput->outputField(m_Fields[x++], getValuesSquaredSummed());
        statOutput->outputField(m_Fields[x++], getActiveBinCount());
        statOutput->outputField(m_Fields[x++], getStatCollectionCount());
        statOutput->outputField(m_Fields[x++], getItemsBinnedCount());

        if ( true == m_includeOutOfBounds ) {
            statOutput->outputField(m_Fields[x++], m_OOBMinCount);
            statOutput->outputField(m_Fields[x++], m_OOBMaxCount);
        }

        // Do we also need to dump the bin counts on output
        if ( true == m_dumpBinsOnOutput ) {
            BinDataType currentBinValue = getBinsMinValue();
            for ( uint32_t y = 0; y < getNumBins(); y++ ) {
                statOutput->outputField(m_Fields[x++], getBinCountByBinStart(currentBinValue));
                // Increment the currentBinValue to get the next bin
                currentBinValue += getBinWidth();
            }
        }
    }

    bool isStatModeSupported(StatisticBase::StatMode_t mode) const override
    {
        switch ( mode ) {
        case StatisticBase::STAT_MODE_COUNT:
        case StatisticBase::STAT_MODE_PERIODIC:
        case StatisticBase::STAT_MODE_DUMP_AT_END:
            return true;
        default:
            return false;
        }
        return false;
    }

private:
    // Bin Map Definition
    using HistoMap_t = std::map<BinDataType, CountType>;

    // Iterator over the histogram bins
    using HistoMapItr_t = typename HistoMap_t::iterator;

    // The minimum value in the Histogram
    BinDataType m_minValue;

    // The width of each Histogram bin
    NumBinsType m_binWidth;

    // The number of bins to be supported
    NumBinsType m_numBins;

    // Out of bounds bins
    CountType m_OOBMinCount;
    CountType m_OOBMaxCount;

    // Count of Items that have binned, (Different than item count as some
    // items may be out of bounds and not binned)
    CountType m_itemsBinnedCount;

    // The sum of all values added into the Histogram, this is calculated and the sum of all values presented
    // to be entered into the Histogram not with bin-width multiplied by the (max-min)/2 of the bin.
    BinDataType m_totalSummed;

    // The sum of values added to the Histogram squared. Allows calculation of derivative statistic
    // values such as variance.
    BinDataType m_totalSummedSqr;

    // A map of the the bin starts to the bin counts
    HistoMap_t m_binsMap;

    // Support
    std::vector<StatisticOutput::fieldHandle_t> m_Fields;
    bool                                        m_dumpBinsOnOutput;
    bool                                        m_includeOutOfBounds;

    inline static const std::string stat_type_ = "Histogram";
};

} // namespace SST::Statistics

#endif // SST_CORE_STATAPI_STATHISTOGRAM_H
