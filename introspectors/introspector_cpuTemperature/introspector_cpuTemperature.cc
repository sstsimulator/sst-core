// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "introspector_cpuTemperature.h"


bool Introspector_cpuTemperature::pullData( Cycle_t current )
{
    //_INTROSPECTOR_DBG("id=%lu currentCycle=%lu \n", Id(), current );
	Component *c;

        printf("introspector_cpuTemperature pulls data @ cycle %ld\n", current ); //current here specifies it's the #th call
	for( Database_t::iterator iter = DatabaseInt.begin();
                            iter != DatabaseInt.end(); ++iter )
    	{
	    c = iter->first;
            std::cout << "Pull data of component ID " << c->Id() << " with dataID = " << iter->second << " and data value = " << c->getIntData(iter->second) << std::endl;
	    
	}
	for( Database_t::iterator iter = DatabaseDouble.begin();
                            iter != DatabaseDouble.end(); ++iter )
    	{
            c = iter->first;
            std::cout << "Pull data of component ID " << c->Id() << " with dataID = " << iter->second << " and data value = " << c->getDoubleData(iter->second) << std::endl;	    
	}

	return false;
}



extern "C" {
Introspector_cpuTemperature* introspector_cpuTemperatureAllocComponent( SST::ComponentId_t id,
                                    SST::Component::Params_t& params )
{
    return new Introspector_cpuTemperature( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT2
BOOST_CLASS_EXPORT(Introspector_cpuTemperature)

BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Introspector_cpuTemperature, bool, SST::Cycle_t)
#endif
