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


#ifndef SST_INFO_H
#define SST_INFO_H

#include <map>
#include <vector>
#include <set>

#include <sst/core/element.h>
#include <sst/core/elementinfo.h>

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
    bool                      doVerbose() const { return m_optionBits & CFG_VERBOSE; }

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


class SSTInfoElement_Outputter {
public:
    /** Output the Parameter Information. 
     * @param Index The Index of the Parameter.
     */
    virtual void outputHumanReadable(int index) = 0;

    /** Create the formatted XML data of the Parameter.
     * @param Index The Index of the Parameter.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    virtual void outputXML(int Index, TiXmlNode* XMLParentElement) = 0;

protected:
    void xmlComment(TiXmlNode* owner, const char* fmt...);
};

class SSTInfoElement_BaseInfo : public SSTInfoElement_Outputter {
public:

    template <typename T>
    SSTInfoElement_BaseInfo( const T *eli ) :
        m_name(eli->name), m_desc(fs(eli->description))
    { }

    SSTInfoElement_BaseInfo( BaseElementInfo &eli ) :
        m_name(eli.getName()), m_desc(eli.getDescription())
    { }

    /* Because PartitionerElementInfo doesn't subclass from BaseElmentInfo... */
    SSTInfoElement_BaseInfo( PartitionerElementInfo &eli ) :
        m_name(eli.getName()), m_desc(eli.getDescription())
    { }


    /** Return the Name of the Element. */
    const std::string& getName() {return m_name;}

    /** Return the Description of the Parameter. */
    const std::string& getDesc() {return m_desc;}

protected:
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

    std::string m_name;
    std::string m_desc;
};


/**
 * The SSTInfo representation of ElementInfoParam object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoParam objects. 
 */
class SSTInfoElement_ParamInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_ParamInfo object.
     * @param elparam Pointer to an ElementInfoParam object.
     */
    SSTInfoElement_ParamInfo(const ElementInfoParam* elparam) :
        SSTInfoElement_BaseInfo(elparam)
    {
        m_defaultValue = (elparam->defaultValue) ? elparam->defaultValue : "REQUIRED";
    }


    /** Return the Default value of the Parameter. */
    const std::string& getDefault() {return m_defaultValue;}

    /** Output the Parameter Information. 
     * @param Index The Index of the Parameter.
     */
    void outputHumanReadable(int index) override;

    /** Create the formatted XML data of the Parameter.
     * @param Index The Index of the Parameter.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
    std::string m_defaultValue;
};


/**
 * The SSTInfo representation of ElementInfoPort object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoPort objects. 
 */
class SSTInfoElement_PortInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_PortInfo object.
     * @param elport Pointer to an ElementInfoPort object.
     */
    SSTInfoElement_PortInfo(const ElementInfoPort* elport) :
        SSTInfoElement_BaseInfo(elport)
    {
        const char **arrayPtr = elport->validEvents;
        while ( arrayPtr && *arrayPtr ) {
            m_validEvents.emplace_back(*arrayPtr);
            arrayPtr++;
        }
    }
    SSTInfoElement_PortInfo(const ElementInfoPort2* elport) :
        SSTInfoElement_BaseInfo(elport)
    {
        m_validEvents = elport->validEvents;
    }

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
    void outputHumanReadable(int index) override;

    /** Create the formatted XML data of the Port.
     * @param Index The Index of the Port.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
    void analyzeValidEventsArray();

    std::vector<std::string> m_validEvents;

};


/**
 * The SSTInfo representation of StatisticInfo object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoPort objects. 
 */
class SSTInfoElement_StatisticInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_StatisticInfo object.
     * @param elstat Pointer to an ElementInfoStatistic object.
     */
    SSTInfoElement_StatisticInfo(const ElementInfoStatistic* els) :
        SSTInfoElement_BaseInfo(els),
        m_units(els->units), m_enableLevel(els->enableLevel)
    { }

    /** Return the Units of the Statistic. */
    const std::string& getUnits() {return m_units;}

    /** Return the enable level of the Statistic. */
    uint8_t getEnableLevel() {return m_enableLevel;}

    /** Output the Statistic Information. 
     * @param Index The Index of the Statistic.
     */
    void outputHumanReadable(int Index) override;

    /** Create the formatted XML data of the Statistic.
     * @param Index The Index of the Statistic.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
    std::string m_units;
    uint8_t m_enableLevel;
};


/**
 * The SSTInfo representation of SubComponentSlotINfo object.
 */
class SSTInfoElement_SubCompSlotInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_StatisticInfo object.
     * @param elstat Pointer to an ElementInfoStatistic object.
     */
    SSTInfoElement_SubCompSlotInfo(const ElementInfoSubComponentSlot* els) :
        SSTInfoElement_BaseInfo(els),
        m_interface(els->superclass)
    { }

    /** Return the Interface which is requires */
    const std::string& getInterface() {return m_interface;}

    /** Output the Statistic Information.
     * @param Index The Index of the Statistic.
     */
    void outputHumanReadable(int Index) override;

    /** Create the formatted XML data of the Statistic.
     * @param Index The Index of the Statistic.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
    std::string m_interface;
};



/**
 * The SSTInfo representation of ElementInfoComponent object.
 *
 * This class is used internally by SSTInfo to load and process
 * ElementInfoComponent objects.
 */
class SSTInfoElement_ComponentInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_ComponentInfo object.
     * @param elc Pointer to an ElementInfoComponent object.
     */
    SSTInfoElement_ComponentInfo(const ElementInfoComponent* elc) :
        SSTInfoElement_BaseInfo(elc), m_category(elc->category)
    {
        m_ParamArray = convertFromELI<SSTInfoElement_ParamInfo>(elc->params);
        m_PortArray = convertFromELI<SSTInfoElement_PortInfo>(elc->ports);
        m_StatisticArray = convertFromELI<SSTInfoElement_StatisticInfo>(elc->stats);
        m_SubCompSlotArray = convertFromELI<SSTInfoElement_SubCompSlotInfo>(elc->subComponents);
    }

    SSTInfoElement_ComponentInfo(ComponentElementInfo* elc) :
        SSTInfoElement_BaseInfo(*elc), m_category(elc->getCategory())
    {
        m_ParamArray = convertFromDB<SSTInfoElement_ParamInfo>(elc->getValidParams());
        m_PortArray = convertFromDB<SSTInfoElement_PortInfo>(elc->getValidPorts());
        m_StatisticArray = convertFromDB<SSTInfoElement_StatisticInfo>(elc->getValidStats());
        m_SubCompSlotArray = convertFromDB<SSTInfoElement_SubCompSlotInfo>(elc->getSubComponentSlots());
    }

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
    void outputHumanReadable(int index) override;

    /** Create the formatted XML data of the Component.
     * @param Index The Index of the Component.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
    uint32_t                                          m_category;
    std::vector<SSTInfoElement_ParamInfo>             m_ParamArray;
    std::vector<SSTInfoElement_PortInfo>              m_PortArray;
    std::vector<SSTInfoElement_StatisticInfo>         m_StatisticArray;
    std::vector<SSTInfoElement_SubCompSlotInfo>       m_SubCompSlotArray;
};


/**
 * The SSTInfo representation of ElementInfoEvent object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoEvent objects. 
 */
class SSTInfoElement_EventInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_EventInfo object.
     * @param ele Pointer to an ElementInfoEvent object.
     */
    SSTInfoElement_EventInfo(const ElementInfoEvent* ele) :
        SSTInfoElement_BaseInfo(ele)
    { }


    /** Output the Event Information. 
     * @param Index The Index of the Event.
     */
    void outputHumanReadable(int index) override;

    /** Create the formatted XML data of the Event.
     * @param Index The Index of the Event.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
};

/**
 * The SSTInfo representation of ElementInfoModule object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoModule objects. 
 */
class SSTInfoElement_ModuleInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_ModuleInfo object.
     * @param elm Pointer to an ElementInfoModule object.
     */
    SSTInfoElement_ModuleInfo(const ElementInfoModule* elm) :
        SSTInfoElement_BaseInfo(elm), m_provides(fs(elm->provides))
    {
        m_ParamArray = convertFromELI<SSTInfoElement_ParamInfo>(elm->params);
    }
    SSTInfoElement_ModuleInfo(ModuleElementInfo* elm) :
        SSTInfoElement_BaseInfo(*elm), m_provides(elm->getInterface())
    {
        m_ParamArray = convertFromDB<SSTInfoElement_ParamInfo>(elm->getValidParams());
    }

    /** Return what class the Module provides. */
    const std::string&           getProvides() { return m_provides;}

    /** Return a Parameter Info Object. 
     * @param index The index of the Parameter.
     */
    SSTInfoElement_ParamInfo* getParamInfo(int index) {return &m_ParamArray[index];}

    /** Output the Module Information. 
     * @param Index The Index of the Module.
     */
    void outputHumanReadable(int index) override;

    /** Create the formatted XML data of the Module.
     * @param Index The Index of the Module.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
    std::string m_provides;
    std::vector<SSTInfoElement_ParamInfo> m_ParamArray;
};

/**
 * The SSTInfo representation of ElementInfoSubComponent object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoSubComponent objects. 
 */
class SSTInfoElement_SubComponentInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_SubComponentInfo object.
     * @param elsc Pointer to an ElementInfoComponent object.
     */
    SSTInfoElement_SubComponentInfo(const ElementInfoSubComponent* elsc) :
        SSTInfoElement_BaseInfo(elsc), m_Provides(elsc->provides)
    {
        m_ParamArray = convertFromELI<SSTInfoElement_ParamInfo>(elsc->params);
        m_PortArray = convertFromELI<SSTInfoElement_PortInfo>(elsc->ports);
        m_StatisticArray = convertFromELI<SSTInfoElement_StatisticInfo>(elsc->stats);
        m_SubCompSlotArray = convertFromELI<SSTInfoElement_SubCompSlotInfo>(elsc->subComponents);
    }

    SSTInfoElement_SubComponentInfo(SubComponentElementInfo* elc) :
        SSTInfoElement_BaseInfo(*elc), m_Provides(elc->getInterface())
    {
        m_ParamArray = convertFromDB<SSTInfoElement_ParamInfo>(elc->getValidParams());
        m_PortArray = convertFromDB<SSTInfoElement_PortInfo>(elc->getValidPorts());
        m_StatisticArray = convertFromDB<SSTInfoElement_StatisticInfo>(elc->getValidStats());
        m_SubCompSlotArray = convertFromDB<SSTInfoElement_SubCompSlotInfo>(elc->getSubComponentSlots());
    }

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
    void outputHumanReadable(int index) override;

    /** Create the formatted XML data of the Component.
     * @param Index The Index of the Component.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

    const std::string& getProvides() const { return m_Provides; }

private:
    std::vector<SSTInfoElement_ParamInfo>             m_ParamArray;
    std::vector<SSTInfoElement_PortInfo>              m_PortArray;
    std::vector<SSTInfoElement_StatisticInfo>         m_StatisticArray;
    std::vector<SSTInfoElement_SubCompSlotInfo>       m_SubCompSlotArray;
    std::string                                       m_Provides;
};

/**
 * The SSTInfo representation of ElementInfoPartitioner object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoPartitioner objects. 
 */
class SSTInfoElement_PartitionerInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_PartitionerInfo object.
     * @param elp Pointer to an ElementInfoPartitioner object.
     */
    SSTInfoElement_PartitionerInfo(const ElementInfoPartitioner* elp) :
        SSTInfoElement_BaseInfo(elp)
    { }

    SSTInfoElement_PartitionerInfo(PartitionerElementInfo* elp) :
        SSTInfoElement_BaseInfo(*elp)
    { }

    /** Output the Partitioner Information. 
     * @param Index The Index of the Partitioner.
     */
    void outputHumanReadable(int index) override;

    /** Create the formatted XML data of the Partitioner.
     * @param Index The Index of the Partitioner.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
};

/**
 * The SSTInfo representation of ElementInfoGenerator object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementInfoGenerator objects. 
 */
class SSTInfoElement_GeneratorInfo : public SSTInfoElement_BaseInfo {
public:
    /** Create a new SSTInfoElement_GeneratorInfo object.
     * @param elg Pointer to an ElementInfoGenerator object.
     */
    SSTInfoElement_GeneratorInfo(const ElementInfoGenerator* elg) :
        SSTInfoElement_BaseInfo(elg)
    { }

    /** Output the Generator Information. 
     * @param Index The Index of the Generator.
     */
    void outputHumanReadable(int index) override;

    /** Create the formatted XML data of the Generator.
     * @param Index The Index of the Generator.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int Index, TiXmlNode* XMLParentElement) override;

private:
};


/**
 * The SSTInfo representation of ElementLibraryInfo object.
 *
 * This class is used internally by SSTInfo to load and process  
 * ElementLibraryInfo objects. 
 */
class SSTInfoElement_LibraryInfo : public SSTInfoElement_Outputter {

public:
    /** Create a new SSTInfoElement_LibraryInfo object.
     * @param eli Pointer to an ElementLibraryInfo object.
     */
    SSTInfoElement_LibraryInfo(const std::string& name, const ElementLibraryInfo* eli) :
        m_name(name)
    {
        m_eli = eli;
        populateLibraryInfo();
    }

    /** Return the Name of the Library. */
    // std::string getLibraryName() {if (m_eli && m_eli->name) return m_eli->name; else return ""; }
    std::string getLibraryName() {return m_name; }

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
    void outputHumanReadable(int LibIndex) override;

    /** Create the formatted XML data of the Library.
     * @param LibIndex The Index of the Library.
     * @param XMLParentElement The parent element to receive the XML data.
     */
    void outputXML(int LibIndex, TiXmlNode* XMLParentElement) override;

private:
    void populateLibraryInfo();
    template<typename T> void addInfoComponent(const T* eic) {m_ComponentArray.emplace_back(const_cast<T*>(eic));}
    template<typename T> void addInfoEvent(const T* eie) {m_EventArray.emplace_back(const_cast<T*>(eie));}
    template<typename T> void addInfoModule(const T* eim) {m_ModuleArray.emplace_back(const_cast<T*>(eim));}
    template<typename T> void addInfoSubComponent(const T* eisc) {m_SubComponentArray.emplace_back(const_cast<T*>(eisc));}
    template<typename T> void addInfoPartitioner(const T* eip) {m_PartitionerArray.emplace_back(const_cast<T*>(eip));}
    template<typename T> void addInfoGenerator(const T* eig) {m_GeneratorArray.emplace_back(const_cast<T*>(eig));}

    const ElementLibraryInfo*                    m_eli;
    const std::string                            m_name;
    std::vector<SSTInfoElement_ComponentInfo>    m_ComponentArray;
    std::vector<SSTInfoElement_EventInfo>        m_EventArray;
    std::vector<SSTInfoElement_ModuleInfo>       m_ModuleArray;
    std::vector<SSTInfoElement_SubComponentInfo> m_SubComponentArray;
    std::vector<SSTInfoElement_PartitionerInfo>  m_PartitionerArray;
    std::vector<SSTInfoElement_GeneratorInfo>    m_GeneratorArray;

};

} // namespace SST

#endif  // SST_INFO_H
