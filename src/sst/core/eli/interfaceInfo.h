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

#ifndef SST_CORE_ELI_INTERFACE_INFO_H
#define SST_CORE_ELI_INTERFACE_INFO_H

#include <iostream>
#include <string>

namespace SST::ELI {

class ProvidesInterface
{
public:
    const std::string& getInterface() const { return iface_; }

    void toString(std::ostream& os) const { os << "      Interface: " << iface_ << "\n"; }

    template <class XMLNode>
    void outputXML(XMLNode* node) const
    {
        node->SetAttribute("Interface", iface_.c_str());
    }

protected:
    template <class T>
    explicit ProvidesInterface(T* UNUSED(t)) :
        iface_(T::ELI_getInterface())
    {}

private:
    std::string iface_;
};

} // namespace SST::ELI

#define SST_ELI_INTERFACE_INFO(interface)       \
    static const std::string ELI_getInterface() \
    {                                           \
        return interface;                       \
    }

#endif // SST_CORE_ELI_INTERFACE_INFO_H
