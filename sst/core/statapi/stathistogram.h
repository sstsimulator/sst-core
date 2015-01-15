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


#ifndef _H_SST_CORE_HISTOGRAM_STATISTIC_
#define _H_SST_CORE_HISTOGRAM_STATISTIC_

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <sst/core/statapi/statbase.h>

namespace SST {
namespace Statistics {

// NOTE: When calling base class members in classes derived from 
//       a templated base class.  The user must use "this->" in 
//       order to call base class members (to avoid a compilier 
//       error) because they are "nondependant named" and the 
//       templated base class is a "dependant named".  The 
//       compilier will not look in dependant named base classes 
//       when looking up independant names.
// See: http://www.parashift.com/c++-faq-lite/nondependent-name-lookup-members.html

/**
    \class Histogram
	Holder of data grouped into pre-determined width bins.
	\tparam HistoBinType is the type of the data held in each bin (i.e. what data type described the width of the bin)
	\tparam HistoCountType is the count type of data held in each bin (i.e. what data type counts the number of items held in the bin itself)
*/
template<class HistoBinType, class HistoCountType>
class HistogramStatistic : public Statistic<HistoBinType> 
{
public:
    /**
        Creates a new bin with a specific bin width
        \param binW The width of the bin
    */
    HistogramStatistic(Component* comp, std::string statName, HistoBinType MinValue, HistoBinType MaxValue, HistoBinType binWidth, bool includeOutOfBounds = false) :
        Statistic<HistoBinType>(comp, statName) 
    {
        initProperties(MinValue, MaxValue, binWidth, includeOutOfBounds);
    }

    /**
        Creates a new bin with a specific bin width
        \param[binW] The width of the bin
        \param[name] Pointer to a name of the histogram variable (this is a description for the statistic engine to use in output)
    */
    HistogramStatistic(Component* comp, const char* statName, HistoBinType MinValue, HistoBinType MaxValue, HistoBinType binWidth, bool includeOutOfBounds = false) :
        Statistic<HistoBinType>(comp, statName) 
    {
        initProperties(MinValue, MaxValue, binWidth, includeOutOfBounds);
    }

    /**
        Count how many bins are active in this histogram
    */
    HistoCountType getActiveBinCount() 
    {
        return m_binsMap.size();
    }

    /**
        Count how many bins are available
    */
    HistoBinType getNumBins() 
    {
        // Note: the maxValue is inclusive
        return ((m_maxValue - m_minValue) + 1) / m_binWidth;
    }

    /**
        Get the width of a bin in this histogram
    */
    HistoBinType getBinWidth() 
    {
        return m_binWidth;
    }

    /**
        Get the count of items in the bin by the start value (e.g. give me the count of items in the bin which begins at value X).
        \return The count of items in the bin else 0.
    */
    HistoCountType getBinCountByBinStart(HistoBinType binStartValue) 
    {
        // Find the Bin Start Value in the Bin Map
        HistoMapItr_t bin_itr = m_binsMap.find(binStartValue);

        // Check to see if the Start Value was found
        if(bin_itr == m_binsMap.end()) {
            // No, return no count for this bin
            return (HistoCountType) 0;
        } else {
            // Yes, return the bin count
            return m_binsMap[binStartValue];
        }
    }

    /**
        Get the smallest start value of a bin in this histogram (i.e. the minimum value possibly represented by this histogram)
    */
    HistoBinType getBinsMinValue() 
    {
        return m_minValue;
    }

    /**
        Get the largest possible value represented by this histogram (i.e. the highest value in any of items bins rounded above to the size of the bin)
    */
    HistoBinType getBinsMaxValue() 
    {
        return m_maxValue;
    }

    /**
        Get the total number of items contained in all bins
        \return The number of items contained in all bins
    */
    uint64_t getItemCount() 
    {
        return this->getCollectionCount();
    }

    /**
        Sum up every item presented for storage in the histogram
        \return The sum of all values added into the histogram
    */
    HistoBinType getValuesSummed() 
    {
        return m_totalSummed;
    }

    void clearStatisticData()
    {
        m_totalSummed = 0;
        m_binsMap.clear();
        this->setCollectionCount(0);
    }
    
    void registerOutputFields(StatisticOutput* statOutput)
    {
        // Check to see if we have registered the Startup Fields        
        m_Fields.push_back(statOutput->registerField<HistoBinType>("MinValue"));
        m_Fields.push_back(statOutput->registerField<HistoBinType>("MaxValue"));  
        m_Fields.push_back(statOutput->registerField<HistoBinType>("BinWidth"));  
        m_Fields.push_back(statOutput->registerField<HistoBinType>("TotalNumBins"));  
        m_Fields.push_back(statOutput->registerField<HistoBinType>("NumActiveBins"));  
        m_Fields.push_back(statOutput->registerField<uint64_t>    ("NumItemsCollected"));  
        
        // Do we need to register additional fields for the end of Sim
//        if (true == m_DumpBinsAtEndOfSim) {
//            m_Fields[4] = statOutput->registerField<HistoBinType>  ("BinDumpStartValue");
//            m_Fields[5] = statOutput->registerField<HistoBinType>  ("BinDumpEndValue");
//            m_Fields[6] = statOutput->registerField<HistoCountType>("BinDumpNumEntries");
//        }
    }
    
    void outputStatisticData(StatisticOutput* statOutput, bool EndOfSimFlag)
    {
        uint64_t x = 0;
        statOutput->outputField(m_Fields[x++], getBinsMinValue());
        statOutput->outputField(m_Fields[x++], getBinsMaxValue());  
        statOutput->outputField(m_Fields[x++], getBinWidth());  
        statOutput->outputField(m_Fields[x++], getNumBins());  
        statOutput->outputField(m_Fields[x++], getActiveBinCount());  
        statOutput->outputField(m_Fields[x++], getItemCount());
/*
        // Perform end of sim Bin Dump if requested
        if ((true == m_DumpBinsAtEndOfSim) && (true == EndOfSimFlag)) {
                
            for(HistoBinType x = getBinStart(); x <= getBinEnd() && NotOverflowDetect; x += getBinWidth()) {
                // Check for OverFlow
                if ((MaxNumValue - getBinWidth()) < x) {
                    NotOverflowDetect = false;
                }
                
                if (getBinCountByBinStart(x) > 0) {
                    statOutput->outputField(m_Fields[0], getBinStart());
                    statOutput->outputField(m_Fields[1], getBinEnd());  
                    statOutput->outputField(m_Fields[2], getBinWidth());  
                    statOutput->outputField(m_Fields[3], getItemCount());

                    statOutput->outputField(m_Fields[4], x);
                    statOutput->outputField(m_Fields[5], NotOverflowDetect ? x + getBinWidth() : MaxNumValue);
                    statOutput->outputField(m_Fields[6], getBinCountByBinStart(x));
                }
            }
        } else {
            statOutput->outputField(m_Fields[0], getBinStart());
            statOutput->outputField(m_Fields[1], getBinEnd());  
            statOutput->outputField(m_Fields[2], getBinWidth());  
            statOutput->outputField(m_Fields[3], getItemCount());
        }
*/
    }

protected:    
    /**
        Adds a new value to the histogram. The correct bin is identified and then incremented. If no bin can be found
        to hold the value then a new bin is created.
    */
    void addData_impl(HistoBinType value) 
    {
        HistoBinType binValue;

        // Check to see if the value is above or below the min/max values
        if (value < m_minValue) {
            if (true == m_includeOutOfBounds) {
              // Add the "Below Min" value to the total summation 
              m_totalSummed += value;
              binValue = m_minValue; // Force the system to count this value
            } else {
                // Dont add this value
                return;
            }
            
        } else {
            if (value > m_maxValue) {
                if (true == m_includeOutOfBounds) {
                    // Add the "Above Max" value to the total summation 
                    m_totalSummed += value;
                    binValue = m_maxValue; // Force the system to count this value
                } else {
                    // Dont add this value
                    return;
                }
            } else {
              // Add the "in limits" value to the total summation 
              m_totalSummed += value;
              binValue = value;
          }
        }
        
        
        // Figure out what the starting bin is and find it in the map
        HistoBinType bin_start = m_binWidth * (binValue / m_binWidth);
        HistoMapItr_t bin_itr = m_binsMap.find(bin_start);

        // Was the bin found?
        if(bin_itr == m_binsMap.end()) {
            // No, so add a value of 1 to this bin
            m_binsMap.insert(std::pair<HistoBinType, HistoCountType>(bin_start, (HistoCountType) 1));
        } else {
            // Yes, Increment the bin count
            bin_itr->second++;
        }
    }
    
    bool isStatModeSupported(StatisticBase::StatMode_t mode) const 
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
    void initProperties(HistoBinType MinValue, HistoBinType MaxValue, HistoBinType binWidth, bool includeOutOfBounds) 
    {    
        m_binWidth = binWidth;
        m_includeOutOfBounds = includeOutOfBounds;
        m_totalSummed = 0;
        m_minValue = MinValue;
        m_maxValue = MaxValue;
    }
    
private:
    /**
    * 	Bin Map Definition
    */
    typedef std::map<HistoBinType, HistoCountType> HistoMap_t;
    /**
    * 	Iterator over the histogram bins
    */
    typedef typename HistoMap_t::iterator HistoMapItr_t;


    /**
        The minimum value in the Histogram
    */
    HistoBinType m_minValue;
    /**
        The maximum bin-value of the Histrogram (i.e. the maximum value rounded up)
    */
    HistoBinType m_maxValue;
    /**
        The width of a Histogram bin
    */
    HistoBinType m_binWidth;
    
    /**
        The sum of all values added into the Histogram, this is calculated and the sum of all values presented
        to be entered into the Histogram not with bin-width multiplied by the (max-min)/2 of the bin.
    */
    HistoBinType m_totalSummed;

    /**
        A map of the the bin starts to the bin counts
    */
    HistoMap_t            m_binsMap;
    
    std::vector<uint64_t> m_Fields;
    bool                  m_includeOutOfBounds;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Statistic<HistoBinType>);
        ar & BOOST_SERIALIZATION_NVP(m_minValue);
        ar & BOOST_SERIALIZATION_NVP(m_maxValue); 
        ar & BOOST_SERIALIZATION_NVP(m_binWidth); 
        ar & BOOST_SERIALIZATION_NVP(m_totalSummed);
        ar & BOOST_SERIALIZATION_NVP(m_binsMap);
        ar & BOOST_SERIALIZATION_NVP(m_Fields);
        ar & BOOST_SERIALIZATION_NVP(m_includeOutOfBounds);
    }
};

} //namespace Statistics
} //namespace SST

//BOOST_CLASS_EXPORT_KEY(SST::Statistics::HistogramStatistic<uint32_t, uint32_t>)

#endif
