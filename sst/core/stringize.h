
#ifndef _H_SST_CORE_STRINGIZE
#define _H_SST_CORE_STRINGIZE

#include <sst_config.h>
#include <string>

namespace SST {

static std::string to_string(int val) {
#ifdef HAVE_STDCXX_0X
	return std::to_string(val);
#else
	char* buffer[32];
	sprintf(buffer, "%d", val);

	std::string buffer_str(buffer);
	return buffer_str;
#endif
};

static std::string to_string(long val) {
#ifdef HAVE_STDCXX_0X
	return std::to_string(val);
#else
        char* buffer[32];
        sprintf(buffer, "%l", val);

        std::string buffer_str(buffer);
        return buffer_str;
#endif
};

static std::string to_string(double val) {
#ifdef HAVE_STDCXX_0X
	return std::to_string(val);
#else
	char* buffer[32];
        sprintf(buffer, "%f", val);

        std::string buffer_str(buffer);
        return buffer_str;
#endif
};

static std::string to_string(float val) {
#ifdef HAVE_STDCXX_0X
	return std::to_string(val);
#else
        char* buffer[32];
        sprintf(buffer, "%f", val);

        std::string buffer_str(buffer);
        return buffer_str;
#endif
};

static std::string to_string(unsigned int val) {
#ifdef HAVE_STDCXX_0X
	return std::to_string(val);
#else
        char* buffer[32];
        sprintf(buffer, "%u", val);

        std::string buffer_str(buffer);
        return buffer_str;
#endif
};

static std::string to_string(unsigned long val) {
#ifdef HAVE_STDCXX_0X
	return std::to_string(val);
#else
        char* buffer[32];
        sprintf(buffer, "%lu", val);

        std::string buffer_str(buffer);
        return buffer_str;
#endif
};

static std::string to_string(uint32_t val) {
#ifdef HAVE_STDCXX_0X
	return std::to_string(val);
#else
        char* buffer[32];
        sprintf(buffer, "%" PRIu32, val);

        std::string buffer_str(buffer);
        return buffer_str;
#endif
};

static std::string to_string(uint64_t val) {
#ifdef HAVE_STDCXX_0X
	return std::to_string(val);
#else
        char* buffer[32];
        sprintf(buffer, "%" PRIu64, val);

        std::string buffer_str(buffer);
        return buffer_str;
#endif
};

}

#define
