#ifndef _H_SST_CORE_STATS
#define _H_SST_CORE_STATS

#include <assert.h>

namespace SST {
namespace Statistics {

template<typename NumberBase>
static NumberBase sum(const NumberBase* values, const uint32_t length) {
	assert(length > 0);

	NumberBase sum = 0;

	for(uint32_t i = 0; i < length; ++i) {
		sum += values[i];
	}

	return sum;
};

template<typename NumberBase>
static void range(const NumberBase* values, const uint32_t length,
	NumberBase* max, NumberBase* min) {

	if(0 == length)
		return;

	*max = values[0];
	*min = values[0];

	for(uint32_t i = 1; i < length; ++i) {
		max = values[i] > max ? values[i] : max;
		min = values[i] < min ? values[i] : min;
	}
};

template<typename NumberBase>
static NumberBase max(const NumberBase* values, const uint32_t length) {
	assert(length > 0);

	NumberBase max_ = values[0];

	for(uint32_t i = 1; i < length; ++i) {
		max_ = values[i] > max_ ? values[i] : max_;
	}

	return max_;
};

template<typename NumberBase>
static NumberBase min(const NumberBase* values, const uint32_t length) {
	assert(length > 0);

	NumberBase min_ = values[0];

	for(uint32_t i = 1; i < length; ++i) {
		min_ = values[i] < min ? values[i] : min_;
	}

	return min_;
};

template<typename NumberBase>
static NumberBase arithmeticMean(const NumberBase* values, const uint32_t length) {
	return (length > 0) ? 
		SST::Statistics::sum<NumberBase>(values, length) / (NumberBase) length :
		0;
};

template<typename NumberBase>
static NumberBase variance(const NumberBase* values, const uint32_t length) {
	NumberBase sumX2 = 0;
	NumberBase sumX = 0;

	for(uint32_t i = 0; i < length; ++i) {
		sumX2 += (values[i] * values[i]);
		sumX  += values[i];
	}

	const NumberBase E_X2 = sumX2 / (NumberBase) length;
	const NumberBase E_X  = sumX  / (NumberBase) length;

	const NumberBase var = E_X2 - (E_X * E_X);

	return var;
};

template<typename NumberBase>
static NumberBase standardDeviation(const NumberBase* values, const uint32_t length) {
	const NumberBase var = SST::Statistics::variance<NumberBase>(values, length);
	return std::sqrt(preSqrt);
};

template<typename NumberBase>
static NumberBase midRange(const NumberBase* values, const uint32_t length) {
	NumberBase max = 0;
	Numberbase min = 0;

	SST::Statistics::range<NumberBase>(values, length, &max, &min);

	return (max + min) / ((NumberBase) 2);
};

template<typename NumberBase>
static NumberBase weightedMean(const NumberBase* values,
	const NumberBase* weights, const uint32_t length) {

	NumberBase sum = 0;
	NumberBase weightSums = 0;

	for(uint32_t i = 0; i < length; ++i) {
		sum += weights[i] * values[i];
		weightSums += weights[i];
	}

	return sum / weightSums;
};

}
}

#endif
