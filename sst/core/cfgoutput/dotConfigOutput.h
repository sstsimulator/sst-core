
#include <sst/core/configGraphOutput.h>

namespace SST {
namespace Core {

class DotConfigGraphOutput : public ConfigGraphOutput {
public:
        DotConfigGraphOutput(const char* path);
	virtual void generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException);
protected:
	void generateDot(const ConfigComponent& comp, const ConfigLinkMap_t& linkMap) const;
	void generateDot(const ConfigLink& link) const;
};

}
}
