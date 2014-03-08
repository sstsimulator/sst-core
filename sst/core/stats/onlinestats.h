
#ifndef _H_SST_CORE_ONLINE_STATS
#define _H_SST_CORE_ONLINE_STATS

namespace SST {
namespace Statistics {

<template typename NumberBase>
class OnlineStatistic {

	public:
		OnlineStatistic() {
			count = 0;
			sum = 0;
			sum_sq = 0;
		}

		NumberBase getSum() {
			return sum;
		}

		NumberBase getSumSquared() {
			return sum_sq;
		}

		void add(NumberBase value) {
			sum += value;
			sum_sq += (value * value);
		}

		void add(NumberBase* values, uint32_t length) {
			for(uint32_t i = 0; i < length; ++i) {
				sum += values[i];
				sum_sq += (values[i] * values[i]);
			}
		}

		NumberBase getArithmeticMean() {
			return (count > 0) ? (sum / (NumberBase) count) : 0;
		}

		NumberBase getVariance() {
			return (count > 0) ?
				((sum_sq * count) - (sum * sum)) :
				0;
		}

		NumberBase getStandardDeviation() {
			return (NumberBase) std::sqrt( (double) getVariance() );
		}

		uint64_t getCount() {
			return count;
		}

	private:
		NumberBase sum;
		NumberBase sum_sq;
		uint64_t count;

};

}
}

#endif
