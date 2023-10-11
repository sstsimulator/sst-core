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


//REMOVE
#include <fstream>
#include <iostream>


using namespace std;
using namespace SST;
using namespace SST::Core;

// Global Variables
static int                                                g_fileProcessedCount;
static std::string                                        g_searchPath;
static std::vector<SSTLibraryInfo>                        g_libInfoArray;
static SSTInfoConfig                                      g_configuration(false);
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

// Could be removed/simplified now without outputHumanReadable
class OverallOutputter
{
public:
    void outputHumanReadable(std::ostream& os);
    void outputXML();
} g_Outputter;

// Forward Declarations
void        initLTDL(const std::string& searchPath);
void        shutdownLTDL();
static void processSSTElementFiles();
static void processSSTElementFiles(std::stringstream&);
void        outputSSTElementInfo();
void        getSSTElementInfo();
void        generateXMLOutputFile();
void        runCurses();
void        getInput();
void        setLibraryInfo(std::vector<std::string>);
void        parseInput(std::string);
void        drawWindows();
void        setInfoText(std::string);
void        printInfo();

int
main(int argc, char* argv[])
{
    // Parse the Command Line and get the Configuration Settings
    int status = g_configuration.parseCmdLine(argc, argv);
    if ( status ) {
        if ( status == 1 ) return 0;
        return 1;
    }

    g_searchPath = g_configuration.getLibPath();

    if (g_configuration.interactiveEnabled()) {
        runCurses();
    }
    else {
        processSSTElementFiles();
    }
    

    return 0;
}

int 
getCursorPos(WINDOW *console)
{
    int pos, h;
    getyx(console, h, pos);
    h += 1; //to please the compiler
    return pos;
}

void 
runCurses() 
{   
    initscr();
    cbreak();
    noecho();
    
    // Initialize Windows
    drawWindows();
    
    // Set initial text to help text
    parseInput("help");

    // Print and start reading input
    printInfo();
    getInput();
    endwin();
}

void 
drawWindows()
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

void 
setLibraryInfo(std::vector<std::string> args)
{
    // Reset and set new filters for SSTInfoConfig g_configuration
    g_fileProcessedCount = 0;
    g_configuration.clearFilterMap();

    if (args == std::vector<std::string>{"all"}) {} //do nothing
    else {
        for (std::string arg : args) {
            g_configuration.addFilter(arg);
        }
    }
    
    std::stringstream outputStream;
    processSSTElementFiles(outputStream);

    setInfoText(outputStream.str());
    textPos = 0;
}

void
getInput()
{
    // Take input
    std::string input = "";
    while(true) {
        int c = wgetch(console);

        if(c == '\n') {
            parseInput(input);

            //Clear input window and string
            drawWindows();
            printInfo();
            input = "";
        }
        // Resizing the window
        else if (c == KEY_RESIZE) {
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
            if (textPos > 0) {
                textPos -= 1;
                printInfo();
            }   
        }
        else if (c == KEY_DOWN) {
            if ((int)textPos < (int)infoText.size() - (int)LINES) {
                textPos += 1;
                printInfo();
            }
        }
        // Regular characters
        else if (c <= 255) {
            input += c;
            wprintw(console, "%c", c);
            wrefresh(console);
        }

        // Make sure the cursor doesn't move
        wmove(console, 1, input.size()+1);
    }
}

void 
parseInput(std::string input)
{
    //output << "Input: " << input << "\n";

    // Split into set of strings
    std::istringstream stream(input);
    std::string word;
    std::vector<std::string> inputWords;
    
    while (stream >> word) {
        //output << word;
        inputWords.push_back(word);
    }

    // Parse
    if (inputWords.size() > 0) {
        std::string command = inputWords[0];
        transform(command.begin(), command.end(), command.begin(), ::tolower); // Convert to lowercase

        std::string text; // Contains output text for cases that don't search through the library
        //output << "Command: " << command << "\n";
        
        // Help messages
        if (inputWords.size() == 1) {
            if (command == "help") {
                text = "SST-INFO\n"
                       "\t This program lists documented Components, SubComponents, Events, Modules, and Partitioners within an Element Library\n\n"
                       "COMMANDS\n"
                       "\t- Help : Displays this help message\n"
                       "\t- List {element.subelement} : Displays selected elements\n"
                       "\t- Open {element.subelement} : ...\n"
                       "\t- Path {subelement(?)} : ...\n\n";

                setInfoText(text);
            }
            else if (command == "list") {
                text = "LIST COMMANDS\n"
                       "\t- List all : Display all available element libraries and their components/subcomponents\n"
                       "\t- List types [element] : Display all types of components/subcomponents within the specified element library(s)\n"
                       "\t- List [element(type)] : Display all components/subcomponents of the specified type ***WIP\n"
                       "\t- List [element[.component|subcomponent]] : Display the specified element/subelement(s)\n\n"
                       "\t'element' - Element Library\n"
                       "\t'type' - Type of Component/Subcomponent\n"
                       "\t'component|subcomponent' - Either a Component or SubComponent defined in the Element Library\n\n"
                       "EXAMPLES\n"
                       "\tlist sst.linear\n"
                       "\tlist types sst\n"
                       "\tlist ariel sst\n"
                       "\tlist sst(ProfileTools)\n"
                       "\tlist coreTestElement(SubComponents)\n"
                       "\netc..."; //needs more

                setInfoText(text);
            }
            else {

            }
        }

        // Parse commands
        else if (inputWords.size() > 1) {
            //Get args
            auto start = std::next(inputWords.begin(), 1);
            auto end = inputWords.end();
            std::vector<std::string> args(start, end);

            if (command == "list") {
                if (args[0] == "types") { // NOT case-insensitive (will be when all input gets converted to lowercase)
                    args.erase(args.begin());// Remove "types" from arg list

                    // Go through g_libInfoArray to find each library listed in args
                    for (std::string library : args) {
                        bool found = false;
                        for (auto libInfo : g_libInfoArray) {
                            if (library == libInfo.getLibraryName()) {
                                // Library found
                                text += library + " BaseTypes:\n";
                                for (auto type : libInfo.getTypeList()) {
                                    text += "\t" + type + "\n";
                                }
                                text += "\n";
                                found = true;
                                break;
                            }
                        }
                        if (!found) { text += "\nError: '" + library + "' Not Found\n\n"; }
                    }
                    
                    setInfoText(text);
                }
                else {
                    g_libInfoArray.clear();
                    setLibraryInfo(args);
                }
            }
            else {
                //More commands here
            }
        }
    }
}

void 
setInfoText(std::string infoString)
{
    std::vector<std::string> stringVec;
    textPos = 0;

    // Splits the string into individual lines and stores them into the infoText vector
    std::stringstream infoStream(infoString);
    std::string line;

    while(std::getline(infoStream, line, '\n')){
        stringVec.push_back((line + "\n"));
    }
    infoText.clear(); //clears memory
    infoText = stringVec;
}

void 
printInfo()
{
    unsigned int posMax = ((int)infoText.size() < LINES-3) ? textPos + infoText.size() : textPos + (LINES-3);

    for (unsigned int i = textPos; i < posMax; i++) {
        const char *cstr = infoText[i].c_str();
        wprintw(info, cstr);
    }
    wrefresh(info);
    wrefresh(console); //moves the cursor back into the console window
    wmove(console, 1, 1);
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

// Interactive addELI
static void
addELI(ElemLoader& loader, const std::string& lib, std::stringstream& outputStream, bool optional)
{
    loader.loadLibrary(lib, outputStream);

    // Check to see if this library loaded into the new ELI
    // Database
    if ( ELI::LoadedLibraries::isLoaded(lib) ) {
        g_fileProcessedCount++;
        g_libInfoArray.emplace_back(lib);
    }
    else if ( !optional ) {
        outputStream << "**** WARNING - PROCESS LIBRARY = " << lib.c_str() << "\n";
    }
    else {
        outputStream <<  "**** " << lib.c_str() << " not Found!\n";
    }
}

static void
processSSTElementFiles()
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
    if ( g_configuration.getOptionBits() & CFG_OUTPUTHUMAN ) { outputSSTElementInfo(); }

    // Do we output an XML File
    if ( g_configuration.getOptionBits() & CFG_OUTPUTXML ) { generateXMLOutputFile(); }
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
        addELI(loader, l, outputStream, g_configuration.processAllElements());
    }

    if ( g_configuration.getOptionBits() & CFG_OUTPUTHUMAN ) { getSSTElementInfo(); }
}

void
generateXMLOutputFile()
{
    g_Outputter.outputXML();
}

void
outputSSTElementInfo()
{
    g_Outputter.outputHumanReadable(std::cout);
}

void
getSSTElementInfo()
{
    // Get Element library info and store into libInfoArray
    for ( size_t x = 0; x < g_libInfoArray.size(); x++ ) {
        g_libInfoArray[x].getLibString(x);
    }
}

void
getLibraryInfo(std::stringstream& outputStream)
{
    outputStream << "PROCESSED " << g_fileProcessedCount << " .so (SST ELEMENT) FILES FOUND IN DIRECTORY(s) " << g_searchPath
                 << "\n";

    //Tell the user what Elements will be displayed
    for ( auto& i : g_configuration.getFilterMap() ) {
        outputStream << "Filtering output on Element = \"" << i.first.c_str();
        //fprintf(stdout, "Filtering output on Element = \"%s", i.first.c_str());

        if ( !i.second.empty() ) {
            outputStream << "." << i.second.c_str();
        }
        outputStream << "\"\n";
    }

    for ( size_t x = 0; x < g_libInfoArray.size(); x++ ) {
        g_libInfoArray[x].outputText(outputStream);
    }
}

void
OverallOutputter::outputHumanReadable(std::ostream& os)
{
    os << "PROCESSED " << g_fileProcessedCount << " .so (SST ELEMENT) FILES FOUND IN DIRECTORY(s) " << g_searchPath
       << "\n";

    // Tell the user what Elements will be displayed
    for ( auto& i : g_configuration.getFilterMap() ) {
        fprintf(stdout, "Filtering output on Element = \"%s", i.first.c_str());
        if ( !i.second.empty() ) fprintf(stdout, ".%s", i.second.c_str());
        fprintf(stdout, "\"\n");
    }

    // Now dump the Library Info
    for ( size_t x = 0; x < g_libInfoArray.size(); x++ ) {
        g_libInfoArray[x].outputHumanReadable(os, x);
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

    // General Info on the Data
    xmlComment(&XMLDocument, "SSTInfo XML Data Generated on %s", TimeStamp);
    xmlComment(&XMLDocument, "%d .so FILES FOUND IN DIRECTORY(s) %s\n", g_fileProcessedCount, g_searchPath.c_str());

    XMLDocument.LinkEndChild(XMLTopLevelElement);

    // Save the XML Document
    XMLDocument.SaveFile(g_configuration.getXMLFilePath().c_str());
}

SSTInfoConfig::SSTInfoConfig(bool suppress_print) : ConfigShared(suppress_print, true)
{
    using namespace std::placeholders;

    m_optionBits   = CFG_OUTPUTHUMAN | CFG_VERBOSE; // Enable normal output by default
    m_XMLFilePath  = "./SSTInfo.xml";               // Default XML File Path
    m_debugEnabled = false;

    // Add the options
    DEF_SECTION_HEADING("Informational Options");
    DEF_FLAG("help", 'h', "Print help message", std::bind(&SSTInfoConfig::printHelp, this, _1));
    DEF_FLAG("version", 'V', "Print SST release version", std::bind(&SSTInfoConfig::outputVersion, this, _1));

    DEF_SECTION_HEADING("Display Options");
    addVerboseOptions(false);
    DEF_FLAG("quiet", 'q', "Quiet/print summary only", std::bind(&SSTInfoConfig::setQuiet, this, _1));
    DEF_FLAG("debug", 'd', "Enable debugging messages", std::bind(&SSTInfoConfig::setEnableDebug, this, _1));
    DEF_FLAG(
        "nodisplay", 'n', "Do not display output [default: off]", std::bind(&SSTInfoConfig::setNoDisplay, this, _1));
    DEF_FLAG(
        "interactive", 'i', "(EXPERIMENTAL) Enable interactive command line mode", std::bind(&SSTInfoConfig::setInteractive, this, _1));
    DEF_SECTION_HEADING("XML Options");
    DEF_FLAG("xml", 'x', "Generate XML data [default:off]", std::bind(&SSTInfoConfig::setXML, this, _1));
    DEF_ARG(
        "outputxml", 'o', "FILE", "Filepath to XML file [default: SSTInfo.xml]",
        std::bind(&SSTInfoConfig::setXMLOutput, this, _1), false);

    DEF_SECTION_HEADING("Library and Path Options");
    DEF_ARG(
        "libs", 'l', "LIBS",
        "Element libraries to process (all, <element>) [default: all]. <element> can be an element library, or it can "
        "be a single element within the library.",
        std::bind(&SSTInfoConfig::setLibs, this, _1), false);
    addLibraryPathOptions();

    DEF_SECTION_HEADING("Advanced Options - Environment");
    addEnvironmentOptions();

    addPositionalCallback(std::bind(&SSTInfoConfig::setPositionalArg, this, _1, _2));
}

SSTInfoConfig::~SSTInfoConfig() {}

std::string
SSTInfoConfig::getUsagePrelude()
{
    std::string prelude = "Usage: ";
    prelude.append(getRunName());
    prelude.append(" [options] [<element[.component|subcomponent]>]\n");
    return prelude;
}

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




#if 0
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
#endif

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

void
SSTLibraryInfo::outputText(std::stringstream& outputStream)
{
    for (auto& map : infoMap) {
        // for ( auto& pair : map ) {
            //pair.second->toString(outputStream);
            //outputStream << "FIRST: " << map.first << "\nSECOND: " << map.second << "\n\n";
            outputStream << "FIRST: " << map.first << "\n\n";

            // bool print = shouldPrintElement(getLibraryName(), pair.first);
            // if ( print ) {
            //     outputStream << pair.first << "\n";
            //     if ( g_configuration.doVerbose() ) pair.second->toString(outputStream);
            // }
            // if ( print ) outputStream << std::endl;
        //}
    }
}

void
SSTLibraryInfo::find()
{
    
}

template <class BaseType>
void
SSTLibraryInfo::getLibString(bool printAll)
{
    std::ofstream output("out.txt");

    // lib is an InfoLibrary
    auto* lib = ELI::InfoDatabase::getLibrary<BaseType>(getLibraryName());
    if ( lib ) {
        // Only print if there is something of that type in the library
        if ( lib->numEntries() != 0 ) {
            // Create map keys based on type name
            std::string baseName = std::string(BaseType::ELI_baseName()) + "s (" + std::to_string(lib->numEntries()) + " total)";

            int idx = 0;
            // lib->getMap returns a map<string, BaseInfo*>.  BaseInfo is
            // actually a Base::BuilderInfo and the implementation is in
            // eli/elementinfo as BuilderInfoImpl
            auto& map = lib->getMap();
            infoMap.insert(make_pair(baseName, map));
            output << "*inserted type: " << baseName << "\n";
        }
    }
    else {
        //os << "No " << BaseType::ELI_baseName() << "s\n";
    }
}

void
SSTLibraryInfo::getLibString(int LibIndex)
{
    bool enableFullElementOutput = !doesLibHaveFilters(getLibraryName());

    // os << "================================================================================\n";
    // os << "ELEMENT LIBRARY " << LibIndex << " = " << getLibraryName() << " (" << getLibraryDescription() << ")"
    //    << "\n";

    getLibString<Component>(enableFullElementOutput);
    getLibString<SubComponent>(enableFullElementOutput);
    getLibString<Module>(enableFullElementOutput);
    getLibString<SST::Partition::SSTPartitioner>(enableFullElementOutput);
    getLibString<SST::Profile::ProfileTool>(enableFullElementOutput);
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
                if ( print ) os << std::endl;
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
