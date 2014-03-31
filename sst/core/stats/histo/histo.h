
#ifndef _H_SST_CORE_STATS_HISTO
#define _H_SST_CORE_STATS_HISTO

#include <stdint.h>
#include <map>

using namespace std;

namespace SST {
namespace Statistics {

template<class HistoType>
class Histogram {
	public:
		Histogram(HistoType binW) {
			binCount = 0;
			binWidth = binW;
			bins = NULL;
			totalSummed = 0;
			itemCount = 0;
		}

		void add(HistoType value) {
			if(NULL == bins) {
				bins = (HistoType*) malloc(sizeof(HistoType));
				bins[0] = 1;
				binCount = 1;

				minVal = (value - (value % binWidth));
				maxVal = minVal + binWidth;

				itemCount = 1;
				totalSummed = value;
			} else {
				if(value < minVal) {
					HistoType newLower = value - (value % binWidth);
					HistoType diff = minVal - newLower;
					int diff_bins = diff / binWidth;

					binCount += (uint32_t) diff_bins;
					HistoType* newBins = (HistoType*) malloc(binCount * sizeof(HistoType));

					for(uint32_t i = 0; i < binCount; ++i) {
						newBins[i] = 0;
					}

					for(uint32_t i = (uint32_t) diff_bins; i < binCount; ++i) {
						newBins[i] = bins[i - diff_bins];
					}

					free(bins);
					bins = newBins;
					minVal = newLower;
				} else if (value >= maxVal) {
					HistoType newMax = value + (binWidth - (value % binWidth));
					HistoType diff = newMax - maxVal;
					int diff_bins = diff / binWidth;

					binCount += (uint32_t) diff_bins;
					HistoType* newBins = (HistoType*) malloc(binCount * sizeof(HistoType));

					for(uint32_t i = 0; i < binCount; ++i) {
						newBins[i] = 0;
					}

					for(uint32_t i = 0; i < (uint32_t) (binCount - diff_bins); ++i) {
						newBins[i] = bins[i];
					}

					free(bins);
					bins = newBins;
					maxVal = newMax;
				}

				HistoType value_from_base = value - minVal;
				int inc_element = value_from_base / binWidth;

				bins[inc_element]++;

				totalSummed += value;
				itemCount++;
			}
		}

		uint32_t getBinCount() {
			return binCount;
		}

		HistoType getBinWidth() {
			return binWidth;
		}

		HistoType* getBinByIndex(int index) {
			return &bins[index];
		}

		HistoType getBinStart() {
			return minVal;
		}

		HistoType getBinMax() {
			return maxVal;
		}

		uint64_t getItemCount() {
			return itemCount;
		}

		HistoType getValuesSummed() {
			return totalSummed;
		}

	private:
		HistoType* bins;
		HistoType binWidth;
		HistoType minVal;
		HistoType maxVal;
		HistoType totalSummed;
		uint64_t itemCount;
		uint32_t binCount;
};

}
}

#endif
