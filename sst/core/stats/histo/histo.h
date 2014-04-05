
#ifndef _H_SST_CORE_STATS_HISTO
#define _H_SST_CORE_STATS_HISTO

#include <sst_config.h>
#include <sst/core/output.h>

#include <stdint.h>
#include <map>

using namespace std;

namespace SST {
namespace Statistics {

template<class HistoBinType, class HistoCountType>
class Histogram {
	public:
		Histogram(HistoBinType binW) {
			totalSummed = 0;
			itemCount = 0;
			binWidth = binW;
			minVal = 0;
			maxVal = 0;
		}

		void add(HistoBinType value) {
			HistoBinType bin_start = value - (value % binWidth);
			histo_itr bin_itr = bins.find(bin_start);
			
			if(bin_itr == bins.end()) {
				bins.insert(std::pair<HistoBinType, HistoCountType>(bin_start, 1));
			} else {
				bin_itr->second++;
			}
			
			totalSummed += value;
			
			minVal = (minVal < bin_start) ? minVal : bin_start;
			maxVal = (maxVal > bin_start) ? maxVal : bin_start;
			
		}

		HistoCountType getBinCount() {
			return bins.size();
		}

		HistoBinType getBinWidth() {
			return binWidth;
		}

		HistoCountType getBinCountByBinStart(HistoBinType v) {
			histo_itr bin_itr = bins.find(v);
			
			if(bin_itr == bins.end()) {
				return (HistoCountType) 0;
			} else {
				return bins[v];
			}
		}

		HistoBinType getBinStart() {
			return minVal;
		}

		HistoBinType getBinEnd() {
			return maxVal;
		}

		HistoCountType getItemCount() {
			return itemCount;
		}

		HistoBinType getValuesSummed() {
			return totalSummed;
		}
		
		typedef typename std::map<HistoBinType, HistoCountType>::iterator histo_itr;
		
	private:
		HistoBinType minVal;
		HistoBinType maxVal;
		HistoBinType binWidth;
		HistoBinType totalSummed;
		HistoCountType itemCount;

		std::map<HistoBinType, HistoCountType> bins;
};

}
}

#endif
