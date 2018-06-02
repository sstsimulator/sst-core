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


#include "sst_config.h"
#include "sst/core/elementinfo.h"

#include <sstream>

namespace SST {

std::map<std::string,LibraryInfo*> SST::ElementLibraryDatabase::libraries;

/**************************************************************************
  BaseElementInfo class functions
**************************************************************************/
std::string
BaseElementInfo::getELIVersionString() {
    std::stringstream stream;
    bool first = true;
    for ( int item : getELICompiledVersion() ) {
        if ( first ) first = false;
        else stream << ".";
        stream << item;
    }
    return stream.str();    
}

/**************************************************************************
  BaseParamsElementInfo class functions
**************************************************************************/
void
BaseParamsElementInfo::initialize_allowedKeys()
{
    // Get the valid parameters into the right data structure for
    // created the components.
    // const std::vector<ElementInfoParam>& params = getValidParams();
    const auto& params = getValidParams();
    for ( auto item : params ) {
        allowedKeys.insert(item.name);
    }
}

std::string
BaseParamsElementInfo::getParametersString() {
    std::stringstream stream;
    stream << "      Parameters (" << getValidParams().size() << " total):"<<  std::endl;
    for ( auto item : getValidParams() ) {
        stream << "        " << item.name << ": "
               << (item.description == NULL ? "<empty>" : item.description)
               << " ("
               << (item.defaultValue == NULL ? "<required>" : item.defaultValue)
               << ")" << std::endl;
    }
    return stream.str();
}


/**************************************************************************
  BaseComponentElementInfo class functions
**************************************************************************/
void
BaseComponentElementInfo::initialize_portnames()
{
    // Need to create the vector of just the port names so we can
    // set allowed ports
    // const std::vector<ElementInfoPort2>& ports = getValidPorts();
    const auto& ports = getValidPorts();
    for ( auto item : ports ) {
        portnames.push_back(item.name);
    }
}

void
BaseComponentElementInfo::initialize_statnames()
{
    // Need to create the vector of just the stat names
    // const std::vector<ElementInfoStatistic>& stats = getValidStats();
    const auto& stats = getValidStats();
    for ( auto item : stats ) {
        statnames.push_back(item.name);
    }
}


std::string
BaseComponentElementInfo::getPortsString() {
    std::stringstream stream;    
    stream << "      Ports (" << getValidPorts().size() << " total):"<<  std::endl;
    for ( auto item : getValidPorts() ) {
        stream << "        " << item.name << ": "
               << (item.description == NULL ? "<empty>" : item.description)
               << std::endl;
    }
    return stream.str();
}

std::string
BaseComponentElementInfo::getSubComponentSlotString() {
    std::stringstream stream;    
    stream << "      SubComponentSlots (" << getSubComponentSlots().size() << " total):"<<  std::endl;
    for ( auto item : getSubComponentSlots() ) {
        stream << "        " << item.name << ": "
               << (item.description == NULL ? "<empty>" : item.description)
               << std::endl;
    }
    return stream.str();
}

std::string
BaseComponentElementInfo::getStatisticsString() {
    std::stringstream stream;    
    stream << "      Statistics (" << getValidStats().size() << " total):"<<  std::endl;
    for ( auto item : getValidStats() ) {
        stream << "        " << item.name << ": "
               << (item.description == NULL ? "<empty>" : item.description)
               << " ("
               << (item.units == NULL ? "<empty>" : item.units)
               << ").  Enable level = " << (int16_t)item.enableLevel << std::endl;
    }
    return stream.str();
}

/**************************************************************************
  ComponentElementInfo class functions
**************************************************************************/
std::string
ComponentElementInfo::toString()
{
    std::stringstream stream;
    stream << "    " << getName() << ": " << getDescription() << std::endl;
    stream << "    Using ELI version " << getELIVersionString() << std::endl;
    stream << "    Compiled on: " << getCompileDate() << ", using file: " << getCompileFile() << std::endl;
    stream << getParametersString();
    stream << getStatisticsString();
    stream << getPortsString();
    stream << getSubComponentSlotString();
    return stream.str();
}


/**************************************************************************
  SubComponentElementInfo class functions
**************************************************************************/
std::string
SubComponentElementInfo::toString()
{
    std::stringstream stream;
    stream << "    " << getName() << ": " << getDescription() << std::endl;
    stream << getParametersString();
    stream << getStatisticsString();
    stream << getPortsString();
    stream << getSubComponentSlotString();
    return stream.str();
}


/**************************************************************************
  ModuleElementInfo class functions
**************************************************************************/
std::string
ModuleElementInfo::toString() {
    std::stringstream stream;
    stream << "    " << getName() << ": " << getDescription() << std::endl;
    stream << getParametersString();
    return stream.str();
}


/**************************************************************************
  PartitionerElementInfo class functions
**************************************************************************/
std::string
PartitionerElementInfo::toString() {
    std::stringstream stream;
    stream << "    " << getName() << ": " << getDescription() << std::endl;
    return stream.str();
}


/**************************************************************************
  LibraryInfo class functions
**************************************************************************/
std::string
LibraryInfo::toString()
{
    std::stringstream stream;
    stream << "  Components: " << std::endl;
    for ( auto item : components ) {
        stream << item.second->toString();
        stream << std::endl;
    }
    if ( components.size() == 0 ) stream << "    <none>" << std::endl;
    
    stream << "  SubComponents: " << std::endl;
    for ( auto item : subcomponents ) {
        stream << item.second->toString();
        stream << std::endl;
    }
    if ( subcomponents.size() == 0 ) stream << "    <none>" << std::endl;
    
    stream << "  Modules: " << std::endl;
    for ( auto item : modules ) {
        stream << item.second->toString();
        stream << std::endl;
    }
    if ( modules.size() == 0 ) stream << "    <none>" << std::endl;
    
    stream << "  Partitioners: " << std::endl;
    for ( auto item : partitioners ) {
        stream << item.second->toString();
        stream << std::endl;
    }
    if ( partitioners.size() == 0 ) stream << "    <none>" << std::endl;
    
    stream << "  Python Module: " << std::endl;
    if ( python_module == NULL ) stream << "    No" << std::endl;
    else stream << "    Yes" << std::endl;
    
    return stream.str();
}


/**************************************************************************
  LibraryInfo class functions
**************************************************************************/
std::string
ElementLibraryDatabase::toString() {
    std::stringstream stream;
    for ( auto item : libraries ) {
        stream << "library : " << item.first << std::endl;
        stream << item.second->toString();
        stream << std::endl;
    }
    return stream.str();
}


} //namespace SST
