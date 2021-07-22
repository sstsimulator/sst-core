// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_COMPONENTEXTENSION_H
#define SST_CORE_COMPONENTEXTENSION_H

#include "sst/core/baseComponent.h"
#include "sst/core/warnmacros.h"

namespace SST {

/**
   ComponentExtension is a class that can be loaded using
   loadComponentExtension<T>(...).  All the calls to the BaseComponent
   APIU will act like they are happening in the nearest SubConmponent
   or Component parent.  Hierarchy will not be kept in the case were a
   ComponentExtension is loaded into a ComponentExtension; they will
   both act like they are in the parent.
*/
class ComponentExtension : public BaseComponent
{

public:
    ComponentExtension(ComponentId_t id);

    virtual ~ComponentExtension() {};
};

} // namespace SST

#endif // SST_CORE_COMPONENTEXTENSION_H
