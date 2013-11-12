
#ifndef _H_SST_CORE_RNG_DISTRIB
#define _H_SST_CORE_RNG_DISTRIB

#include <sst_config.h>

namespace SST {
namespace RNG {

class RandomDistribution {

	public:
		virtual double getNextDouble() = 0;

};

}
}

#endif
