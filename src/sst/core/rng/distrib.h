// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_CORE_RNG_DISTRIB
#define _H_SST_CORE_RNG_DISTRIB


namespace SST {
namespace RNG {

/**
 * \class SSTRandomDistribution
 * Base class of statistical distributions in SST.
 */
class SSTRandomDistribution {

	public:
		/**
			Obtains the next double from the distribution
			\return The next double in the distribution being sampled
		*/
		virtual double getNextDouble() = 0;

		/** 
			Destroys the distribution
		*/
		virtual ~SSTRandomDistribution() {};

		/**
			Creates the base (abstract) class of a distribution
		*/
		SSTRandomDistribution() {};

};

}
}

#endif
