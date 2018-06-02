// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_PROFILE_H
#define SST_CORE_CORE_PROFILE_H

#include <chrono>
#include <sst/core/warnmacros.h>

namespace SST {
namespace Core {
namespace Profile {



#ifdef __SST_ENABLE_PROFILE__

#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 7)
#define CLOCK std::chrono::system_clock
#else
#define CLOCK std::chrono::steady_clock
#endif


typedef CLOCK::time_point ProfData_t;

inline ProfData_t now()
{
    return CLOCK::now();
}

inline double getElapsed(const ProfData_t &begin, const ProfData_t &end)
{
    std::chrono::duration<double> elapsed = (end - begin);
    return elapsed.count();
}


inline double getElapsed(const ProfData_t &since)
{
    return getElapsed(since, now());
}


#else
typedef double ProfData_t;

inline ProfData_t now()
{
    return 0.0;
}

inline double getElapsed(const ProfData_t &UNUSED(begin), const ProfData_t &UNUSED(end))
{
    return 0.0;
}


inline double getElapsed(const ProfData_t &UNUSED(since))
{
    return 0.0;
}



#endif



}
}
}

#endif
