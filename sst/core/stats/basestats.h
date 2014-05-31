
#ifndef _H_SST_CORE_BASE_STATISTICS
#define _H_SST_CORE_BASE_STATISTICS

#include <string>
#include <cstdio>

namespace SST {
namespace Statistics {

class BaseStatistic {

	public:
		BaseStatistic(std::string statName) {
				name = (char*) malloc(sizeof(char) * (statName.size() + 1));
				sprintf(name, "%s", statName.c_str());
			 }
		BaseStatistic(char* statName) { name = statName; }

		const char* getName() {
			return name;
		}

	protected:
		char* name;

};

}
}

#endif
