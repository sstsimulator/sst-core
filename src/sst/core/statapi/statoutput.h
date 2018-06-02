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

#ifndef _H_SST_CORE_STATISTICS_OUTPUT
#define _H_SST_CORE_STATISTICS_OUTPUT

#include "sst/core/sst_types.h"
#include <sst/core/warnmacros.h>
#include <sst/core/module.h>
#include <sst/core/params.h>
#include <sst/core/statapi/statfieldinfo.h>
#include <sst/core/statapi/statbase.h>
#include <unordered_map>

#include <mutex>

// Default Settings for Statistic Output and Load Level
#define STATISTICSDEFAULTOUTPUTNAME "sst.statOutputConsole"
#define STATISTICSDEFAULTLOADLEVEL 0

namespace SST {
class BaseComponent;
class Simulation;
namespace Statistics {
class StatisticProcessingEngine;
class StatisticGroup;

////////////////////////////////////////////////////////////////////////////////
    
/**
    \class StatisticOutput

	Forms the base class for statistics output generation within the SST core. 
	Statistics are gathered by the statistic objects and then processed sent to 
	the derived output object either periodically or by event and/or also at 
	the end of the simulation.  A single statistic output will be created by the 
	simulation (per node) and will collect the data per its design.
*/
class StatisticOutput : public Module
{
public:    
    typedef StatisticFieldInfo::fieldType_t   fieldType_t;  
    typedef StatisticFieldInfo::fieldHandle_t fieldHandle_t;
    typedef std::vector<StatisticFieldInfo*>  FieldInfoArray_t;                          
    typedef std::unordered_map<std::string, fieldHandle_t>  FieldNameMap_t;                          

public:    
    /** Construct a base StatisticOutput
     * @param outputParameters - The parameters for the statistic Output.
     */
    StatisticOutput(Params& outputParameters);
    ~StatisticOutput();

    /** Return the Statistic Output name */
    std::string& getStatisticOutputName() {return m_statOutputName;}

    /** Return the parameters for the StatisticOutput */
    Params& getOutputParameters() {return m_outputParameters;}

    /** True if this StatOutput can handle StatisticGroups */
    virtual bool acceptsGroups() const { return false; }


/////////////////
// Methods for Registering Fields (Called by Statistic Objects)
public:
    /** Register a field to be output (templated function)
     * @param fieldName - The name of the field.
     * @return The handle of the registered field or -1 if type is not supported.
     * Note: Any field names (of the same data type) that are previously  
     *       registered by a statistic will return the previously
     *       handle.
     */
    // Field Registration
    // ONLY SUPPORTED TYPES ARE int32_t, uint32_t, int64_t, uint64_t, float, double
    template<typename T>
    fieldHandle_t registerField(const char* fieldName)
    {
        StatisticFieldInfo::fieldType_t FieldType = StatisticFieldInfo::StatisticFieldInfo::getFieldTypeFromTemplate<T>();
        if ( FieldType == StatisticFieldInfo::UNDEFINED )
            return -1;

        auto res = generateFileHandle(addFieldToLists(fieldName, FieldType));
        implRegisteredField(res);
        return res;
    }
    
//    /** Adjust the hierarchy of the fields (FUTURE SUPPORT)
//     * @param fieldHandle - The handle of the field to adjust.
//     * @param Level - The level of the field.
//     * @param parent - The parent field of the field.
//     */
//    void setFieldHierarchy(fieldHandle_t fieldHandle, uint32_t Level, fieldHandle_t parent);
    
    /** Return the information on a registered field via the field handle.
     * @param fieldHandle - The handle of the registered field.
     * @return Pointer to the registered field info.
     */
    // Get the Field Information object, NULL is returned if not found
    StatisticFieldInfo* getRegisteredField(fieldHandle_t fieldHandle);
    
    /** Return the information on a registered field via known names.
     * @param componentName - The name of the component.
     * @param statisticName - The name of the statistic.
     * @param fieldName - The name of the field .
     * @return Pointer to the registered field info.
     */
    // Get Registered Fields
    // ONLY SUPPORTED TYPES ARE int32_t, uint32_t, int64_t, uint64_t, float, double
    template<typename T>
    StatisticFieldInfo* getRegisteredField(const char* statisticName, const char* fieldName)
    {
        StatisticFieldInfo*             NewStatFieldInfo;
        StatisticFieldInfo*             ExistingStatFieldInfo;
        StatisticFieldInfo::fieldType_t FieldType = StatisticFieldInfo::StatisticFieldInfo::getFieldTypeFromTemplate<T>();

        NewStatFieldInfo = new StatisticFieldInfo(statisticName, fieldName, FieldType);

        // Now search the FieldNameMap_t of type for a matching entry
        FieldNameMap_t::const_iterator found = m_outputFieldNameMap.find(NewStatFieldInfo->getFieldUniqueName());
        if (found != m_outputFieldNameMap.end()) {
            // We found a map entry, now get the StatFieldInfo from the m_outputFieldInfoArray at the index given by the map
            // and then delete the NewStatFieldInfo to prevent a leak
            ExistingStatFieldInfo = m_outputFieldInfoArray[found->second];
            delete NewStatFieldInfo;
            return ExistingStatFieldInfo;
        }

        delete NewStatFieldInfo;
        return NULL;
    }    
    
    /** Return the array of registered field infos. */
    FieldInfoArray_t& getFieldInfoArray() {return m_outputFieldInfoArray;}

/////////////////
    /** Output field data.  
     * @param fieldHandle - The handle of the registered field.
     * @param data - The data to be output.
     */
    // Methods for Outputting Fields  (Called by Statistic Objects)
    // Output fields (will call virtual functions of Derived Output classes)
    void outputField(fieldHandle_t fieldHandle, int32_t data);  
    void outputField(fieldHandle_t fieldHandle, uint32_t data);  
    void outputField(fieldHandle_t fieldHandle, int64_t data);  
    void outputField(fieldHandle_t fieldHandle, uint64_t data);  
    void outputField(fieldHandle_t fieldHandle, float data);  
    void outputField(fieldHandle_t fieldHandle, double data);
    
    /** Output field data.  
     * @param type - The field type to get name of.
     * @return String name of the field type.
     */
    const char* getFieldTypeShortName(fieldType_t type);
    
protected:
    friend class SST::Simulation;
    friend class SST::Statistics::StatisticProcessingEngine;

    // Routine to have Output Check its options for validity
    /** Have the Statistic Output check its parameters
     * @return True if all parameters are ok; False if a parameter is missing or incorrect.
     */ 
    virtual bool checkOutputParameters() = 0;

    /** Have Statistic Object print out its usage and parameter info. 
     *  Called when checkOutputParameters() returns false */
    virtual void printUsage() = 0;


    virtual void implStartRegisterFields(StatisticBase *UNUSED(statistic)) {}
    virtual void implRegisteredField(fieldHandle_t UNUSED(fieldHandle)) {}
    virtual void implStopRegisterFields() {}

    // Simulation Events
    /** Indicate to Statistic Output that simulation has started.
      * Allows object to perform any setup required. */ 
    virtual void startOfSimulation() = 0;

    /** Indicate to Statistic Output that simulation has ended.
      * Allows object to perform any shutdown required. */ 
    virtual void endOfSimulation() = 0;
    
    // Start / Stop of output
    /** Indicate to Statistic Output that a statistic is about to send data to be output
      * Allows object to perform any initialization before output. */ 
    virtual void implStartOutputEntries(StatisticBase* statistic) = 0;

    /** Indicate to Statistic Output that a statistic is finished sending data to be output
      * Allows object to perform any cleanup. */ 
    virtual void implStopOutputEntries() = 0;

    virtual void implStartRegisterGroup(StatisticGroup* UNUSED(group)) {}
    virtual void implStopRegisterGroup() {}
    virtual void implStartOutputGroup(StatisticGroup* UNUSED(group)) {}
    virtual void implStopOutputGroup() {}
    // Field Outputs
    /** Implementation of outputField() for derived classes.  
      * Perform the actual implementation of the output. */ 
    virtual void implOutputField(fieldHandle_t fieldHandle, int32_t data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, uint32_t data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, int64_t data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, uint64_t data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, float data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, double data) = 0;


private:

    // Start / Stop of register Fields
    void registerStatistic(StatisticBase *stat);
    void registerGroup(StatisticGroup *group);

    void startRegisterFields(StatisticBase *statistic);
    void stopRegisterFields();

    // Start / Stop of output
    void outputEntries(StatisticBase* statistic, bool endOfSimFlag);
    void startOutputEntries(StatisticBase* statistic);
    void stopOutputEntries();

    void outputGroup(StatisticGroup* group, bool endOfSimFlag);
    void startOutputGroup(StatisticGroup* group);
    void stopOutputGroup();

    // Other support functions
    StatisticFieldInfo* addFieldToLists(const char* fieldName, fieldType_t fieldType);
    fieldHandle_t generateFileHandle(StatisticFieldInfo* FieldInfo);


protected:     
    StatisticOutput() {;} // For serialization only
    void setStatisticOutputName(std::string name) {m_statOutputName = name;}

    void lock() { m_lock.lock(); }
    void unlock() { m_lock.unlock(); }

private:
    std::string      m_statOutputName;
    Params           m_outputParameters;
    FieldInfoArray_t m_outputFieldInfoArray;
    FieldNameMap_t   m_outputFieldNameMap;
    fieldHandle_t    m_highestFieldHandle;
    std::string      m_currentFieldStatName;
    std::recursive_mutex  m_lock;

};                          

} //namespace Statistics
} //namespace SST

#endif
