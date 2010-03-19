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

#include "introspector_cpu.h"


bool Introspector_cpu::pullData( Cycle_t current )
{
    //_INTROSPECTOR_DBG("id=%lu currentCycle=%lu \n", Id(), current );
	Component *c;

        printf("introspector_cpu pulls data @ cycle %ld\n", current ); //current here specifies it's the #th call
	for( DatabaseInt_t::iterator iter = DatabaseInt.begin();
                            iter != DatabaseInt.end(); ++iter )
    	{
	    c = iter->first;
            std::cout << "Pull data of component ID " << c->Id() << " with dataID = " << iter->second << " and data value = " << c->getIntData(iter->second) << std::endl;
	    
	}
	for( DatabaseDouble_t::iterator iter = DatabaseDouble.begin();
                            iter != DatabaseDouble.end(); ++iter )
    	{
            printf("Pull data of ID %lu with value = %lf\n", iter->first, *(iter->second));
	    
	}

	return false;
}



extern "C" {
Introspector_cpu* introspector_cpuAllocIntrospector( SST::ComponentId_t id,
                                    SST::Component::Params_t& params )
{
    return new Introspector_cpu( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT2
BOOST_CLASS_EXPORT(Introspector_cpu)

BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Introspector_cpu, bool, SST::Cycle_t)
#endif
