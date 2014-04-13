
#ifndef _H_SST_CORE_MEMUSE
#define _H_SST_CORE_MEMUSE

#include "sst_config.h"

#include <sys/resource.h>
#include <boost/mpi.hpp>

namespace SST {
namespace Core {

uint64_t maxLocalMemSize();
uint64_t maxGlobalMemSize();
uint64_t maxLocalPageFaults();
uint64_t globalPageFaults();

}
}


#endif
