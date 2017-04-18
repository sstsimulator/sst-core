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


#ifndef SST_INFO_H
#define SST_INFO_H

#include <map>
#include <vector>
#include <set>

#include <sst/core/element.h>
#include <sst/core/elementinfo.h>

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
    typedef std::multimap<std::string, std::string> FilterMap_t;
    /** Create a new SSTInfo configuration and parse the Command Line. */
    SSTInfoConfig();
    ~SSTInfoConfig();

    /** Parse the Command Line.
     * @param argc The number of arguments passed to the application
     * @param argv The array of arguments
     */
    int parseCmdLine(int argc, char* argv[]);

    /** Return the list of elements to be processed. */
    std::set<std::string> getElementsToProcessArray()
    {
        std::set<std::string> res;
        for ( auto &i : m_filters )
            res.insert(i.first);
        return res;
    }

    /** Return the filter map */
    FilterMap_t& getFilterMap() { return m_filters; }

    /** Return the bit field of various command line options enabled. */
    unsigned int              getOptionBits() {return m_optionBits;}

    /** Return the user defined path the XML File. */
    std::string&              getXMLFilePath() {return m_XMLFilePath;}

    /** Is debugging output enabled? */
    bool                      debugEnabled() const { return m_debugEnabled; }
    bool                      processAllElements() const { return m_filters.empty(); }

private:
    void outputUsage();
    void outputVersion();
    void addFilter(std::string name);

private:
    char*                     m_AppName;
    std::vector<std::string>  m_elementsToProcess;
    unsigned int              m_optionBits;
    std::string               m_XMLFilePath;
    bool                      m_debugEnabled;
    FilterMap_t               m_filters;
};


template<typename TO, typename FROM>
std::vector<TO> convertFromELI(const FROM *ptr) {
    std::vector<TO> res;
    if ( ptr != NULL ) {
        while ( ptr->name ) {
            res.emplace_back(ptr);
            ptr++;
        }
    }
    return res;
}

template<typename TO, typename FROM>
std::vector<TO> convertFromDB(const std::vector<FROM> &input) {
    std::vector<TO> res;
    for ( auto &i : input ) {
        res.emplace_back(&i);
    }
    return res;
}

std::string fs(const char* x) {
    if ( x == NULL ) return "";
    return std::string(x);
}

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
    SSTInfoElement_ParamInfo(const ElementInfoParam* elparam) :
        m_name(elparam->name), m_desc(fs(elparam->description))
    {
        m_defaultValue = (elparam->defaultValue) ? elparam->defaultValue : "REQUIRED";
    }

    /** Return the Name of the Parameter. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the Parameter. */
    const std::string& getDesc() {return m_desc;}

    /** Return the Default value of the Parameter. */
    const std::string& getDefault() {return m_defaultValue;}

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
    std::string m_name;
    std::string m_desc;
    std::string m_defaultValue;
};


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
    SSTInfoElement_PortInfo(const ElementInfoPort* elport) :
        m_name(elport->name), m_desc(fs(elport->description))
    {
        const char **arrayPtr = elport->validEvents;
        while ( arrayPtr && *arrayPtr ) {
            m_validEvents.emplace_back(*arrayPtr);
            arrayPtr++;
        }
    }
    SSTInfoElement_PortInfo(const ElementInfoPort2* elport) :
        m_name(elport->name), m_desc(fs(elport->description))
    {
        m_validEvents = elport->validEvents;
    }

    /** Return the Name of the Port. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the Port. */
    const std::string& getDesc() {return m_desc;}

    /** Return the array of Valid Events related to the Port. */
    const std::vector<std::string>& getValidEvents() {return m_validEvents;}

    /** Return the number of Valid Events related to the Port. */
    int         getNumberOfValidEvents()    {return m_validEvents.size();}

    /** Return the a specific Valid Events.
     * @param index The index of the Valid Event.
     */
    const std::string& getValidEvent(unsigned int index) { return m_validEvents.at(index);}

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

    std::string m_name;
    std::string m_desc;
    std::vector<std::string> m_validEvents;

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
    SSTInfoElement_StatisticInfo(const ElementInfoStatistic* els) :
        m_name(els->name), m_desc(fs(els->description)),
        m_units(els->units), m_enableLevel(els->enableLevel)
    { }

    /** Return the Name of the Statistic. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the Statistic. */
    const std::string& getDesc() {return m_desc;}

    /** Return the Units of the Statistic. */
    const std::string& getUnits() {return m_units;}

    /** Return the enable level of the Statistic. */
    uint8_t getEnableLevel() {return m_enableLevel;}

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
    std::string m_name;
    std::string m_desc;
    std::string m_units;
    uint8_t m_enableLevel;
};




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
    SSTInfoElement_ComponentInfo(const ElementInfoComponent* elc) :
        m_name(elc->name), m_desc(fs(elc->description)), m_category(elc->category)
    {
        m_ParamArray = convertFromELI<SSTInfoElement_ParamInfo>(elc->params);
        m_PortArray = convertFromELI<SSTInfoElement_PortInfo>(elc->ports);
        m_StatisticArray = convertFromELI<SSTInfoElement_StatisticInfo>(elc->stats);
    }

    SSTInfoElement_ComponentInfo(ComponentElementInfo* elc) :
        m_name(elc->getName()), m_desc(elc->getDescription()), m_category(elc->getCategory())
    {
        m_ParamArray = convertFromDB<SSTInfoElement_ParamInfo>(elc->getValidParams());
        m_PortArray = convertFromDB<SSTInfoElement_PortInfo>(elc->getValidPorts());
        m_StatisticArray = convertFromDB<SSTInfoElement_StatisticInfo>(elc->getValidStats());
    }

    /** Return the Name of the Component. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the Component. */
    const std::string& getDesc() {return m_desc;}

    /** Return a Parameter Info Object. 
     * @param index The index of the Parameter.
     */
    SSTInfoElement_ParamInfo* getParamInfo(int index) {return &m_ParamArray[index];}

    /** Return a Port Info Object. 
     * @param index The index of the Port.
     */
    SSTInfoElement_PortInfo*  getPortInfo(int index) {return &m_PortArray[index];}

    /** Return a Statistic Enable Info Object. 
     * @param index The index of the Statistic Enable.
     */
    SSTInfoElement_StatisticInfo*  getStatisticInfo(int index) {return &m_StatisticArray[index];}

    /** Return the Category value of the Component. */
    uint32_t              getCategoryValue() {return m_category;}

    /** Return the name of the Category of the Component. */
    std::string         getCategoryString() const;

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
    std::string                                       m_name;
    std::string                                       m_desc;
    uint32_t                                          m_category;
    std::vector<SSTInfoElement_ParamInfo>             m_ParamArray;
    std::vector<SSTInfoElement_PortInfo>              m_PortArray;
    std::vector<SSTInfoElement_StatisticInfo>         m_StatisticArray;
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
    SSTInfoElement_EventInfo(const ElementInfoEvent* ele) :
        m_name(ele->name), m_desc(fs(ele->description))
    { }

    /** Return the Name of the Event. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the Event. */
    const std::string& getDesc() {return m_desc;}

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
    std::string                                       m_name;
    std::string                                       m_desc;
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
    SSTInfoElement_ModuleInfo(const ElementInfoModule* elm) :
        m_name(elm->name), m_desc((elm->description)), m_provides(fs(elm->provides))
    {
        m_ParamArray = convertFromELI<SSTInfoElement_ParamInfo>(elm->params);
    }
    SSTInfoElement_ModuleInfo(ModuleElementInfo* elm) :
        m_name(elm->getName()), m_desc(elm->getDescription()), m_provides(elm->getInterface())
    {
        m_ParamArray = convertFromDB<SSTInfoElement_ParamInfo>(elm->getValidParams());
    }


    /** Return the Name of the Module. */
    const std::string&           getName() {return m_name;}

    /** Return the Description of the Module. */
    const std::string&           getDesc() {return m_desc;}

    /** Return what class the Module provides. */
    const std::string&           getProvides() { return m_provides;}

    /** Return a Parameter Info Object. 
     * @param index The index of the Parameter.
     */
    SSTInfoElement_ParamInfo* getParamInfo(int index) {return &m_ParamArray[index];}

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
    std::string m_name;
    std::string m_desc;
    std::string m_provides;
    std::vector<SSTInfoElement_ParamInfo> m_ParamArray;
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
    SSTInfoElement_SubComponentInfo(const ElementInfoSubComponent* elsc) :
        m_name(elsc->name), m_desc(fs(elsc->description))
    {
        m_ParamArray = convertFromELI<SSTInfoElement_ParamInfo>(elsc->params);
        m_PortArray = convertFromELI<SSTInfoElement_PortInfo>(elsc->ports);
        m_StatisticArray = convertFromELI<SSTInfoElement_StatisticInfo>(elsc->stats);
    }

    SSTInfoElement_SubComponentInfo(SubComponentElementInfo* elc) :
        m_name(elc->getName()), m_desc(elc->getDescription())
    {
        m_ParamArray = convertFromDB<SSTInfoElement_ParamInfo>(elc->getValidParams());
        m_PortArray = convertFromDB<SSTInfoElement_PortInfo>(elc->getValidPorts());
        m_StatisticArray = convertFromDB<SSTInfoElement_StatisticInfo>(elc->getValidStats());
    }

    /** Return the Name of the SubComponent. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the SubComponent. */
    const std::string& getDesc() {return m_desc;}

    /** Return a Parameter Info Object. 
     * @param index The index of the Parameter.
     */
    SSTInfoElement_ParamInfo* getParamInfo(int index) {return &m_ParamArray[index];}

    /** Return a Statistic Enable Info Object. 
     * @param index The index of the Statistic Enable.
     */
    SSTInfoElement_StatisticInfo*  getStatisticInfo(int index) {return &m_StatisticArray[index];}

    /** Return a Port Info Object. 
     * @param index The index of the Port.
     */
    SSTInfoElement_PortInfo*  getPortInfo(int index) {return &m_PortArray[index];}

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
    std::string                                       m_name;
    std::string                                       m_desc;
    std::vector<SSTInfoElement_ParamInfo>             m_ParamArray;
    std::vector<SSTInfoElement_PortInfo>              m_PortArray;
    std::vector<SSTInfoElement_StatisticInfo>         m_StatisticArray;
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
    SSTInfoElement_PartitionerInfo(const ElementInfoPartitioner* elp) :
        m_name(elp->name), m_desc(fs(elp->description))
    {
    }

    SSTInfoElement_PartitionerInfo(PartitionerElementInfo* elp) :
        m_name(elp->getName()), m_desc(elp->getDescription())
    {
    }

    /** Return the Name of the Partitioner. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the Partitioner. */
    const std::string& getDesc() {return m_desc;}

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
    std::string m_name;
    std::string m_desc;
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
    SSTInfoElement_GeneratorInfo(const ElementInfoGenerator* elg) :
        m_name(elg->name), m_desc(fs(elg->description))
    {
    }

    /** Return the Name of the Partitioner. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the Partitioner. */
    const std::string& getDesc() {return m_desc;}

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
    std::string m_name;
    std::string m_desc;
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
    SSTInfoElement_ComponentInfo*    getInfoComponent(int Index)    {return &m_ComponentArray[Index];}

    /** Return a specific SSTInfoElement_EventInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_EventInfo*        getInfoEvent(int Index)        {return &m_EventArray[Index];}

    /** Return a specific SSTInfoElement_ModuleInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_ModuleInfo*       getInfoModule(int Index)       {return &m_ModuleArray[Index];}

    /** Return a specific SSTInfoElement_SubComponentInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_SubComponentInfo*    getInfoSubComponent(int Index)    {return &m_SubComponentArray[Index];}

    /** Return a specific SSTInfoElement_PartitionerInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_PartitionerInfo*  getInfoPartitioner(int Index)  {return &m_PartitionerArray[Index];}

    /** Return a specific SSTInfoElement_GeneratorInfo object. 
     * @param Index The index of the object.
     */
    SSTInfoElement_GeneratorInfo*    getInfoGenerator(int Index)    {return &m_GeneratorArray[Index];}

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
    template<typename T> void addInfoComponent(const T* eic) {m_ComponentArray.emplace_back(const_cast<T*>(eic));}
    template<typename T> void addInfoEvent(const T* eie) {m_EventArray.emplace_back(const_cast<T*>(eie));}
    template<typename T> void addInfoModule(const T* eim) {m_ModuleArray.emplace_back(const_cast<T*>(eim));}
    template<typename T> void addInfoSubComponent(const T* eisc) {m_SubComponentArray.emplace_back(const_cast<T*>(eisc));}
    template<typename T> void addInfoPartitioner(const T* eip) {m_PartitionerArray.emplace_back(const_cast<T*>(eip));}
    template<typename T> void addInfoGenerator(const T* eig) {m_GeneratorArray.emplace_back(const_cast<T*>(eig));}

    const ElementLibraryInfo*                 m_eli;
    std::vector<SSTInfoElement_ComponentInfo>    m_ComponentArray;
    std::vector<SSTInfoElement_EventInfo>        m_EventArray;
    std::vector<SSTInfoElement_ModuleInfo>       m_ModuleArray;
    std::vector<SSTInfoElement_SubComponentInfo> m_SubComponentArray;
    std::vector<SSTInfoElement_PartitionerInfo>  m_PartitionerArray;
    std::vector<SSTInfoElement_GeneratorInfo>    m_GeneratorArray;

};

} // namespace SST

#endif  // SST_INFO_H
