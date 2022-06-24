// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_PROFILE_PROFILETOOL_H
#define SST_CORE_PROFILE_PROFILETOOL_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

namespace SST {

class Params;

namespace Profile {

/**
   ProfileTool is a class loadable through the factory which allows
   dynamic addition of profiling capabililites to profile points.
*/
class ProfileTool
{

public:
    SST_ELI_DECLARE_BASE(ProfileTool)
    // maybe declare extern to limit compile times??
    SST_ELI_DECLARE_CTOR(ProfileToolId_t, const std::string&)
    SST_ELI_DECLARE_INFO(
      ELI::ProvidesParams,
      ELI::ProvidesInterface)

    ProfileTool(ProfileToolId_t id, const std::string& name);

    virtual ~ProfileTool() {}

    ProfileToolId_t getId() { return my_id; }

    std::string getName() { return name; }

    virtual void outputData(FILE* fp) = 0;

protected:
    const uint64_t    my_id;
    const std::string name;
};

} // namespace Profile
} // namespace SST

// Register profile tools.  Must register an interface
// (API) first, then you can register a subcomponent that implements
// it
#define SST_ELI_REGISTER_PROFILETOOL_API(cls, ...)            \
    SST_ELI_DECLARE_NEW_BASE(SST::Profile::ProfileTool,::cls) \
    SST_ELI_NEW_BASE_CTOR(ProfileToolId_t,const std::string&,##__VA_ARGS__)

#define SST_ELI_REGISTER_PROFILETOOL_DERIVED_API(cls, base, ...) \
    SST_ELI_DECLARE_NEW_BASE(::base,::cls)                       \
    SST_ELI_NEW_BASE_CTOR(ProfileToolId_t,const std::string&,##__VA_ARGS__)

#define SST_ELI_REGISTER_PROFILETOOL(cls, interface, lib, name, version, desc)          \
    SST_ELI_REGISTER_DERIVED(::interface,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
    SST_ELI_INTERFACE_INFO(#interface)

#endif // SST_CORE_SUBCOMPONENT_H
