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

// Component Category Definitions
#define COMPONENT_CATEGORY_UNCATEGORIZED 0x00
#define COMPONENT_CATEGORY_PROCESSOR      0x01
#define COMPONENT_CATEGORY_MEMORY         0x02
#define COMPONENT_CATEGORY_NETWORK        0x04
#define COMPONENT_CATEGORY_SYSTEM         0x08

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

struct ElementInfoPort {
    const char *name;
    const char *description;
    const char **validEvents;
};

struct ElementInfoComponent {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    componentAllocate alloc;
    const ElementInfoParam *params;
    const ElementInfoPort *ports;
    uint32_t category;
};

struct ElementInfoIntrospector {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    introspectorAllocate alloc;
    const ElementInfoParam *params;
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
    const ElementInfoParam *params;
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
