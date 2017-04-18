// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
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
#include <sst/core/element.h>
#include "sst/core/build_info.h"

#include "sst/core/env/envquery.h"
#include "sst/core/env/envconfig.h"

#include <sst/core/tinyxml/tinyxml.h>
#include <sst/core/sstinfo.h>


using namespace std;
using namespace SST;
using namespace SST::Core;

// Global Variables
int                                      g_fileProcessedCount;
std::string                              g_searchPath;
std::vector<SSTInfoElement_LibraryInfo>  g_libInfoArray;
SSTInfoConfig                            g_configuration;

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

    const ElementLibraryInfo* pELI = (lib == "SST_CORE") ?
        loader.loadCoreInfo() :
        loader.loadLibrary(lib, g_configuration.debugEnabled());
    if ( pELI != NULL ) {
        if ( g_configuration.debugEnabled() )
            fprintf(stdout, "Found!\n");
        // Build
        g_fileProcessedCount++;
        g_libInfoArray.emplace_back(pELI);
    } else if ( !optional ) {
        fprintf(stderr, "**** WARNING - UNABLE TO PROCESS LIBRARY = %s\n", lib.c_str());
    } else if ( g_configuration.debugEnabled() ) {
        fprintf(stdout, "**** Not Found!\n");
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
        processLibs.insert("SST_CORE"); // Core libraries
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


void outputSSTElementInfo()
{
    fprintf (stdout, "PROCESSED %d .so (SST ELEMENT) FILES FOUND IN DIRECTORY(s) %s\n", g_fileProcessedCount, g_searchPath.c_str());

    // Tell the user what Elements will be displayed
    for ( auto &i : g_configuration.getFilterMap() ) {
        fprintf(stdout, "Filtering output on Element = \"%s", i.first.c_str());
        if ( !i.second.empty() )
            fprintf(stdout, ".%s", i.second.c_str());
        fprintf(stdout, "\"\n");
    }

    // Now dump the Library Info
    for (size_t x = 0; x < g_libInfoArray.size(); x++) {
        g_libInfoArray[x].outputLibraryInfo(x);
    }
}

void generateXMLOutputFile()
{
    unsigned int            x;
    char                    Comment[256];
    char                    TimeStamp[32];
    std::time_t             now = std::time(NULL);
    std::tm*                ptm = std::localtime(&now);
    FILE* pFile;
    
    // Check to see that the file path is valid by trying to create the file
    pFile = fopen (g_configuration.getXMLFilePath().c_str() , "w+");
    if (pFile == NULL) {
        fprintf(stderr, "\n");
        fprintf(stderr, "================================================================================\n");
        fprintf(stderr, "ERROR: Unable to create XML File %s\n", g_configuration.getXMLFilePath().c_str());
        fprintf(stderr, "================================================================================\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "\n");
        return;
    } else {
     fclose (pFile);
    }
    
    // Create a Timestamp Format: 2015.02.15_20:20:00
    std::strftime(TimeStamp, 32, "%Y.%m.%d_%H:%M:%S", ptm);
    
    fprintf(stdout, "\n");
    fprintf(stdout, "================================================================================\n");
    fprintf(stdout, "GENERATING XML FILE SSTInfo.xml as %s\n", g_configuration.getXMLFilePath().c_str());
    fprintf(stdout, "================================================================================\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
    
    // Create the XML Document     
    TiXmlDocument XMLDocument;

    // XML Declaration
	TiXmlDeclaration* XMLDecl = new TiXmlDeclaration("1.0", "", "");
	
	// General Info on the Data
	sprintf(Comment, "SSTInfo XML Data Generated on %s", TimeStamp);
	TiXmlComment* XMLStartComment = new TiXmlComment(Comment);

    sprintf (Comment, "%d .so FILES FOUND IN DIRECTORY(s) %s\n", g_fileProcessedCount, g_searchPath.c_str());
	TiXmlComment* XMLNumElementsComment = new TiXmlComment(Comment);

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
        g_libInfoArray[x].generateLibraryInfoXMLData(x, XMLTopLevelElement);
    }

	// Add the entries into the XML Document
	XMLDocument.LinkEndChild(XMLDecl);
	XMLDocument.LinkEndChild(XMLStartComment);
	XMLDocument.LinkEndChild(XMLNumElementsComment);
	XMLDocument.LinkEndChild(XMLTopLevelElement);
    
    // Save the XML Document
	XMLDocument.SaveFile(g_configuration.getXMLFilePath().c_str());
}


SSTInfoConfig::SSTInfoConfig()
{
    m_optionBits = CFG_OUTPUTHUMAN;  // Enable normal output by default
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
    cout << "  -l, --libs=LIBS          {all, <elementname>} - Element Library9(s) to process\n";
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
        char c = getopt_long(argc, argv, "hvdnxo:l:", longOpts, &opt_idx);
        if ( c == -1 )
            break;

        switch (c) {
        case 'h':
            outputUsage();
            return 1;
        case 'v':
            outputVersion();
            return 1;
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


void SSTInfoElement_LibraryInfo::populateLibraryInfo()
{
    const ElementInfoComponent*    eic;
    const ElementInfoEvent*        eie;
    const ElementInfoModule*       eim;
    const ElementInfoSubComponent* eisc;
    const ElementInfoPartitioner*  eip;
    const ElementInfoGenerator*    eig;

    // Are there any Components
    if (NULL != m_eli->components) {
        // Get a pointer to the array
        eic = m_eli->components;
        // If the name is NULL, we have reached the last item
        while (NULL != eic->name) {
            addInfoComponent(eic);
            eic++;  // Get the next item
        }
    }

    // Are there any Events
    if (NULL != m_eli->events) {
        // Get a pointer to the array
        eie = m_eli->events;
        // If the name is NULL, we have reached the last item
        while (NULL != eie->name) {
            addInfoEvent(eie);
            eie++;  // Get the next item
        }
    }

    // Are there any Modules
    if (NULL != m_eli->modules) {
        // Get a pointer to the array
        eim = m_eli->modules;
        // If the name is NULL, we have reached the last item
        while (NULL != eim->name) {
            addInfoModule(eim);
            eim++;  // Get the next item
        }
    }

    // Are there any SubComponents
    if (NULL != m_eli->subcomponents) {
        // Get a pointer to the array
        eisc = m_eli->subcomponents;
        // If the name is NULL, we have reached the last item
        while (NULL != eisc->name) {
            addInfoSubComponent(eisc);
            eisc++;  // Get the next item
        }
    }

    // Are there any Partitioners
    if (NULL != m_eli->partitioners) {
        // Get a pointer to the array
        eip = m_eli->partitioners;
        // If the name is NULL, we have reached the last item
        while (NULL != eip->name) {
            addInfoPartitioner(eip);
            eip++;  // Get the next item
        }
    }

    // Are there any Generators
    if (NULL != m_eli->generators) {
        // Get a pointer to the array
        eig = m_eli->generators;
        // If the name is NULL, we have reached the last item
        while (NULL != eig->name) {
            addInfoGenerator(eig);
            eig++;  // Get the next item
        }
    }

    LibraryInfo* lib = ElementLibraryDatabase::getLibraryInfo(m_eli->name);
    if ( lib ) {
        for ( auto &i : lib->components )
            addInfoComponent(i.second);
        for ( auto &i : lib->subcomponents )
            addInfoSubComponent(i.second);
        for ( auto &i : lib->modules )
            addInfoModule(i.second);
        for ( auto &i : lib->partitioners )
            addInfoPartitioner(i.second);
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

void SSTInfoElement_LibraryInfo::outputLibraryInfo(int LibIndex)
{
    int                          x;
    int                          numObjects;
    SSTInfoElement_ComponentInfo*    eic;
    SSTInfoElement_EventInfo*        eie;
    SSTInfoElement_ModuleInfo*       eim;
    SSTInfoElement_SubComponentInfo* eisc;
    SSTInfoElement_PartitionerInfo*  eip;
    SSTInfoElement_GeneratorInfo*    eig;

    bool enableFullElementOutput = !doesLibHaveFilters(getLibraryName());

    fprintf(stdout, "================================================================================\n");
    fprintf(stdout, "ELEMENT %d = %s (%s)\n", LibIndex, getLibraryName().c_str(), getLibraryDescription().c_str());

    // Are we to output the Full Element Information
    if (true == enableFullElementOutput) {

        numObjects = getNumberOfLibraryComponents();
        fprintf(stdout, "   NUM COMPONENTS    = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eic = getInfoComponent(x);
            eic->outputComponentInfo(x);
        }

        numObjects = getNumberOfLibraryEvents();
        fprintf(stdout, "   NUM EVENTS        = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eie = getInfoEvent(x);
            eie->outputEventInfo(x);
        }

        numObjects = getNumberOfLibraryModules();
        fprintf(stdout, "   NUM MODULES       = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eim = getInfoModule(x);
            eim->outputModuleInfo(x);
        }

        numObjects = getNumberOfLibrarySubComponents();
        fprintf(stdout, "   NUM SUBCOMPONENTS = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eisc = getInfoSubComponent(x);
            eisc->outputSubComponentInfo(x);
        }

        numObjects = getNumberOfLibraryPartitioners();
        fprintf(stdout, "   NUM PARTITIONERS  = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eip = getInfoPartitioner(x);
            eip->outputPartitionerInfo(x);
        }

        numObjects = getNumberOfLibraryGenerators();
        fprintf(stdout, "   NUM GENERATORS    = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eig = getInfoGenerator(x);
            eig->outputGeneratorInfo(x);
        }
    } else {
        // Check each component one at a time
        numObjects = getNumberOfLibraryComponents();
        for (x = 0; x < numObjects; x++) {

            eic = getInfoComponent(x);
            // See if this component is to be outputed
            if (shouldPrintElement(getLibraryName(), eic->getName())) {
                eic->outputComponentInfo(x);
            }
        }

        numObjects = getNumberOfLibrarySubComponents();
        for (x = 0; x < numObjects; x++) {

            eisc = getInfoSubComponent(x);
            // See if this component is to be outputed
            if (shouldPrintElement(getLibraryName(), eisc->getName())) {
                eisc->outputSubComponentInfo(x);
            }
        }

    }
}

void SSTInfoElement_LibraryInfo::generateLibraryInfoXMLData(int LibIndex, TiXmlNode* XMLParentElement)
{
    int                          x;
    int                          numObjects;
    SSTInfoElement_ComponentInfo*    eic;
    SSTInfoElement_EventInfo*        eie;
    SSTInfoElement_ModuleInfo*       eim;
//    SSTInfoElement_SubComponentInfo* eisc;
    SSTInfoElement_PartitionerInfo*  eip;
    SSTInfoElement_GeneratorInfo*    eig;
    char                         Comment[256];
    
    // Build the Element to Represent the Library
	TiXmlElement* XMLLibraryElement = new TiXmlElement("Element");
	XMLLibraryElement->SetAttribute("Index", LibIndex);
	XMLLibraryElement->SetAttribute("Name", getLibraryName().c_str());
	XMLLibraryElement->SetAttribute("Description", getLibraryDescription().c_str());

	// Get the Num Objects and Display an XML comment about them
    numObjects = getNumberOfLibraryComponents();
	sprintf(Comment, "NUM COMPONENTS = %d", numObjects);
	TiXmlComment* XMLLibComponentsComment = new TiXmlComment(Comment);
	XMLLibraryElement->LinkEndChild(XMLLibComponentsComment);
    for (x = 0; x < numObjects; x++) {
        eic = getInfoComponent(x);
        eic->generateComponentInfoXMLData(x, XMLLibraryElement);
    }

    numObjects = getNumberOfLibraryEvents();        
	sprintf(Comment, "NUM EVENTS = %d", numObjects);
	TiXmlComment* XMLLibEventsComment = new TiXmlComment(Comment);
	XMLLibraryElement->LinkEndChild(XMLLibEventsComment);
    for (x = 0; x < numObjects; x++) {
        eie = getInfoEvent(x);
        eie->generateEventInfoXMLData(x, XMLLibraryElement);
    }

    numObjects = getNumberOfLibraryModules();       
	sprintf(Comment, "NUM MODULES = %d", numObjects);
	TiXmlComment* XMLLibModulesComment = new TiXmlComment(Comment);
	XMLLibraryElement->LinkEndChild(XMLLibModulesComment);
    for (x = 0; x < numObjects; x++) {
        eim = getInfoModule(x);
        eim->generateModuleInfoXMLData(x, XMLLibraryElement);
    }

// TODO: Dump SubComponent info to XML.  Turned off for 5.0 since SSTWorkbench
//       chokes if format is changed.  
//    numObjects = getNumberOfLibrarySubComponents();
//    sprintf(Comment, "NUM SUBCOMPONENTS = %d", numObjects);
//    TiXmlComment* XMLLibSubComponentsComment = new TiXmlComment(Comment);
//    XMLLibraryElement->LinkEndChild(XMLLibSubComponentsComment);
//    for (x = 0; x < numObjects; x++) {
//        eisc = getInfoSubComponent(x);
//        eisc->generateSubComponentInfoXMLData(x, XMLLibraryElement);
//    }
    
    numObjects = getNumberOfLibraryPartitioners();  
	sprintf(Comment, "NUM PARTITIONERS = %d", numObjects);
	TiXmlComment* XMLLibPartitionersComment = new TiXmlComment(Comment);
	XMLLibraryElement->LinkEndChild(XMLLibPartitionersComment);
    for (x = 0; x < numObjects; x++) {
        eip = getInfoPartitioner(x);
        eip->generatePartitionerInfoXMLData(x, XMLLibraryElement);
    }

    numObjects = getNumberOfLibraryGenerators();    
	sprintf(Comment, "NUM GENERATORS = %d", numObjects);
	TiXmlComment* XMLLibGeneratorsComment = new TiXmlComment(Comment);
	XMLLibraryElement->LinkEndChild(XMLLibGeneratorsComment);
    for (x = 0; x < numObjects; x++) {
        eig = getInfoGenerator(x);
        eig->generateGeneratorInfoXMLData(x, XMLLibraryElement);
    }

    // Add this Library Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLLibraryElement);
}

void SSTInfoElement_ParamInfo::outputParameterInfo(int index)
{
    fprintf(stdout, "            PARAMETER %d = %s (%s) [%s]\n", index, getName().c_str(), getDesc().c_str(), getDefault().c_str());
}

void SSTInfoElement_ParamInfo::generateParameterInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Parameter
	TiXmlElement* XMLParameterElement = new TiXmlElement("Parameter");
	XMLParameterElement->SetAttribute("Index", Index);
	XMLParameterElement->SetAttribute("Name", getName().c_str());
	XMLParameterElement->SetAttribute("Description", getDesc().c_str());
	XMLParameterElement->SetAttribute("Default", getDefault().c_str());

    // Add this Parameter Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLParameterElement);
}

void SSTInfoElement_PortInfo::outputPortInfo(int index)
{
    fprintf(stdout, "            PORT %d [%zu Valid Events] = %s (%s)\n", index,  m_validEvents.size(), getName().c_str(), getDesc().c_str());

    // Print out the Valid Events Info
    for (unsigned int x = 0; x < m_validEvents.size(); x++) {
        fprintf(stdout, "               VALID EVENT %d = %s\n", x, getValidEvent(x).c_str());
    }
}

void SSTInfoElement_PortInfo::generatePortInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    char          Comment[256];
    TiXmlElement* XMLValidEventElement;

    // Build the Element to Represent the Component
	TiXmlElement* XMLPortElement = new TiXmlElement("Port");
	XMLPortElement->SetAttribute("Index", Index);
	XMLPortElement->SetAttribute("Name", getName().c_str());
	XMLPortElement->SetAttribute("Description", getDesc().c_str());

	// Get the Num Valid Event and Display an XML comment about them
    sprintf(Comment, "NUM Valid Events = %zu", m_validEvents.size());
    TiXmlComment* XMLValidEventsComment = new TiXmlComment(Comment);
    XMLPortElement->LinkEndChild(XMLValidEventsComment);
	
    for (unsigned int x = 0; x < m_validEvents.size(); x++) {
        // Build the Element to Represent the ValidEvent
        XMLValidEventElement = new TiXmlElement("PortValidEvent");
        XMLValidEventElement->SetAttribute("Index", x);
        XMLValidEventElement->SetAttribute("Event", getValidEvent(x).c_str());
        
        // Add the ValidEvent element to the Port Element
        XMLPortElement->LinkEndChild(XMLValidEventElement);
    }

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLPortElement);
}



void SSTInfoElement_StatisticInfo::outputStatisticInfo(int index)
{
    fprintf(stdout, "            STATISTIC %d = %s [%s] (%s) Enable Level = %d\n", index, getName().c_str(), getUnits().c_str(), getDesc().c_str(), getEnableLevel());
}

void SSTInfoElement_StatisticInfo::generateStatisticXMLData(int UNUSED(Index), TiXmlNode* UNUSED(XMLParentElement))
{
// TODO: Dump Statistic info to XML.  Turned off for 5.0 since SSTWorkbench
//       chokes if format is changed.  
//    // Build the Element to Represent the Parameter
//    TiXmlElement* XMLStatElement = new TiXmlElement("Statistic");
//    XMLStatElement->SetAttribute("Index", Index);
//    XMLStatElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
//    XMLStatElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());
//    XMLStatElement->SetAttribute("Units", (NULL == getUnits()) ? "" : getUnits());
//    XMLStatElement->SetAttribute("EnableLevel", getEnableLevel());
//
//    // Add this Parameter Element to the Parent Element
//    XMLParentElement->LinkEndChild(XMLStatElement);
}

void SSTInfoElement_ComponentInfo::outputComponentInfo(int index)
{
    // Print out the Component Info
    fprintf(stdout, "      COMPONENT %d = %s [%s] (%s)\n", index, getName().c_str(), getCategoryString().c_str(), getDesc().c_str());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %ld\n", m_ParamArray.size());
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputParameterInfo(x);
    }

    // Print out the Port Info
    fprintf(stdout, "         NUM PORTS = %ld\n", m_PortArray.size());
    for (unsigned int x = 0; x < m_PortArray.size(); x++) {
        getPortInfo(x)->outputPortInfo(x);
    }

    // Print out the Port Info
    fprintf(stdout, "         NUM STATISTICS = %ld\n", m_StatisticArray.size());
    for (unsigned int x = 0; x < m_StatisticArray.size(); x++) {
        getStatisticInfo(x)->outputStatisticInfo(x);
    }
}

void SSTInfoElement_ComponentInfo::generateComponentInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    char Comment[256];

    // Build the Element to Represent the Component
	TiXmlElement* XMLComponentElement = new TiXmlElement("Component");
	XMLComponentElement->SetAttribute("Index", Index);
	XMLComponentElement->SetAttribute("Name", getName().c_str());
	XMLComponentElement->SetAttribute("Description", getDesc().c_str());
	XMLComponentElement->SetAttribute("Category", getCategoryString().c_str());

	// Get the Num Parameters and Display an XML comment about them
    sprintf(Comment, "NUM PARAMETERS = %ld", m_ParamArray.size());
    TiXmlComment* XMLParamsComment = new TiXmlComment(Comment);
    XMLComponentElement->LinkEndChild(XMLParamsComment);
	
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->generateParameterInfoXMLData(x, XMLComponentElement);
    }

    // Get the Num Ports and Display an XML comment about them
    sprintf(Comment, "NUM PORTS = %ld", m_PortArray.size());
    TiXmlComment* XMLPortsComment = new TiXmlComment(Comment);
    XMLComponentElement->LinkEndChild(XMLPortsComment);
	
    for (unsigned int x = 0; x < m_PortArray.size(); x++) {
        getPortInfo(x)->generatePortInfoXMLData(x, XMLComponentElement);
    }

	// Get the Num Statistics and Display an XML comment about them
    sprintf(Comment, "NUM STATISTICS = %ld", m_StatisticArray.size());
    TiXmlComment* XMLStatComment = new TiXmlComment(Comment);
    XMLComponentElement->LinkEndChild(XMLStatComment);
	
    for (unsigned int x = 0; x < m_StatisticArray.size(); x++) {
        getStatisticInfo(x)->generateStatisticXMLData(x, XMLComponentElement);
    }

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLComponentElement);
}

std::string SSTInfoElement_ComponentInfo::getCategoryString() const
{
    static const struct {
        uint32_t key;
        std::string txt;
    } table [] = {
        {COMPONENT_CATEGORY_PROCESSOR, "PROCESSOR COMPONENT"},
        {COMPONENT_CATEGORY_MEMORY,    "MEMORY COMPONENT"},
        {COMPONENT_CATEGORY_NETWORK,   "NETWORK COMPONENT"},
        {COMPONENT_CATEGORY_SYSTEM,    "SYSTEM COMPONENT"}
    };
    std::string res;

    // Build a string list of the component catagories assigned to the component
    if (0 < m_category) {
        for ( size_t i = 0 ; i < (sizeof(table)/sizeof(table[0])); i++ ) {
            if ( m_category & table[i].key ) {
                if (0 != res.length()) {
                    res += ", ";
                }
                res += table[i].txt;
            }
        }
    } else {
        res = "UNCATEGORIZED COMPONENT";
    }
    return res;
}


void SSTInfoElement_EventInfo::outputEventInfo(int index)
{
    fprintf(stdout, "      EVENT %d = %s (%s)\n", index, getName().c_str(), getDesc().c_str());
}

void SSTInfoElement_EventInfo::generateEventInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Event
	TiXmlElement* XMLEventElement = new TiXmlElement("Event");
	XMLEventElement->SetAttribute("Index", Index);
	XMLEventElement->SetAttribute("Name", getName().c_str());
	XMLEventElement->SetAttribute("Description", getDesc().c_str());

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLEventElement);
}

void SSTInfoElement_ModuleInfo::outputModuleInfo(int index)
{
    fprintf(stdout, "      MODULE %d = %s (%s) {%s}\n", index, getName().c_str(), getDesc().c_str(), getProvides().c_str());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %zu\n", m_ParamArray.size());
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputParameterInfo(x);
    }
}

void SSTInfoElement_ModuleInfo::generateModuleInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    char Comment[256];

    // Build the Element to Represent the Module
	TiXmlElement* XMLModuleElement = new TiXmlElement("Module");
	XMLModuleElement->SetAttribute("Index", Index);
	XMLModuleElement->SetAttribute("Name", getName().c_str());
	XMLModuleElement->SetAttribute("Description", getDesc().c_str());
	XMLModuleElement->SetAttribute("Provides", getProvides().c_str());
	
	// Get the Num Parameters and Display an XML comment about them
    sprintf(Comment, "NUM PARAMETERS = %zu", m_ParamArray.size());
    TiXmlComment* XMLParamsComment = new TiXmlComment(Comment);
    XMLModuleElement->LinkEndChild(XMLParamsComment);
	
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->generateParameterInfoXMLData(x, XMLModuleElement);
    }

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLModuleElement);
}

void SSTInfoElement_SubComponentInfo::outputSubComponentInfo(int index)
{
    // Print out the Component Info
    fprintf(stdout, "      SUBCOMPONENT %d = %s (%s)\n", index, getName().c_str(), getDesc().c_str());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %zu\n", m_ParamArray.size());
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputParameterInfo(x);
    }

    // Print out the Port Info
    fprintf(stdout, "         NUM STATISTICS = %zu\n", m_StatisticArray.size());
    for (unsigned int x = 0; x < m_StatisticArray.size(); x++) {
        getStatisticInfo(x)->outputStatisticInfo(x);
    }
}

void SSTInfoElement_SubComponentInfo::generateSubComponentInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    char Comment[256];

    // Build the Element to Represent the Component
	TiXmlElement* XMLSubComponentElement = new TiXmlElement("SubComponent");
	XMLSubComponentElement->SetAttribute("Index", Index);
	XMLSubComponentElement->SetAttribute("Name", getName().c_str());
	XMLSubComponentElement->SetAttribute("Description", getDesc().c_str());

	// Get the Num Parameters and Display an XML comment about them
    sprintf(Comment, "NUM PARAMETERS = %zu", m_ParamArray.size());
    TiXmlComment* XMLParamsComment = new TiXmlComment(Comment);
    XMLSubComponentElement->LinkEndChild(XMLParamsComment);
	
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->generateParameterInfoXMLData(x, XMLSubComponentElement);
    }

	// Get the Num Statistics and Display an XML comment about them
    sprintf(Comment, "NUM STATISTICS = %zu", m_StatisticArray.size());
    TiXmlComment* XMLStatComment = new TiXmlComment(Comment);
    XMLSubComponentElement->LinkEndChild(XMLStatComment);
	
    for (unsigned int x = 0; x < m_StatisticArray.size(); x++) {
        getStatisticInfo(x)->generateStatisticXMLData(x, XMLSubComponentElement);
    }

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLSubComponentElement);
}

void SSTInfoElement_PartitionerInfo::outputPartitionerInfo(int index)
{
    fprintf(stdout, "      PARTITIONER %d = %s (%s)\n", index, getName().c_str(), getDesc().c_str());
}

void SSTInfoElement_PartitionerInfo::generatePartitionerInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Partitioner
	TiXmlElement* XMLPartitionerElement = new TiXmlElement("Partitioner");
	XMLPartitionerElement->SetAttribute("Index", Index);
	XMLPartitionerElement->SetAttribute("Name", getName().c_str());
	XMLPartitionerElement->SetAttribute("Description", getDesc().c_str());
	
    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLPartitionerElement);
}

void SSTInfoElement_GeneratorInfo::outputGeneratorInfo(int index)
{
    fprintf(stdout, "      GENERATOR %d = %s (%s)\n", index, getName().c_str(), getDesc().c_str());
}

void SSTInfoElement_GeneratorInfo::generateGeneratorInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Generator
	TiXmlElement* XMLGeneratorElement = new TiXmlElement("Generator");
	XMLGeneratorElement->SetAttribute("Index", Index);
	XMLGeneratorElement->SetAttribute("Name", getName().c_str());
	XMLGeneratorElement->SetAttribute("Description", getDesc().c_str());
	
    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLGeneratorElement);
}



