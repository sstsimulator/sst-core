
#ifndef _SST_CORE_CONFIG_OUTPUT_XML
#define _SST_CORE_CONFIG_OUTPUT_XML

#include <sst/core/configGraph.h>
#include <sst/core/configGraphOutput.h>

namespace SST {
namespace Core {

class XMLConfigGraphOutput : public ConfigGraphOutput {
public:
        XMLConfigGraphOutput(const char* path);
	virtual void generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException);
protected:
        void generateXML(const std::string indent, const ConfigComponent& comp,
		const ConfigLinkMap_t& linkMap) const;
        void generateXML(const std::string indent, const ConfigLink& link,
		const ConfigComponentMap_t& compMap) const;
};

}
}

#endif
