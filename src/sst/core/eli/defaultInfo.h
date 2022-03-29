// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_ELI_DEFAULTINFO_H
#define SST_CORE_ELI_DEFAULTINFO_H

#include <string>
#include <vector>

namespace SST {
namespace ELI {

class ProvidesDefaultInfo
{
    friend class ModuleDocOldEli;

public:
    const std::string&      getLibrary() const { return lib_; }
    const std::string&      getDescription() const { return desc_; }
    const std::string&      getName() const { return name_; }
    const std::vector<int>& getVersion() const { return version_; }
    const std::string&      getCompileFile() const { return file_; }
    const std::string&      getCompileDate() const { return date_; }
    const std::vector<int>& getELICompiledVersion() const;

    std::string getELIVersionString() const;

    void toString(std::ostream& os) const;

    template <class XMLNode>
    void outputXML(XMLNode* node) const
    {
        node->SetAttribute("Name", getName().c_str());
        node->SetAttribute("Description", getDescription().c_str());
    }

    template <class T>
    ProvidesDefaultInfo(const std::string& lib, const std::string& name, T* UNUSED(t)) :
        lib_(lib),
        name_(name),
        desc_(T::ELI_getDescription()),
        version_(T::ELI_getVersion()),
        file_(T::ELI_getCompileFile()),
        date_(T::ELI_getCompileDate())
    {}

protected:
    template <class T>
    ProvidesDefaultInfo(T* t) : ProvidesDefaultInfo(T::ELI_getLibrary(), T::ELI_getName(), t)
    {}

private:
    std::string      lib_;
    std::string      name_;
    std::string      desc_;
    std::vector<int> version_;
    std::string      file_;
    std::string      date_;
    std::vector<int> compiled_;
};

} // namespace ELI
} // namespace SST

#define SST_ELI_INSERT_COMPILE_INFO()                     \
    static const std::string& ELI_getCompileDate()        \
    {                                                     \
        static std::string time      = __TIME__;          \
        static std::string date      = __DATE__;          \
        static std::string date_time = date + " " + time; \
        return date_time;                                 \
    }                                                     \
    static const std::string ELI_getCompileFile() { return __FILE__; }

#define SST_ELI_DEFAULT_INFO(lib, name, version, desc)                                                              \
    SST_ELI_INSERT_COMPILE_INFO()                                                                                   \
    static constexpr unsigned      majorVersion() { return SST::SST_ELI_getMajorNumberFromVersion(version); }       \
    static constexpr unsigned      minorVersion() { return SST::SST_ELI_getMinorNumberFromVersion(version); }       \
    static constexpr unsigned      tertiaryVersion() { return SST::SST_ELI_getTertiaryNumberFromVersion(version); } \
    static const std::vector<int>& ELI_getVersion()                                                                 \
    {                                                                                                               \
        static std::vector<int> var = version;                                                                      \
        return var;                                                                                                 \
    }                                                                                                               \
    static const char* ELI_getLibrary() { return lib; }                                                             \
    static const char* ELI_getName() { return name; }                                                               \
    static const char* ELI_getDescription() { return desc; }

#define SST_ELI_ELEMENT_VERSION(...) \
    {                                \
        __VA_ARGS__                  \
    }

#endif // SST_CORE_ELI_DEFAULTINFO_H
