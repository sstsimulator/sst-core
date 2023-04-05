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

#include "sst_config.h"

#include "sst/core/sstinfo.h"

#include "sst/core/build_info.h"
#include "sst/core/component.h"
#include "sst/core/elemLoader.h"
#include "sst/core/env/envconfig.h"
#include "sst/core/env/envquery.h"
#include "sst/core/part/sstpart.h"
#include "sst/core/sstpart.h"
#include "sst/core/subcomponent.h"
#include "sst/core/warnmacros.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <dirent.h>
#include <dlfcn.h>
#include <getopt.h>
#include <list>
#include <sys/stat.h>
#include <ncurses.h>

using namespace std;
using namespace SST;
using namespace SST::Core;

// Global Variables
static int                                                g_fileProcessedCount;
static std::string                                        g_searchPath;
static std::vector<SSTLibraryInfo>                        g_libInfoArray;
static SSTInfoConfig                                      g_configuration;
static std::map<std::string, const ElementInfoGenerator*> g_foundGenerators;
static WINDOW                                             *info;
static WINDOW                                             *console;
static std::vector<std::string>                           infoText;
static unsigned int                                       textPos;

void
dprintf(FILE* fp, const char* fmt, ...)
{
    if ( g_configuration.doVerbose() ) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        va_end(args);
    }
}

static void
xmlComment(TiXmlNode* owner, const char* fmt...)
{
    ssize_t size = 128;

retry:
    char*   buf = (char*)calloc(size, 1);
    va_list ap;
    va_start(ap, fmt);
    int res = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    if ( res > size ) {
        free(buf);
        size = res + 1;
        goto retry;
    }

    TiXmlComment* comment = new TiXmlComment(buf);
    owner->LinkEndChild(comment);
}

class OverallOutputter
{
public:
    void outputHumanReadable(std::stringstream&);
    void outputXML();
} g_Outputter;

// Forward Declarations
void        initLTDL(const std::string& searchPath);
void        shutdownLTDL();
static void processSSTElementFiles(std::stringstream&);
void        outputSSTElementInfo(std::stringstream&);
void        generateXMLOutputFile();
void        run();
void        getInput();
void        drawWindows();
void        setInfoText(std::string);
void        printInfo();

int
main(int argc, char* argv[])
{
    // Parse the Command Line and get the Configuration Settings
    if ( g_configuration.parseCmdLine(argc, argv) ) { return -1; }

    std::vector<std::string>               overridePaths;
    Environment::EnvironmentConfiguration* sstEnv = Environment::getSSTEnvironmentConfiguration(overridePaths);
    g_searchPath                                  = "";
    std::set<std::string> groupNames              = sstEnv->getGroupNames();

    for ( auto groupItr = groupNames.begin(); groupItr != groupNames.end(); groupItr++ ) {
        SST::Core::Environment::EnvironmentConfigGroup* currentGroup = sstEnv->getGroupByName(*groupItr);
        std::set<std::string>                           groupKeys    = currentGroup->getKeys();

        for ( auto keyItr = groupKeys.begin(); keyItr != groupKeys.end(); keyItr++ ) {
            const std::string key = *keyItr;

            if ( key.size() > 6 && key.substr(key.size() - 6) == "LIBDIR" ) {
                if ( g_searchPath.size() > 0 ) { g_searchPath.append(":"); }

                g_searchPath.append(currentGroup->getValue(key));
            }
        }
    }

    // Run curses
    initscr();
    cbreak();
    noecho();
    run();

    return 0;
}

int getCursorPos(WINDOW *console)
{
    int pos, _;
    getyx(console, _, pos);
    _ += 1; //to please the compiler
    return pos;
}

void run() 
{   
    // Initialize Windows
    drawWindows();
    
    // Set initial text to complete info text
    std::stringstream outputStream;
    processSSTElementFiles(outputStream);
    setInfoText(outputStream.str());
    textPos = 0;

    printInfo();
    getInput();
    endwin();
}

void printInfo()
{
    for (unsigned int i = textPos; i < textPos+LINES; i++) {
        const char *cstr = infoText[i].c_str();
        wprintw(info, cstr);
    }
    wrefresh(info);
    wrefresh(console); //moves the cursor back into the console window
}

void getInput()
{
    // Take input
    std::string input = "";
    while(true) {
        int c = wgetch(console);

        if(c == '\n') {
            //parseInput()
            //printInfo(input);

            // Erase and redraw
            drawWindows();
            printInfo();
            input = "";
        }
        // Resizing the window
        else if (c == KEY_RESIZE) {
            endwin();
            wrefresh(info);
            wrefresh(console);
            drawWindows();
            printInfo();
        }
        // Handle backspaces
        else if (c == KEY_BACKSPACE) {
            int pos = getCursorPos(console);
            if (pos > 1) {
                wprintw(console, "\b \b");
                input.pop_back();
            }
        }

        // Scrolling
        else if (c == KEY_UP) {
            //printInfo("UP");
            if (textPos > 0) {
                textPos -= 1;
            }
            printInfo();

        }
        else if (c == KEY_DOWN) {
            //printInfo("DOWN");
            if (textPos < infoText.size()-LINES) {
                textPos += 1;
            }
            printInfo();
        }

        // Regular characters
        else if (c <= 255) {
            input += c;
            wprintw(console, "%c", c);
            wrefresh(console);
        }
    }
}

void drawWindows()
{
    // Reset windows for redraws
    werase(info);
    werase(console);
    delwin(info);
    delwin(console);

    info = newwin(LINES-3, COLS, 0, 0);
    console = newwin(3, COLS, LINES-3, 0);


    // Parameters
    scrollok(info, true);
    scrollok(console, false);
    keypad(console, true);

    box(console, 0, 0);
    mvwprintw(console, 0, 1, " Console ");
    wmove(console, 1, 1);
    wrefresh(info);
    wrefresh(console);
}

void setInfoText(std::string infoString) 
{
    // Splits the string into individual lines and stores them into the infoText vector
    std::string delimiter = "\n";
    size_t pos = 0;
    std::string line;
    while ((pos = infoString.find(delimiter)) != std::string::npos) {
        line = infoString.substr(0, pos);
        line.append("\n");
        infoText.push_back(line);
        infoString.erase(0, pos + delimiter.length());
    }
}

static void
addELI(ElemLoader& loader, const std::string& lib, bool optional)
{

    if ( g_configuration.debugEnabled() ) fprintf(stdout, "Looking for library \"%s\"\n", lib.c_str());

    std::stringstream err_sstr;
    loader.loadLibrary(lib, err_sstr);

    // Check to see if this library loaded into the new ELI
    // Database
    if ( ELI::LoadedLibraries::isLoaded(lib) ) {
        g_fileProcessedCount++;
        g_libInfoArray.emplace_back(lib);
    }
    else if ( !optional ) {
        fprintf(stderr, "**** WARNING - UNABLE TO PROCESS LIBRARY = %s\n", lib.c_str());
        if ( g_configuration.debugEnabled() ) { std::cerr << err_sstr.str() << std::endl; }
    }
    else {
        fprintf(stderr, "**** %s not Found!\n", lib.c_str());
        // regardless of debug - force error printing
        std::cerr << err_sstr.str() << std::endl;
    }
}

static void
processSSTElementFiles(std::stringstream& outputStream)
{
    std::vector<bool>        EntryProcessedArray;
    ElemLoader               loader(g_searchPath);
    std::vector<std::string> potentialLibs;
    loader.getPotentialElements(potentialLibs);

    // Which libraries should we (attempt) to process
    std::set<std::string> processLibs(g_configuration.getElementsToProcessArray());
    if ( processLibs.empty() ) {
        for ( auto& i : potentialLibs )
            processLibs.insert(i);
    }

    for ( auto l : processLibs ) {
        addELI(loader, l, g_configuration.processAllElements());
    }

    // Do we output in Human Readable form
    if ( g_configuration.getOptionBits() & CFG_OUTPUTHUMAN ) { outputSSTElementInfo(outputStream); }

    // Do we output an XML File
    if ( g_configuration.getOptionBits() & CFG_OUTPUTXML ) { generateXMLOutputFile(); }
}

void
generateXMLOutputFile()
{
    g_Outputter.outputXML();
}

void
outputSSTElementInfo(std::stringstream& outputStream)
{
    g_Outputter.outputHumanReadable(outputStream);
}

void
OverallOutputter::outputHumanReadable(std::stringstream& outputStream)
{
    outputStream << "PROCESSED " << g_fileProcessedCount << " .so (SST ELEMENT) FILES FOUND IN DIRECTORY(s) " << g_searchPath
                 << "\n";

    // Tell the user what Elements will be displayed
    for ( auto& i : g_configuration.getFilterMap() ) {
        fprintf(stdout, "Filtering output on Element = \"%s", i.first.c_str());
        if ( !i.second.empty() ) fprintf(stdout, ".%s", i.second.c_str());
        fprintf(stdout, "\"\n");
    }

    // Now dump the Library Info
    for ( size_t x = 0; x < g_libInfoArray.size(); x++ ) {
        g_libInfoArray[x].outputHumanReadable(outputStream, x);
    }
}

void
OverallOutputter::outputXML()
{
    unsigned int x;
    char         TimeStamp[32];
    std::time_t  now = std::time(nullptr);
    std::tm*     ptm = std::localtime(&now);

    // Create a Timestamp Format: 2015.02.15_20:20:00
    std::strftime(TimeStamp, 32, "%Y.%m.%d_%H:%M:%S", ptm);

    dprintf(stdout, "\n");
    dprintf(stdout, "================================================================================\n");
    dprintf(stdout, "GENERATING XML FILE SSTInfo.xml as %s\n", g_configuration.getXMLFilePath().c_str());
    dprintf(stdout, "================================================================================\n");
    dprintf(stdout, "\n");
    dprintf(stdout, "\n");

    // Create the XML Document
    TiXmlDocument XMLDocument;

    // Set the Top Level Element
    TiXmlElement* XMLTopLevelElement = new TiXmlElement("SSTInfoXML");

    // Set the File Information
    TiXmlElement* XMLFileInfoElement = new TiXmlElement("FileInfo");
    XMLFileInfoElement->SetAttribute("SSTInfoVersion", PACKAGE_VERSION);
    XMLFileInfoElement->SetAttribute("FileFormat", "1.0");
    XMLFileInfoElement->SetAttribute("TimeStamp", TimeStamp);
    XMLFileInfoElement->SetAttribute("FilesProcessed", g_fileProcessedCount);
    XMLFileInfoElement->SetAttribute("SearchPath", g_searchPath.c_str());

    // Add the File Information to the Top Level Element
    XMLTopLevelElement->LinkEndChild(XMLFileInfoElement);

    // Now Generate the XML Data that represents the Library Info,
    // and add the data to the Top Level Element
    for ( x = 0; x < g_libInfoArray.size(); x++ ) {
        g_libInfoArray[x].outputXML(x, XMLTopLevelElement);
    }

    // Add the entries into the XML Document
    // XML Declaration
    TiXmlDeclaration* XMLDecl = new TiXmlDeclaration("1.0", "", "");
    XMLDocument.LinkEndChild(XMLDecl);
    //
    // General Info on the Data
    xmlComment(&XMLDocument, "SSTInfo XML Data Generated on %s", TimeStamp);
    xmlComment(&XMLDocument, "%d .so FILES FOUND IN DIRECTORY(s) %s\n", g_fileProcessedCount, g_searchPath.c_str());

    XMLDocument.LinkEndChild(XMLTopLevelElement);

    // Save the XML Document
    XMLDocument.SaveFile(g_configuration.getXMLFilePath().c_str());
}

SSTInfoConfig::SSTInfoConfig()
{
    m_optionBits   = CFG_OUTPUTHUMAN | CFG_VERBOSE; // Enable normal output by default
    m_XMLFilePath  = "./SSTInfo.xml";               // Default XML File Path
    m_debugEnabled = false;
}

SSTInfoConfig::~SSTInfoConfig() {}

void
SSTInfoConfig::outputUsage()
{
    cout << "Usage: " << m_AppName << " [options]" << endl;
    cout << "  -h, --help               Print Help Message\n";
    cout << "  -v, --version            Print SST Package Release Version\n";
    cout << "  -d, --debug              Enabled debugging messages\n";
    cout << "  -n, --nodisplay          Do not display output - default is off\n";
    cout << "  -x, --xml                Generate XML data - default is off\n";
    cout << "  -o, --outputxml=FILE     File path to XML file. Default is SSTInfo.xml\n";
    cout << "  -q, --quiet              Quiet/print summary only\n";
    cout << endl;
}

void
SSTInfoConfig::outputVersion()
{
    cout << "SST Release Version " PACKAGE_VERSION << endl;
}

int
SSTInfoConfig::parseCmdLine(int argc, char* argv[])
{
    m_AppName = argv[0];

    static const struct option longOpts[] = { { "help", no_argument, nullptr, 'h' },
                                              { "version", no_argument, nullptr, 'v' },
                                              { "debug", no_argument, nullptr, 'd' },
                                              { "nodisplay", no_argument, nullptr, 'n' },
                                              { "xml", no_argument, nullptr, 'x' },
                                              { "quiet", no_argument, nullptr, 'q' },
                                              { "outputxml", required_argument, nullptr, 'o' },
                                              { "elemenfilt", required_argument, nullptr, 0 },
                                              { nullptr, 0, nullptr, 0 } };
    while ( 1 ) {
        int       opt_idx = 0;
        const int intC    = getopt_long(argc, argv, "hvqdnxo:l:", longOpts, &opt_idx);
        if ( intC == -1 ) break;

        const char c = static_cast<char>(intC);

        switch ( c ) {
        case 'h':
            outputUsage();
            return 1;
        case 'v':
            outputVersion();
            return 1;
        case 'q':
            m_optionBits &= ~CFG_VERBOSE;
            break;
        case 'd':
            m_debugEnabled = true;
            break;
        case 'n':
            m_optionBits &= ~CFG_OUTPUTHUMAN;
            break;
        case 'x':
            m_optionBits |= CFG_OUTPUTXML;
            break;
        case 'o':
            m_XMLFilePath = optarg;
            break;
        case 'l':
        {
            addFilter(optarg);
            break;
        }
        case 0:
            if ( !strcmp(longOpts[opt_idx].name, "elemnfilt") ) { addFilter(optarg); }
            break;
        }
    }

    while ( optind < argc ) {
        addFilter(argv[optind++]);
    }

    return 0;
}

void
SSTInfoConfig::addFilter(const std::string& name_str)
{
    std::string name(name_str);
    if ( name.size() > 3 && name.substr(0, 3) == "lib" ) name = name.substr(3);

    size_t dotLoc = name.find(".");
    if ( dotLoc == std::string::npos ) { m_filters.insert(std::make_pair(name, "")); }
    else {
        m_filters.insert(std::make_pair(std::string(name, 0, dotLoc), std::string(name, dotLoc + 1)));
    }
}

bool
doesLibHaveFilters(const std::string& libName)
{
    auto range = g_configuration.getFilterMap().equal_range(libName);
    for ( auto x = range.first; x != range.second; ++x ) {
        if ( x->second != "" ) return true;
    }
    return false;
}

bool
shouldPrintElement(const std::string& libName, const std::string& elemName)
{
    auto range = g_configuration.getFilterMap().equal_range(libName);
    if ( range.first == range.second ) return true;
    for ( auto x = range.first; x != range.second; ++x ) {
        if ( x->second == "" ) return true;
        if ( x->second == elemName ) return true;
    }
    return false;
}

template <class BaseType>
void
SSTLibraryInfo::outputHumanReadable(std::ostream& os, bool printAll)
{
    // lib is an InfoLibrary
    auto* lib = ELI::InfoDatabase::getLibrary<BaseType>(getLibraryName());
    if ( lib ) {
        // Only print if there is something of that type in the library
        if ( lib->numEntries() != 0 ) {
            os << BaseType::ELI_baseName() << "s (" << lib->numEntries() << " total)\n";
            int idx = 0;
            // lib->getMap returns a map<string, BaseInfo*>.  BaseInfo is
            // actually a Base::BuilderInfo and the implementation is in
            // BuildInfoImpl
            for ( auto& pair : lib->getMap() ) {
                bool print = printAll || shouldPrintElement(getLibraryName(), pair.first);
                if ( print ) {
                    os << "   " << BaseType::ELI_baseName() << " " << idx << ": " << pair.first << "\n";
                    if ( g_configuration.doVerbose() ) pair.second->toString(os);
                }
                ++idx;
                os << std::endl;
            }
        }
    }
    else {
        os << "No " << BaseType::ELI_baseName() << "s\n";
    }
}

void
SSTLibraryInfo::outputHumanReadable(std::ostream& os, int LibIndex)
{
    bool enableFullElementOutput = !doesLibHaveFilters(getLibraryName());

    os << "================================================================================\n";
    os << "ELEMENT LIBRARY " << LibIndex << " = " << getLibraryName() << " (" << getLibraryDescription() << ")"
       << "\n";

    outputHumanReadable<Component>(os, enableFullElementOutput);
    outputHumanReadable<SubComponent>(os, enableFullElementOutput);
    outputHumanReadable<Module>(os, enableFullElementOutput);
    outputHumanReadable<SST::Partition::SSTPartitioner>(os, enableFullElementOutput);
    outputHumanReadable<SST::Profile::ProfileTool>(os, enableFullElementOutput);
}

template <class BaseType>
void
SSTLibraryInfo::outputXML(TiXmlElement* XMLLibraryElement)
{
    auto* lib = ELI::InfoDatabase::getLibrary<BaseType>(getLibraryName());
    if ( lib ) {
        int numObjects = lib->numEntries();
        xmlComment(XMLLibraryElement, "Num %ss = %d", BaseType::ELI_baseName(), numObjects);
        int idx = 0;
        for ( auto& pair : lib->getMap() ) {
            TiXmlElement* XMLElement = new TiXmlElement(BaseType::ELI_baseName());
            XMLElement->SetAttribute("Index", idx);
            pair.second->outputXML(XMLElement);
            XMLLibraryElement->LinkEndChild(XMLElement);
            idx++;
        }
    }
    else {
        xmlComment(XMLLibraryElement, "No %ss", BaseType::ELI_baseName());
    }
}

void
SSTLibraryInfo::outputXML(int LibIndex, TiXmlNode* XMLParentElement)
{
    TiXmlElement* XMLLibraryElement = new TiXmlElement("Element");
    XMLLibraryElement->SetAttribute("Index", LibIndex);
    XMLLibraryElement->SetAttribute("Name", getLibraryName().c_str());
    XMLLibraryElement->SetAttribute("Description", getLibraryDescription().c_str());

    outputXML<Component>(XMLLibraryElement);
    outputXML<SubComponent>(XMLLibraryElement);
    outputXML<Module>(XMLLibraryElement);
    outputXML<SST::Partition::SSTPartitioner>(XMLLibraryElement);
    XMLParentElement->LinkEndChild(XMLLibraryElement);
}
