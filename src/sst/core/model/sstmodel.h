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


#ifndef SST_CORE_MODEL_H
#define SST_CORE_MODEL_H

namespace SST {

class ConfigGraph;

/** Base class for Model Generation
 */
class SSTModelDescription {

	public:
		SSTModelDescription();
		virtual ~SSTModelDescription() {};
        /** Create the ConfigGraph
         *
         * This function should be overridden by subclasses.
         *
         * This function is responsible for reading any configuration
         * files and generating a ConfigGraph object.
         */
		virtual ConfigGraph* createConfigGraph() = 0;

};

}

#endif
