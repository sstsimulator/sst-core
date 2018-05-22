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




void dprintf(FILE *fp, const char *fmt, ...) {
    if ( g_configuration.doVerbose() ) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        va_end(args);
    }
}




class OverallOutputter : public SSTInfoElement_Outputter {
public:
    void outputHumanReadable(int index);
    void outputXML(int Index, TiXmlNode* XMLParentElement);
} g_Outputter;

ElementLibraryInfo info_empty_eli = {
    "",
    "",
    NULL,
    NULL,   // Events
    NULL,   // Introspectors
    NULL,
    NULL,
    NULL, // partitioners,
    NULL,  // Python Module Generator
    NULL // generators,
};

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

    const ElementLibraryInfo* pELI = (lib == "sst") ?
        loader.loadCoreInfo() :
        loader.loadLibrary(lib, g_configuration.debugEnabled());

    if ( NULL == pELI ) {
        // Check to see if this library loaded into the new ELI
        // Database
        if ( NULL != ElementLibraryDatabase::getLibraryInfo(lib) ) {
            pELI = &info_empty_eli;
        }
    }
    
    if ( pELI != NULL ) {
        if ( g_configuration.debugEnabled() )
            fprintf(stdout, "Found!\n");
        // Build
        g_fileProcessedCount++;
        g_libInfoArray.emplace_back(lib, pELI);
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
        processLibs.insert("sst"); // Core libraries
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
    g_Outputter.outputXML(0, NULL);
}

void outputSSTElementInfo()
{
    g_Outputter.outputHumanReadable(0);
}

void OverallOutputter::outputHumanReadable(int UNUSED(Index))
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
        g_libInfoArray[x].outputHumanReadable(x);
    }
}

void OverallOutputter::outputXML(int UNUSED(Index), TiXmlNode* UNUSED(XMLParentElement))
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
    if (m_name != "sst" && NULL != m_eli->partitioners) {
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

    LibraryInfo* lib = ElementLibraryDatabase::getLibraryInfo(m_name);
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

void SSTInfoElement_LibraryInfo::outputHumanReadable(int LibIndex)
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
            eic->outputHumanReadable(x);
        }

        numObjects = getNumberOfLibraryEvents();
        fprintf(stdout, "   NUM EVENTS        = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eie = getInfoEvent(x);
            eie->outputHumanReadable(x);
        }

        numObjects = getNumberOfLibraryModules();
        fprintf(stdout, "   NUM MODULES       = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eim = getInfoModule(x);
            eim->outputHumanReadable(x);
        }

        numObjects = getNumberOfLibrarySubComponents();
        fprintf(stdout, "   NUM SUBCOMPONENTS = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eisc = getInfoSubComponent(x);
            eisc->outputHumanReadable(x);
        }

        numObjects = getNumberOfLibraryPartitioners();
        fprintf(stdout, "   NUM PARTITIONERS  = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eip = getInfoPartitioner(x);
            eip->outputHumanReadable(x);
        }

        numObjects = getNumberOfLibraryGenerators();
        fprintf(stdout, "   NUM GENERATORS    = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eig = getInfoGenerator(x);
            eig->outputHumanReadable(x);
        }
    } else {
        // Check each component one at a time
        numObjects = getNumberOfLibraryComponents();
        for (x = 0; x < numObjects; x++) {

            eic = getInfoComponent(x);
            // See if this component is to be outputed
            if (shouldPrintElement(getLibraryName(), eic->getName())) {
                eic->outputHumanReadable(x);
            }
        }

        numObjects = getNumberOfLibrarySubComponents();
        for (x = 0; x < numObjects; x++) {

            eisc = getInfoSubComponent(x);
            // See if this component is to be outputed
            if (shouldPrintElement(getLibraryName(), eisc->getName())) {
                eisc->outputHumanReadable(x);
            }
        }

    }
}

void SSTInfoElement_LibraryInfo::outputXML(int LibIndex, TiXmlNode* XMLParentElement)
{
    int                          x;
    int                          numObjects;
    SSTInfoElement_ComponentInfo*    eic;
    SSTInfoElement_EventInfo*        eie;
    SSTInfoElement_ModuleInfo*       eim;
    SSTInfoElement_SubComponentInfo* eisc;
    SSTInfoElement_PartitionerInfo*  eip;
    SSTInfoElement_GeneratorInfo*    eig;

    // Build the Element to Represent the Library
	TiXmlElement* XMLLibraryElement = new TiXmlElement("Element");
	XMLLibraryElement->SetAttribute("Index", LibIndex);
	XMLLibraryElement->SetAttribute("Name", getLibraryName().c_str());
	XMLLibraryElement->SetAttribute("Description", getLibraryDescription().c_str());

	// Get the Num Objects and Display an XML comment about them
    numObjects = getNumberOfLibraryComponents();
	xmlComment(XMLLibraryElement, "NUM COMPONENTS = %d", numObjects);
    for (x = 0; x < numObjects; x++) {
        eic = getInfoComponent(x);
        eic->outputXML(x, XMLLibraryElement);
    }

    numObjects = getNumberOfLibraryEvents();
	xmlComment(XMLLibraryElement, "NUM EVENTS = %d", numObjects);
    for (x = 0; x < numObjects; x++) {
        eie = getInfoEvent(x);
        eie->outputXML(x, XMLLibraryElement);
    }

    numObjects = getNumberOfLibraryModules();
	xmlComment(XMLLibraryElement, "NUM MODULES = %d", numObjects);
    for (x = 0; x < numObjects; x++) {
        eim = getInfoModule(x);
        eim->outputXML(x, XMLLibraryElement);
    }

// TODO: Dump SubComponent info to XML.  Turned off for 5.0 since SSTWorkbench
//       chokes if format is changed.  
    numObjects = getNumberOfLibrarySubComponents();
    xmlComment(XMLLibraryElement, "NUM SUBCOMPONENTS = %d", numObjects);
    for (x = 0; x < numObjects; x++) {
        eisc = getInfoSubComponent(x);
        eisc->outputXML(x, XMLLibraryElement);
    }

    numObjects = getNumberOfLibraryPartitioners();  
    xmlComment(XMLLibraryElement, "NUM PARTITIONERS = %d", numObjects);
    for (x = 0; x < numObjects; x++) {
        eip = getInfoPartitioner(x);
        eip->outputXML(x, XMLLibraryElement);
    }

    numObjects = getNumberOfLibraryGenerators();
    xmlComment(XMLLibraryElement, "NUM GENERATORS = %d", numObjects);
    for (x = 0; x < numObjects; x++) {
        eig = getInfoGenerator(x);
        eig->outputXML(x, XMLLibraryElement);
    }

    // Add this Library Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLLibraryElement);
}

void SSTInfoElement_ParamInfo::outputHumanReadable(int index)
{
    fprintf(stdout, "            PARAMETER %d = %s (%s) [%s]\n", index, getName().c_str(), getDesc().c_str(), getDefault().c_str());
}

void SSTInfoElement_ParamInfo::outputXML(int Index, TiXmlNode* XMLParentElement)
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

void SSTInfoElement_PortInfo::outputHumanReadable(int index)
{
    fprintf(stdout, "            PORT %d [%zu Valid Events] = %s (%s)\n", index,  m_validEvents.size(), getName().c_str(), getDesc().c_str());

    // Print out the Valid Events Info
    for (unsigned int x = 0; x < m_validEvents.size(); x++) {
        fprintf(stdout, "               VALID EVENT %d = %s\n", x, getValidEvent(x).c_str());
    }
}

void SSTInfoElement_PortInfo::outputXML(int Index, TiXmlNode* XMLParentElement)
{
    TiXmlElement* XMLValidEventElement;

    // Build the Element to Represent the Component
	TiXmlElement* XMLPortElement = new TiXmlElement("Port");
	XMLPortElement->SetAttribute("Index", Index);
	XMLPortElement->SetAttribute("Name", getName().c_str());
	XMLPortElement->SetAttribute("Description", getDesc().c_str());

	// Get the Num Valid Event and Display an XML comment about them
    xmlComment(XMLPortElement, "NUM Valid Events = %zu", m_validEvents.size());

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



void SSTInfoElement_StatisticInfo::outputHumanReadable(int index)
{
    fprintf(stdout, "            STATISTIC %d = %s [%s] (%s) Enable Level = %d\n", index, getName().c_str(), getUnits().c_str(), getDesc().c_str(), getEnableLevel());
}

void SSTInfoElement_StatisticInfo::outputXML(int UNUSED(Index), TiXmlNode* UNUSED(XMLParentElement))
{
    // Build the Element to Represent the Parameter
    TiXmlElement* XMLStatElement = new TiXmlElement("Statistic");
    XMLStatElement->SetAttribute("Index", Index);
    XMLStatElement->SetAttribute("Name", m_name);
    XMLStatElement->SetAttribute("Description", m_desc);
    XMLStatElement->SetAttribute("Units", m_units);
    XMLStatElement->SetAttribute("EnableLevel", getEnableLevel());

    // Add this Parameter Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLStatElement);
}


void SSTInfoElement_SubCompSlotInfo::outputHumanReadable(int index) {
    fprintf(stdout, "            SUB COMPONENT SLOT %d = %s (%s) [%s]\n", index, getName().c_str(), getDesc().c_str(), m_interface.c_str());
}

void SSTInfoElement_SubCompSlotInfo::outputXML(int Index, TiXmlNode* XMLParentElement) {
    TiXmlElement* element = new TiXmlElement("SubComponentSlot");
    element->SetAttribute("Index", Index);
    element->SetAttribute("Name", m_name);
    element->SetAttribute("Description", m_desc);
    element->SetAttribute("Interface", m_interface);

    XMLParentElement->LinkEndChild(element);
}

void SSTInfoElement_ComponentInfo::outputHumanReadable(int index)
{
    // Print out the Component Info
    fprintf(stdout, "      COMPONENT %d = %s [%s] (%s)\n", index, getName().c_str(), getCategoryString().c_str(), getDesc().c_str());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %ld\n", m_ParamArray.size());
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputHumanReadable(x);
    }

    // Print out the Port Info
    fprintf(stdout, "         NUM PORTS = %ld\n", m_PortArray.size());
    for (unsigned int x = 0; x < m_PortArray.size(); x++) {
        getPortInfo(x)->outputHumanReadable(x);
    }

    // Print out the Port Info
    fprintf(stdout, "         NUM STATISTICS = %ld\n", m_StatisticArray.size());
    for (unsigned int x = 0; x < m_StatisticArray.size(); x++) {
        getStatisticInfo(x)->outputHumanReadable(x);
    }

    fprintf(stdout, "         NUM SUBCOMPONENT SLOTS = %ld\n", m_SubCompSlotArray.size());
    for (unsigned int x = 0; x < m_SubCompSlotArray.size(); x++) {
        m_SubCompSlotArray[x].outputHumanReadable(x);
    }
}

void SSTInfoElement_ComponentInfo::outputXML(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Component
	TiXmlElement* XMLComponentElement = new TiXmlElement("Component");
	XMLComponentElement->SetAttribute("Index", Index);
	XMLComponentElement->SetAttribute("Name", getName().c_str());
	XMLComponentElement->SetAttribute("Description", getDesc().c_str());
	XMLComponentElement->SetAttribute("Category", getCategoryString().c_str());

	// Get the Num Parameters and Display an XML comment about them
    xmlComment(XMLComponentElement, "NUM PARAMETERS = %ld", m_ParamArray.size());

    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputXML(x, XMLComponentElement);
    }

    // Get the Num Ports and Display an XML comment about them
    xmlComment(XMLComponentElement, "NUM PORTS = %ld", m_PortArray.size());

    for (unsigned int x = 0; x < m_PortArray.size(); x++) {
        getPortInfo(x)->outputXML(x, XMLComponentElement);
    }

	// Get the Num Statistics and Display an XML comment about them
    xmlComment(XMLComponentElement, "NUM STATISTICS = %ld", m_StatisticArray.size());

    for (unsigned int x = 0; x < m_StatisticArray.size(); x++) {
        getStatisticInfo(x)->outputXML(x, XMLComponentElement);
    }

    xmlComment(XMLComponentElement, "NUM SUBCOMPONENT SLOTS = %zu", m_SubCompSlotArray.size());
    for (unsigned int x = 0; x < m_SubCompSlotArray.size(); x++) {
        m_SubCompSlotArray[x].outputXML(x, XMLComponentElement);
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

    // Build a string list of the component categories assigned to the component
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


void SSTInfoElement_EventInfo::outputHumanReadable(int index)
{
    fprintf(stdout, "      EVENT %d = %s (%s)\n", index, getName().c_str(), getDesc().c_str());
}

void SSTInfoElement_EventInfo::outputXML(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Event
	TiXmlElement* XMLEventElement = new TiXmlElement("Event");
	XMLEventElement->SetAttribute("Index", Index);
	XMLEventElement->SetAttribute("Name", getName().c_str());
	XMLEventElement->SetAttribute("Description", getDesc().c_str());

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLEventElement);
}

void SSTInfoElement_ModuleInfo::outputHumanReadable(int index)
{
    fprintf(stdout, "      MODULE %d = %s (%s) {%s}\n", index, getName().c_str(), getDesc().c_str(), getProvides().c_str());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %zu\n", m_ParamArray.size());
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputHumanReadable(x);
    }
}

void SSTInfoElement_ModuleInfo::outputXML(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Module
	TiXmlElement* XMLModuleElement = new TiXmlElement("Module");
	XMLModuleElement->SetAttribute("Index", Index);
	XMLModuleElement->SetAttribute("Name", getName().c_str());
	XMLModuleElement->SetAttribute("Description", getDesc().c_str());
	XMLModuleElement->SetAttribute("Provides", getProvides().c_str());

	// Get the Num Parameters and Display an XML comment about them
    xmlComment(XMLModuleElement, "NUM PARAMETERS = %zu", m_ParamArray.size());

    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputXML(x, XMLModuleElement);
    }

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLModuleElement);
}

void SSTInfoElement_SubComponentInfo::outputHumanReadable(int index)
{
    // Print out the Component Info
    fprintf(stdout, "      SUBCOMPONENT %d = %s (%s)\n", index, getName().c_str(), getDesc().c_str());

    fprintf(stdout, "         PROVIDES INTERFACE = %s\n", m_Provides.c_str());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %zu\n", m_ParamArray.size());
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputHumanReadable(x);
    }

    // Print out the Port Info
    fprintf(stdout, "         NUM PORTS = %ld\n", m_PortArray.size());
    for (unsigned int x = 0; x < m_PortArray.size(); x++) {
        getPortInfo(x)->outputHumanReadable(x);
    }

    // Print out the Statistics Info
    fprintf(stdout, "         NUM STATISTICS = %zu\n", m_StatisticArray.size());
    for (unsigned int x = 0; x < m_StatisticArray.size(); x++) {
        getStatisticInfo(x)->outputHumanReadable(x);
    }

    fprintf(stdout, "         NUM SUBCOMPONENT SLOTS = %ld\n", m_SubCompSlotArray.size());
    for (unsigned int x = 0; x < m_SubCompSlotArray.size(); x++) {
        m_SubCompSlotArray[x].outputHumanReadable(x);
    }
}

void SSTInfoElement_SubComponentInfo::outputXML(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Component
	TiXmlElement* XMLSubComponentElement = new TiXmlElement("SubComponent");
	XMLSubComponentElement->SetAttribute("Index", Index);
	XMLSubComponentElement->SetAttribute("Name", getName().c_str());
	XMLSubComponentElement->SetAttribute("Description", getDesc().c_str());
    XMLSubComponentElement->SetAttribute("Interface", getProvides().c_str());

	// Get the Num Parameters and Display an XML comment about them
    xmlComment(XMLSubComponentElement, "NUM PARAMETERS = %zu", m_ParamArray.size());

    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputXML(x, XMLSubComponentElement);
    }

    // Get the Num Ports and Display an XML comment about them
    xmlComment(XMLSubComponentElement, "NUM PORTS = %ld", m_PortArray.size());

    for (unsigned int x = 0; x < m_PortArray.size(); x++) {
        getPortInfo(x)->outputXML(x, XMLSubComponentElement);
    }

	// Get the Num Statistics and Display an XML comment about them
    xmlComment(XMLSubComponentElement, "NUM STATISTICS = %zu", m_StatisticArray.size());

    for (unsigned int x = 0; x < m_StatisticArray.size(); x++) {
        getStatisticInfo(x)->outputXML(x, XMLSubComponentElement);
    }

    xmlComment(XMLSubComponentElement, "NUM SUBCOMPONENT SLOTS = %zu", m_SubCompSlotArray.size());
    for (unsigned int x = 0; x < m_SubCompSlotArray.size(); x++) {
        m_SubCompSlotArray[x].outputXML(x, XMLSubComponentElement);
    }

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLSubComponentElement);
}

void SSTInfoElement_PartitionerInfo::outputHumanReadable(int index)
{
    fprintf(stdout, "      PARTITIONER %d = %s (%s)\n", index, getName().c_str(), getDesc().c_str());
}

void SSTInfoElement_PartitionerInfo::outputXML(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Partitioner
	TiXmlElement* XMLPartitionerElement = new TiXmlElement("Partitioner");
	XMLPartitionerElement->SetAttribute("Index", Index);
	XMLPartitionerElement->SetAttribute("Name", getName().c_str());
	XMLPartitionerElement->SetAttribute("Description", getDesc().c_str());

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLPartitionerElement);
}

void SSTInfoElement_GeneratorInfo::outputHumanReadable(int index)
{
    fprintf(stdout, "      GENERATOR %d = %s (%s)\n", index, getName().c_str(), getDesc().c_str());
}

void SSTInfoElement_GeneratorInfo::outputXML(int Index, TiXmlNode* XMLParentElement)
{
    // Build the Element to Represent the Generator
	TiXmlElement* XMLGeneratorElement = new TiXmlElement("Generator");
	XMLGeneratorElement->SetAttribute("Index", Index);
	XMLGeneratorElement->SetAttribute("Name", getName().c_str());
	XMLGeneratorElement->SetAttribute("Description", getDesc().c_str());

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLGeneratorElement);
}



void SSTInfoElement_Outputter::xmlComment(TiXmlNode* owner, const char* fmt...)
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
