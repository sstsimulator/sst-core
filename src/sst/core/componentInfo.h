// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_COMPONENTINFO_H
#define SST_CORE_COMPONENTINFO_H

#include <sst/core/sst_types.h>

#include <unordered_set>
#include <string>
#include <functional>

#include <sst/core/configGraph.h>


namespace SST {

class LinkMap;
class Component;

struct ComponentInfo {

public:
    typedef std::vector<std::string>      statEnableList_t;        /*!< List of Enabled Statistics */
    typedef std::vector<Params>           statParamsList_t;        /*!< List of Enabled Statistics Parameters */

private:
    friend class Simulation;
    const ComponentId_t id;
    const std::string name;
    const std::string type;
    LinkMap* link_map;
    Component* component;
    std::map<std::string, ComponentInfo> subComponents;
    const Params *params;

    statEnableList_t *enabledStats;
    statParamsList_t *statParams;

    inline void setComponent(Component* comp) { component = comp; }


public:
    /* Old ELI Style */
    ComponentInfo(ComponentId_t id, const std::string &name, const std::string &type, const Params *params, LinkMap* link_map);

    /* New ELI Style */
    ComponentInfo(const ConfigComponent *ccomp, LinkMap* link_map);
    ComponentInfo(ComponentInfo &&o);
    ~ComponentInfo();

    inline ComponentId_t getID() const { return id; }

    inline const std::string& getName() const { return name; }

    inline const std::string& getType() const { return type; }

    inline Component* getComponent() const { return component; }

    inline LinkMap* getLinkMap() const { return link_map; }

    inline const Params* getParams() const { return params; }

    inline std::map<std::string, ComponentInfo>& getSubComponents() { return subComponents; }

    void setStatEnablement(statEnableList_t *enabled, statParamsList_t *params) {
        enabledStats = enabled;
        statParams = params;
    }

    statEnableList_t* getStatEnableList() const { return enabledStats; }
    statParamsList_t* getStatParams() const { return statParams; }

    void clearStatEnablement() {
        enabledStats = NULL;
        statParams = NULL;
    }

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
        ComponentInfo infoKey(0, key, "", NULL, NULL);
        auto value = dataByName.find(&infoKey);
        if ( value == dataByName.end() ) return NULL;
        return *value;
    }

    ComponentInfo* getByID(const ComponentId_t key) const {
        ComponentInfo infoKey(key, "", "", NULL, NULL);
        auto value = dataByID.find(&infoKey);
        if ( value == dataByID.end() ) return NULL;
        return *value;
    }

    bool empty() {
        return dataByName.empty();
    }

    void clear() {
        for ( auto i : dataByName ) {
            delete i; 
        }
        dataByName.clear();
        dataByID.clear();
    }
};
    
} //namespace SST

#endif // SST_CORE_COMPONENTINFO_H
