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

#ifndef SST_CORE_COMPLEX_H
#define SST_CORE_COMPLEX_H

#include <cfloat>
#include <complex>

namespace SST::Core {

// Traits for getting information about complex types
template <class T>
struct complex_properties
{
    static constexpr bool is_complex = false;
    using real_t                     = void;
};

template <class T>
struct complex_properties<std::complex<T>>
{
    static constexpr bool is_complex = true;
    using real_t                     = T;
};

#ifndef __STDC_NO_COMPLEX__

template <>
struct complex_properties<float _Complex>
{
    static constexpr bool is_complex = true;
    using real_t                     = float;
};

template <>
struct complex_properties<double _Complex>
{
    static constexpr bool is_complex = true;
    using real_t                     = double;
};

template <>
struct complex_properties<long double _Complex>
{

    static constexpr bool is_complex = true;
    using real_t                     = long double;
};

#endif // __STDC_NO_COMPLEX__

} // namespace SST::Core

#endif // SST_CORE_COMPLEX_H
