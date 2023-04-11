// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODULE_H
#define SST_CORE_MODULE_H

#include "sst/core/eli/elementinfo.h"

namespace SST {
/**
   Module is a tag class used with the loadModule function.
 */
class Module
{
public:
    SST_ELI_DECLARE_BASE(Module)
    // declare extern to limit compile times
    SST_ELI_DECLARE_CTORS(ELI_CTOR(SST::Params&), ELI_CTOR(Component*, SST::Params&))
    SST_ELI_DECLARE_INFO_EXTERN(ELI::ProvidesParams)
    Module() {}
    virtual ~Module() {}
};
} // namespace SST

// Very hackish way to get a deprecation warning for
// SST_ELI_REGISTER_MODULE, but there are no standard ways to do this
class sst_eli_fake_deprecated_class
{
public:
    static constexpr int fake_deprecated_function()
        __attribute__((deprecated("SST_ELI_REGISTER_MODULE is deprecated and will be removed in SST 13. Please use the "
                                  "new SST_ELI_REGISTER_MODULE_API and SST_ELI_REGISTER_MODULE_DERIVED macros")))
    {
        return 0;
    }
};

#define SST_ELI_REGISTER_MODULE(cls, lib, name, version, desc, interface)                            \
    static const int SST_ELI_FAKE_VALUE = sst_eli_fake_deprecated_class::fake_deprecated_function(); \
    SST_ELI_REGISTER_DERIVED(SST::Module,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)              \
    SST_ELI_INTERFACE_INFO(interface)

// New way to register modules.  Must register an interface (API)
// first, then you can register a module that implements it
#define SST_ELI_REGISTER_MODULE_API(cls, ...)   \
    SST_ELI_DECLARE_NEW_BASE(SST::Module,::cls) \
    SST_ELI_NEW_BASE_CTOR(SST::Params&,##__VA_ARGS__)

#define SST_ELI_REGISTER_MODULE_DERIVED_API(cls, base, ...) \
    SST_ELI_DECLARE_NEW_BASE(::base,::cls)                  \
    SST_ELI_NEW_BASE_CTOR(SST::Params&,##__VA_ARGS__)

#define SST_ELI_REGISTER_MODULE_DERIVED(cls, lib, name, version, desc, interface)       \
    SST_ELI_REGISTER_DERIVED(::interface,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
    SST_ELI_INTERFACE_INFO(#interface)

#endif // SST_CORE_MODULE_H
