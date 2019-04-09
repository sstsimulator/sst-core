#ifndef SST_CORE_PORTS_INFO_H
#define SST_CORE_PORTS_INFO_H

#include <vector>
#include <string>

#include <sst/core/eli/elibase.h>

namespace SST {
namespace ELI {


template <class T, class Enable=void>
struct InfoPorts {
  static const std::vector<SST::ElementInfoPort2>& get() {
    static std::vector<SST::ElementInfoPort2> var = { };
    return var;
  }
};

template <class T>
struct InfoPorts<T,
      typename MethodDetect<decltype(T::ELI_getPorts())>::type> {
  static const std::vector<SST::ElementInfoPort2>& get() {
    return T::ELI_getPorts();
  }
};


class ProvidesPorts {
 public:
  const std::vector<std::string>& getPortnames() { return portnames; }
  const std::vector<ElementInfoPort2>& getValidPorts() const {
    return ports_;
  }

  void toString(std::ostream& os) const;

  template <class XMLNode> void outputXML(XMLNode* node) const {
    // Build the Element to Represent the Component
    int idx = 0;
    for (auto& port : ports_){
      auto* XMLPortElement = new XMLNode("Port");
      XMLPortElement->SetAttribute("Index", idx);
      XMLPortElement->SetAttribute("Name", port.name);
      XMLPortElement->SetAttribute("Description", port.description ? port.description : "none");
      node->LinkEndChild(XMLPortElement);
      ++idx;
    }
  }

 protected:
  template <class T> ProvidesPorts(T* UNUSED(t)) :
    ports_(InfoPorts<T>::get())
  {
    init();
  }

 private:
  void init();

  std::vector<std::string> portnames;
  std::vector<ElementInfoPort2> ports_;

};

}
}

#define SST_ELI_DOCUMENT_PORTS(...)                              \
    static const std::vector<SST::ElementInfoPort2>& ELI_getPorts() { \
        static std::vector<SST::ElementInfoPort2> var = { __VA_ARGS__ } ;      \
        return var; \
    }

#endif

