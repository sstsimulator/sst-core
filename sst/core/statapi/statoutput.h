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

#ifndef _H_SST_CORE_STATISTICS_OUTPUT
#define _H_SST_CORE_STATISTICS_OUTPUT

#include "sst/core/sst_types.h"
#include <sst/core/serialization.h>
#include <sst/core/module.h>
#include <sst/core/params.h>
#include <sst/core/statapi/statbase.h>

// Default Settings for Statistic Output and Load Level
#define STATISTICSDEFAULTOUTPUTNAME "sst.statOutputConsole"
#define STATISTICSDEFAULTLOADLEVEL 0

namespace SST {
class Component;    
class Simulation;    
namespace Statistics {
class StatisticProcessingEngine;

////////////////////////////////////////////////////////////////////////////////
// Quick Support structures for checking type (Can's use std::is_same as it is only available in C++11)
template <typename, typename> struct is_type_same { static const bool value = false;};
template <typename T> struct is_type_same<T,T> { static const bool value = true;};    

////////////////////////////////////////////////////////////////////////////////
    
/**
    \class StatisticFieldInfo

	The class for representing Statistic Output Fields  
*/
class StatisticFieldInfo
{
public:
    /** Supported Field Types */
    enum fieldType_t {UNDEFINED, UINT32, UINT64, INT32, INT64, FLOAT, DOUBLE, STR};
    typedef int32_t fieldHandle_t;

public:
    /** Construct a StatisticFieldInfo
     * @param statName - Name of the statistic registering this field.
     * @param fieldName - Name of the Field to be assigned.
     * @param fieldType - Data type of the field.
     */
    StatisticFieldInfo(const char* statName, const char* fieldName, fieldType_t fieldType);
   
    // Get Field Data
    /** Return the statistic name related to this field info */
    inline const std::string& getStatName() const {return m_statName;}
    /** Return the field name related to this field info */
    inline const std::string& getFieldName() const {return m_fieldName;}
    /** Return the field type related to this field info */
    fieldType_t getFieldType() {return m_fieldType;}
    
    /** Compare two field info structures 
     * @param fieldType - a FieldInfo to compare against.
     * @return True if the Field Info structures are the same.
     */
    bool operator==(StatisticFieldInfo& FieldInfo1); 
    
    /** Set the field handle 
     * @param handle - The assigned field handle for this FieldInfo
     */
    void setFieldHandle(fieldHandle_t handle) {m_fieldHandle = handle;}

    /** Get the field handle 
     * @return The assigned field handle.
     */
    fieldHandle_t getFieldHandle() {return m_fieldHandle;}

protected:
    StatisticFieldInfo(){}; // For serialization only
    
private:   
    std::string   m_statName; 
    std::string   m_fieldName; 
    fieldType_t   m_fieldType;
    fieldHandle_t m_fieldHandle;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(m_statName); 
        ar & BOOST_SERIALIZATION_NVP(m_fieldName); 
        ar & BOOST_SERIALIZATION_NVP(m_fieldType);
        ar & BOOST_SERIALIZATION_NVP(m_fieldHandle);
    }
};
    
////////////////////////////////////////////////////////////////////////////////

/**
    \class StatisticOutput

	Forms the base class for statistics output generation within the SST core. 
	Statistics are gathered by the statistic objects and then processed sent to 
	the derived output object either periodically or by event and/or also at 
	the end of the simuation.  A single statistic output will be created by the 
	simuation (per node) and will collect the data per its design.
*/
class StatisticOutput : public Module
{
public:    
    typedef StatisticFieldInfo::fieldType_t   fieldType_t;  
    typedef StatisticFieldInfo::fieldHandle_t fieldHandle_t;
    typedef std::vector<StatisticFieldInfo*>  FieldInfoArray_t;                          

public:    
    /** Construct a base StatisticOutput
     * @param outputParameters - The parameters for the statistic Output.
     */
    StatisticOutput(Params& outputParameters);
    ~StatisticOutput();

    /** Return the Statistic Output name */
    std::string& getStatisticOutputName() {return m_statOutputName;}
    
    /** Return the statistics load level for the system */
    uint8_t getStatisticLoadLevel() {return m_statLoadLevel;}
    
    /** Return the parameters for the StatisticOutput */
    Params& getOutputParameters() {return m_outputParameters;}

/////////////////
// Methods for Registering Fields (Called by Statistic Objects)
public:
    /** Register a field to be output (templated function)
     * @param fieldName - The name of the field.
     * @return The handle of the registerd field or -1 if type is not supported.
     * Note: Any field names (of the same data type) that are previously  
     *       registered by a statistic will return the previously
     *       handle.
     */
    // Field Registration
    // ONLY SUPPORTED TYPES ARE int32_t, uint32_t, int64_t, uint64_t, float, double, const char*
    template<typename T>
    fieldHandle_t registerField(const char* fieldName)
    {
        if (is_type_same<T, int32_t    >::value){return generateFileHandle(addFieldToLists(fieldName, StatisticFieldInfo::INT32)); }
        if (is_type_same<T, uint32_t   >::value){return generateFileHandle(addFieldToLists(fieldName, StatisticFieldInfo::UINT32));}
        if (is_type_same<T, int64_t    >::value){return generateFileHandle(addFieldToLists(fieldName, StatisticFieldInfo::INT64)); }
        if (is_type_same<T, uint64_t   >::value){return generateFileHandle(addFieldToLists(fieldName, StatisticFieldInfo::UINT64));}
        if (is_type_same<T, float      >::value){return generateFileHandle(addFieldToLists(fieldName, StatisticFieldInfo::FLOAT)); }
        if (is_type_same<T, double     >::value){return generateFileHandle(addFieldToLists(fieldName, StatisticFieldInfo::DOUBLE));}
        if (is_type_same<T, const char*>::value){return generateFileHandle(addFieldToLists(fieldName, StatisticFieldInfo::STR));   }

        //TODO: IF WE GET THERE, GENERATE AN ERROR AS THIS IS AN UNSUPPORTED TYPE 
        return -1;
    }
    
//    /** Adjust the heirarchy of the fields (FUTURE SUPPORT)
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
    // Get Registerd Fields
    // ONLY SUPPORTED TYPES ARE int32_t, uint32_t, int64_t, uint64_t, float, double, const char*
    template<typename T>
    StatisticFieldInfo* getRegisteredField(const char* statisticName, const char* fieldName)
    {
        StatisticFieldInfo*             NewStatFieldInfo;
        StatisticFieldInfo*             ExistingStatFieldInfo;
        StatisticFieldInfo::fieldType_t FieldType = StatisticFieldInfo::UNDEFINED;
        
        // Figure out the Field Type
        if (is_type_same<T, int32_t    >::value) {FieldType = StatisticFieldInfo::INT32; }
        if (is_type_same<T, uint32_t   >::value) {FieldType = StatisticFieldInfo::UINT32;}
        if (is_type_same<T, int64_t    >::value) {FieldType = StatisticFieldInfo::INT64; }
        if (is_type_same<T, uint64_t   >::value) {FieldType = StatisticFieldInfo::UINT64;}
        if (is_type_same<T, float      >::value) {FieldType = StatisticFieldInfo::FLOAT; }
        if (is_type_same<T, double     >::value) {FieldType = StatisticFieldInfo::DOUBLE;}
        if (is_type_same<T, const char*>::value) {FieldType = StatisticFieldInfo::STR;   }
        
        NewStatFieldInfo = new StatisticFieldInfo(statisticName, fieldName, FieldType);
    
        // Now search the FieldInfoArray of type for a matching entry
        for (FieldInfoArray_t::iterator it_v = m_outputFieldInfoArray.begin(); it_v != m_outputFieldInfoArray.end(); it_v++) {
            ExistingStatFieldInfo = *it_v;
            
            // The New Stat Field already exists, dont add it
            if (*ExistingStatFieldInfo == *NewStatFieldInfo) {
                delete NewStatFieldInfo;
                return  ExistingStatFieldInfo;
            }
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
    void outputField(fieldHandle_t fieldHandle, const char* data);
    
    /** Output field data.  
     * @param type - The field type to get name of.
     * @return String name of the field type.
     */
    const char* getFieldTypeShortName(fieldType_t type);
    
protected:    
    friend class SST::Component;
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

    // Field Outputs
    /** Implementation of outputField() for derived classes.  
      * Perform the actual implementation of the output. */ 
    virtual void implOutputField(fieldHandle_t fieldHandle, int32_t data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, uint32_t data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, int64_t data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, uint64_t data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, float data) = 0;  
    virtual void implOutputField(fieldHandle_t fieldHandle, double data) = 0;
    virtual void implOutputField(fieldHandle_t fieldHandle, const char* data) = 0;

private:    
    // Start / Stop of register Fields
    void startRegisterFields(const char* componentName, const char* statisticName);
    void stopRegisterFields();
    
    // Set the Statistic Load Level
    void setStatisticLoadLevel(uint8_t loadLevel) {m_statLoadLevel = loadLevel;}
    
    // Start / Stop of output
    void startOutputEntries(StatisticBase* statistic);
    void stopOutputEntries();
    
    // Other support functions
    StatisticFieldInfo* addFieldToLists(const char* fieldName, fieldType_t fieldType);
    fieldHandle_t generateFileHandle(StatisticFieldInfo* FieldInfo);

protected:     
    StatisticOutput() {;} // For serialization only
    void setStatisticOutputName(std::string name) {m_statOutputName = name;}
    
private:
    std::string      m_statOutputName;
    Params           m_outputParameters;
    FieldInfoArray_t m_outputFieldInfoArray;
    fieldHandle_t    m_highestFieldHandle;
    std::string      m_currentFieldCompName;
    std::string      m_currentFieldStatName;
    uint8_t          m_statLoadLevel;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Module);
        ar & BOOST_SERIALIZATION_NVP(m_outputParameters);
        ar & BOOST_SERIALIZATION_NVP(m_outputFieldInfoArray);
        ar & BOOST_SERIALIZATION_NVP(m_highestFieldHandle);
        ar & BOOST_SERIALIZATION_NVP(m_currentFieldCompName);
        ar & BOOST_SERIALIZATION_NVP(m_currentFieldStatName);
        ar & BOOST_SERIALIZATION_NVP(m_statLoadLevel);
    }
};                          

} //namespace Statistics
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Statistics::StatisticFieldInfo)
BOOST_CLASS_EXPORT_KEY(SST::Statistics::StatisticOutput)

#endif
