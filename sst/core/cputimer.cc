
#include <sst_config.h>
#include "cputimer.h"

double sst_get_cpu_time() {
	struct timeval the_time;
	gettimeofday(&the_time, 0);

	return ((double) the_time.tv_sec) + (((double) the_time.tv_usec) * 1.0e-6);
}
