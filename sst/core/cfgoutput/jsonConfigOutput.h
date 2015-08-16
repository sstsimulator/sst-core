
#ifndef _SST_CORE_CONFIG_OUTPUT_JSON
#define _SST_CORE_CONFIG_OUTPUT_JSON

#include <sst/core/configGraph.h>
#include <sst/core/configGraphOutput.h>

namespace SST {
namespace Core {

class JSONConfigGraphOutput : public ConfigGraphOutput {
public:
        JSONConfigGraphOutput(const char* path);
	virtual void generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException);
protected:
	void generateJSON(const std::string indent, const ConfigComponent& comp,
                const ConfigLinkMap_t& linkMap) const;
};

}
}

#endif
