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

#ifndef SST_CORE_LINKMAP_H
#define SST_CORE_LINKMAP_H

#include <sst/core/sst_types.h>

#include <string>
#include <map>

#include <sst/core/component.h>
#include <sst/core/link.h>

namespace SST {

/**
 * Maps port names to the Links that are connected to it
 */
class LinkMap {

private:
    std::map<std::string,Link*> linkMap;
    const std::vector<std::string> * allowedPorts;
    std::vector<std::string> selfPorts;

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
    //     if ( *x != *y ) return false; // aka, both NULL
    //     return true;
    // }

    // bool checkPort(const std::string &name) const
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
    //     if ( NULL != allowedPorts ) {
    //         for ( std::vector<std::string>::const_iterator i = allowedPorts->begin() ; i != allowedPorts->end() ; ++i ) {
    //             /* Compare name with stored name, which may have wildcards */
    //             if ( checkPort(i->c_str(), x) ) {
    //                 found = true;
    //                 break;
    //             }
    //         }
    //     }
    //     return found;
    // }

    // bool checkPort(const std::string &name) const
    // {
    //     const char *x = name.c_str();
    //     bool found = false;
    //     if ( NULL != allowedPorts ) {
    //         for ( std::vector<std::string>::const_iterator i = allowedPorts->begin() ; i != allowedPorts->end() ; ++i ) {
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
    LinkMap() : allowedPorts(NULL) {}
    ~LinkMap() {
        // Delete all the links in the map
        for ( std::map<std::string,Link*>::iterator it = linkMap.begin(); it != linkMap.end(); ++it ) {
            delete it->second;
        }
        linkMap.clear();
    }

    /**
     * Set the list of allowed port names from the ElementInfoPort
     */
    void setAllowedPorts(const std::vector<std::string> *p)
    {
        allowedPorts = p;
    }

    /**
     * Add a port name to the list of allowed ports.
     * Used by SelfLinks, as these are undocumented.
     */
    void addSelfPort(std::string& name)
    {
        selfPorts.push_back(name);
    }

    bool isSelfPort(const std::string& name) const {
        for ( std::vector<std::string>::const_iterator i = selfPorts.begin() ; i != selfPorts.end() ; ++i ) {
            /* Compare name with stored name, which may have wildcards */
            // if ( checkPort(i->c_str(), x) ) {
            if ( name == *i ) {
                return true;
            }
        }
        return false;
    }
    
    /** Inserts a new pair of name and link into the map */
    void insertLink(std::string name, Link* link) {
        linkMap.insert(std::pair<std::string,Link*>(name,link));
    }

    /** Returns a Link pointer for a given name */
    Link* getLink(std::string name) {
//         if ( !checkPort(name) ) {
// #ifdef USE_PARAM_WARNINGS
//             std::cerr << "Warning:  Using undocumented port '" << name << "'." << std::endl;
// #endif
//         }
        std::map<std::string,Link*>::iterator it = linkMap.find(name);
        if ( it == linkMap.end() ) return NULL;
        else return it->second;
    }

    /**
       Checks to see if LinkMap is empty.
       @return True if Link map is empty, false otherwise
    */
    bool empty() {
        return linkMap.empty();
    }
    
    // FIXME: Kludge for now, fix later.  Need to make LinkMap look
    // like a regular map instead.
    /** Return a reference to the internal map */
    std::map<std::string,Link*>& getLinkMap() {
        return linkMap;
    }

};

} // namespace SST

#endif // SST_CORE_LINKMAP_H
