// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
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
    
class ConfigSSTInfo {
public:    
    ConfigSSTInfo();
    ~ConfigSSTInfo();
    int parseCmdLine(int argc, char* argv[]);

    std::vector<std::string>* getElementsToProcessArray() {return &m_elementsToProcess;}
    std::vector<std::string>* getFilteredElementNamesArray() {return &m_filteredElementNames;}
    std::vector<std::string>* getFilteredElementComponentNamesArray() {return &m_filteredElementComponentNames;}
    unsigned int              getOptionBits() {return m_optionBits;}
    std::string&              getXMLFilePath() {return m_XMLFilePath;}
private:
    void outputUsage();
    void outputVersion();
    
private:      
    char*                                                   m_AppName;
    boost::program_options::options_description*            m_configDesc;
    boost::program_options::options_description*            m_hiddenDesc;
    boost::program_options::positional_options_description* m_posDesc;
    boost::program_options::variables_map*                  m_vm;
    
    std::vector<std::string>                                m_elementsToProcess;    std::vector<std::string>                                m_filteredElementNames;
    std::vector<std::string>                                m_filteredElementComponentNames;
    unsigned int                                            m_optionBits;
    std::string                                             m_XMLFilePath;
};    
    
class SSTElement_ParamInfo {
public:
    SSTElement_ParamInfo(const ElementInfoParam* elparam)
    {
        // Save the Object
        m_elparam = elparam;
    }

    const char* getName() {return m_elparam->name;}
    const char* getDesc() {return m_elparam->description;}
    const char* getDefault() {return (m_elparam->defaultValue) ? m_elparam->defaultValue : "REQUIRED";}

    void outputParameterInfo(int Index);
    void generateParameterInfoXMLData(int Index, TiXmlNode* XMLParentElement);
    
private:    
    const ElementInfoParam* m_elparam;
};
    
void PopulateParams(const ElementInfoParam* ptrParams, std::vector<SSTElement_ParamInfo*>* ptrParamArray)
{
    // Populate the Parameter Array
    if (NULL != ptrParams) {
        while (NULL != ptrParams->name) {
            // Create a new SSTElement_ParamInfo and add it to the m_ParamArray
            SSTElement_ParamInfo* ptrParamInfo = new SSTElement_ParamInfo(ptrParams);
            ptrParamArray->push_back(ptrParamInfo);

            // If the name is NULL, we have reached the last item
            ptrParams++;  // Get the next structure item
        }
    }
}

class SSTElement_PortInfo {
public:
    SSTElement_PortInfo(const ElementInfoPort* elport)
    {
        // Save the Object
        m_numValidEvents = 0;
        m_elport = elport;
        
        analyzeValidEventsArray();
    }

    const char* getName() {return m_elport->name;}
    const char* getDesc() {return m_elport->description;}
    const char** getValidEvents() {return m_elport->validEvents;}

    int         getNumberOfValidEvents()    {return m_numValidEvents;}
    const char* getValidEvent(unsigned int index); 
    
    void outputPortInfo(int Index);
    void generatePortInfoXMLData(int Index, TiXmlNode* XMLParentElement);
    
    void analyzeValidEventsArray();
    
private:    
    const ElementInfoPort*   m_elport;
    unsigned int             m_numValidEvents;
};
    
void PopulatePorts(const ElementInfoPort* ptrPorts, std::vector<SSTElement_PortInfo*>* ptrPortArray)
{
    // Populate the Ports Array
    if (NULL != ptrPorts) {
        while (NULL != ptrPorts->name) {
            // Create a new SSTElement_PortInfo and add it to the m_PortArray
            SSTElement_PortInfo* ptrPortInfo = new SSTElement_PortInfo(ptrPorts);
            ptrPortArray->push_back(ptrPortInfo);

            // If the name is NULL, we have reached the last item
            ptrPorts++;  // Get the next structure item
        }
    }
}

class SSTElement_ComponentInfo {
public:
    SSTElement_ComponentInfo(const ElementInfoComponent* elc)
    {
        const ElementInfoParam* ptrParams;
        const ElementInfoPort*  ptrPorts;
        
        // Save the Object
        m_elc = elc;

        ptrParams = elc->params;  // Pointer to the Params Structure Array
        PopulateParams(ptrParams, &m_ParamArray);        

        ptrPorts = elc->ports;  // Pointer to the Ports Structure Array
        PopulatePorts(ptrPorts, &m_PortArray);
        
        buildCategoryString();        
    }
    
    const char*           getName() {return m_elc->name;}
    const char*           getDesc() {return m_elc->description;}
    SSTElement_ParamInfo* getParamInfo(int index) {return m_ParamArray[index];}
    SSTElement_PortInfo*  getPortInfo(int index) {return m_PortArray[index];}
    uint32_t              getCategoryValue() {return m_elc->category;}
    const char*           getCategoryString() {return m_CategoryString.c_str();}

    void outputComponentInfo(int Index);
    void generateComponentInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    void buildCategoryString();
    
private:    
    const ElementInfoComponent*        m_elc;
    std::vector<SSTElement_ParamInfo*> m_ParamArray;
    std::vector<SSTElement_PortInfo*>  m_PortArray;
    std::string                        m_CategoryString;
};

class SSTElement_IntrospectorInfo {
public:
    SSTElement_IntrospectorInfo(const ElementInfoIntrospector* eli)
    {
        const ElementInfoParam* ptrParams;
        
        // Save the Object
        m_eli = eli;

        ptrParams = eli->params;  // Pointer to the Params Structure Array
        PopulateParams(ptrParams, &m_ParamArray);        
    }

    const char*           getName() {return m_eli->name;}
    const char*           getDesc() {return m_eli->description;}
    SSTElement_ParamInfo* getParamInfo(int index) {return m_ParamArray[index];}

    void outputIntrospectorInfo(int Index);
    void generateIntrospectorInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoIntrospector*     m_eli;
    std::vector<SSTElement_ParamInfo*> m_ParamArray;
};

class SSTElement_EventInfo {
public:
    SSTElement_EventInfo(const ElementInfoEvent* ele)
    {
        // Save the Object
        m_ele = ele;
    }

    const char* getName() {return m_ele->name;}
    const char* getDesc() {return m_ele->description;}

    void outputEventInfo(int Index);
    void generateEventInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoEvent* m_ele;
};

class SSTElement_ModuleInfo {
public:
    SSTElement_ModuleInfo(const ElementInfoModule* elm)
    {
        const ElementInfoParam* ptrParams;
        
        // Save the Object
        m_elm = elm;

        ptrParams = elm->params;  // Pointer to the Params Structure Array
        PopulateParams(ptrParams, &m_ParamArray);        
    }

    const char*           getName() {return m_elm->name;}
    const char*           getDesc() {return m_elm->description;}
    SSTElement_ParamInfo* getParamInfo(int index) {return m_ParamArray[index];}

    void outputModuleInfo(int Index);
    void generateModuleInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoModule*           m_elm;
    std::vector<SSTElement_ParamInfo*> m_ParamArray;
};

class SSTElement_PartitionerInfo {
public:
    SSTElement_PartitionerInfo(const ElementInfoPartitioner* elp)
    {
        // Save the Object
        m_elp = elp;
    }

    const char* getName() {return m_elp->name;}
    const char* getDesc() {return m_elp->description;}

    void outputPartitionerInfo(int Index);
    void generatePartitionerInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoPartitioner* m_elp;
};

class SSTElement_GeneratorInfo {
public:
    SSTElement_GeneratorInfo(const ElementInfoGenerator* elg)
    {
        // Save the Object
        m_elg = elg;
    }

    const char* getName() {return m_elg->name;}
    const char* getDesc() {return m_elg->description;}

    void outputGeneratorInfo(int Index);
    void generateGeneratorInfoXMLData(int Index, TiXmlNode* XMLParentElement); 
    
private:    
    const ElementInfoGenerator* m_elg;
};


class SSTElement_LibraryInfo {

public:
    SSTElement_LibraryInfo(const ElementLibraryInfo* eli)
    {
        m_eli = eli;
        populateLibraryInfo();
    }

    void populateLibraryInfo();
    
    std::string getLibraryName() {if (m_eli && m_eli->name) return m_eli->name; else return ""; }
    std::string getLibraryDescription() {if (m_eli && m_eli->description) return m_eli->description; else return ""; }

    int         getNumberOfLibraryComponents()    {return m_ComponentArray.size();}
    int         getNumberOfLibraryIntrospectors() {return m_IntrospectorArray.size();}
    int         getNumberOfLibraryEvents()        {return m_EventArray.size();}
    int         getNumberOfLibraryModules()       {return m_ModuleArray.size();}
    int         getNumberOfLibraryPartitioners()  {return m_PartitionerArray.size();}
    int         getNumberOfLibraryGenerators()    {return m_GeneratorArray.size();}

    
    const ElementLibraryInfo*    getLibraryInfo() {return m_eli;}

    SSTElement_ComponentInfo*    getInfoComponent(int Index)    {return m_ComponentArray[Index];}
    SSTElement_IntrospectorInfo* getInfoIntrospector(int Index) {return m_IntrospectorArray[Index];}
    SSTElement_EventInfo*        getInfoEvent(int Index)        {return m_EventArray[Index];}
    SSTElement_ModuleInfo*       getInfoModule(int Index)       {return m_ModuleArray[Index];}
    SSTElement_PartitionerInfo*  getInfoPartitioner(int Index)  {return m_PartitionerArray[Index];}
    SSTElement_GeneratorInfo*    getInfoGenerator(int Index)    {return m_GeneratorArray[Index];}

    void outputLibraryInfo(int LibIndex);
    void generateLibraryInfoXMLData(int LibIndex, TiXmlNode* XMLParentElement);

private:
    void addInfoComponent(const ElementInfoComponent* eic) {m_ComponentArray.push_back(new SSTElement_ComponentInfo(eic));}
    void addInfoIntrospector(const ElementInfoIntrospector* eii) {m_IntrospectorArray.push_back(new SSTElement_IntrospectorInfo(eii));}
    void addInfoEvent(const ElementInfoEvent* eie) {m_EventArray.push_back(new SSTElement_EventInfo(eie));}
    void addInfoModule(const ElementInfoModule* eim) {m_ModuleArray.push_back(new SSTElement_ModuleInfo(eim));}
    void addInfoPartitioner(const ElementInfoPartitioner* eip) {m_PartitionerArray.push_back(new SSTElement_PartitionerInfo(eip));}
    void addInfoGenerator(const ElementInfoGenerator* eig) {m_GeneratorArray.push_back(new SSTElement_GeneratorInfo(eig));}
    
    const ElementLibraryInfo*                 m_eli;
    std::vector<SSTElement_ComponentInfo*>    m_ComponentArray;
    std::vector<SSTElement_IntrospectorInfo*> m_IntrospectorArray;
    std::vector<SSTElement_EventInfo*>        m_EventArray;
    std::vector<SSTElement_ModuleInfo*>       m_ModuleArray;
    std::vector<SSTElement_PartitionerInfo*>  m_PartitionerArray;
    std::vector<SSTElement_GeneratorInfo*>    m_GeneratorArray;

};

} // namespace SSTINFO

#endif  // SST_INFO_H
