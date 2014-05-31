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


#ifndef _H_SST_CORE_STATS_ACCUMULATOR
#define _H_SST_CORE_STATS_ACCUMULATOR

#include <sst/core/stats/basestats.h>

namespace SST {
namespace Statistics {

/**
	\class Accumulator

	Allows the online gathering of statistical information about a single quantity. The basic 
	statistics are captured online removing the need to keep a copy of the values of interest.

	@tparam NumberBase A template for the basic numerical type of values
*/
<template typename NumberBase>
class Accumulator : public SST::Statistics::BaseStatistic {

	public:
		/**
			Create a new Accumulator class with initial values set to a zero count,
			zero sum statistic of interest.
		*/
		Accumulator() {
			count = 0;
			sum = 0;
			sum_sq = 0;
		}

		/**
			Provides the sum of the values presented so far.
			@return The sum of values presented to the class so far.
		*/
		NumberBase getSum() {
			return sum;
		}

		/**
			Provides the sum of each value squared presented to the class so far.
			@return The sum of squared values presented to the class so far.
		*/
		NumberBase getSumSquared() {
			return sum_sq;
		}

		/**
			Present a new value to the class to be included in the statistics.
			@param value New value to be presented
		*/
		void add(NumberBase value) {
			sum += value;
			sum_sq += (value * value);
			count++;
		}

		/**
			Present an array of values to the class to be included in the statistics.
			@param values The array of values to be added to the statistics collection
			@param length The length of the array being presented
		*/
		void add(NumberBase* values, uint32_t length) {
			for(uint32_t i = 0; i < length; ++i) {
				sum += values[i];
				sum_sq += (values[i] * values[i]);
			}

			count += (uint64_t) length;
		}

		/**
			Get the arithmetic mean of the values presented so far
			@return The arithmetic mean of the values presented so far.
		*/
		NumberBase getArithmeticMean() {
			return (count > 0) ? (sum / (NumberBase) count) : 0;
		}

		/**
			Get the variance of the values presented so far
			@return The variance of the values presented so far
		*/
		NumberBase getVariance() {
			return (count > 0) ?
				((sum_sq * count) - (sum * sum)) :
				0;
		}

		/**
			Get the standard deviation of the values presented so far
			@return The standard deviation of the values presented so far
		*/
		NumberBase getStandardDeviation() {
			return (NumberBase) std::sqrt( (double) getVariance() );
		}

		/**
			Get a count of the number of elements presented to the statistics collection so far.
			@return Count the number of values presented to the class.
		*/
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
