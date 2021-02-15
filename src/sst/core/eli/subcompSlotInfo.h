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

#ifndef SST_CORE_SUBCOMPSLOTINFO_H
#define SST_CORE_SUBCOMPSLOTINFO_H

#include <string>
#include <vector>

#include "sst/core/eli/elibase.h"

namespace SST {
namespace ELI {

template <class T, class Enable=void>
struct InfoSubs {
  static const std::vector<SST::ElementInfoSubComponentSlot>& get() {
    static std::vector<SST::ElementInfoSubComponentSlot> var = { };
    return var;
  }
};

template <class T>
struct InfoSubs<T,
      typename MethodDetect<decltype(T::ELI_getSubComponentSlots())>::type> {
  static const std::vector<SST::ElementInfoSubComponentSlot>& get() {
    return T::ELI_getSubComponentSlots();
  }
};

class ProvidesSubComponentSlots {
 public:
  const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() const {
    return slots_;
  }

  void toString(std::ostream& os) const;

  template <class XMLNode> void outputXML(XMLNode* node) const {
    int idx = 0;
    for (const ElementInfoSubComponentSlot& slot : slots_){
      auto* element = new XMLNode("SubComponentSlot");
      element->SetAttribute("Index", idx);
      element->SetAttribute("Name", slot.name);
      element->SetAttribute("Description", slot.description ? slot.description : "none");
      element->SetAttribute("Interface", slot.superclass ? slot.superclass : "none");
      node->LinkEndChild(element);
      ++idx;
    }
  }

 protected:
  template <class T> ProvidesSubComponentSlots(T* UNUSED(t)) :
    slots_(InfoSubs<T>::get())
  {
  }

 private:
  std::vector<ElementInfoSubComponentSlot> slots_;
};

}
}

#define SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(...)                              \
    static const std::vector<SST::ElementInfoSubComponentSlot>& ELI_getSubComponentSlots() { \
    static std::vector<SST::ElementInfoSubComponentSlot> var = { __VA_ARGS__ } ; \
        return var; \
    }

#endif

