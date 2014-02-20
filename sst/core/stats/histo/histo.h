
#ifndef _H_SST_CORE_STATS_HISTO
#define _H_SST_CORE_STATS_HISTO

#include <stdint.h>
#include <map>

using namespace std;

template<class HistoType>
class HistoBin {
	public:
		HistoBin(HistoType bValue) {
			baseValue = bValue;
			binCount = 0;
		}

		~HistoBin() {

		}

		void increment() {
			binCount++;
		}

		HistoType getBaseValue() {
			return baseValue;
		}

		uint64_t getCount() {
			return binCount;
		}

	private:
		HistoType baseValue;
		uint64_t binCount;
};

template<class HistoType>
class Histogram {
	public:
		Histogram(HistoType binW) {
			binCount = 1;
			binWidth = binW;

			// Create an empty bin
			bins = new std::vector<HistoBin<HistoType>* >();
			minVal = 0;
			maxVal = binW - 1;
			bins->push_back(new HistoBin<HistoType>(0));
		}

		void add(HistoType value) {
			if(value < minVal) {
				// Create a new lower bins
				HistoType diff = minVal - value;
				HistoType newMin = value - (value % binWidth);
				uint32_t createEntries = diff / binWidth;

				std::vector<HistoBin<HistoType>* >* newBins = new std::vector<HistoBin<HistoType>* >();
				for(uint32_t i = 0; i < createEntries; ++i) {
					newBins->push_back( new HistoBin<HistoType>(newMin + (i * binWidth)));
				}

				// Copy over the existing bins
				for(uint32_t i = 0; i < binCount; ++i) {
					newBins->push_back(bins->at(i));
				}

				binCount += createEntries;

				// Update to the new list of bins
				delete bins;
				minVal = newMin;
				bins = newBins;
			} else if (value > maxVal) {
				HistoType newMax = value;

				// If we get an uneven (i.e. no binWidth) value, round up
				if(value % binWidth > 0) {
					newMax = value + (binWidth  - (value % binWidth));
				}

				uint32_t createEntries = ((newMax - maxVal) / binWidth ) + 1;
				std::vector<HistoBin<HistoType>* >* newBins = new std::vector<HistoBin<HistoType>* >();

				for(unsigned int i = 0; i < binCount; ++i) {
					newBins->push_back(bins->at(i));
				}

				for(uint32_t i = 0; i < createEntries; ++i) {
					newBins->push_back(new HistoBin<HistoType>(maxVal + (i * binWidth)));
				}

				binCount += createEntries;

				delete bins;
				maxVal = (newMax - 1);
				bins = newBins;
			}

			int inc_entry = (value - minVal) / binWidth;
			bins->at(inc_entry)->increment();
		}

		uint32_t getBinCount() {
			return binCount;
		}

		HistoType getBinWidth() {
			return binWidth;
		}

		HistoBin<HistoType>* getBinByIndex(int index) {
			return bins->at(index);
		}

	private:
		std::vector<HistoBin<HistoType>* >* bins;
		HistoType binWidth;
		HistoType minVal;
		HistoType maxVal;
		uint32_t binCount;
};

#endif
