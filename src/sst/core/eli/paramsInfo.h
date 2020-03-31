// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_PARAMS_INFO_H
#define SST_CORE_PARAMS_INFO_H

#include <vector>
#include <string>
#include "sst/core/params.h"
#include "sst/core/eli/elibase.h"

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
       XMLParameterElement->SetAttribute("Description", param.description ? param.description : "none");
       XMLParameterElement->SetAttribute("Default", param.defaultValue ? param.defaultValue : "none");
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
