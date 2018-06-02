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


#include <sst_config.h>
#include "cputimer.h"

double sst_get_cpu_time() {
	struct timeval the_time;
	gettimeofday(&the_time, 0);

	return ((double) the_time.tv_sec) + (((double) the_time.tv_usec) * 1.0e-6);
}
