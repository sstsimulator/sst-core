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


#ifndef _H_SST_CORE_STATS_HISTO
#define _H_SST_CORE_STATS_HISTO

#include <sst_config.h>
#include <sst/core/output.h>

#include <stdint.h>
#include <map>

using namespace std;

namespace SST {
namespace Statistics {

/**
    \class Histogram
	Holder of data grouped into pre-determined width bins.
	\tparam HistoBinType is the type of the data held in each bin (i.e. what data type described the width of the bin)
	\tparam HistoCountType is the count type of data held in each bin (i.e. what data type counts the number of items held in the bin itself)
*/
template<class HistoBinType, class HistoCountType>
class Histogram {
	public:
		/**
			Creates a new bin with a specific bin width
			\param binW The width of the bin
		*/
		Histogram(HistoBinType binW) {
			totalSummed = 0;
			itemCount = 0;
			binWidth = binW;
			minVal = 0;
			maxVal = 0;
		}

		/**
			Adds a new value to the histogram. The correct bin is identified and then incremented. If no bin can be found
			to hold the value then a new bin is created.
		*/
		void add(HistoBinType value) {
			HistoBinType bin_start = binWidth * (value / binWidth);
			histo_itr bin_itr = bins.find(bin_start);

			if(bin_itr == bins.end()) {
				bins.insert(std::pair<HistoBinType, HistoCountType>(bin_start, (HistoCountType) 1));
			} else {
				bin_itr->second++;
			}

			itemCount++;
			totalSummed += value;

			if(1 == itemCount) {
				minVal = bin_start;
				maxVal = bin_start;
			} else {
				minVal = (minVal < bin_start) ? minVal : bin_start;
				maxVal = (maxVal > bin_start) ? maxVal : bin_start;
			}
		}

		/**
			Count how many bins are active in this histogram
		*/
		HistoCountType getBinCount() {
			return bins.size();
		}

		/**
			Get the width of a bin in this histogram
		*/
		HistoBinType getBinWidth() {
			return binWidth;
		}

		/**
			Get the count of items in the bin by the start value (e.g. give me the count of items in the bin which begins at value X).
			\return The count of items in the bin else 0.
		*/
		HistoCountType getBinCountByBinStart(HistoBinType v) {
			histo_itr bin_itr = bins.find(v);

			if(bin_itr == bins.end()) {
				return (HistoCountType) 0;
			} else {
				return bins[v];
			}
		}

		/**
			Get the smallest start value of a bin in this histogram (i.e. the minimum value possibly represented by this histogram)
		*/
		HistoBinType getBinStart() {
			return minVal;
		}

		/**
			Get the largest possible value represented by this histogram (i.e. the highest value in any of items bins rounded above to the size of the bin)
		*/
		HistoBinType getBinEnd() {
			return maxVal;
		}

		/**
			Get the total number of items contained in all bins
			\return The number of items contained in all bins
		*/
		HistoCountType getItemCount() {
			return itemCount;
		}

		/**
			Sum up every item presented for storage in the histogram
			\return The sum of all values added into the histogram
		*/
		HistoBinType getValuesSummed() {
			return totalSummed;
		}

        /**
         * Iterator over the histogram bins
         */
		typedef typename std::map<HistoBinType, HistoCountType>::iterator histo_itr;

	private:
		/**
			The minimum value in the Histogram
		*/
		HistoBinType minVal;
		/**
			The maximum bin-value of the Histrogram (i.e. the maximum value rounded up)
		*/
		HistoBinType maxVal;
		/**
			The width of a Histogram bin
		*/
		HistoBinType binWidth;
		/**
			The sum of all values added into the Histogram, this is calculated and the sum of all values presented
			to be entered into the Histogram not with bin-width multiplied by the (max-min)/2 of the bin.
		*/
		HistoBinType totalSummed;
		/**
			Total count of items added to the Histogram
		*/
		HistoCountType itemCount;

		/**
			A map of the the bin starts to the bin counts
		*/
		std::map<HistoBinType, HistoCountType> bins;
};

}
}

#endif
