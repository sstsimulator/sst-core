// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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

    /** @return Filter map */
    FilterMap_t& getFilterMap() { return m_filters; }

    /** @return Bit field of various command line options enabled. */
    unsigned int getOptionBits() { return m_optionBits; }

    /** @return User defined path the XML File. */
    std::string& getXMLFilePath() { return m_XMLFilePath; }

    /** @return True if the debugging output is enabled, otherwise False */
    bool debugEnabled() const { return m_debugEnabled; }
    /** @return True if the m_filter multimap is emtpy, otherwise False */
    bool processAllElements() const { return m_filters.empty(); }
    /** @return True if command line options are enabled and verbose configuration is valid, otherwise False */
    bool doVerbose() const { return m_optionBits & CFG_VERBOSE; }
    /** @return True if interactive is enabled, otherwise False */
    bool interactiveEnabled() const { return m_interactive; }
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

    int setInteractive(const std::string& UNUSED(arg))
    {
        m_interactive = true;
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
    bool                     m_interactive;
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

    // Contains info strings for each individual component, subcomponent, etc.
    struct ComponentInfo
    {
        std::string                        componentName;
        std::vector<std::string>           stringIndexer; // Used to maintain order of strings in infoMap
        std::map<std::string, std::string> infoMap;
    };

    /** Return the map of all component info for the Library. */
    std::map<std::string, std::vector<ComponentInfo>> getComponentInfo() { return m_components; }

    /** Store all Library Information. */
    void setAllLibraryInfo();

    /** Output the Library Information.
     * @param LibIndex The Index of the Library.
     */
    void outputHumanReadable(std::ostream& os, int LibIndex);

    /** Create the formatted XML data of the Library.
     * @param Index The Index of the Library.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement);

    /** Put text into info map*/
    void setLibraryInfo(std::string baseName, std::string componentName, std::string info);

    /** Return text from info map based on filters */
    void outputText(std::stringstream& os);

    /** Set filters based on search term */
    void filterSearch(std::stringstream& outputStream, std::string tag, std::string searchTerm);

    /** Clear highlight characters from info strings */
    void clearHighlights();

    /** Filter output from info map
     * @return True if the library filter is defined, otherwise False
     */
    bool getFilter() { return m_libraryFilter; }

    /**
     * Clears the component filter and sets the internal library filter status
     * @param libFilter
     */
    void resetFilters(bool libFilter) { m_libraryFilter = libFilter, m_componentFilters.clear(); }

    /**
     * Sets the internal library filter status
     * @param libFilter
     */
    void setLibraryFilter(bool filter) { m_libraryFilter = filter; }

    /**
     * Adds the component filter string to the end of the internal vector of components
     * @param component
     */
    int setComponentFilter(std::string component)
    {
        for ( auto& pair : m_components ) {
            for ( auto& comp : pair.second ) {
                if ( comp.componentName == component ) {
                    m_componentFilters.push_back(component);
                    return 0;
                }
            }
        }
        return 1;
    }

    template <class BaseType>
    void setAllLibraryInfo();
    template <class BaseType>
    void outputHumanReadable(std::ostream& os, bool printAll);
    template <class BaseType>
    void outputXML(TiXmlElement* node);

    std::string getLibraryDescription() { return ""; }

private:
    // Stores all component info, keyed by their "BaseTypes" (component, subcomponent, module, etc.)
    std::map<std::string, std::vector<ComponentInfo>> m_components;
    std::vector<std::string>                          m_componentNames;
    bool                                              m_libraryFilter = false;
    std::vector<std::string>                          m_componentFilters;
    std::string                                       m_name;
};

#ifdef HAVE_CURSES
#include <ncurses.h>
/**
 * Handles all ncurses window operations for interactive SSTInfo.
 */
class InteractiveWindow
{
public:
    InteractiveWindow()
    {
        info    = newwin(LINES - 3, COLS, 0, 0);
        console = newwin(3, COLS, LINES - 3, 0);
    }

    /* Draw/redraw info and console windows */
    void draw(bool drawConsole = true);

    /* Toggle display of the autofill box */
    void toggleAutofillBox();

    /* Start up the interactive window */
    void start();

    /* Main loop for user input */
    void getInput();

    /* Prints SST-info text to the info window */
    void printInfo();

    void printConsole(const char* input)
    {
        DISABLE_WARN_FORMAT_SECURITY
        wprintw(console, input);
        REENABLE_WARNING
    }
    void resetCursor(int pos) { wmove(console, 1, pos); }
    int  getCursorPos() { return getcurx(console); }

private:
    WINDOW* info;
    WINDOW* console;
    WINDOW* autofillBox;
    bool    autofillEnabled;
};
#endif

} // namespace SST


#endif // SST_CORE_SST_INFO_H
