
#ifndef _H_SST_CORE_CONFIG_OUTPUT_PYTHON
#define _H_SST_CORE_CONFIG_OUTPUT_PYTHON

#include <sst/core/configGraph.h>
#include <sst/core/configGraphOutput.h>

namespace SST {
namespace Core {

class PythonConfigGraphOutput : public ConfigGraphOutput {
public:
        PythonConfigGraphOutput(const char* path);

        virtual void generate(const Config* cfg,
		ConfigGraph* graph) throw(ConfigGraphOutputException);

protected:
	char* makePythonSafeWithPrefix(const std::string name, const std::string prefix) const;
	void makeBufferPythonSafe(char* buffer) const;
	char* makeEscapeSafe(const std::string input) const;
	bool strncmp(const char* a, const char* b, const size_t n) const;
	bool isMultiLine(const char* check) const;
	bool isMultiLine(const std::string check) const;
};

}
}

#endif
