// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_CORE_STATS
#define _H_SST_CORE_STATS

#include <assert.h>

namespace SST {
namespace Statistics {

/**
 	Sums (adds up) all of the values presented as an array of length.
	@param values An array of values to be summed up
	@param length The length of the array in elements
	@return Returns the values from 0 to length summed up
*/
template<typename NumberBase>
static NumberBase sum(const NumberBase* values, const uint32_t length) {
	assert(length > 0);

	NumberBase sum = 0;

	for(uint32_t i = 0; i < length; ++i) {
		sum += values[i];
	}

	return sum;
};

/**
	Calculates the maximum and minimum of the numbers on the values array.
	@param values An array of numbers to be searched for min and max
	@param length The length of the array in elements
	@param max The output - the maximum of the numbers in values
	@param min The output - the minimum of the numbers in values
*/
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

/**
	Calculates the maximum number in the array values
	@param values An array of numbers to be searched for the max
	@param length The length of the array of values in elements
	@return The maximum value found
*/
template<typename NumberBase>
static NumberBase max(const NumberBase* values, const uint32_t length) {
	assert(length > 0);

	NumberBase max_ = values[0];

	for(uint32_t i = 1; i < length; ++i) {
		max_ = values[i] > max_ ? values[i] : max_;
	}

	return max_;
};

/**
	Calculates the minimum number in the array of values
	@param values An array of numbers to be searched for the min
	@param length The length of the array of values in elements
	@return The minimum value found
*/
template<typename NumberBase>
static NumberBase min(const NumberBase* values, const uint32_t length) {
	assert(length > 0);

	NumberBase min_ = values[0];

	for(uint32_t i = 1; i < length; ++i) {
		min_ = values[i] < min ? values[i] : min_;
	}

	return min_;
};

/**
	Calculates the arithemetic mean of a set of values (sum of values divided by length)
	@param values An array of numbers to be averaged
	@param length The length of the array of values in elemetns
	@return The arithmetic mean of the values
*/
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
