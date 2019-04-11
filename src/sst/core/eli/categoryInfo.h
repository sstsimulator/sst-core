#ifndef SST_CORE_CATEGORY_INFO_H
#define SST_CORE_CATEGORY_INFO_H

#include <vector>
#include <string>

namespace SST {
namespace ELI {

class ProvidesCategory {
 public:
  uint32_t category() const {
    return cat_;
  }

  static const char* categoryName(int cat){
    switch(cat){
    case COMPONENT_CATEGORY_PROCESSOR:
      return "PROCESSOR COMPONENT";
    case COMPONENT_CATEGORY_MEMORY:
      return "MEMORY COMPONENT";
    case COMPONENT_CATEGORY_NETWORK:
      return "NETWORK COMPONENT";
    case COMPONENT_CATEGORY_SYSTEM:
      return "SYSTEM COMPONENT";
    default:
      return "UNCATEGORIZED COMPONENT";
    }
  }

  void toString(std::ostream& UNUSED(os)) const {
    os << "CATEGORY: " << categoryName(cat_) << "\n";
  }

  template <class XMLNode> void outputXML(XMLNode* UNUSED(node)){
  }

 protected:
  template <class T> ProvidesCategory(T* UNUSED(t)) :
    cat_(T::ELI_getCategory())
  {
  }

 private:
  uint32_t cat_;
};

#define SST_ELI_CATEGORY_INFO(cat) \
  static uint32_t ELI_getCategory() {  \
    return cat; \
  }

}
}

#endif


