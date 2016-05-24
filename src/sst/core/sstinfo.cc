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

#include "sst/core/serialization.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <cstdio>
#include <cerrno>
#include <list>
#include <dirent.h>
#include <sys/stat.h>
#include <ltdl.h>
#include <dlfcn.h>

#include <sst/core/elemLoader.h>
#include <sst/core/element.h>
#include "sst/core/build_info.h"

#include "sst/core/env/envquery.h"
#include "sst/core/env/envconfig.h"

#include <sst/core/tinyxml/tinyxml.h>
#include <sst/core/sstinfo.h>

namespace po = boost::program_options;

using namespace std;
using namespace SST;

// Global Variables
ElemLoader*                              g_loader;
int                                      g_fileProcessedCount;
std::string				 g_searchPath;
std::vector<SSTInfoElement_LibraryInfo*> g_libInfoArray;
SSTInfoConfig                            g_configuration;

// Forward Declarations
void initLTDL(std::string searchPath); 
void shutdownLTDL(); 
void processSSTElementFiles(std::string searchPath);
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
    SST::Core::Environment::EnvironmentConfiguration* sstEnv =
	SST::Core::Environment::getSSTEnvironmentConfiguration(overridePaths);
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

    g_loader = new ElemLoader(g_searchPath);

    // Read in the Element files and process them
    processSSTElementFiles(g_searchPath);

    // Do we output in Human Readable form
    if (g_configuration.getOptionBits() & CFG_OUTPUTHUMAN) {
        outputSSTElementInfo();
    }

    // Do we output an XML File
    if (g_configuration.getOptionBits() & CFG_OUTPUTXML) {
        generateXMLOutputFile();
    }

    delete g_loader;

    return 0;
}



void processSSTElementFiles(std::string searchPath)
{
    std::string             targetDir = searchPath;
    std::string             dirEntryPath;
    std::string             dirEntryName;
    std::string             elementName;
    DIR*                    pDir;
    struct dirent*          pDirEntry;
    struct stat             dirEntryStat;
    bool                    isDir;
    size_t                  indexExt;
    size_t                  indexLib;
    const ElementLibraryInfo* pELI;
    SSTInfoElement_LibraryInfo* pLibInfo;
    unsigned int            x;
    bool                    testAllEntries = false;
    std::vector<bool>       EntryProcessedArray;

    // Tell the user what libraries (if not all selected) will be processed
    if (g_configuration.getElementsToProcessArray()->size() > 0) {
        if (g_configuration.getElementsToProcessArray()->at(0) != "all") {
            for (x = 0; x < g_configuration.getElementsToProcessArray()->size(); x++) {
                fprintf (stdout, "Looking For Library \"%s\"\n", g_configuration.getElementsToProcessArray()->at(x).c_str());
            }
        }
    }
    
    // Init global var
    g_fileProcessedCount = 0;

    // Figure out if we are supposed to check all entrys, and setup array that 
    // tracks what entries have been processed
    for (x = 0; x < g_configuration.getElementsToProcessArray()->size(); x++) {
        EntryProcessedArray.push_back(false);
        if (g_configuration.getElementsToProcessArray()->at(x) == "all") {
            testAllEntries = true;
            EntryProcessedArray[x] = true;
        }
    }
    
    // Open the target directory
    pDir = opendir(searchPath.c_str());
    if (NULL != pDir) {

        // Dir is open, now read the entrys
        while ((pDirEntry = readdir(pDir)) != 0) {

            // Get the Name of the Entry; and prepend the path
            dirEntryName = pDirEntry->d_name;
            dirEntryPath = targetDir + "/" + dirEntryName;

            // Decide if the entry is a File or a Sub-Directory
            if (0 == stat(dirEntryPath.c_str(), &dirEntryStat)) {
                isDir = dirEntryStat.st_mode & S_IFDIR;
            } else {
                // Failed to open the directory for some reason
                fprintf(stderr, "ERROR: %s - Unable to get stat info on Directory Entry %s\n", strerror(errno), dirEntryPath.c_str());
                return;
            }
            
            for (x = 0; x < g_configuration.getElementsToProcessArray()->size(); x++) {
                // Check to see if we are supposed to process just this file or all files
                if ((true == testAllEntries) || (dirEntryName == g_configuration.getElementsToProcessArray()->at(x))) {

                    // Now only check out files that have the .so extension (.so must 
                    // be at the end of the file name) and a 'lib' in the front of the file
                    indexExt = dirEntryName.find(".so");
                    indexLib = dirEntryName.find("lib");
                    if ((false == isDir) && 
                        (indexExt != std::string::npos) && 
                        (indexExt == dirEntryName.length() - 3) &&
                        (indexLib != std::string::npos) && 
                        (indexLib == 0)) {
        
                        // Well as far as we can tell this is some sort of element library
                        // Lets strip off the lib and .so to get an Element Name
                        elementName = dirEntryName.substr(3, dirEntryName.length() - 6);
                        
                        g_fileProcessedCount++;
                        
                        // Now we process the file and populate our internal structures
//                      fprintf(stderr, "**** DEBUG - PROCESSING DIR ENTRY NAME = %s; ELEM NAME = %s; TYPE = %d; DIR FLAG = %d\n", dirEntryName.c_str(), elementName.c_str(), pDirEntry->d_type, isDir);
                        pELI = g_loader->loadLibrary(elementName, true);
                        if (pELI != NULL) {
                            // Build
                            pLibInfo = new SSTInfoElement_LibraryInfo(pELI); 
                            g_libInfoArray.push_back(pLibInfo);
                            EntryProcessedArray[x] = true;
                        }
                    }
                }
            }
        }
        
        // Now check to see if we processed all entries
        if (false == testAllEntries) {
            for (x = 0; x < g_configuration.getElementsToProcessArray()->size(); x++) {
                if (false == EntryProcessedArray[x]) {
                    std::string name = g_configuration.getElementsToProcessArray()->at(x);
                    fprintf(stderr, "**** WARNING - UNABLE TO PROCESS LIBRARY = %s - BECAUSE IT WAS NOT FOUND\n", name.c_str());
                }
            }
        }
        
        // Finished, close the directory
        closedir(pDir);
    } else {
        // Failed to open the directory for some reason
        fprintf(stderr, "ERROR: %s - When trying to open Directory %s\n", strerror(errno), targetDir.c_str());
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
    SSTInfoElement_LibraryInfo* pLibInfo;

    fprintf (stdout, "PROCESSED %d .so (SST ELEMENT) FILES FOUND IN DIRECTORY %s\n", g_fileProcessedCount, g_searchPath.c_str());

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
        pLibInfo = g_libInfoArray[x];
        pLibInfo->outputLibraryInfo(x);
    }
}

void generateXMLOutputFile()
{
    unsigned int            x;
    SSTInfoElement_LibraryInfo* pLibInfo;
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

    sprintf (Comment, "%d .so FILES FOUND IN DIRECTORY %s\n", g_fileProcessedCount, g_searchPath.c_str());
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
        pLibInfo = g_libInfoArray[x];
        pLibInfo->generateLibraryInfoXMLData(x, XMLTopLevelElement);
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
    
    // Setup the Visible Options
    m_configDesc = new po::options_description( "options" );
    m_configDesc->add_options()
        ("help,h", "Print Help Message")
        ("version,v", "Print SST Package Release Version")
        ("nodisplay,n", "Do Not Display Output - default is off")
        ("xml,x", "Generate XML data - default is off")
        ("outputxml,o", po::value< string >( &m_XMLFilePath ), "Filepath XML file - default is ./SSTInfo.xml")
        ("libs,l", po::value< std::vector<std::string> >()->multitoken(), 
            "{all | <elementname>} - Element Library(s) to process - default is 'all'")
    ; 
    
    m_hiddenDesc = new po::options_description( "" );
    m_hiddenDesc->add_options()
        ("elemfilt", po::value< std::vector<std::string> >()->multitoken(), 
            "[<elem[.comp]>] - Element Library(s) and components to filter on output - default is to show all")
    ; 

    m_posDesc = new po::positional_options_description();
    m_posDesc->add("elemfilt", -1);
    
    m_vm = new po::variables_map();
}

SSTInfoConfig::~SSTInfoConfig()
{
    delete m_configDesc;
    delete m_hiddenDesc;
    delete m_posDesc;
    delete m_vm;
}

void SSTInfoConfig::outputUsage()
{
    cout << "Usage: " << m_AppName << " [<element[.component|subcomponent]>] "<< " [options]" << endl;
    cout << *m_configDesc << endl;
}

void SSTInfoConfig::outputVersion()
{
    cout << "SST Release Version " PACKAGE_VERSION << endl;
}

int SSTInfoConfig::parseCmdLine(int argc, char* argv[])
{
    unsigned int              x;
    size_t                    foundIndex;
    std::string               filterName;
    std::string               libraryName;
    std::string               fullLibraryName;
    std::vector<std::string>  filteredNames;

    m_AppName = argv[0];

    po::options_description cmdline_options;
    cmdline_options.add(*m_configDesc).add(*m_hiddenDesc);

    // Parse the Command line into something we can analyze
    try {
        po::parsed_options parsed = po::command_line_parser(argc,argv).options(cmdline_options).positional(*m_posDesc).run();
        po::store( parsed, *m_vm );
        po::notify(*m_vm);
    }
    catch (exception& e) {
        // Tell the user the usage when something is wrong on parameters
        cout << "Error: " << e.what() << endl;
        outputUsage();        
        return -1;
    }
    
    // Check if user is asking for help
    if (m_vm->count("help")) {
        outputUsage();        
        return 1;
    }
    
    // Check if user is asking for version
    if (m_vm->count("version")) {
        outputVersion();
        return 1;
    }

    // Check if user is asking for XML Output
    if (m_vm->count("xml")) {
        m_optionBits |= CFG_OUTPUTXML;
    }

    // Check if user is asking for NO Text display
    if (m_vm->count("nodisplay")) {
        m_optionBits &= ~CFG_OUTPUTHUMAN;
    }

    // Check if user is setting the XML output filepath
    if (m_vm->count("outputxml")) {
    }

    // Get the List of filters on the output
    if (m_vm->count("elemfilt")) {
        // Get the list from the command parser
        filteredNames = (*m_vm)["elemfilt"].as< std::vector<std::string> >();
        
        // Look the Name over and decide if it is an element name only or a 
        // element.component name, and then add it to the appropriate list
        for (x = 0; x < filteredNames.size(); x++) {
            filterName = filteredNames[x];
            foundIndex = filterName.find(".");
            
            if (foundIndex != string::npos) {   
                // We found a "." in the name so assume that this is a element.component
                m_filteredElementComponentNames.push_back(filterName);
            } else {
                // No "." found, this is an element only name
                m_filteredElementNames.push_back(filterName);
            }
        }
    }    
    
    // Get the List of libs that the user may trying to use
    if (m_vm->count("libs")) {
        // Get the list from the command parser
        m_elementsToProcess = (*m_vm)["libs"].as< std::vector<std::string> >();
        
        // Walk through each element and make sure it has a "lib" in front 
        // and a ".so" in back if not, add them
        for (x = 0; x < m_elementsToProcess.size(); x++) {
            fullLibraryName = "";
            libraryName = m_elementsToProcess[x];

            // See if "lib" is at front of the library name; if not, append to full name
            foundIndex = libraryName.find("lib");
            if (foundIndex != 0) {
                fullLibraryName += "lib";
            }
            
            // Add the Library Name to the full name
            fullLibraryName += libraryName;
            
            // See if ".so" is at back of the library name; if not, postpend to full name
            foundIndex = libraryName.rfind(".so");
            if (foundIndex != libraryName.length() - 3)
            {
                fullLibraryName += ".so";
            }
            
            // Write the full name back to the m_elementsToProcess
            m_elementsToProcess[x] = fullLibraryName;
        }
    } else {
        // Build the default value
        m_elementsToProcess.push_back(std::string("all"));
    }
    
//// DEBUG    
//cout << "DEBUG ELEMENTS TO PROCESS = " << m_elementsToProcess.size() << endl;
//for (x = 0; x < m_elementsToProcess.size(); x++) {
//    cout << " LIBRARY ELEMENT TO PROCESS #" << x << " = " << m_elementsToProcess[x] << endl;
//}
//cout << "DEBUG ELEMENT NAMES TO FILTER = " << m_filteredElementNames.size() << endl;
//for (x = 0; x < m_filteredElementNames.size(); x++) {
//    cout << " ELEMENT NAME TO FILTER #" << x << " = " << m_filteredElementNames[x] << endl;
//}
//cout << "DEBUG ELEMENT.COMP NAMES TO FILTER = " << m_filteredElementComponentNames.size() << endl;
//for (x = 0; x < m_filteredElementComponentNames.size(); x++) {
//    cout << " ELEMENT.COMP NAME TO FILTER #" << x << " = " << m_filteredElementComponentNames[x] << endl;
//}
//cout << "DEBUG OPTIONBITS = " << m_optionBits << endl;
//return 1;
//// DEBUG    

    return 0;
}


void SSTInfoElement_LibraryInfo::populateLibraryInfo()
{
    const ElementInfoComponent*    eic;
    const ElementInfoIntrospector* eii;
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

    // Are there any Introspectors
    if (NULL != m_eli->introspectors) {
        // Get a pointer to the array
        eii = m_eli->introspectors;
        // If the name is NULL, we have reached the last item
        while (NULL != eii->name) {
            addInfoIntrospector(eii);
            eii++;  // Get the next item
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
    SSTInfoElement_IntrospectorInfo* eii;
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
        
        numObjects = getNumberOfLibraryIntrospectors(); 
        fprintf(stdout, "   NUM INTROSPECTORS = %d\n", numObjects);
        for (x = 0; x < numObjects; x++) {
            eii = getInfoIntrospector(x);
            eii->outputIntrospectorInfo(x);
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
    SSTInfoElement_IntrospectorInfo* eii;
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
    
    numObjects = getNumberOfLibraryIntrospectors(); 
	sprintf(Comment, "NUM INTROSPECTORS = %d", numObjects);
	TiXmlComment* XMLLibIntrospectorsComment = new TiXmlComment(Comment);
	XMLLibraryElement->LinkEndChild(XMLLibIntrospectorsComment);
    for (x = 0; x < numObjects; x++) {
        eii = getInfoIntrospector(x);
        eii->generateIntrospectorInfoXMLData(x, XMLLibraryElement);
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

void SSTInfoElement_IntrospectorInfo::outputIntrospectorInfo(int index)
{
    fprintf(stdout, "      INTROSPECTOR %d = %s (%s)\n", index, getName(), getDesc());

    // Print out the Parameter Info
    fprintf(stdout, "         NUM PARAMETERS = %ld\n", m_ParamArray.size());
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->outputParameterInfo(x);
    }
}

void SSTInfoElement_IntrospectorInfo::generateIntrospectorInfoXMLData(int Index, TiXmlNode* XMLParentElement)
{
    char Comment[256];

    // Build the Element to Represent the Introspector
	TiXmlElement* XMLIntrospectorElement = new TiXmlElement("Introspector");
	XMLIntrospectorElement->SetAttribute("Index", Index);
	XMLIntrospectorElement->SetAttribute("Name", (NULL == getName()) ? "" : getName());
	XMLIntrospectorElement->SetAttribute("Description", (NULL == getDesc()) ? "" : getDesc());
	
	// Get the Num Parameters and Display an XML comment about them
    sprintf(Comment, "NUM PARAMETERS = %ld", m_ParamArray.size());
    TiXmlComment* XMLParamsComment = new TiXmlComment(Comment);
    XMLIntrospectorElement->LinkEndChild(XMLParamsComment);
	
    for (unsigned int x = 0; x < m_ParamArray.size(); x++) {
        getParamInfo(x)->generateParameterInfoXMLData(x, XMLIntrospectorElement);
    }

    // Add this Element to the Parent Element
    XMLParentElement->LinkEndChild(XMLIntrospectorElement);
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




