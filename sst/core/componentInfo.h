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

#ifndef SST_CORE_COMPONENTINFO_H
#define SST_CORE_COMPONENTINFO_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <unordered_set>
#include <string>
#include <functional>

/* #include <sst/core/clock.h> */
/* #include <sst/core/oneshot.h> */
/* #include <sst/core/event.h> */
/* //#include <sst/core/params.h> */
/* //#include <sst/core/link.h> */
/* //#include <sst/core/timeConverter.h> */
/* #include "sst/core/simulation.h" */
/* #include "sst/core/unitAlgebra.h" */
/* #include "sst/core/statapi/statbase.h" */

namespace SST {

class LinkMap;
class Component;
    
struct ComponentInfo {

    friend class Simulation;
    
private:
    const ComponentId_t id;
    const std::string name;
    const std::string type;
    LinkMap* link_map;
    Component* component;

    inline void setComponent(Component* comp) { component = comp; }

    
public:
    ComponentInfo(ComponentId_t id, std::string name, std::string type, LinkMap* link_map) :
        id(id),
        name(name),
        type(type),
        link_map(link_map)
    {}

    ~ComponentInfo();

    // ComponentInfo() :
    //     id(0),
    //     name(""),
    //     type(""),
    //     link_map(NULL)
    // {}

    inline ComponentId_t getID() const { return id; }
    
    inline const std::string& getName() const { return name; }

    inline const std::string& getType() const { return type; }

    inline Component* getComponent() const { return component; }
    
    inline LinkMap* getLinkMap() const { return link_map; }


    struct HashName {
        size_t operator() (const ComponentInfo* info) const {
            std::hash<std::string> hash;
            return hash(info->name);
        }
    };

    struct EqualsName {
        bool operator() (const ComponentInfo* lhs, const ComponentInfo* rhs) const {
            return lhs->name == rhs->name;
        }
    };

    struct HashID {
        size_t operator() (const ComponentInfo* info) const {
            std::hash<ComponentId_t> hash;
            return hash(info->id);
        }
    };

    struct EqualsID {
        bool operator() (const ComponentInfo* lhs, const ComponentInfo* rhs) const {
            return lhs->id == rhs->id;
        }
    };
    
};


class ComponentInfoMap {
private:
    std::unordered_set<ComponentInfo*, ComponentInfo::HashName, ComponentInfo::EqualsName> dataByName;
    std::unordered_set<ComponentInfo*, ComponentInfo::HashID, ComponentInfo::EqualsID> dataByID;

public:
    typedef std::unordered_set<ComponentInfo*, ComponentInfo::HashName, ComponentInfo::EqualsName>::const_iterator const_iterator;

    const_iterator begin() const {
        return dataByName.begin();
    }

    const_iterator end() const {
        return dataByName.end();
    }

    ComponentInfoMap() {}

    void insert(ComponentInfo* info) {
        dataByName.insert(info);
        dataByID.insert(info);
    }

    ComponentInfo* getByName(const std::string& key) const {
        ComponentInfo infoKey(0, key, "", NULL);
        auto value = dataByName.find(&infoKey);
        if ( value == dataByName.end() ) return NULL;
        return *value;
    }

    ComponentInfo* getByID(const ComponentId_t key) const {
        ComponentInfo infoKey(key, "", "", NULL);
        auto value = dataByID.find(&infoKey);
        if ( value == dataByID.end() ) return NULL;
        return *value;
    }

};
    
} //namespace SST

#endif // SST_CORE_COMPONENTINFO_H
