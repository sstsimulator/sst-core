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

#ifndef SST_CORE_ELI_ATTRIBUTE_INFO_H
#define SST_CORE_ELI_ATTRIBUTE_INFO_H

#include "sst/core/eli/elibase.h"
#include "sst/core/warnmacros.h"

#include <string>
#include <vector>

namespace SST {
namespace ELI {

template <class T, class Enable = void>
struct GetAttributes
{
    static const std::vector<SST::ElementInfoAttribute>& get()
    {
        static std::vector<SST::ElementInfoAttribute> var = {};
        return var;
    }
};

template <class T>
struct GetAttributes<T, typename MethodDetect<decltype(T::ELI_getAttributes())>::type>
{
    static const std::vector<SST::ElementInfoAttribute>& get() { return T::ELI_getAttributes(); }
};

class ProvidesAttributes
{
public:
    const std::vector<ElementInfoAttribute>& getAttributes() const { return attributes_; }

    void toString(std::ostream& os) const;

    template <class XMLNode>
    void outputXML(XMLNode* node) const
    {
        // Build the Element to Represent the Component
        int idx = 0;
        for ( const ElementInfoAttribute& attribute : attributes_ ) {
            ;
            // Build the Element to Represent the Attribute
            auto* XMLAttributeElement = new XMLNode("Attribute");
            XMLAttributeElement->SetAttribute("Index", idx);
            XMLAttributeElement->SetAttribute("Name", attribute.name);
            XMLAttributeElement->SetAttribute("Value", attribute.value ? attribute.value : "none");
            node->LinkEndChild(XMLAttributeElement);
            ++idx;
        }
    }

protected:
    template <class T>
    ProvidesAttributes(T* UNUSED(t)) : attributes_(GetAttributes<T>::get())
    {}

private:
    std::vector<ElementInfoAttribute> attributes_;
};

} // namespace ELI
} // namespace SST

// clang-format off
#define SST_ELI_DOCUMENT_ATTRIBUTES(...)                                                                           \
    static const std::vector<SST::ElementInfoAttribute>& ELI_getAttributes()                                       \
    {                                                                                                              \
        static std::vector<SST::ElementInfoAttribute> var  = { __VA_ARGS__ };                                      \
        auto parent = SST::ELI::GetAttributes<                                                                     \
            std::conditional<(__EliDerivedLevel > __EliBaseLevel), __LocalEliBase, __ParentEliBase>::type>::get(); \
        SST::ELI::combineEliInfo(var, parent);                                                                     \
        return var;                                                                                                \
    }
// clang-format on

#define SST_ELI_DELETE_ATTRIBUTE(attribute) \
    {                                       \
        attribute, nullptr                  \
    }

#endif // SST_CORE_ELI_ATTRIBUTE_INFO_H
