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

#ifndef SST_CORE_STATS_INFO_H
#define SST_CORE_STATS_INFO_H

#include <vector>
#include <string>
#include "sst/core/eli/elibase.h"

namespace SST {
namespace ELI {

template <class T, class Enable=void>
struct InfoStats {
  static const std::vector<SST::ElementInfoStatistic>& get() {
    static std::vector<SST::ElementInfoStatistic> var = { };
    return var;
  }
};

template <class T>
struct InfoStats<T,
      typename MethodDetect<decltype(T::ELI_getStatistics())>::type> {
  static const std::vector<SST::ElementInfoStatistic>& get() {
    return T::ELI_getStatistics();
  }
};

class ProvidesStats {
 private:
  std::vector<std::string> statnames;
  std::vector<ElementInfoStatistic> stats_;

  void init(){
    for (auto& item : stats_) {
        statnames.push_back(item.name);
    }
  }

 protected:
  template <class T> ProvidesStats(T* UNUSED(t)) :
    stats_(InfoStats<T>::get())
  {
    init();
  }


 public:
  const std::vector<ElementInfoStatistic>& getValidStats() const {
    return stats_;
  }
  const std::vector<std::string>& getStatnames() const { return statnames; }

  void toString(std::ostream& os) const;

  template <class XMLNode> void outputXML(XMLNode* node) const {
    // Build the Element to Represent the Component
    int idx = 0;
    for (const ElementInfoStatistic& stat : stats_){
      // Build the Element to Represent the Parameter
      auto* XMLStatElement = new XMLNode("Statistic");
      XMLStatElement->SetAttribute("Index", idx);
      XMLStatElement->SetAttribute("Name", stat.name);
      XMLStatElement->SetAttribute("Description", stat.description ? stat.description : "none");
      XMLStatElement->SetAttribute("Units", stat.units ? stat.units : "none");
      XMLStatElement->SetAttribute("EnableLevel", stat.enableLevel);
      node->LinkEndChild(XMLStatElement);
      ++idx;
    }
  }
};

}
}

#define SST_ELI_DOCUMENT_STATISTICS(...)                                \
    static const std::vector<SST::ElementInfoStatistic>& ELI_getStatistics() {  \
        static std::vector<SST::ElementInfoStatistic> var = { __VA_ARGS__ } ;  \
        return var; \
    }

#endif

