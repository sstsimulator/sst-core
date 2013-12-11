
#ifndef _H_SST_CORE_RNG_DISTRIB
#define _H_SST_CORE_RNG_DISTRIB


namespace SST {
namespace RNG {

class SSTRandomDistribution {

	public:
		virtual double getNextDouble() = 0;

};

}
}

#endif
