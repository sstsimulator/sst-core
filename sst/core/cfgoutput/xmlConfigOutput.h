
#include <sst/core/configGraphOutput.h>

namespace SST {
namespace Core {

class XMLConfigGraphOutput : public ConfigGraphOutput {
public:
        XMLConfigGraphOutput(const char* path);
	virtual void generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException);
};

}
}
