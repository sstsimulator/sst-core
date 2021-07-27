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

#ifndef SST_CORE_LINKMAP_H
#define SST_CORE_LINKMAP_H

#include "sst/core/component.h"
#include "sst/core/link.h"
#include "sst/core/sst_types.h"

#include <map>
#include <string>

namespace SST {

/**
 * Maps port names to the Links that are connected to it
 */
class LinkMap
{

private:
    std::map<std::string, Link*> linkMap;
    // const std::vector<std::string> * allowedPorts;
    std::vector<std::string>     selfPorts;

    // bool checkPort(const char *def, const char *offered) const
    // {
    //     const char * x = def;
    //     const char * y = offered;

    //     /* Special case.  Name of '*' matches everything */
    //     if ( *x == '*' && *(x+1) == '\0' ) return true;

    //     do {
    //         if ( *x == '%' && (*(x+1) == '(' || *(x+1) == 'd') ) {
    //             // We have a %d or %(var)d to eat
    //             x++;
    //             if ( *x == '(' ) {
    //                 while ( *x && (*x != ')') ) x++;
    //                 x++;  /* *x should now == 'd' */
    //             }
    //             if ( *x != 'd') /* Malformed string.  Fail all the things */
    //                 return false;
    //             x++; /* Finish eating the variable */
    //             /* Now, eat the corresponding digits of y */
    //             while ( *y && isdigit(*y) ) y++;
    //         }
    //         if ( *x != *y ) return false;
    //         if ( *x == '\0' ) return true;
    //         x++;
    //         y++;
    //     } while ( *x && *y );
    //     if ( *x != *y ) return false; // aka, both nullptr
    //     return true;
    // }

    // bool checkPort(const std::string& name) const
    // {
    //     // First check to see if this is a self port
    //     for ( std::vector<std::string>::const_iterator i = selfPorts.begin() ; i != selfPorts.end() ; ++i ) {
    //         /* Compare name with stored name, which may have wildcards */
    //         // if ( checkPort(i->c_str(), x) ) {
    //         if ( name == *i ) {
    //             return true;
    //         }
    //     }

    //     // If no a self port, check against info in library manifest
    //     Component::isValidPortForComponent(
    //     const char *x = name.c_str();
    //     bool found = false;
    //     if ( nullptr != allowedPorts ) {
    //         for ( std::vector<std::string>::const_iterator i = allowedPorts->begin() ; i != allowedPorts->end() ; ++i
    //         ) {
    //             /* Compare name with stored name, which may have wildcards */
    //             if ( checkPort(i->c_str(), x) ) {
    //                 found = true;
    //                 break;
    //             }
    //         }
    //     }
    //     return found;
    // }

    // bool checkPort(const std::string& name) const
    // {
    //     const char *x = name.c_str();
    //     bool found = false;
    //     if ( nullptr != allowedPorts ) {
    //         for ( std::vector<std::string>::const_iterator i = allowedPorts->begin() ; i != allowedPorts->end() ; ++i
    //         ) {
    //             /* Compare name with stored name, which may have wildcards */
    //             if ( checkPort(i->c_str(), x) ) {
    //                 found = true;
    //                 break;
    //             }
    //         }
    //     }

    //     if ( !found ) { // Check self ports
    //         for ( std::vector<std::string>::const_iterator i = selfPorts.begin() ; i != selfPorts.end() ; ++i ) {
    //             /* Compare name with stored name, which may have wildcards */
    //             // if ( checkPort(i->c_str(), x) ) {
    //             if ( name == *i ) {
    //                 found = true;
    //                 break;
    //             }
    //         }
    //     }

    //     return found;
    // }

public:
#if !SST_BUILDING_CORE
    LinkMap() /*: allowedPorts(nullptr)*/ __attribute__((
        deprecated("LinkMap class was not intended to be used outside of SST Core and will be removed in SST 12.")))
#else
    LinkMap() /*: allowedPorts(nullptr)*/
#endif
    {}

#if !SST_BUILDING_CORE
    ~LinkMap() __attribute__((
        deprecated("LinkMap class was not intended to be used outside of SST Core and will be removed in SST 12.")))
#else
    ~LinkMap()
#endif
    {
        // Delete all the links in the map
        for ( std::map<std::string, Link*>::iterator it = linkMap.begin(); it != linkMap.end(); ++it ) {
            delete it->second;
        }
        linkMap.clear();
    }

    // /**
    //  * Set the list of allowed port names from the ElementInfoPort
    //  */
    // void setAllowedPorts(const std::vector<std::string> *p)
    // {
    //     allowedPorts = p;
    // }

    /**
     * Add a port name to the list of allowed ports.
     * Used by SelfLinks, as these are undocumented.
     */
#if !SST_BUILDING_CORE
    void addSelfPort(const std::string& name) __attribute__((
        deprecated("LinkMap class was not intended to be used outside of SST Core and will be removed in SST 12.")))
#else
    void                          addSelfPort(const std::string& name)
#endif
    {
        selfPorts.push_back(name);
    }

#if !SST_BUILDING_CORE
    bool isSelfPort(const std::string& name) const __attribute__((
        deprecated("LinkMap class was not intended to be used otuside of SST Core and will be removed in SST 12.")))
#else
    bool                          isSelfPort(const std::string& name) const
#endif
    {
        for ( std::vector<std::string>::const_iterator i = selfPorts.begin(); i != selfPorts.end(); ++i ) {
            /* Compare name with stored name, which may have wildcards */
            // if ( checkPort(i->c_str(), x) ) {
            if ( name == *i ) { return true; }
        }
        return false;
    }

    /** Inserts a new pair of name and link into the map */
#if !SST_BUILDING_CORE
    void insertLink(const std::string& name, Link* link) __attribute__((
        deprecated("LinkMap class was not intended to be used outside of SST Core and will be removed in SST 12.")))
#else
    void                          insertLink(const std::string& name, Link* link)
#endif
    {
        linkMap.insert(std::pair<std::string, Link*>(name, link));
    }

#if !SST_BUILDING_CORE
    void removeLink(const std::string& name) __attribute__((
        deprecated("LinkMap class was not intended to be used outside of SST Core and will be removed in SST 12.")))
#else
    void                          removeLink(const std::string& name)
#endif
    {
        linkMap.erase(name);
    }

    /** Returns a Link pointer for a given name */
#if !SST_BUILDING_CORE
    Link* getLink(const std::string& name) __attribute__((
        deprecated("LinkMap class was not intended to be used outside of SST Core and will be removed in SST 12.")))
#else
    Link*                         getLink(const std::string& name)
#endif
    {

        //         if ( !checkPort(name) ) {
        // #ifdef USE_PARAM_WARNINGS
        //             std::cerr << "Warning:  Using undocumented port '" << name << "'." << std::endl;
        // #endif
        //         }
        std::map<std::string, Link*>::iterator it = linkMap.find(name);
        if ( it == linkMap.end() )
            return nullptr;
        else
            return it->second;
    }

    /**
       Checks to see if LinkMap is empty.
       @return True if Link map is empty, false otherwise
    */
#if !SST_BUILDING_CORE
    bool empty() __attribute__((
        deprecated("LinkMap class was not intended to be used outside of SST Core and will be removed in SST 12.")))
#else
    bool                          empty()
#endif
    {
        return linkMap.empty();
    }

    // FIXME: Kludge for now, fix later.  Need to make LinkMap look
    // like a regular map instead.
    /** Return a reference to the internal map */
#if !SST_BUILDING_CORE
    std::map<std::string, Link*>& getLinkMap() __attribute__((
        deprecated("LinkMap class was not intended to be used outside of SST Core and will be removed in SST 12.")))
#else
    std::map<std::string, Link*>& getLinkMap()
#endif
    {
        return linkMap;
    }
};

} // namespace SST

#endif // SST_CORE_LINKMAP_H
