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

#ifndef SST_CORE_COMPONENTINFO_H
#define SST_CORE_COMPONENTINFO_H

#include <sst/core/sst_types.h>
#include <sst/core/params.h>

#include <unordered_set>
#include <string>
#include <functional>

namespace SST {

class LinkMap;
class BaseComponent;

class ConfigComponent;
class ComponentInfoMap;

namespace Statistics {
class StatisticInfo;
}

class ComponentInfo {

public:
    typedef std::vector<Statistics::StatisticInfo>      statEnableList_t;        /*!< List of Enabled Statistics */

private:
    friend class Simulation;
    friend class BaseComponent;
    friend class ComponentInfoMap;
    const ComponentId_t id;
    const std::string name;
    const std::string slot_name;
    int slot_num;
    const std::string type;
    LinkMap* link_map;
    BaseComponent* component;
    
    std::vector<ComponentInfo> subComponents;
    const Params *params;

    statEnableList_t * enabledStats;
    std::vector<double> coordinates;

    inline void setComponent(BaseComponent* comp) { component = comp; }

    /* Lookup Key style constructor */
    ComponentInfo(ComponentId_t id, const std::string &name);
    void finalizeLinkConfiguration();
    void prepareForComplete();

public:
    /* Old ELI Style subcomponent constructor */
    ComponentInfo(const std::string &type, const Params *params, const ComponentInfo *parent);

    /* New ELI Style */
    ComponentInfo(ConfigComponent *ccomp, const std::string& name, LinkMap* link_map);
    ComponentInfo(ComponentInfo &&o);
    ~ComponentInfo();

    inline ComponentId_t getID() const { return id; }

    inline const std::string& getName() const { return name; }

    inline const std::string& getSlotName() const { return slot_name; }

    inline int getSlotNum() const { return slot_num; }

    inline const std::string& getType() const { return type; }

    inline BaseComponent* getComponent() const { return component; }

    inline LinkMap* getLinkMap() const { return link_map; }

    inline const Params* getParams() const { return params; }

    // inline std::map<std::string, ComponentInfo>& getSubComponents() { return subComponents; }
    inline std::vector<ComponentInfo>& getSubComponents() { return subComponents; }

    ComponentInfo* findSubComponent(std::string slot, int slot_num);
    ComponentInfo* findSubComponent(ComponentId_t id);
    std::vector<LinkId_t> getAllLinkIds() const;

    statEnableList_t* getStatEnableList() { return enabledStats; }

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
    std::unordered_set<ComponentInfo*, ComponentInfo::HashID, ComponentInfo::EqualsID> dataByID;

public:
    typedef std::unordered_set<ComponentInfo*, ComponentInfo::HashID, ComponentInfo::EqualsID>::const_iterator const_iterator;

    const_iterator begin() const {
        return dataByID.begin();
    }

    const_iterator end() const {
        return dataByID.end();
    }

    ComponentInfoMap() {}

    void insert(ComponentInfo* info) {
        dataByID.insert(info);
    }

    ComponentInfo* getByID(const ComponentId_t key) const {
        ComponentInfo infoKey(COMPONENT_ID_MASK(key), "");
        auto value = dataByID.find(&infoKey);
        if ( value == dataByID.end() ) return NULL;
        if ( SUBCOMPONENT_ID_MASK(key) != 0 ) {
            // Looking for a subcomponent
            return (*value)->findSubComponent(key);
        }
        return *value;
    }

    bool empty() {
        return dataByID.empty();
    }

    void clear() {
        for ( auto i : dataByID ) {
            delete i;
        }
        dataByID.clear();
    }
};
    
} //namespace SST

#endif // SST_CORE_COMPONENTINFO_H
