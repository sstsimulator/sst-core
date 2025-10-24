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

#ifndef SST_CORE_CHECKPOINTABLE_INFO_H
#define SST_CORE_CHECKPOINTABLE_INFO_H

#include "sst/core/eli/elibase.h"
#include "sst/core/warnmacros.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace SST::ELI {

template <typename, typename = void>
struct GetCheckpointable
{
    static bool get() { return false; }
};

template <class T>
struct GetCheckpointable<T, std::void_t<decltype(T::ELI_isCheckpointable())>>
{
    static bool get() { return T::ELI_isCheckpointable(); }
};


class ProvidesCheckpointable
{
public:
    bool isCheckpointable() const { return checkpointable_; }

    void toString(std::ostream& os) const
    {
        os << "      Checkpointable: " << (checkpointable_ ? "true" : "false") << "\n";
    }

    template <class XMLNode>
    void outputXML(XMLNode* UNUSED(node))
    {
        node->SetAttribute("Checkpointable", checkpointable_);
    }

protected:
    template <class T>
    explicit ProvidesCheckpointable(T* UNUSED(t)) :
        checkpointable_(GetCheckpointable<T>::get())
    {}

private:
    bool checkpointable_;
};

#define SST_ELI_IS_CHECKPOINTABLE()    \
    static bool ELI_isCheckpointable() \
    {                                  \
        return true;                   \
    }

} // namespace SST::ELI

#endif
