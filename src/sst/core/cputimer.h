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

#ifndef SST_CORE_CPU_TIMER_H
#define SST_CORE_CPU_TIMER_H

#include <cstdint>
#include <string>
#include <sys/time.h>

/**
 * @return Current CPU time in seconds using the time of day. Timezone information is not filled.
 */
double sst_get_cpu_time();

/**
 * @return Current CPU time in us using the time of day. Timezone information is not filled.
 */
uint64_t sst_get_cpu_time_usec();

/**
   Returns a string with the current time in HH:MM:SS format
*/
std::string sst_get_current_time();

#endif // SST_CORE_CPU_TIMER_H
