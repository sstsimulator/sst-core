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

#ifndef SST_CORE_ELI_PORTS_INFO_H
#define SST_CORE_ELI_PORTS_INFO_H

#include "sst/core/eli/elibase.h"

#include <string>
#include <vector>

namespace SST {
namespace ELI {

template <class T, class Enable = void>
struct InfoPorts
{
    static const std::vector<SST::ElementInfoPort>& get()
    {
        static std::vector<SST::ElementInfoPort> var = {};
        return var;
    }
};

template <class T>
struct InfoPorts<T, typename MethodDetect<decltype(T::ELI_getPorts())>::type>
{
    static const std::vector<SST::ElementInfoPort>& get() { return T::ELI_getPorts(); }
};

class ProvidesPorts
{
public:
    const std::vector<std::string>&     getPortnames() { return portnames; }
    const std::vector<ElementInfoPort>& getValidPorts() const { return ports_; }

    void toString(std::ostream& os) const;

    template <class XMLNode>
    void outputXML(XMLNode* node) const
    {
        // Build the Element to Represent the Component
        int idx = 0;
        for ( auto& port : ports_ ) {
            auto* XMLPortElement = new XMLNode("Port");
            XMLPortElement->SetAttribute("Index", idx);
            XMLPortElement->SetAttribute("Name", port.name);
            XMLPortElement->SetAttribute("Description", port.description ? port.description : "none");
            node->LinkEndChild(XMLPortElement);
            ++idx;
        }
    }

protected:
    template <class T>
    ProvidesPorts(T* UNUSED(t)) : ports_(InfoPorts<T>::get())
    {
        init();
    }

private:
    void init();

    std::vector<std::string>     portnames;
    std::vector<ElementInfoPort> ports_;
};

} // namespace ELI
} // namespace SST

// clang-format off
#define SST_ELI_DOCUMENT_PORTS(...)                                                                                \
    static const std::vector<SST::ElementInfoPort>& ELI_getPorts()                                                 \
    {                                                                                                              \
        static std::vector<SST::ElementInfoPort> var    = { __VA_ARGS__ };                                         \
        auto parent = SST::ELI::InfoPorts<                                                                         \
            std::conditional<(__EliDerivedLevel > __EliBaseLevel), __LocalEliBase, __ParentEliBase>::type>::get(); \
        SST::ELI::combineEliInfo(var, parent);                                                                     \
        return var;                                                                                                \
    }
// clang-format on

#define SST_ELI_DELETE_PORT(port) \
    {                             \
        port, nullptr, {}         \
    }

#endif // SST_CORE_ELI_PORTS_INFO_H
