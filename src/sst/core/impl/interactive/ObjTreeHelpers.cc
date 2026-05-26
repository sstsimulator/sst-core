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


#include "sst/core/impl/interactive/ObjTreeHelpers.h"

#include "sst/core/impl/interactive/NumericHandle.h"

namespace SST::Core::Serialization {

bool
ObjTreeComparison::evaluateComparison(unsigned index, [[maybe_unused]] SST::Core::Serialization::ObjTreeCont* treeRoot)
{

    // Get the error conditions out of the way
    if ( objectsToCompare.empty() || operators.empty() || (operators.size() < index) ||
         (objectsToCompare.size() < (index * 2)) || (operators[index] == Op::INVALID) ) {
        return false;
    }

    if ( operators[index] == Op::CHANGED ) {
        bool result =
            std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[index * 2])->hasChanged();
        std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[index * 2])->syncFromSim();
        return result;
    }
    else {
        std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[index * 2])->syncFromSim();
        std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[(index * 2) + 1])->syncFromSim();

        auto lhs = SST::Core::Serialization::NumericHandle::from(
            std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[index * 2]).get());
        auto rhs = SST::Core::Serialization::NumericHandle::from(
            std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[(index * 2) + 1]).get());
        switch ( operators[index] ) {
        case Op::LT:
            return lhs < rhs;
        case Op::LTE:
            return lhs <= rhs;
        case Op::GT:
            return lhs > rhs;
        case Op::GTE:
            return lhs >= rhs;
        case Op::EQ:
            return lhs == rhs;
        case Op::NEQ:
            return lhs != rhs;
        default:
            return false;
        }
    }
}


}; // namespace SST::Core::Serialization
