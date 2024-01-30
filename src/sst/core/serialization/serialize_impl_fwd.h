// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_IMPL_FWD_H
#define SST_CORE_SERIALIZATION_SERIALIZE_IMPL_FWD_H

// This file is used to forward declare the serialize_impl template
// when needed to implement a specific version for a class.

// Will also need serializer class for the function definition in
// serialize_impl
#include "sst/core/serialization/serializer_fwd.h"

namespace SST {
namespace Core {
namespace Serialization {

template <class T, class Enable>
class serialize_impl;


} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_SERIALIZE_IMPL_FWD_H
