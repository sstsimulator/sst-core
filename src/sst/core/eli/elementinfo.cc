// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/eli/elementinfo.h"

#include "sst/core/eli/elibase.h"

#include <ostream>
#include <string>
#include <vector>

namespace SST::ELI {

/**************************************************************************
  BaseElementInfo class functions
**************************************************************************/

void
force_instantiate_bool(bool UNUSED(b), const char* UNUSED(name))
{}

static const std::vector<int> SST_ELI_COMPILED_VERSION = { 0, 9, 0 };

std::string
ProvidesDefaultInfo::getELIVersionString() const
{
    std::string str;
    const char* delim = "";
    for ( auto item : SST_ELI_COMPILED_VERSION ) {
        str += delim;
        delim = ".";
        str += std::to_string(item);
    }
    return str;
}

const std::vector<int>&
ProvidesDefaultInfo::getELICompiledVersion() const
{
    return SST_ELI_COMPILED_VERSION;
}

void
ProvidesParams::toString(std::ostream& os) const
{
    os << "      Parameters (" << getValidParams().size() << " total)\n";
    for ( const ElementInfoParam& item : getValidParams() ) {
        os << "         " << item.name << ": " << (item.description == nullptr ? " <empty>" : item.description) << " "
           << " [" << (item.defaultValue == nullptr ? "<required>" : item.defaultValue) << "]\n";
    }
}

void
ProvidesPorts::toString(std::ostream& os) const
{
    os << "      Ports (" << getValidPorts().size() << " total)\n";
    for ( auto& item : getValidPorts() ) {
        os << "         " << item.name << ": " << (item.description == nullptr ? "<empty>" : item.description) << "\n";
    }
}

void
ProvidesSubComponentSlots::toString(std::ostream& os) const
{
    os << "      SubComponent Slots (" << getSubComponentSlots().size() << " total)\n";
    for ( auto& item : getSubComponentSlots() ) {
        os << "         " << item.name << ": " << (item.description == nullptr ? "<empty>" : item.description) << " ["
           << (item.superclass == nullptr ? "<none>" : item.superclass) << "]\n";
    }
}

void
ProvidesProfilePoints::toString(std::ostream& os) const
{
    os << "      Profile Points (" << getProfilePoints().size() << " total)\n";
    for ( auto& item : getProfilePoints() ) {
        os << "         " << item.name << ": " << (item.description == nullptr ? "<empty>" : item.description) << " ["
           << (item.superclass == nullptr ? "<none>" : item.superclass) << "]\n";
    }
}

void
ProvidesStats::toString(std::ostream& os) const
{
    os << "      Statistics (" << getValidStats().size() << " total)\n";
    for ( auto& item : getValidStats() ) {
        os << "         " << item.name << ": " << (item.description == nullptr ? "<empty>" : item.description) << ", "
           << " (units = \"" << (item.units == nullptr ? "<empty>" : item.units) << "\")"
           << " Enable level = " << (int16_t)item.enableLevel << "\n";
    }
}

void
ProvidesDefaultInfo::toString(std::ostream& os) const
{
    if ( !getAlias().empty() ) os << "      " << getName() << " is aliased as: " << getAlias() << std::endl;
    os << "      Description: " << getDescription() << std::endl;
    os << "      ELI version: " << getELIVersionString() << std::endl;
    os << "      Compiled using file: " << getCompileFile() << std::endl;
}

void
ProvidesAttributes::toString(std::ostream& os) const
{
    os << "      Attributes (" << getAttributes().size() << " total)\n";
    for ( const ElementInfoAttribute& item : getAttributes() ) {
        os << "         " << item.name << " = " << (item.value == nullptr ? "<empty>" : item.value) << "\n";
    }
}

void
ProvidesPorts::init()
{
    for ( auto& item : ports_ ) {
        portnames.push_back(item.name);
    }
}

void
ProvidesParams::init()
{
    for ( auto& item : params_ ) {
        allowedKeys.push_back(item.name);
    }
}

} // namespace SST::ELI
