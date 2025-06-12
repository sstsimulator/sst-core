// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SST_INFO_H
#define SST_CORE_SST_INFO_H

#include "sst/core/configShared.h"
#include "sst/core/eli/elementinfo.h"

#include <cstdio>
#include <map>
#include <set>
#include <string>
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
    using FilterMap_t = std::multimap<std::string, std::string>;
    /** Create a new SSTInfo configuration and parse the Command Line. */
    explicit SSTInfoConfig(bool suppress_print);
    ~SSTInfoConfig() override = default;

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


    /** @return True if the m_filter multimap is emtpy, otherwise False */
    bool processAllElements() const { return m_filters.empty(); }
    /** @return True if command line options are enabled and verbose configuration is valid, otherwise False */
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
    int parseHelp(std::string UNUSED(arg)) { return printUsage(); }

    SST_CONFIG_DECLARE_OPTION_NOVAR(help, std::bind(&SSTInfoConfig::parseHelp, this, std::placeholders::_1));


    static int parseVersion(std::string UNUSED(arg))
    {
        fprintf(stderr, "SST Release Version %s\n", PACKAGE_VERSION);
        return 1; /* Should not continue, but clean exit */
    }

    SST_CONFIG_DECLARE_OPTION_NOVAR(version, std::bind(&SSTInfoConfig::parseVersion, std::placeholders::_1));


    // Display Options
    int parseQuiet(const std::string& UNUSED(arg))
    {
        m_optionBits &= ~CFG_VERBOSE;
        return 0;
    }

    SST_CONFIG_DECLARE_OPTION_NOVAR(quiet, std::bind(&SSTInfoConfig::parseQuiet, this, std::placeholders::_1));


    SST_CONFIG_DECLARE_OPTION(bool, debugEnabled, false, &StandardConfigParsers::flag_set_true);


    int parseNoDisplay(const std::string& UNUSED(arg))
    {
        m_optionBits &= ~CFG_OUTPUTHUMAN;
        return 0;
    }

    SST_CONFIG_DECLARE_OPTION_NOVAR(no_display, std::bind(&SSTInfoConfig::parseNoDisplay, this, std::placeholders::_1));


    SST_CONFIG_DECLARE_OPTION(bool, interactiveEnabled, false, &StandardConfigParsers::flag_set_true);


    // XML Options

    int parseXML(const std::string& UNUSED(arg))
    {
        m_optionBits |= CFG_OUTPUTXML;
        return 0;
    }

    SST_CONFIG_DECLARE_OPTION_NOVAR(xml, std::bind(&SSTInfoConfig::parseXML, this, std::placeholders::_1));

    SST_CONFIG_DECLARE_OPTION(
        std::string, XMLFilePath, "./SSTInfo.xml", &StandardConfigParsers::from_string<std::string>);


    // Library and path options

    int parseLibs(const std::string& arg)
    {
        addFilter(arg);
        return 0;
    }

    SST_CONFIG_DECLARE_OPTION_NOVAR(libs, std::bind(&SSTInfoConfig::parseLibs, this, std::placeholders::_1));


private:
    char*                    m_AppName;
    std::vector<std::string> m_elementsToProcess;
    unsigned int             m_optionBits;
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
    explicit SSTLibraryInfo(const std::string& name) :
        m_name(name)
    {}

    /** Return the Name of the Library. */
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
