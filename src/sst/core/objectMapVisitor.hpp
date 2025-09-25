// Copyright (c) 2025 Tactical Computing Labs, LLC.
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//
#pragma once

#ifndef __OBJECTMAPVISITOR_HPP__
#define __OBJECTMAPVISITOR_HPP__

#include <string>
#include <vector>

#include "sst_config.h"
#include "sst/core/baseComponent.h"
#include "sst/core/stringize.h"

namespace SST { namespace Core {

struct ObjectMapVisitor {

    ObjectMapVisitor() = default;

    template<typename T>
    void operator()(T * inst) {
    }

    template<>
    void operator()(SST::Core::Serialization::ObjectMapDeferred<BaseComponent> * inst) {
std::cout << "HERE1" << std::endl;
      if(inst == nullptr) {
         return;
      }

std::cout << "HERE2" << std::endl;
      if ( inst->isFundamental() ) {
          return;
      }

std::cout << "HERE3" << std::endl;
      std::multimap<std::string, SST::Core::Serialization::ObjectMap*> const& vars = inst->getVariables();

std::cout << "HERE4" << std::endl;
      if(vars.size() < 1) { return; } 

      for ( auto const& x : vars ) {
std::cout << x.first << std::endl;
            (*this)(x.second);
      }

    }

    template<>
    void operator()(Core::Serialization::ObjectMap * inst) {
      if(inst == nullptr) {
         return;
      }

      if ( inst->isFundamental() ) {
          return;
      }

std::cout << inst->getName() << std::endl;

      auto & vars = inst->getVariables();

      if(vars.size() < 1) { return; } 

      for ( auto & x : vars ) {
std::cout << x.first << std::endl;
            (*this)(x.second);
      }
   }

};

} /* end namespace Core */ } // end namespace SST

#endif
