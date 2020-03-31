// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_INFO_H
#define SST_INFO_H

#include <map>
#include <vector>
#include <set>

#include "sst/core/eli/elementinfo.h"
#include "sst/core/tinyxml/tinyxml.h"

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
class SSTInfoConfig {
public:
    typedef std::multimap<std::string, std::string> FilterMap_t;
    /** Create a new SSTInfo configuration and parse the Command Line. */
    SSTInfoConfig();
    ~SSTInfoConfig();

    /** Parse the Command Line.
     * @param argc The number of arguments passed to the application
     * @param argv The array of arguments
     */
    int parseCmdLine(int argc, char* argv[]);

    /** Return the list of elements to be processed. */
    std::set<std::string> getElementsToProcessArray()
    {
        std::set<std::string> res;
        for ( auto &i : m_filters )
            res.insert(i.first);
        return res;
    }

    /** Return the filter map */
    FilterMap_t& getFilterMap() { return m_filters; }

    /** Return the bit field of various command line options enabled. */
    unsigned int              getOptionBits() {return m_optionBits;}

    /** Return the user defined path the XML File. */
    std::string&              getXMLFilePath() {return m_XMLFilePath;}

    /** Is debugging output enabled? */
    bool                      debugEnabled() const { return m_debugEnabled; }
    bool                      processAllElements() const { return m_filters.empty(); }
    bool                      doVerbose() const { return m_optionBits & CFG_VERBOSE; }

private:
    void outputUsage();
    void outputVersion();
    void addFilter(const std::string& name);

private:
    char*                     m_AppName;
    std::vector<std::string>  m_elementsToProcess;
    unsigned int              m_optionBits;
    std::string               m_XMLFilePath;
    bool                      m_debugEnabled;
    FilterMap_t               m_filters;
};

/**
 * The SSTInfo representation of ElementLibraryInfo object.
 *
 * This class is used internally by SSTInfo to load and process
 * ElementLibraryInfo objects.
 */
class SSTLibraryInfo {

public:
    /** Create a new SSTInfoElement_LibraryInfo object.
     * @param eli Pointer to an ElementLibraryInfo object.
     */
    SSTLibraryInfo(const std::string& name) :
        m_name(name)
    {
    }

    /** Return the Name of the Library. */
    // std::string getLibraryName() {if (m_eli && m_eli->name) return m_eli->name; else return ""; }
    std::string getLibraryName() {return m_name; }

    /** Output the Library Information.
     * @param LibIndex The Index of the Library.
     */
    void outputHumanReadable(std::ostream& os, int LibIndex);

    /** Create the formatted XML data of the Library.
     * @param LibIndex The Index of the Library.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement);

    template <class BaseType> void outputHumanReadable(std::ostream& os, bool printAll);
    template <class BaseType> void outputXML(TiXmlElement* node);

    std::string getLibraryDescription() {
      return "";
    }

 private:
    std::string m_name;

};


} // namespace SST

#endif  // SST_INFO_H
