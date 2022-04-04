// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CATEGORY_INFO_H
#define SST_CORE_CATEGORY_INFO_H

#include "sst/core/eli/elibase.h"
#include "sst/core/warnmacros.h"

#include <iostream>
#include <string>
#include <vector>

namespace SST {
namespace ELI {

class ProvidesCategory
{
public:
    uint32_t category() const { return cat_; }

    static const char* categoryName(int cat)
    {
        switch ( cat ) {
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

    void toString(std::ostream& UNUSED(os)) const { os << "      CATEGORY: " << categoryName(cat_) << "\n"; }

    template <class XMLNode>
    void outputXML(XMLNode* UNUSED(node))
    {}

protected:
    template <class T>
    ProvidesCategory(T* UNUSED(t)) : cat_(T::ELI_getCategory())
    {}

private:
    uint32_t cat_;
};

#define SST_ELI_CATEGORY_INFO(cat) \
    static uint32_t ELI_getCategory() { return cat; }

} // namespace ELI
} // namespace SST

#endif
