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

#ifndef SST_CORE_ELI_PROFILEPOINTINFO_H
#define SST_CORE_ELI_PROFILEPOINTINFO_H

#include "sst/core/eli/elibase.h"

#include <string>
#include <type_traits>
#include <vector>

namespace SST::ELI {

template <typename, typename = void>
struct InfoProfilePoints
{
    static const std::vector<SST::ElementInfoProfilePoint>& get()
    {
        static std::vector<SST::ElementInfoProfilePoint> var = {};
        return var;
    }
};

template <class T>
struct InfoProfilePoints<T, std::void_t<decltype(T::ELI_getProfilePoints())>>
{
    static const std::vector<SST::ElementInfoProfilePoint>& get() { return T::ELI_getProfilePoints(); }
};

class ProvidesProfilePoints
{
public:
    const std::vector<ElementInfoProfilePoint>& getProfilePoints() const { return points_; }

    void toString(std::ostream& os) const;

    template <class XMLNode>
    void outputXML(XMLNode* node) const
    {
        int idx = 0;
        for ( const ElementInfoProfilePoint& point : points_ ) {
            auto* element = new XMLNode("ProfilePoint");
            element->SetAttribute("Index", idx);
            element->SetAttribute("Name", point.name);
            element->SetAttribute("Description", point.description ? point.description : "none");
            element->SetAttribute("Interface", point.superclass ? point.superclass : "none");
            node->LinkEndChild(element);
            ++idx;
        }
    }

protected:
    template <class T>
    explicit ProvidesProfilePoints(T* UNUSED(t)) :
        points_(InfoProfilePoints<T>::get())
    {}

private:
    std::vector<ElementInfoProfilePoint> points_;
};

} // namespace SST::ELI

// clang-format off
#define SST_ELI_DOCUMENT_PROFILE_POINTS(...)                                                                  \
    static const std::vector<SST::ElementInfoProfilePoint>& ELI_getProfilePoints()                            \
    {                                                                                                         \
        static std::vector<SST::ElementInfoProfilePoint> var = { __VA_ARGS__ };                               \
        auto parent = SST::ELI::InfoProfilePoints<                                                            \
           std::conditional_t<(__EliDerivedLevel > __EliBaseLevel), __LocalEliBase, __ParentEliBase>>::get(); \
        SST::ELI::combineEliInfo(var, parent);                                                                \
        return var;                                                                                           \
    }
// clang-format on
#define SST_ELI_DELETE_PROFILE_POINT(point) { point, nullptr, nullptr }

#endif // SST_CORE_ELI_PROFILEPOINTINFO_H
