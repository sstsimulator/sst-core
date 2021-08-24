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

#ifndef SST_CORE_SERIALIZATION_SER_PTR_TYPE_H
#define SST_CORE_SERIALIZATION_SER_PTR_TYPE_H

#include <sprockit/ptr_type.h>
#include <sprockit/serializable.h>

namespace sprockit {

class serializable_ptr_type : public ptr_type, public serializable
{
public:
    typedef sprockit::refcount_ptr<serializable_ptr_type> ptr;
};

} // namespace sprockit

#endif // SST_CORE_SERIALIZATION_SER_PTR_TYPE_H
