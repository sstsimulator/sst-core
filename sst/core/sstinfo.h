// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_INFO_H
#define SST_INFO_H

namespace SST { 
    
// CONFIGURATION BITS    
#define CFG_FORMATHUMAN 0x00000001
    
class ConfigSSTInfo {
public:    
    ConfigSSTInfo();
    ~ConfigSSTInfo();
    int parseCmdLine(int argc, char* argv[]);

    std::vector<std::string>* GetElementsToProcessArray() {return &m_elementsToProcess;}
    unsigned int              GetOptionBits() {return m_optionBits;}
    
private:      
    boost::program_options::options_description* m_configDesc;
    boost::program_options::variables_map*       m_vm;
    std::vector<std::string>                     m_elementsToProcess;
    unsigned int                                 m_optionBits;
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

    void outputParameterInfo(int Index);
    
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

class SSTElement_ComponentInfo {
public:
    SSTElement_ComponentInfo(const ElementInfoComponent* elc)
    {
        const ElementInfoParam* ptrParams;
        
        // Save the Object
        m_elc = elc;

        ptrParams = elc->params;  // Pointer to the Params Structure Array
        PopulateParams(ptrParams, &m_ParamArray);        
    }
    
    const char*           getName() {return m_elc->name;}
    const char*           getDesc() {return m_elc->description;}
    SSTElement_ParamInfo* getParamInfo(int index) {return m_ParamArray[index];}

    void outputComponentInfo(int Index);
    
private:    
    const ElementInfoComponent*        m_elc;
    std::vector<SSTElement_ParamInfo*> m_ParamArray;
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
    
    std::string getLibraryName() {if (m_eli) return m_eli->name; else return ""; }
    std::string getLibraryDescription() {if (m_eli) return m_eli->description; else return ""; }

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

    void OutputLibraryInfo(int LibIndex);

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
