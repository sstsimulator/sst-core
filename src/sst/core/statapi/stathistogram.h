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


#ifndef _H_SST_CORE_HISTOGRAM_STATISTIC_
#define _H_SST_CORE_HISTOGRAM_STATISTIC_

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>

#include <sst/core/statapi/statbase.h>
#include <sst/core/statapi/statoutput.h>

namespace SST {
namespace Statistics {

// NOTE: When calling base class members in classes derived from 
//       a templated base class.  The user must use "this->" in 
//       order to call base class members (to avoid a compiler 
//       error) because they are "nondependant named" and the 
//       templated base class is a "dependant named".  The 
//       compiler will not look in dependant named base classes 
//       when looking up independent names.
// See: http://www.parashift.com/c++-faq-lite/nondependent-name-lookup-members.html

/**
    \class HistogramStatistic
	Holder of data grouped into pre-determined width bins.
	\tparam BinDataType is the type of the data held in each bin (i.e. what data type described the width of the bin)
*/
#define CountType   uint64_t
#define NumBinsType uint32_t

template<class BinDataType>
class HistogramStatistic : public Statistic<BinDataType> 
{
public:

    HistogramStatistic(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams)
		: Statistic<BinDataType>(comp, statName, statSubId, statParams)
    {
        // Identify what keys are Allowed in the parameters
        Params::KeySet_t allowedKeySet;
        allowedKeySet.insert("minvalue");
        allowedKeySet.insert("binwidth");
        allowedKeySet.insert("numbins");
        allowedKeySet.insert("dumpbinsonoutput");
        allowedKeySet.insert("includeoutofbounds");
        statParams.pushAllowedKeys(allowedKeySet);
        
        // Process the Parameters
        m_minValue = statParams.find<BinDataType>("minvalue", 0);
        m_binWidth = statParams.find<NumBinsType>("binwidth", 5000);
        m_numBins = statParams.find<NumBinsType>("numbins", 100);
        m_dumpBinsOnOutput = statParams.find<bool>("dumpbinsonoutput", true);
        m_includeOutOfBounds = statParams.find<bool>("includeoutofbounds", true);
        
        // Initialize other properties
        m_totalSummed = 0;
        m_totalSummedSqr = 0;
        m_OOBMinCount = 0;
        m_OOBMaxCount = 0;
        m_itemsBinnedCount = 0;
        this->setCollectionCount(0);

        // Set the Name of this Statistic
        this->setStatisticTypeName("Histogram");
    }

    ~HistogramStatistic() {}

protected:    
    /**
        Adds a new value to the histogram. The correct bin is identified and then incremented. If no bin can be found
        to hold the value then a new bin is created.
    */
    void addData_impl(BinDataType value) override
    {
        // Check to see if the value is above or below the min/max values
        if (value < getBinsMinValue()) {   
            m_OOBMinCount++;
            return; 
        }
        if (value > getBinsMaxValue()) {
            m_OOBMaxCount++;
            return; 
        } 

        // This value is to be binned...
        // Add the "in limits" value to the total summation's 
        m_totalSummed += value;
        m_totalSummedSqr += (value * value);
        
        // Increment the Binned count (note this <= to the Statistics added Item Count)
        m_itemsBinnedCount++;
        
        // Figure out what the starting bin is and find it in the map
        // To support signed and unsigned values along with floating point types,
        // the calculation to find the bin_start value must be done in floating point
        // then converted to BinDataType
        double calc1 = (double)value / (double)m_binWidth;  
        double calc2 = floor(calc1);             // Find the floor of the value
        double calc3 = m_binWidth * calc2;
        BinDataType  bin_start = (BinDataType)calc3;
//      printf("DEBUG: value = %d, junk1 = %f, calc2 = %f, calc3 = %f : bin_start = %d, item count = %ld, \n", value, calc1, calc2, calc3, bin_start, getStatCollectionCount());        

        HistoMapItr_t bin_itr = m_binsMap.find(bin_start);
        
        // Was the bin found?
        if(bin_itr == m_binsMap.end()) {
            // No, Create the bin and set a value of 1 to it
            m_binsMap.insert(std::pair<BinDataType, CountType>(bin_start, (CountType) 1));
        } else {
            // Yes, Increment the specific bin's count
            bin_itr->second++;
        }
    }

private:    
    /** Count how many bins are active in this histogram */
    NumBinsType getActiveBinCount() 
    {
        return m_binsMap.size();
    }

    /** Count how many bins are available */
    NumBinsType getNumBins() 
    {
        return m_numBins;
    }

    /** Get the width of a bin in this histogram */
    NumBinsType getBinWidth() 
    {
        return m_binWidth;
    }

    /**
        Get the count of items in the bin by the start value (e.g. give me the count of items in the bin which begins at value X).
        \return The count of items in the bin else 0.
    */
    CountType getBinCountByBinStart(BinDataType binStartValue) 
    {
        // Find the Bin Start Value in the Bin Map
        HistoMapItr_t bin_itr = m_binsMap.find(binStartValue);

        // Check to see if the Start Value was found
        if(bin_itr == m_binsMap.end()) {
            // No, return no count for this bin
            return (CountType) 0;
        } else {
            // Yes, return the bin count
            return m_binsMap[binStartValue];
        }
    }

    /**
        Get the smallest start value of a bin in this histogram (i.e. the minimum value possibly represented by this histogram)
    */
    BinDataType getBinsMinValue() 
    {
        return m_minValue;
    }

    /**
        Get the largest possible value represented by this histogram (i.e. the highest value in any of items bins rounded above to the size of the bin)
    */
    BinDataType getBinsMaxValue() 
    {
        // Compute the max value based on the width * num bins offset by minvalue
        return (m_binWidth * m_numBins) + m_minValue - 1;
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
    CountType getItemsBinnedCount() 
    {
        // Get the number of items added to this statistic that were binned.
        return m_itemsBinnedCount;
    }

    /**
        Sum up every item presented for storage in the histogram
        \return The sum of all values added into the histogram
    */
    BinDataType getValuesSummed() 
    {
        return m_totalSummed;
    }

    /**
	Sum up every squared value entered into the Histogram.
	\return The sum of all values added after squaring into the Histogram
    */
    BinDataType getValuesSquaredSummed() {
        return m_totalSummedSqr;
    }

    void clearStatisticData() override
    {
        m_totalSummed = 0;
        m_totalSummedSqr = 0;
        m_OOBMinCount = 0;
        m_OOBMaxCount = 0;
        m_itemsBinnedCount = 0;
        m_binsMap.clear();
        this->setCollectionCount(0);
    }
    
    void registerOutputFields(StatisticOutput* statOutput) override
    {
        // Check to see if we have registered the Startup Fields        
        m_Fields.push_back(statOutput->registerField<BinDataType>("BinsMinValue"));
        m_Fields.push_back(statOutput->registerField<BinDataType>("BinsMaxValue"));  
        m_Fields.push_back(statOutput->registerField<NumBinsType>("BinWidth"));  
        m_Fields.push_back(statOutput->registerField<NumBinsType>("TotalNumBins"));  
        m_Fields.push_back(statOutput->registerField<BinDataType>("Sum"));
        m_Fields.push_back(statOutput->registerField<BinDataType>("SumSQ"));
        m_Fields.push_back(statOutput->registerField<NumBinsType>("NumActiveBins"));  
        m_Fields.push_back(statOutput->registerField<CountType>  ("NumItemsCollected"));
        m_Fields.push_back(statOutput->registerField<CountType>  ("NumItemsBinned"));

        if (true == m_includeOutOfBounds) {
                m_Fields.push_back(statOutput->registerField<CountType>("NumOutOfBounds-MinValue"));
                m_Fields.push_back(statOutput->registerField<CountType>("NumOutOfBounds-MaxValue"));
        }

        // Do we also need to dump the bin counts on output
        if (true == m_dumpBinsOnOutput) {
            BinDataType binLL;
            BinDataType binUL;
            
            for (uint32_t y = 0; y < getNumBins(); y++) {
                // Figure out the upper and lower values for this bin
	        binLL = (y * (uint64_t)getBinWidth()) + getBinsMinValue(); // Force full 64-bit multiply -mpf 10/8/15
                binUL = binLL + getBinWidth() - 1;
                // Build the string name for this bin and add it as a field
                std::stringstream ss;
                ss << "Bin" << y << ":" << binLL << "-" << binUL;
                m_Fields.push_back(statOutput->registerField<CountType>(ss.str().c_str()));
            }
        }
    }

    void outputStatisticData(StatisticOutput* statOutput, bool UNUSED(EndOfSimFlag)) override
    {
        uint32_t x = 0;
        statOutput->outputField(m_Fields[x++], getBinsMinValue());
        statOutput->outputField(m_Fields[x++], getBinsMaxValue());
        statOutput->outputField(m_Fields[x++], getBinWidth());
        statOutput->outputField(m_Fields[x++], getNumBins());
        statOutput->outputField(m_Fields[x++], getValuesSummed());
        statOutput->outputField(m_Fields[x++], getValuesSquaredSummed());
        statOutput->outputField(m_Fields[x++], getActiveBinCount());
        statOutput->outputField(m_Fields[x++], getStatCollectionCount());
        statOutput->outputField(m_Fields[x++], getItemsBinnedCount());
        
        if (true == m_includeOutOfBounds) {
            statOutput->outputField(m_Fields[x++], m_OOBMinCount);
            statOutput->outputField(m_Fields[x++], m_OOBMaxCount);
        }

        // Do we also need to dump the bin counts on output
        if (true == m_dumpBinsOnOutput) {
            BinDataType currentBinValue = getBinsMinValue();
            for (uint32_t y = 0; y < getNumBins(); y++) {
                statOutput->outputField(m_Fields[x++], getBinCountByBinStart(currentBinValue));
                // Increment the currentBinValue to get the next bin
                currentBinValue += getBinWidth();
            }
        }
    }

    bool isStatModeSupported(StatisticBase::StatMode_t mode) const override
    {
        if (mode == StatisticBase::STAT_MODE_COUNT) {
            return true;
        }
        if (mode == StatisticBase::STAT_MODE_PERIODIC) {
            return true;
        }
        return false;
    }
    
private:
    // Bin Map Definition
    typedef std::map<BinDataType, CountType> HistoMap_t;

    // Iterator over the histogram bins 
    typedef typename HistoMap_t::iterator HistoMapItr_t;

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
    std::vector<uint32_t> m_Fields;
    bool                  m_dumpBinsOnOutput;
    bool                  m_includeOutOfBounds;

};

} //namespace Statistics
} //namespace SST

#endif
