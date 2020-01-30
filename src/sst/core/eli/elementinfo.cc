// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/eli/elementinfo.h"

namespace SST {

/**************************************************************************
  BaseElementInfo class functions
**************************************************************************/
namespace ELI {

static const std::vector<int> SST_ELI_COMPILED_VERSION = {0, 9, 0};

std::string
ProvidesDefaultInfo::getELIVersionString() const
{
    std::stringstream stream;
    bool first = true;
    for (int item : SST_ELI_COMPILED_VERSION) {
        if ( first ) first = false;
        else stream << ".";
        stream << item;
    }
    return stream.str();    
}

const std::vector<int>&
ProvidesDefaultInfo::getELICompiledVersion() const
{
  return SST_ELI_COMPILED_VERSION;
}

void
ProvidesParams::toString(std::ostream& os) const
{
    os << "         NUM PARAMETERS = " << getValidParams().size() << "\n";
    int index = 0;
    for (const ElementInfoParam& item : getValidParams() ) {
      os << "            PARAMETER " << index
         << " = " << item.name
         << " (" << (item.description == nullptr ? "<empty>" : item.description) << ")"
         << " [" << (item.defaultValue == nullptr ? "<required>" : item.defaultValue) << "]\n";
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
         << " (" << (item.description == nullptr ? "<empty>" : item.description) << ")\n";
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
         << " (" << (item.description == nullptr ? "<empty>" : item.description) << ")"
         << " [" << (item.superclass == nullptr ? "<none>" : item.superclass) << "]\n";
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
         << " [" << (item.description == nullptr ? "<empty>" : item.description) << "]"
         << " (" << (item.units == nullptr ? "<empty>" : item.units) << ")"
         << " Enable level = " << (int16_t)item.enableLevel << "\n";
        ++index;
    }
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
} //namespace SST
