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

#ifndef SST_CORE_ELI_STATS_INFO_H
#define SST_CORE_ELI_STATS_INFO_H

#include "sst/core/eli/elibase.h"

#include <string>
#include <type_traits>
#include <vector>

namespace SST::ELI {

template <typename, typename = void>
struct InfoStats
{
    static const std::vector<SST::ElementInfoStatistic>& get()
    {
        static std::vector<SST::ElementInfoStatistic> var = {};
        return var;
    }
};

template <typename T>
struct InfoStats<T, std::void_t<decltype(T::ELI_getStatistics())>>
{
    static const std::vector<SST::ElementInfoStatistic>& get() { return T::ELI_getStatistics(); }
};

class ProvidesStats
{
private:
    std::vector<ElementInfoStatistic> stats_;

protected:
    template <class T>
    explicit ProvidesStats(T* UNUSED(t)) :
        stats_(InfoStats<T>::get())
    {}

public:
    const std::vector<ElementInfoStatistic>& getValidStats() const { return stats_; }

    void toString(std::ostream& os) const;

    template <class XMLNode>
    void outputXML(XMLNode* node) const
    {
        // Build the Element to Represent the Component
        int idx = 0;
        for ( const auto& stat : stats_ ) {
            // Build the Element to Represent the Parameter
            auto* XMLStatElement = new XMLNode("Statistic");
            XMLStatElement->SetAttribute("Index", idx);
            XMLStatElement->SetAttribute("Name", stat.name);
            XMLStatElement->SetAttribute("Description", stat.description ? stat.description : "none");
            XMLStatElement->SetAttribute("Units", stat.units ? stat.units : "none");
            XMLStatElement->SetAttribute("EnableLevel", stat.enable_level);
            node->LinkEndChild(XMLStatElement);
            ++idx;
        }
    }
};

} // namespace SST::ELI

// clang-format off
#define SST_ELI_DOCUMENT_STATISTICS(...)                                                                    \
    static const std::vector<SST::ElementInfoStatistic>& ELI_getStatistics()                                \
    {                                                                                                       \
        static std::vector<SST::ElementInfoStatistic> var    = { __VA_ARGS__ };                             \
        auto parent = SST::ELI::InfoStats<                                                                  \
         std::conditional_t<(__EliDerivedLevel > __EliBaseLevel), __LocalEliBase, __ParentEliBase>>::get(); \
        SST::ELI::combineEliInfo(var, parent);                                                              \
        return var;                                                                                         \
    }
// clang-format on

#define SST_ELI_DELETE_STAT(stat) { stat, nullptr, nullptr, 0 }

#endif
