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


#include "sst_config.h"
#include "sst/core/elementinfo.h"

#include <sstream>

namespace SST {

std::map<std::string,LibraryInfo*> SST::ElementLibraryDatabase::libraries;

void
BaseComponentElementInfo::initialize()
{
    // Need to create the vector of just the port names so we can
    // set allowed ports
    // const std::vector<ElementInfoPort2>& ports = getValidPorts();
    const auto& ports = getValidPorts();
    for ( auto item : ports ) {
        portnames.push_back(item.name);
    }

    // Need to create the vector of just the stat names
    // const std::vector<ElementInfoStatistic>& stats = getValidStats();
    const auto& stats = getValidStats();
    for ( auto item : stats ) {
        statnames.push_back(item.name);
    }

    // Get the valid parameters into the right data structure for
    // created the components.
    // const std::vector<ElementInfoParam>& params = getValidParams();
    const auto& params = getValidParams();
    for ( auto item : params ) {
        allowedKeys.insert(item.name);
    }
}

std::string
BaseComponentElementInfo::getParametersString() {
    std::stringstream stream;
    stream << "    Parameters (" << getValidParams().size() << " total):"<<  std::endl;
    for ( auto item : getValidParams() ) {
        stream << "      " << item.name << ": "
               << (item.description == NULL ? "<empty>" : item.description)
               << " ("
               << (item.defaultValue == NULL ? "<required>" : item.defaultValue)
               << ")" << std::endl;
    }
    return stream.str();
}

std::string
BaseComponentElementInfo::getStatisticsString() {
    std::stringstream stream;    
    stream << "    Statistics (" << getValidStats().size() << " total):"<<  std::endl;
    for ( auto item : getValidStats() ) {
        stream << "      " << item.name << ": "
               << (item.description == NULL ? "<empty>" : item.description)
               << " ("
               << (item.units == NULL ? "<empty>" : item.units)
               << ").  Enable level = " << (int16_t)item.enableLevel << std::endl;
    }
    return stream.str();
}

std::string
BaseComponentElementInfo::getPortsString() {
    std::stringstream stream;    
    stream << "    Ports (" << getValidPorts().size() << " total):"<<  std::endl;
    for ( auto item : getValidPorts() ) {
        stream << "      " << item.name << ": "
               << (item.description == NULL ? "<empty>" : item.description)
               << std::endl;
    }
    return stream.str();
}


} //namespace SST
