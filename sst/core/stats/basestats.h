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

#ifndef _H_SST_CORE_BASE_STATISTICS
#define _H_SST_CORE_BASE_STATISTICS

#include <string>
#include <cstdio>

namespace SST {
namespace Statistics {

/**
        \class BaseStatistics

	Forms the base class for statistics gathering within the SST core. Statistics are
	gathered by the core and processed into various (extensible) output forms. Statistics
	are expected to be named so that they can be located in the simulation output files.

*/
class BaseStatistic {

	public:
		/**
			Constructor for the BaseStatistics class. In this for the
			string provided is copied into a buffer within the class (which
			consumes memory, for statistics with many components callers may
			want to use the alternative constructor which provides a pointer
			that is not copied). Default is for the statistic to be created
			enabled for use

			\param[statName] The name of the statistic being collected
		*/
		BaseStatistic(std::string statName) : enabled(true) {
				name = (char*) malloc(sizeof(char) * (statName.size() + 1));
				sprintf(name, "%s", statName.c_str());
			 }

		/**
			Constructor for the BaseStatistic class. This form of the
			constructor takes a pointer and does NOT perform a copy of the
			string (i.e. assumes that the pointer will remain live for the
			duration of the statistic's use). Default is for the statistic
			to be created enabled for use

			\param[statName] A pointer to a name of the statistic being collected, this pointer must remain live for the duration of the statistic's use but can be modifed since it is read on use.
		*/
		BaseStatistic(char* statName) : enabled(true) { name = statName; }

		/**
			Get the name of the statistic
			\return The name of the statistic
		*/
		const char* getName() {
			return name;
		}

		/**
			Enable the statistic for collection
		*/
		void enable() {
			enabled = true;
		}

		/**
			Disable the statistic collection
		*/
		void disable() {
			enabled = false;
		}

		/**
			Query whether the statistic is currently enabled
			\return true if the statistics collection is currently enabled, otherwise false
		*/
		bool isEnabled() {
			return enabled;
		}

	protected:
		char* name;
		bool  enabled;

};

}
}

#endif
