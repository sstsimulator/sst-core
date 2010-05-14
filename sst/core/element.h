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

#ifndef SST_ELEMENT_H
#define SST_ELEMENT_H

#include <sst/component.h>

namespace SST {
    typedef Component* (*componentAllocate)(ComponentId_t, Component::Params_t&);
    typedef void (*eventInitialize)(void);

    struct ElementInfoComponent {
        const char *name;
        const char *description;
        void (*printHelp)(FILE *output);
        componentAllocate alloc;
    };

    struct ElementInfoEvent {
        const char *name;
        const char *description;
        void (*printHelp)(FILE *output);
        eventInitialize init;
    };

    struct ElementLibraryInfo {
        const char *name;
        const char *description;
        const struct ElementInfoComponent* components;
        const struct ElementInfoEvent* events;
    };
};

#endif // SST_ELEMENT_H
