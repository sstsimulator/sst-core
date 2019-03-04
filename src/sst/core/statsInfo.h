#ifndef SST_CORE_STATS_INFO_H
#define SST_CORE_STATS_INFO_H

#include <vector>
#include <string>
#include <sst/core/elibase.h>
#include <sst/core/oldELI.h>

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

  template <class U> ProvidesStats(OldELITag& UNUSED(tag), U* u)
  {
    auto *s = u->stats;
    while ( NULL != s && NULL != s->name ) {
        stats_.emplace_back(*s);
        s++;
    }
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
      XMLStatElement->SetAttribute("Description", stat.description);
      if (stat.units) XMLStatElement->SetAttribute("Units", stat.units);
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

