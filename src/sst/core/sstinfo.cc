// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/warnmacros.h>

#include <cstdio>
#include <cerrno>
#include <list>
#include <dirent.h>
#include <sys/stat.h>
#include <ltdl.h>
#include <dlfcn.h>
#include <cstdlib>
#include <getopt.h>
#include <ctime>

#include <sst/core/elemLoader.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/part/sstpart.h>
#include <sst/core/sstpart.h>
#include "sst/core/build_info.h"

#include "sst/core/env/envquery.h"
#include "sst/core/env/envconfig.h"

#include <sst/core/tinyxml/tinyxml.h>
#include <sst/core/sstinfo.h>


using namespace std;
using namespace SST;
using namespace SST::Core;

// Global Variables
static int                                              g_fileProcessedCount;
static std::string                                        g_searchPath;
static std::vector<SSTLibraryInfo>                        g_libInfoArray;
static SSTInfoConfig                                      g_configuration;
static std::map<std::string, const ElementInfoGenerator*> g_foundGenerators;


void dprintf(FILE *fp, const char *fmt, ...) {
    if ( g_configuration.doVerbose() ) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        va_end(args);
    }
}


static void xmlComment(TiXmlNode* owner, const char* fmt...)
{
    ssize_t size = 128;

retry:
    char *buf = (char*)calloc(size, 1);
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


class OverallOutputter {
public:
    void outputHumanReadable(std::ostream& os);
    void outputXML();
} g_Outputter;


// Forward Declarations
void initLTDL(std::string searchPath);
void shutdownLTDL();
static void processSSTElementFiles();
void outputSSTElementInfo();
void generateXMLOutputFile();


int main(int argc, char *argv[])
{
    // Parse the Command Line and get the Configuration Settings
    if (g_configuration.parseCmdLine(argc, argv)) {
        return -1;
    }

    std::vector<std::string> overridePaths;
    Environment::EnvironmentConfiguration* sstEnv = Environment::getSSTEnvironmentConfiguration(overridePaths);
    g_searchPath = "";
    std::set<std::string> groupNames = sstEnv->getGroupNames();

    for(auto groupItr = groupNames.begin(); groupItr != groupNames.end(); groupItr++) {
        SST::Core::Environment::EnvironmentConfigGroup* currentGroup =
            sstEnv->getGroupByName(*groupItr);
        std::set<std::string> groupKeys = currentGroup->getKeys();

        for(auto keyItr = groupKeys.begin(); keyItr != groupKeys.end(); keyItr++) {
            const std::string key = *keyItr;

            if(key.size() > 6 && key.substr(key.size() - 6) == "LIBDIR") {
                if(g_searchPath.size() > 0) {
                    g_searchPath.append(":");
                }

                g_searchPath.append(currentGroup->getValue(key));
            }
        }
    }


    // Read in the Element files and process them
    processSSTElementFiles();

    return 0;
}


static void addELI(ElemLoader &loader, const std::string &lib, bool optional)
{

    if ( g_configuration.debugEnabled() )
        fprintf (stdout, "Looking for library \"%s\"\n", lib.c_str());

    loader.loadLibrary(lib, g_configuration.debugEnabled());

    // Check to see if this library loaded into the new ELI
    // Database
    if (ELI::LoadedLibraries::isLoaded(lib)) {
        g_fileProcessedCount++;
        g_libInfoArray.emplace_back(lib);
    } else if (!optional){
        fprintf(stderr, "**** WARNING - UNABLE TO PROCESS LIBRARY = %s\n", lib.c_str());
    } else {
        fprintf(stdout, "**** %s not Found!\n", lib.c_str());
    }

}


static void processSSTElementFiles()
{
    std::vector<bool>       EntryProcessedArray;
    ElemLoader              loader(g_searchPath);

    std::vector<std::string> potentialLibs = loader.getPotentialElements();

    // Which libraries should we (attempt) to process
    std::set<std::string> processLibs(g_configuration.getElementsToProcessArray());
    if ( processLibs.empty() ) {
        for ( auto & i : potentialLibs )
            processLibs.insert(i);
    }

    for ( auto l : processLibs ) {
        addELI(loader, l, g_configuration.processAllElements());
    }


    // Do we output in Human Readable form
    if (g_configuration.getOptionBits() & CFG_OUTPUTHUMAN) {
        outputSSTElementInfo();
    }

    // Do we output an XML File
    if (g_configuration.getOptionBits() & CFG_OUTPUTXML) {
        generateXMLOutputFile();
    }

}

void generateXMLOutputFile()
{
    g_Outputter.outputXML();
}

void outputSSTElementInfo()
{
    g_Outputter.outputHumanReadable(std::cout);
}

void OverallOutputter::outputHumanReadable(std::ostream& os)
{
    os << "PROCESSED " << g_fileProcessedCount
       << " .so (SST ELEMENT) FILES FOUND IN DIRECTORY(s) "
       << g_searchPath << "\n";

    // Tell the user what Elements will be displayed
    for ( auto &i : g_configuration.getFilterMap() ) {
        fprintf(stdout, "Filtering output on Element = \"%s", i.first.c_str());
        if ( !i.second.empty() )
            fprintf(stdout, ".%s", i.second.c_str());
        fprintf(stdout, "\"\n");
    }

    // Now dump the Library Info
    for (size_t x = 0; x < g_libInfoArray.size(); x++) {
        g_libInfoArray[x].outputHumanReadable(os,x);
    }
}

void OverallOutputter::outputXML()
{
    unsigned int            x;
    char                    TimeStamp[32];
    std::time_t             now = std::time(NULL);
    std::tm*                ptm = std::localtime(&now);

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
    for (x = 0; x < g_libInfoArray.size(); x++) {
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
    m_optionBits = CFG_OUTPUTHUMAN|CFG_VERBOSE;  // Enable normal output by default
    m_XMLFilePath = "./SSTInfo.xml"; // Default XML File Path
    m_debugEnabled = false;
}

SSTInfoConfig::~SSTInfoConfig()
{
}

void SSTInfoConfig::outputUsage()
{
    cout << "Usage: " << m_AppName << " [<element[.component|subcomponent]>] "<< " [options]" << endl;
    cout << "  -h, --help               Print Help Message\n";
    cout << "  -v, --version            Print SST Package Release Version\n";
    cout << "  -d, --debug              Enabled debugging messages\n";
    cout << "  -n, --nodisplay          Do not display output - default is off\n";
    cout << "  -x, --xml                Generate XML data - default is off\n";
    cout << "  -o, --outputxml=FILE     File path to XML file. Default is SSTInfo.xml\n";
    cout << "  -l, --libs=LIBS          {all, <elementname>} - Element Library(s) to process\n";
    cout << endl;
}

void SSTInfoConfig::outputVersion()
{
    cout << "SST Release Version " PACKAGE_VERSION << endl;
}

int SSTInfoConfig::parseCmdLine(int argc, char* argv[])
{
    m_AppName = argv[0];

    static const struct option longOpts[] = {
        {"help",        no_argument,        0, 'h'},
        {"quiet",       no_argument,        0, 'q'},
        {"version",     no_argument,        0, 'v'},
        {"debug",       no_argument,        0, 'd'},
        {"nodisplay",   no_argument,        0, 'n'},
        {"xml",         no_argument,        0, 'x'},
        {"outputxml",   required_argument,  0, 'o'},
        {"libs",        required_argument,  0, 'l'},
        {"elemenfilt",  required_argument,  0, 0},
        {NULL, 0, 0, 0}
    };
    while (1) {
        int opt_idx = 0;
        const int intC = getopt_long(argc, argv, "hvqdnxo:l:", longOpts, &opt_idx);
        if ( intC == -1 )
            break;

  	const char c = static_cast<char>(intC);

        switch (c) {
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
        case 'l': {
            addFilter( optarg );
            break;
        }
        case 0:
            if ( !strcmp(longOpts[opt_idx].name, "elemnfilt" ) ) {
                addFilter( optarg );
            }
            break;
        }

    }

    while ( optind < argc ) {
        addFilter( argv[optind++] );
    }

    return 0;
}


void SSTInfoConfig::addFilter(std::string name)
{
    if ( name.size() > 3 && name.substr(0, 3) == "lib" )
        name = name.substr(3);

    size_t dotLoc = name.find(".");
    if ( dotLoc == std::string::npos ) {
        m_filters.insert(std::make_pair(name, ""));
    } else {
        m_filters.insert(std::make_pair(
                    std::string(name, 0, dotLoc),
                    std::string(name, dotLoc+1)));
    }

}


bool doesLibHaveFilters(const std::string &libName)
{
    auto range = g_configuration.getFilterMap().equal_range(libName);
    for ( auto x = range.first ; x != range.second ; ++x ) {
        if ( x->second != "" )
            return true;
    }
    return false;
}

bool shouldPrintElement(const std::string &libName, const std::string elemName)
{
    auto range = g_configuration.getFilterMap().equal_range(libName);
    if ( range.first == range.second )
        return true;
    for ( auto x = range.first ; x != range.second ; ++x ) {
        if ( x->second == "" )
            return true;
        if ( x->second == elemName )
            return true;
    }
    return false;
}

template <class BaseType>
void SSTLibraryInfo::outputHumanReadable(std::ostream& os, bool printAll)
{
  auto* lib = ELI::InfoDatabase::getLibrary<BaseType>(getLibraryName());
  if (lib){
    os << "Num " << BaseType::ELI_baseName() << "s = " << lib->numEntries() << "\n";
    int idx = 0;
    for (auto& pair : lib->getMap()){
      bool print = printAll || shouldPrintElement(getLibraryName(), pair.first);
      if (print){
        os << "      " << BaseType::ELI_baseName() << " " << idx << ": " << pair.first << "\n";
        pair.second->toString(os);
      }
      ++idx;
    }
  } else {
    os << "No " << BaseType::ELI_baseName() << "s\n";
  }
}

void
SSTLibraryInfo::outputHumanReadable(std::ostream& os, int LibIndex)
{
    bool enableFullElementOutput = !doesLibHaveFilters(getLibraryName());

    os << "================================================================================\n";
    os << "ELEMENT " << LibIndex << " = " << getLibraryName()
       << " (" << getLibraryDescription() << ")" << "\n";

    outputHumanReadable<Component>(os, enableFullElementOutput);
    outputHumanReadable<SubComponent>(os, enableFullElementOutput);
    outputHumanReadable<Module>(os, enableFullElementOutput);
    //outputHumanReadable(); events
    outputHumanReadable<SST::Partition::SSTPartitioner>(os, enableFullElementOutput);
}

template <class BaseType>
void SSTLibraryInfo::outputXML(TiXmlElement* XMLLibraryElement)
{
  auto* lib = ELI::InfoDatabase::getLibrary<BaseType>(getLibraryName());
  if (lib){
    int numObjects = lib->numEntries();
    xmlComment(XMLLibraryElement, "Num %ss = %d", BaseType::ELI_baseName(), numObjects);
    int idx = 0;
    for (auto& pair : lib->getMap()){
      TiXmlElement* XMLElement = new TiXmlElement("Component");
      XMLElement->SetAttribute("Index", idx);
      pair.second->outputXML(XMLElement);
      XMLLibraryElement->LinkEndChild(XMLElement);
      idx++;
    }
  } else {
    xmlComment(XMLLibraryElement, "No %ss", BaseType::ELI_baseName());
  }
}

void
SSTLibraryInfo::outputXML(int LibIndex, TiXmlNode *XMLParentElement)
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


