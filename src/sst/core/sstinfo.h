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

#ifndef SST_CORE_SST_INFO_H
#define SST_CORE_SST_INFO_H

#include "sst/core/configShared.h"
#include "sst/core/eli/elementinfo.h"

#include <map>
#include <set>
#include <vector>

#include "tinyxml/tinyxml.h"

class TiXmlNode;

namespace SST {

// CONFIGURATION BITS
#define CFG_OUTPUTHUMAN 0x00000001
#define CFG_OUTPUTXML   0x00000002
#define CFG_VERBOSE     0x00000004

/**
 * The SSTInfo Configuration class.
 *
 * This class will parse the command line, and setup internal
 * lists of elements and components to be processed.
 */
class SSTInfoConfig : public ConfigShared
{
public:
    typedef std::multimap<std::string, std::string> FilterMap_t;
    /** Create a new SSTInfo configuration and parse the Command Line. */
    SSTInfoConfig(bool suppress_print);
    ~SSTInfoConfig();

    /** Return the list of elements to be processed. */
    std::set<std::string> getElementsToProcessArray()
    {
        std::set<std::string> res;
        for ( auto& i : m_filters )
            res.insert(i.first);
        return res;
    }

    /** Clears the current filter map */
    void clearFilterMap() { m_filters.clear(); }

    /** Return the filter map */
    FilterMap_t& getFilterMap() { return m_filters; }

    /** Return the bit field of various command line options enabled. */
    unsigned int getOptionBits() { return m_optionBits; }

    /** Return the user defined path the XML File. */
    std::string& getXMLFilePath() { return m_XMLFilePath; }

    /** Is debugging output enabled? */
    bool debugEnabled() const { return m_debugEnabled; }
    bool processAllElements() const { return m_filters.empty(); }
    bool doVerbose() const { return m_optionBits & CFG_VERBOSE; }
    void addFilter(const std::string& name);

protected:
    std::string getUsagePrelude() override;

private:
    void outputUsage();

    int setPositionalArg(int UNUSED(num), const std::string& arg)
    {
        addFilter(arg);
        return 0;
    }

    // Functions to set options
    int printHelp(const std::string& UNUSED(arg))
    {
        printUsage();
        return 1;
    }

    int outputVersion(const std::string& UNUSED(arg))
    {
        fprintf(stderr, "SST Release Version %s\n", PACKAGE_VERSION);
        return 1;
    }

    int setEnableDebug(const std::string& UNUSED(arg))
    {
        m_debugEnabled = true;
        return 0;
    }

    int setNoDisplay(const std::string& UNUSED(arg))
    {
        m_optionBits &= ~CFG_OUTPUTHUMAN;
        return 0;
    }

    int setXML(const std::string& UNUSED(arg))
    {
        m_optionBits |= CFG_OUTPUTXML;
        return 0;
    }

    int setXMLOutput(const std::string& UNUSED(arg))
    {
        m_XMLFilePath = arg;
        return 0;
    }

    int setLibs(const std::string& UNUSED(arg))
    {
        addFilter(arg);
        return 0;
    }

    int setQuiet(const std::string& UNUSED(arg))
    {
        m_optionBits &= ~CFG_VERBOSE;
        return 0;
    }

private:
    char*                    m_AppName;
    std::vector<std::string> m_elementsToProcess;
    unsigned int             m_optionBits;
    std::string              m_XMLFilePath;
    bool                     m_debugEnabled;
    FilterMap_t              m_filters;
};

/**
 * The SSTInfo representation of ElementLibraryInfo object.
 *
 * This class is used internally by SSTInfo to load and process
 * ElementLibraryInfo objects.
 */
class SSTLibraryInfo
{

public:
    /** Create a new SSTInfoElement_LibraryInfo object.
     * @param eli Pointer to an ElementLibraryInfo object.
     */
    SSTLibraryInfo(const std::string& name) : m_name(name) {}

    /** Return the Name of the Library. */
    // std::string getLibraryName() {if (m_eli && m_eli->name) return m_eli->name; else return ""; }
    std::string getLibraryName() { return m_name; }

    /** Output the Library Information.
     * @param LibIndex The Index of the Library.
     */
    void outputHumanReadable(std::ostream& os, int LibIndex);

    /** Create the formatted XML data of the Library.
     * @param LibIndex The Index of the Library.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement);

    template <class BaseType>
    void outputHumanReadable(std::ostream& os, bool printAll);
    template <class BaseType>
    void outputXML(TiXmlElement* node);

    std::string getLibraryDescription() { return ""; }

    std::set<std::string> getTypeList() { return typeList; }

private:
    std::string m_name;
    std::set<std::string> typeList;
};

} // namespace SST

#endif // SST_CORE_SST_INFO_H
