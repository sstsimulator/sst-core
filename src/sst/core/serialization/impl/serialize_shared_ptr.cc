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

#include "sst_config.h"

#include "sst/core/serialization/serialize.h"

#include <cstddef>
#include <memory>

namespace SST::Core::Serialization {

// Instantiate some templates to make sure all combinations are compilable

template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int, int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int, int[], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int, int[], size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int, size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int[], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int[], int[], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int[], int[], size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int[], size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int, int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int, int[], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int, int[], size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int, size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int[], int[], int, std::shared_ptr<int[]>>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int[], int[], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int[], int[], size_t, std::shared_ptr<int[]>>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int[], int[], size_t>>;

template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int, int[10], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int, int[10], size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int[10], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int[10], int[10], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int[10], int[10], size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::shared_ptr, int[10], size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int, int[10], int>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int, int[10], size_t>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int[10], int[10], int, std::shared_ptr<int[10]>>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int[10], int[10], int>>;
template class serialize_impl<
    pvt::shared_ptr_wrapper<std::weak_ptr, int[10], int[10], size_t, std::shared_ptr<int[10]>>>;
template class serialize_impl<pvt::shared_ptr_wrapper<std::weak_ptr, int[10], int[10], size_t>>;

} // namespace SST::Core::Serialization
