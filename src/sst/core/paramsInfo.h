#ifndef SST_CORE_PARAMS_INFO_H
#define SST_CORE_PARAMS_INFO_H

#include <vector>
#include <string>
#include <sst/core/params.h>
#include <sst/core/elibase.h>
#include <sst/core/oldELI.h>

namespace SST {
namespace ELI {

template <class T, class Enable=void>
struct GetParams {
  static const std::vector<SST::ElementInfoParam>& get() {
    static std::vector<SST::ElementInfoParam> var = { };
    return var;
  }
};

template <class T>
struct GetParams<T,
      typename MethodDetect<decltype(T::ELI_getParams())>::type> {
  static const std::vector<SST::ElementInfoParam>& get() {
    return T::ELI_getParams();
  }
};

class ProvidesParams {
 public:
   const std::vector<ElementInfoParam>& getValidParams() const {
     return params_;
   }

   void toString(std::ostream& os) const;

   template <class XMLNode> void outputXML(XMLNode* node) const {
     // Build the Element to Represent the Component
     int idx = 0;
     for (const ElementInfoParam& param : params_){;
       // Build the Element to Represent the Parameter
       auto* XMLParameterElement = new XMLNode("Parameter");
       XMLParameterElement->SetAttribute("Index", idx);
       XMLParameterElement->SetAttribute("Name", param.name);
       XMLParameterElement->SetAttribute("Description", param.description);
       if (param.defaultValue) XMLParameterElement->SetAttribute("Default", param.defaultValue);
       node->LinkEndChild(XMLParameterElement);
       ++idx;
     }
   }

   const Params::KeySet_t& getParamNames() const {
     return allowedKeys;
   }

 protected:
  template <class T> ProvidesParams(T* UNUSED(t)) :
   params_(GetParams<T>::get())
  {
   init();
  }

  template <class U> ProvidesParams(OldELITag& UNUSED(tag), U* u)
  {
     auto *p = u->params;
     while (NULL != p && NULL != p->name) {
      params_.emplace_back(*p);
      p++;
     }
     init();
  }

 private:
  void init();

  Params::KeySet_t allowedKeys;
  std::vector<ElementInfoParam> params_;
};

}
}

#define SST_ELI_DOCUMENT_PARAMS(...)                              \
    static const std::vector<SST::ElementInfoParam>& ELI_getParams() { \
        static std::vector<SST::ElementInfoParam> var = { __VA_ARGS__ } ; \
        return var; \
    }

#endif
