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


#ifndef SST_INFO_H
#define SST_INFO_H

namespace SST { 
    
// CONFIGURATION BITS    
#define CFG_OUTPUTHUMAN 0x00000001
#define CFG_OUTPUTXML   0x00000002

/**
 * The SSTInfo Configuration class.
 *
 * This class will parse the command line, and setup internal 
 * lists of elements and components to be processed. 
 */
class SSTInfoConfig {
public:    
    /** Create a new SSTInfo configuration and parse the Command Line. */
    SSTInfoConfig();
    ~SSTInfoConfig();

    /** Parse the Command Line.
     * @param argc The number of arguments passed to the application
     * @param argv The array of arguments
     */
    int parseCmdLine(int argc, char* argv[]);

    /** Return the list of elements to be processed. */
    std::vector<std::string>* getElementsToProcessArray() {return &m_elementsToProcess;}
    
    /** Return the list of filtered element names. */
    std::vector<std::string>* getFilteredElementNamesArray() {return &m_filteredElementNames;}
    
    /** Return the list of filtered element.component names. */
    std::vector<std::string>* getFilteredElementComponentNamesArray() {return &m_filteredElementComponentNames;}
    
    /** Return the bit field of various command line options enabled. */
    unsigned int              getOptionBits() {return m_optionBits;}
    
    /** Return the user defined path the XML File. */
    std::string&              getXMLFilePath() {return m_XMLFilePath;}

    /** Is debugging output enabled? */
    bool                      debugEnabled() const { return m_debugEnabled; }

private:
    void outputUsage();
    void outputVersion();

private:
    char*                     m_AppName;
    std::vector<std::string>  m_elementsToProcess;
    std::vector<std::string>  m_filteredElementNames;
    std::vector<std::string>  m_filteredElementComponentNames;
    unsigned int              m_optionBits;
    std::string               m_XMLFilePath;
    bool                      m_debugEnabled;
};

/**
 * The SSTInfo representation of ElementInfoParam object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoParam objects. 
 */
class SSTInfoElement_ParamInfo {
public:
    /** Create a new SSTInfoElement_ParamInfo object.
     * @param elparam Pointer to an ElementInfoParam object.
     */
    SSTInfoElement_ParamInfo(const ElementInfoParam* elparam)
    {
        // Save the Object
        m_elparam = elparam;
    }

    /** Return the Name of the Parameter. */
    const char* getName() {return m_elparam->name;}

    /** Return the Description of the Parameter. */
    const char* getDesc() {return m_elparam->description;}
    
    /** Return the Default value of the Parameter. */
    const char* getDefault() {return (m_elparam->defaultValue) ? m_elparam->defaultValue : "REQUIRED";}

    /** Output the Parameter Information. 
     * @param Index The Index of the Parameter.
     */
    void outputParameterInfo(int Index);

    /** Create the formatted XML data of the Parameter.
     * @param Index The Index of the Parameter.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateParameterInfoXMLData(int Index, TiXmlNode* XMLParentElement);
    
private:    
    const ElementInfoParam* m_elparam;
};
    
void PopulateParams(const ElementInfoParam* ptrParams, std::vector<SSTInfoElement_ParamInfo*>* ptrParamArray)
{
    // Populate the Parameter Array
    if (NULL != ptrParams) {
        while (NULL != ptrParams->name) {
            // Create a new SSTInfoElement_ParamInfo and add it to the m_ParamArray
            SSTInfoElement_ParamInfo* ptrParamInfo = new SSTInfoElement_ParamInfo(ptrParams);
            ptrParamArray->push_back(ptrParamInfo);

            // If the name is NULL, we have reached the last item
            ptrParams++;  // Get the next structure item
        }
    }
}

/**
 * The SSTInfo representation of ElementInfoPort object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoPort objects. 
 */
class SSTInfoElement_PortInfo {
public:
    /** Create a new SSTInfoElement_PortInfo object.
     * @param elport Pointer to an ElementInfoPort object.
     */
    SSTInfoElement_PortInfo(const ElementInfoPort* elport)
    {
        // Save the Object
        m_numValidEvents = 0;
        m_elport = elport;
        
        analyzeValidEventsArray();
    }

    /** Return the Name of the Port. */
    const char* getName() {return m_elport->name;}

    /** Return the Description of the Port. */
    const char* getDesc() {return m_elport->description;}

    /** Return the array of Valid Events related to the Port. */
    const char** getValidEvents() {return m_elport->validEvents;}

    /** Return the number of Valid Events related to the Port. */
    int         getNumberOfValidEvents()    {return m_numValidEvents;}

    /** Return the a specific Valid Events.
     * @param index The index of the Valid Event.
     */
    const char* getValidEvent(unsigned int index); 
    
    /** Output the Port Information. 
     * @param Index The Index of the Port.
     */
    void outputPortInfo(int Index);

    /** Create the formatted XML data of the Port.
     * @param Index The Index of the Port.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generatePortInfoXMLData(int Index, TiXmlNode* XMLParentElement);

private:    
    void analyzeValidEventsArray();
    
    const ElementInfoPort*   m_elport;
    unsigned int             m_numValidEvents;
};
    
void PopulatePorts(const ElementInfoPort* ptrPorts, std::vector<SSTInfoElement_PortInfo*>* ptrPortArray)
{
    // Populate the Ports Array
    if (NULL != ptrPorts) {
        while (NULL != ptrPorts->name) {
            // Create a new SSTInfoElement_PortInfo and add it to the m_PortArray
            SSTInfoElement_PortInfo* ptrPortInfo = new SSTInfoElement_PortInfo(ptrPorts);
            ptrPortArray->push_back(ptrPortInfo);

            // If the name is NULL, we have reached the last item
            ptrPorts++;  // Get the next structure item
        }
    }
}

/**
 * The SSTInfo representation of ElementInfoPort object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoPort objects. 
 */
class SSTInfoElement_StatisticInfo {
public:
    /** Create a new SSTInfoElement_StatisticInfo object.
     * @param elstat Pointer to an ElementInfoStatistic object.
     */
    SSTInfoElement_StatisticInfo(const ElementInfoStatistic* elstat)
    {
        // Save the Object
        m_elstat = elstat;
    }

    /** Return the Name of the Statistic. */
    const char* getName() {return m_elstat->name;}

    /** Return the Description of the Statistic. */
    const char* getDesc() {return m_elstat->description;}

    /** Return the Units of the Statistic. */
    const char* getUnits() {return m_elstat->units;}

    /** Return the enable level of the Statistic. */
    const uint8_t getEnableLevel() {return m_elstat->enableLevel;}
    
    /** Output the Statistic Information. 
     * @param Index The Index of the Statistic.
     */
    void outputStatisticInfo(int Index);

    /** Create the formatted XML data of the Statistic.
     * @param Index The Index of the Statistic.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateStatisticXMLData(int Index, TiXmlNode* XMLParentElement);

private:    
    const ElementInfoStatistic*   m_elstat;
};
    
void PopulateStatistic(const ElementInfoStatistic* ptrStats, std::vector<SSTInfoElement_StatisticInfo*>* ptrStatArray)
{
    // Populate the Statistics Array
    if (NULL != ptrStats) {
        while (NULL != ptrStats->name) {
            // Create a new SSTInfoElement_Statistic and add it to the m_StatisticArray
            SSTInfoElement_StatisticInfo* ptrStatInfo = new SSTInfoElement_StatisticInfo(ptrStats);
            ptrStatArray->push_back(ptrStatInfo);

            // If the name is NULL, we have reached the last item
            ptrStats++;  // Get the next structure item
        }
    }
}

/**
 * The SSTInfo representation of ElementInfoComponent object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoComponent objects. 
 */
class SSTInfoElement_ComponentInfo {
public:
    /** Create a new SSTInfoElement_ComponentInfo object.
     * @param elc Pointer to an ElementInfoComponent object.
     */
    SSTInfoElement_ComponentInfo(const ElementInfoComponent* elc)
    {
        const ElementInfoParam*            ptrParams;
        const ElementInfoPort*             ptrPorts;
        const ElementInfoStatistic*        ptrStats;
        
        // Save the Object
        m_elc = elc;

        ptrParams = elc->params;  // Pointer to the Params Structure Array
        PopulateParams(ptrParams, &m_ParamArray);        

        ptrPorts = elc->ports;  // Pointer to the Ports Structure Array
        PopulatePorts(ptrPorts, &m_PortArray);
        
        buildCategoryString();        

        ptrStats = elc->stats;  // Pointer to the Stats Structure Array
        PopulateStatistic(ptrStats, &m_StatisticArray);
    }
    
    /** Return the Name of the Component. */
    const char* getName() {return m_elc->name;}

    /** Return the Description of the Component. */
    const char* getDesc() {return m_elc->description;}

    /** Return a Parameter Info Object. 
     * @param index The index of the Parameter.
     */
    SSTInfoElement_ParamInfo* getParamInfo(int index) {return m_ParamArray[index];}

    /** Return a Port Info Object. 
     * @param index The index of the Port.
     */
    SSTInfoElement_PortInfo*  getPortInfo(int index) {return m_PortArray[index];}

    /** Return a Statistic Enable Info Object. 
     * @param index The index of the Statistic Enable.
     */
    SSTInfoElement_StatisticInfo*  getStatisticInfo(int index) {return m_StatisticArray[index];}

    /** Return the Category value of the Component. */
    uint32_t              getCategoryValue() {return m_elc->category;}

    /** Return the name of the Category of the Component. */
    const char*           getCategoryString() {return m_CategoryString.c_str();}

    /** Output the Component Information. 
     * @param Index The Index of the Component.
     */
    void outputComponentInfo(int Index);
    
    /** Create the formatted XML data of the Component.
     * @param Index The Index of the Component.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateComponentInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    void buildCategoryString();
    
    const ElementInfoComponent*                       m_elc;
    std::vector<SSTInfoElement_ParamInfo*>            m_ParamArray;
    std::vector<SSTInfoElement_PortInfo*>             m_PortArray;
    std::vector<SSTInfoElement_StatisticInfo*>        m_StatisticArray;
    std::string                                       m_CategoryString;
};

/**
 * The SSTInfo representation of ElementInfoIntrospector object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoIntrospector objects. 
 */
class SSTInfoElement_IntrospectorInfo {
public:
    /** Create a new SSTInfoElement_IntrospectorInfo object.
     * @param eli Pointer to an ElementInfoIntrospector object.
     */
    SSTInfoElement_IntrospectorInfo(const ElementInfoIntrospector* eli)
    {
        const ElementInfoParam* ptrParams;
        
        // Save the Object
        m_eli = eli;

        ptrParams = eli->params;  // Pointer to the Params Structure Array
        PopulateParams(ptrParams, &m_ParamArray);        
    }

    /** Return the Name of the Introspector. */
    const char*           getName() {return m_eli->name;}

    /** Return the Description of the Introspector. */
    const char*           getDesc() {return m_eli->description;}

    /** Return a Parameter Info Object. 
     * @param index The index of the Parameter.
     */
    SSTInfoElement_ParamInfo* getParamInfo(int index) {return m_ParamArray[index];}

    /** Output the Introspector Information. 
     * @param Index The Index of the Introspector.
     */
    void outputIntrospectorInfo(int Index);
    
    /** Create the formatted XML data of the Introspector.
     * @param Index The Index of the Introspector.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateIntrospectorInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoIntrospector*     m_eli;
    std::vector<SSTInfoElement_ParamInfo*> m_ParamArray;
};

/**
 * The SSTInfo representation of ElementInfoEvent object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoEvent objects. 
 */
class SSTInfoElement_EventInfo {
public:
    /** Create a new SSTInfoElement_EventInfo object.
     * @param ele Pointer to an ElementInfoEvent object.
     */
    SSTInfoElement_EventInfo(const ElementInfoEvent* ele)
    {
        // Save the Object
        m_ele = ele;
    }

    /** Return the Name of the Event. */
    const char* getName() {return m_ele->name;}

    /** Return the Description of the Event. */
    const char* getDesc() {return m_ele->description;}

    /** Output the Event Information. 
     * @param Index The Index of the Event.
     */
    void outputEventInfo(int Index);
    
    /** Create the formatted XML data of the Event.
     * @param Index The Index of the Event.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateEventInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoEvent* m_ele;
};

/**
 * The SSTInfo representation of ElementInfoModule object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoModule objects. 
 */
class SSTInfoElement_ModuleInfo {
public:
    /** Create a new SSTInfoElement_ModuleInfo object.
     * @param elm Pointer to an ElementInfoModule object.
     */
    SSTInfoElement_ModuleInfo(const ElementInfoModule* elm)
    {
        const ElementInfoParam* ptrParams;
        
        // Save the Object
        m_elm = elm;

        ptrParams = elm->params;  // Pointer to the Params Structure Array
        PopulateParams(ptrParams, &m_ParamArray);        
    }

    /** Return the Name of the Module. */
    const char*           getName() {return m_elm->name;}

    /** Return the Description of the Module. */
    const char*           getDesc() {return m_elm->description;}

    /** Return what class the Module provides. */
    const char*           getProvides() { return m_elm->provides;}

    /** Return a Parameter Info Object. 
     * @param index The index of the Parameter.
     */
    SSTInfoElement_ParamInfo* getParamInfo(int index) {return m_ParamArray[index];}

    /** Output the Module Information. 
     * @param Index The Index of the Module.
     */
    void outputModuleInfo(int Index);
    
    /** Create the formatted XML data of the Module.
     * @param Index The Index of the Module.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateModuleInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoModule*           m_elm;
    std::vector<SSTInfoElement_ParamInfo*> m_ParamArray;
};

/**
 * The SSTInfo representation of ElementInfoSubComponent object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoSubComponent objects. 
 */
class SSTInfoElement_SubComponentInfo {
public:
    /** Create a new SSTInfoElement_SubComponentInfo object.
     * @param elsc Pointer to an ElementInfoComponent object.
     */
    SSTInfoElement_SubComponentInfo(const ElementInfoSubComponent* elsc)
    {
        const ElementInfoParam*            ptrParams;
        const ElementInfoStatistic*        ptrStats;
        
        // Save the Object
        m_elsc = elsc;

        ptrParams = elsc->params;  // Pointer to the Params Structure Array
        PopulateParams(ptrParams, &m_ParamArray);        

        ptrStats = elsc->stats;  // Pointer to the Stats Structure Array
        PopulateStatistic(ptrStats, &m_StatisticArray);
    }
    
    /** Return the Name of the SubComponent. */
    const char* getName() {return m_elsc->name;}

    /** Return the Description of the SubComponent. */
    const char* getDesc() {return m_elsc->description;}

    /** Return a Parameter Info Object. 
     * @param index The index of the Parameter.
     */
    SSTInfoElement_ParamInfo* getParamInfo(int index) {return m_ParamArray[index];}

    /** Return a Statistic Enable Info Object. 
     * @param index The index of the Statistic Enable.
     */
    SSTInfoElement_StatisticInfo*  getStatisticInfo(int index) {return m_StatisticArray[index];}

    /** Output the SubComponent Information. 
     * @param Index The Index of the SubComponent.
     */
    void outputSubComponentInfo(int Index);
    
    /** Create the formatted XML data of the Component.
     * @param Index The Index of the Component.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateSubComponentInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoSubComponent*                    m_elsc;
    std::vector<SSTInfoElement_ParamInfo*>            m_ParamArray;
    std::vector<SSTInfoElement_StatisticInfo*>        m_StatisticArray;
};

/**
 * The SSTInfo representation of ElementInfoPartitioner object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoPartitioner objects. 
 */
class SSTInfoElement_PartitionerInfo {
public:
    /** Create a new SSTInfoElement_PartitionerInfo object.
     * @param elp Pointer to an ElementInfoPartitioner object.
     */
    SSTInfoElement_PartitionerInfo(const ElementInfoPartitioner* elp)
    {
        // Save the Object
        m_elp = elp;
    }

    /** Return the Name of the Partitioner. */
    const char* getName() {return m_elp->name;}

    /** Return the Description of the Partitioner. */
    const char* getDesc() {return m_elp->description;}

    /** Output the Partitioner Information. 
     * @param Index The Index of the Partitioner.
     */
    void outputPartitionerInfo(int Index);

    /** Create the formatted XML data of the Partitioner.
     * @param Index The Index of the Partitioner.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generatePartitionerInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoPartitioner* m_elp;
};

/**
 * The SSTInfo representation of ElementInfoGenerator object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoGenerator objects. 
 */
class SSTInfoElement_GeneratorInfo {
public:
    /** Create a new SSTInfoElement_GeneratorInfo object.
     * @param elg Pointer to an ElementInfoGenerator object.
     */
    SSTInfoElement_GeneratorInfo(const ElementInfoGenerator* elg)
    {
        // Save the Object
        m_elg = elg;
    }

    /** Return the Name of the Generator. */
    const char* getName() {return m_elg->name;}

    /** Return the Description of the Generator. */
    const char* getDesc() {return m_elg->description;}

    /** Output the Generator Information. 
     * @param Index The Index of the Generator.
     */
    void outputGeneratorInfo(int Index);

    /** Create the formatted XML data of the Generator.
     * @param Index The Index of the Generator.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateGeneratorInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoGenerator* m_elg;
};


/**
 * The SSTInfo representation of ElementLibraryInfo object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementLibraryInfo objects. 
 */
class SSTInfoElement_LibraryInfo {

public:
    /** Create a new SSTInfoElement_LibraryInfo object.
     * @param eli Pointer to an ElementLibraryInfo object.
     */
    SSTInfoElement_LibraryInfo(const ElementLibraryInfo* eli)
    {
        m_eli = eli;
        populateLibraryInfo();
    }

    /** Return the Name of the Library. */
    std::string getLibraryName() {if (m_eli && m_eli->name) return m_eli->name; else return ""; }

    /** Return the Description of the Library. */
    std::string getLibraryDescription() {if (m_eli && m_eli->description) return m_eli->description; else return ""; }

    /** Return the number of Components within the Library. */
    int         getNumberOfLibraryComponents()    {return m_ComponentArray.size();}

    /** Return the number of Introspectors within the Library. */
    int         getNumberOfLibraryIntrospectors() {return m_IntrospectorArray.size();}

    /** Return the number of Events within the Library. */
    int         getNumberOfLibraryEvents()        {return m_EventArray.size();}

    /** Return the number of Modules within the Library. */
    int         getNumberOfLibraryModules()       {return m_ModuleArray.size();}

    /** Return the number of SubComponents within the Library. */
    int         getNumberOfLibrarySubComponents()    {return m_SubComponentArray.size();}

    /** Return the number of Partitioners within the Library. */
    int         getNumberOfLibraryPartitioners()  {return m_PartitionerArray.size();}

    /** Return the number of Generators within the Library. */
    int         getNumberOfLibraryGenerators()    {return m_GeneratorArray.size();}

    /** Return the ElementLibraryInfo object. */
    const ElementLibraryInfo*    getLibraryInfo() {return m_eli;}

    /** Return a specific SSTInfoElement_ComponentInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_ComponentInfo*    getInfoComponent(int Index)    {return m_ComponentArray[Index];}

    /** Return a specific SSTInfoElement_IntrospectorInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_IntrospectorInfo* getInfoIntrospector(int Index) {return m_IntrospectorArray[Index];}

    /** Return a specific SSTInfoElement_EventInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_EventInfo*        getInfoEvent(int Index)        {return m_EventArray[Index];}

    /** Return a specific SSTInfoElement_ModuleInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_ModuleInfo*       getInfoModule(int Index)       {return m_ModuleArray[Index];}

    /** Return a specific SSTInfoElement_SubComponentInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_SubComponentInfo*    getInfoSubComponent(int Index)    {return m_SubComponentArray[Index];}

    /** Return a specific SSTInfoElement_PartitionerInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_PartitionerInfo*  getInfoPartitioner(int Index)  {return m_PartitionerArray[Index];}

    /** Return a specific SSTInfoElement_GeneratorInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_GeneratorInfo*    getInfoGenerator(int Index)    {return m_GeneratorArray[Index];}

    /** Output the Library Information. 
     * @param LibIndex The Index of the Library.
     */
    void outputLibraryInfo(int LibIndex);

    /** Create the formatted XML data of the Library.
     * @param LibIndex The Index of the Library.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void generateLibraryInfoXMLData(int LibIndex, TiXmlNode* XMLParentElement);

private:
    void populateLibraryInfo();
    void addInfoComponent(const ElementInfoComponent* eic) {m_ComponentArray.push_back(new SSTInfoElement_ComponentInfo(eic));}
    void addInfoIntrospector(const ElementInfoIntrospector* eii) {m_IntrospectorArray.push_back(new SSTInfoElement_IntrospectorInfo(eii));}
    void addInfoEvent(const ElementInfoEvent* eie) {m_EventArray.push_back(new SSTInfoElement_EventInfo(eie));}
    void addInfoModule(const ElementInfoModule* eim) {m_ModuleArray.push_back(new SSTInfoElement_ModuleInfo(eim));}
    void addInfoSubComponent(const ElementInfoSubComponent* eisc) {m_SubComponentArray.push_back(new SSTInfoElement_SubComponentInfo(eisc));}
    void addInfoPartitioner(const ElementInfoPartitioner* eip) {m_PartitionerArray.push_back(new SSTInfoElement_PartitionerInfo(eip));}
    void addInfoGenerator(const ElementInfoGenerator* eig) {m_GeneratorArray.push_back(new SSTInfoElement_GeneratorInfo(eig));}
    
    const ElementLibraryInfo*                 m_eli;
    std::vector<SSTInfoElement_ComponentInfo*>    m_ComponentArray;
    std::vector<SSTInfoElement_IntrospectorInfo*> m_IntrospectorArray;
    std::vector<SSTInfoElement_EventInfo*>        m_EventArray;
    std::vector<SSTInfoElement_ModuleInfo*>       m_ModuleArray;
    std::vector<SSTInfoElement_SubComponentInfo*> m_SubComponentArray;
    std::vector<SSTInfoElement_PartitionerInfo*>  m_PartitionerArray;
    std::vector<SSTInfoElement_GeneratorInfo*>    m_GeneratorArray;

};

} // namespace SST

#endif  // SST_INFO_H
