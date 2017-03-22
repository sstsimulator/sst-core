// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

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
static void processSSTElementFiles(std::string searchPath);
bool areOutputFiltersEnabled();
bool testNameAgainstOutputFilters(std::string testElemName, std::string testCompName);
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
    processSSTElementFiles(g_searchPath);

    return 0;
}


static void addELI(ElemLoader &loader, const std::string &lib, bool optional)
{

    if ( g_configuration.debugEnabled() )
        fprintf (stdout, "Looking for library \"%s\"\n", lib.c_str());

    const ElementLibraryInfo* pELI = (lib == "libsst") ?
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


static void processSSTElementFiles(std::string searchPath)
{
    std::vector<bool>       EntryProcessedArray;
    ElemLoader              loader(g_searchPath);

    std::vector<std::string> potentialLibs = loader.getPotentialElements();

    // Which libraries should we (attempt) to process
    std::vector<std::string> processLibs;
    if ( g_configuration.processAllElements() ) {
        processLibs = potentialLibs;
        processLibs.push_back("libsst"); // Core libraries
    } else {
        processLibs = *g_configuration.getElementsToProcessArray();
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

bool areOutputFiltersEnabled()
{
    int filteredElementArraySize = g_configuration.getFilteredElementNamesArray()->size();
    int filteredElemCompArraySize = g_configuration.getFilteredElementComponentNamesArray()->size();

    // Check if there are no Filters
    if ((0 == filteredElementArraySize) && (0 == filteredElemCompArraySize)) {
        // A filters exists, so we need return true to indicate that the 
        // output filters are enabled.
        return false;
    }
    return true;
}

bool testNameAgainstOutputFilters(std::string testElemName, std::string testCompName)
{
    int x;
    int filteredElementArraySize = g_configuration.getFilteredElementNamesArray()->size();
    int filteredElemCompArraySize = g_configuration.getFilteredElementComponentNamesArray()->size();
    std::string filterName;
    std::string elemCompName;

    // Check to see if the Output Filters are disabled
    if (false == areOutputFiltersEnabled()) {
        // Filters are Disabled, so always allow the output 
        return true;
    }

    // Check the Element Name, against the Element Filter list, if there is
    // a match, then allow the Element to be displayed.  
    // NOTE: The Element name is also passed in when checking a component
    //       if the Element is in both Filter lists, the Element will always be
    //       displayed
    for (x = 0; x < filteredElementArraySize; x++) {
        filterName = g_configuration.getFilteredElementNamesArray()->at(x);
        if (testElemName == filterName) {
            return true;
        }
    }

    elemCompName = testElemName + "." + testCompName; 
    // Check the Element.Component Name, against the Element.Component Filter list, 
    // if there is a match, then allow the Component to be displayed.  
    for (x = 0; x < filteredElemCompArraySize; x++) {
        filterName = g_configuration.getFilteredElementComponentNamesArray()->at(x);
        if (elemCompName == filterName) {
            return true;
        }
    }

    // The Name
    return false;    
}

void outputSSTElementInfo()
{
    unsigned int            x;

    fprintf (stdout, "PROCESSED %d .so (SST ELEMENT) FILES FOUND IN DIRECTORY(s) %s\n", g_fileProcessedCount, g_searchPath.c_str());

    // Tell the user what Elements will be displayed
    if (g_configuration.getFilteredElementNamesArray()->size() > 0) {
        for (x = 0; x < g_configuration.getFilteredElementNamesArray()->size(); x++) {
            fprintf (stdout, "Filtering output on Element = \"%s\"\n", g_configuration.getFilteredElementNamesArray()->at(x).c_str());
        }
    }
    // Tell the user what Element.Component will be displayed
    if (g_configuration.getFilteredElementComponentNamesArray()->size() > 0) {
        for (x = 0; x < g_configuration.getFilteredElementComponentNamesArray()->size(); x++) {
            fprintf (stdout, "Filtering output on Element.Component|SubComponent = \"%s\"\n", g_configuration.getFilteredElementComponentNamesArray()->at(x).c_str());
        }
    }

    // Now dump the Library Info
    for (x = 0; x < g_libInfoArray.size(); x++) {
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
            m_elementsToProcess.push_back( optarg );

            std::string fullLibraryName = "";
            std::string libraryName = optarg;

            // See if "lib" is at front of the library name; if not, append to full name
            if ( libraryName.compare(0, 3, "lib") )
                fullLibraryName = "lib";

            // Add the Library Name to the full name
            fullLibraryName += libraryName;

            // Write the full name back to the m_elementsToProcess
            m_elementsToProcess.push_back(fullLibraryName);
            break;
        }
        case 0:
            if ( !strcmp(longOpts[opt_idx].name, "elemnfilt" ) ) {
                std::string arg = optarg;
                // Look the Name over and decide if it is an element name only or a 
                // element.component name, and then add it to the appropriate list
                if ( arg.find(".") != string::npos ) {
                    // We found a "." in the name so assume that this is a element.component
                    m_filteredElementComponentNames.push_back(arg);
                } else {
                    // No "." found, this is an element only name
                    m_filteredElementNames.push_back(arg);
                }
            }
            break;
        }

    }

    while ( optind < argc ) {
        std::string arg = argv[optind++];
        if ( arg.find(".") != string::npos ) {
            // We found a "." in the name so assume that this is a element.component
            m_filteredElementComponentNames.push_back(arg);
        } else {
            // No "." found, this is an element only name
            m_filteredElementNames.push_back(arg);
        }
    }

    // Get the List of libs that the user may trying to use
    m_processAllElements = m_elementsToProcess.empty();

    return 0;
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
    bool                         enableFullElementOutput = true;
    bool                         elemHeaderBeenOutput = false;

    // Test to see if the Output Filters are Enabled, if yes, then
    // we need to do some more checking
    if (true == areOutputFiltersEnabled()) {
        // Test this element name to see if it is in the filter list, if yes
        // then we allow its full output
        enableFullElementOutput = testNameAgainstOutputFilters(getLibraryName(), "");
    }

    // Are we to output the Full Element Information
    if (true == enableFullElementOutput) {
       
        fprintf(stdout, "================================================================================\n");
        fprintf(stdout, "ELEMENT %d = %s (%s)\n", LibIndex, getLibraryName().c_str(), getLibraryDescription().c_str());
        
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
            if (true == testNameAgainstOutputFilters(getLibraryName(), eic->getName())) {
            
                // Check to see if we have output'ed the Element header the at least once                 
                if (false == elemHeaderBeenOutput) {
                    fprintf(stdout, "================================================================================\n");
                    fprintf(stdout, "ELEMENT %d = %s (%s)\n", LibIndex, getLibraryName().c_str(), getLibraryDescription().c_str());
                    elemHeaderBeenOutput = true;
                }
                
                eic->outputComponentInfo(x);
            }
        }
        
        numObjects = getNumberOfLibrarySubComponents();
        for (x = 0; x < numObjects; x++) {
            
            eisc = getInfoSubComponent(x);
            
            // See if this component is to be outputed
            if (true == testNameAgainstOutputFilters(getLibraryName(), eisc->getName())) {
            
                // Check to see if we have output'ed the Element header the at least once                 
                if (false == elemHeaderBeenOutput) {
                    fprintf(stdout, "================================================================================\n");
                    fprintf(stdout, "ELEMENT %d = %s (%s)\n", LibIndex, getLibraryName().c_str(), getLibraryDescription().c_str());
                    elemHeaderBeenOutput = true;
                }
                
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
    fprintf(stdout, "            PARAMETER %d = %s (%s) [%s]\n", index, getName(), getDesc(), getDefault());
}

void SSTInfoElement_ParamInfo::generateParameterInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Parameter
	TiXmlElement* XMLParameterElement = new TiXmlElement("Parameter");
	XMLParameterElement->SetAttribute("Index", Index);
	XMLParameterElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLParameterElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());
	XMLParameterElement->SetAttribute("Default", (NULL == getDefault()) ? "" : getDefault());

    // Add this Parameter Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLParameterElement);
}

void SSTInfoElement_PortInfo::outputPortInfo(int index)
{
    fprintf(stdout, "            PORT %d [%d Valid Events] = %s (%s)\n", index,  m_numValidEvents, getName(), getDesc());

    // Print out the Valid Events Info
    for (unsigned int x = 0; x < m_numValidEvents; x++) {
        fprintf(stdout, "               VALID EVENT %d = %s\n", x, getValidEvent(x));
    }
}

void SSTInfoElement_PortInfo::generatePortInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    char          Comment[256];
    TiXmlElement* XMLValidEventElement;

    // Build the Element to Represent the Component
	TiXmlElement* XMLPortElement = new TiXmlElement("Port");
	XMLPortElement->SetAttribute("Index", Index);
	XMLPortElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLPortElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());

	// Get the Num Valid Event and Display an XML comment about them
    sprintf(Comment, "NUM Valid Events = %d", m_numValidEvents);
    TiXmlComment* XMLValidEventsComment = new TiXmlComment(Comment);
    XMLPortElement->LinkEndChild(XMLValidEventsComment);
	
    for (unsigned int x = 0; x < m_numValidEvents; x++) {
        // Build the Element to Represent the ValidEvent
        XMLValidEventElement = new TiXmlElement("PortValidEvent");
        XMLValidEventElement->SetAttribute("Index", x);
        XMLValidEventElement->SetAttribute("Event", (NULL == getValidEvent(x)) ? "" : getValidEvent(x));
        
        // Add the ValidEvent element to the Port Element
        XMLPortElement->LinkEndChild(XMLValidEventElement);
    }

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLPortElement);
}

void SSTInfoElement_PortInfo::analyzeValidEventsArray()
{
    const char** pValidEventStringArray;
    const char*  ValidEventText;
    unsigned int NumValidEvents = 0;

    // Populate the Valid Events Array
    if (NULL != m_elport->validEvents) {
        
        // Temp vars to point to the array of strings and the actual string
        pValidEventStringArray = m_elport->validEvents;
        ValidEventText         = *pValidEventStringArray;

        while (NULL != ValidEventText){
            NumValidEvents++;
            
            pValidEventStringArray++;  // Get the next ptr to the string
            ValidEventText = *pValidEventStringArray;
        }
    }
    m_numValidEvents = NumValidEvents;
}


const char* SSTInfoElement_PortInfo::getValidEvent(unsigned int index)
{
    const char** pValidEventStringArray;

    if (m_numValidEvents > index) {
        pValidEventStringArray = m_elport->validEvents;
        if (NULL != pValidEventStringArray[index]) {
            return pValidEventStringArray[index];
        }
    }

   // Illegal index value or NULL string at index, then just return NULL
   return NULL;
}

void SSTInfoElement_StatisticInfo::outputStatisticInfo(int index)
{
    fprintf(stdout, "            STATISTIC %d = %s [%s] (%s) Enable Level = %d\n", index, getName(), getUnits(), getDesc(), getEnableLevel());
}

void SSTInfoElement_StatisticInfo::generateStatisticXMLData(int Index, TiXmlNode* XMLParentElement)
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
    fprintf(stdout, "      COMPONENT %d = %s [%s] (%s)\n", index, getName(), getCategoryString(), getDesc());

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
	XMLComponentElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLComponentElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());
	XMLComponentElement->SetAttribute("Category", (NULL == getCategoryString()) ? "" : getCategoryString());

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

void SSTInfoElement_ComponentInfo::buildCategoryString()
{
    m_CategoryString.clear();
    
    // Build a string list of the component catagories assigned to the component
    if (0 < m_elc->category) {
        if (m_elc->category & COMPONENT_CATEGORY_PROCESSOR) {
            if (0 != m_CategoryString.length()) {
                m_CategoryString += ", ";  
            }
            m_CategoryString += "PROCESSOR COMPONENT";  
        }
        if (m_elc->category & COMPONENT_CATEGORY_MEMORY) {
            if (0 != m_CategoryString.length()) {
                m_CategoryString += ", ";  
            }
            m_CategoryString += "MEMORY COMPONENT";  
        }
        if (m_elc->category & COMPONENT_CATEGORY_NETWORK) {
            if (0 != m_CategoryString.length()) {
                m_CategoryString += ", ";  
            }
            m_CategoryString += "NETWORK COMPONENT";  
        }
        if (m_elc->category & COMPONENT_CATEGORY_SYSTEM) {
            if (0 != m_CategoryString.length()) {
                m_CategoryString += ", ";  
            }
            m_CategoryString += "SYSTEM COMPONENT";  
        }
        
    } else {
        m_CategoryString = "UNCATEGORIZED COMPONENT";
    }
}


void SSTInfoElement_EventInfo::outputEventInfo(int index)
{
    fprintf(stdout, "      EVENT %d = %s (%s)\n", index, getName(), getDesc());
}

void SSTInfoElement_EventInfo::generateEventInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Event
	TiXmlElement* XMLEventElement = new TiXmlElement("Event");
	XMLEventElement->SetAttribute("Index", Index);
	XMLEventElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLEventElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());
	
    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLEventElement);
}

void SSTInfoElement_ModuleInfo::outputModuleInfo(int index)
{
    fprintf(stdout, "      MODULE %d = %s (%s) {%s}\n", index, getName(), getDesc(), getProvides());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %ld\n", m_ParamArray.size());
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
	XMLModuleElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLModuleElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());
	XMLModuleElement->SetAttribute("Provides", (NULL == getProvides()) ? "" : getProvides());
	
	// Get the Num Parameters and Display an XML comment about them
    sprintf(Comment, "NUM PARAMETERS = %ld", m_ParamArray.size());
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
    fprintf(stdout, "      SUBCOMPONENT %d = %s (%s)\n", index, getName(), getDesc());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %ld\n", m_ParamArray.size());
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputParameterInfo(x);
    }

    // Print out the Port Info
    fprintf(stdout, "         NUM STATISTICS = %ld\n", m_StatisticArray.size());
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
	XMLSubComponentElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLSubComponentElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());

	// Get the Num Parameters and Display an XML comment about them
    sprintf(Comment, "NUM PARAMETERS = %ld", m_ParamArray.size());
    TiXmlComment* XMLParamsComment = new TiXmlComment(Comment);
    XMLSubComponentElement->LinkEndChild(XMLParamsComment);
	
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->generateParameterInfoXMLData(x, XMLSubComponentElement);
    }

	// Get the Num Statistics and Display an XML comment about them
    sprintf(Comment, "NUM STATISTICS = %ld", m_StatisticArray.size());
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
    fprintf(stdout, "      PARTITIONER %d = %s (%s)\n", index, getName(), getDesc());
}

void SSTInfoElement_PartitionerInfo::generatePartitionerInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Partitioner
	TiXmlElement* XMLPartitionerElement = new TiXmlElement("Partitioner");
	XMLPartitionerElement->SetAttribute("Index", Index);
	XMLPartitionerElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLPartitionerElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());
	
    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLPartitionerElement);
}

void SSTInfoElement_GeneratorInfo::outputGeneratorInfo(int index)
{
    fprintf(stdout, "      GENERATOR %d = %s (%s)\n", index, getName(), getDesc());
}

void SSTInfoElement_GeneratorInfo::generateGeneratorInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Generator
	TiXmlElement* XMLGeneratorElement = new TiXmlElement("Generator");
	XMLGeneratorElement->SetAttribute("Index", Index);
	XMLGeneratorElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLGeneratorElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());
	
    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLGeneratorElement);
}




