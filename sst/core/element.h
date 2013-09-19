// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_ELEMENT_H
#define SST_CORE_ELEMENT_H

#include <sst/core/sst_types.h>

//#include <sst/core/component.h>
//#include <sst/core/params.h>

namespace SST {
class Component;
class ConfigGraph;
class Introspector;
class Module;
class Params;
 
typedef Component* (*componentAllocate)(ComponentId_t, Params&);
typedef Introspector* (*introspectorAllocate)(Params&);
typedef void (*eventInitialize)(void);
typedef Module* (*moduleAllocate)(Params&);
 typedef Module* (*moduleAllocateWithComponent)(Component*, Params&);
typedef void (*partitionFunction)(ConfigGraph*,int);
typedef void (*generateFunction)(ConfigGraph*, std::string options, int ranks);
 
struct ElementInfoParam {
    const char *name;
    const char *description;
};

struct ElementInfoComponent {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    componentAllocate alloc;
    const ElementInfoParam *params;
};

struct ElementInfoIntrospector {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    introspectorAllocate alloc;
    ElementInfoParam *params;
};

struct ElementInfoEvent {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    eventInitialize init;
};

struct ElementInfoModule {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    moduleAllocate alloc;
    moduleAllocateWithComponent alloc_with_comp;
    ElementInfoParam *params;
};

struct ElementInfoPartitioner {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    partitionFunction func;
};

struct ElementInfoGenerator {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    generateFunction func;
};

struct ElementLibraryInfo {
    const char *name;
    const char *description;
    const struct ElementInfoComponent* components;
    const struct ElementInfoEvent* events;
    const struct ElementInfoIntrospector* introspectors;
    /* const struct ElementInfoSubcomponent* subcomponents; */
    const struct ElementInfoModule* modules;
    const struct ElementInfoPartitioner* partitioners;
    const struct ElementInfoGenerator* generators;
};
} //namespace SST

#endif // SST_CORE_ELEMENT_H
