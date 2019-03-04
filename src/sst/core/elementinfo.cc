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
#include <sst/core/statapi/statbase.h>

#include <sstream>

namespace SST {

/**************************************************************************
  BaseElementInfo class functions
**************************************************************************/
namespace ELI {

std::unique_ptr<std::set<std::string>> LoadedLibraries::loaded_{};

std::string
ProvidesDefaultInfo::getELIVersionString() const
{
    std::stringstream stream;
    bool first = true;
    for ( int item : getELICompiledVersion() ) {
        if ( first ) first = false;
        else stream << ".";
        stream << item;
    }
    return stream.str();    
}

void
ProvidesParams::toString(std::ostream& os) const
{
    os << "         NUM PARAMETERS = " << getValidParams().size() << "\n";
    int index = 0;
    for (const ElementInfoParam& item : getValidParams() ) {
      os << "            PARAMETER " << index
         << " = " << item.name
         << " (" << (item.description == NULL ? "<empty>" : item.description) << ")"
         << " [" << (item.defaultValue == NULL ? "<required>" : item.defaultValue) << "]\n";
      index++;
    }
}

void
ProvidesPorts::toString(std::ostream& os) const
{
    os << "         NUM PORTS = " << getValidPorts().size() << "\n";
    int index = 0;
    for ( auto& item : getValidPorts() ) {
      os << "            PORT " << index
         << " = " << item.name
         << " (" << (item.description == NULL ? "<empty>" : item.description) << ")\n";
      ++index;
    }
}

void
ProvidesSubComponentSlots::toString(std::ostream& os) const
{
    os << "         NUM SUBCOMPONENT SLOTS = " << getSubComponentSlots().size() << "\n";
    int index = 0;
    for ( auto& item : getSubComponentSlots() ) {
      os << "            SUB COMPONENT SLOT " << index
         << " = " << item.name
         << " (" << (item.description == NULL ? "<empty>" : item.description) << ")"
         << " [" << (item.superclass == NULL ? "<none>" : item.superclass) << "]\n";
      ++index;
    }
}

void
ProvidesStats::toString(std::ostream& os) const
{
    os << "         NUM STATISTICS = " << getValidStats().size() << "\n";
    int index = 0;
    for ( auto& item : getValidStats() ) {
      os << "            STATISTIC " << index
         << " = " << item.name
         << " [" << (item.description == NULL ? "<empty>" : item.description) << "]"
         << " (" << (item.units == NULL ? "<empty>" : item.units) << ")"
         << " Enable level = " << (int16_t)item.enableLevel << "\n";
    }
    ++index;
}

void
ProvidesDefaultInfo::toString(std::ostream &os) const
{
  os << "    " << getName() << ": " << getDescription() << std::endl;
  os << "    Using ELI version " << getELIVersionString() << std::endl;
  os << "    Compiled on: " << getCompileDate() << ", using file: "
         << getCompileFile() << std::endl;
}


void
ProvidesPorts::init()
{
  for (auto& item : ports_) {
    portnames.push_back(item.name);
  }
}


void
ProvidesParams::init()
{
  for (auto& item : params_){
    allowedKeys.insert(item.name);
  }
}

}





/**************************************************************************
  LibraryInfo class functions
**************************************************************************/
#if 0
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
    for ( auto item : *libraries ) {
        stream << "library : " << item.first << std::endl;
        stream << item.second->toString();
        stream << std::endl;
    }
    return stream.str();
}
#endif

} //namespace SST
