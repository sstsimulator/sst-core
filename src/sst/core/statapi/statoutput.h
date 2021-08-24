// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_STATAPI_STATOUTPUT_H
#define SST_CORE_STATAPI_STATOUTPUT_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/module.h"
#include "sst/core/params.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statfieldinfo.h"
#include "sst/core/warnmacros.h"

#include <mutex>
#include <unordered_map>

// Default Settings for Statistic Output and Load Level
#define STATISTICSDEFAULTOUTPUTNAME     "sst.statOutputConsole"
#define STATISTICSDEFAULTLOADLEVEL      0
#define STATISTICLOADLEVELUNINITIALIZED 0xff

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
class StatisticOutput
{
public:
    SST_ELI_DECLARE_BASE(StatisticOutput)
    SST_ELI_DECLARE_INFO_EXTERN(ELI::ProvidesParams)
    SST_ELI_DECLARE_CTOR_EXTERN(SST::Params&)

    using fieldType_t      = StatisticFieldInfo::fieldType_t;
    using fieldHandle_t    = StatisticFieldInfo::fieldHandle_t;
    using FieldInfoArray_t = std::vector<StatisticFieldInfo*>;
    using FieldNameMap_t   = std::unordered_map<std::string, fieldHandle_t>;

public:
    ~StatisticOutput();

    /** Return the Statistic Output name */
    std::string& getStatisticOutputName() { return m_statOutputName; }

    /** Return the parameters for the StatisticOutput */
    Params& getOutputParameters() { return m_outputParameters; }

    /** True if this StatOutput can handle StatisticGroups */
    virtual bool acceptsGroups() const { return false; }

    virtual void output(StatisticBase* statistic, bool endOfSimFlag) = 0;

    virtual bool supportsDynamicRegistration() const { return false; }

    /////////////////
    // Methods for Registering Fields (Called by Statistic Objects)
public:
    // by default, no params to return
    static const std::vector<SST::ElementInfoParam>& ELI_getParams()
    {
        static std::vector<SST::ElementInfoParam> var {};
        return var;
    }

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

    // Simulation Events
    /** Indicate to Statistic Output that simulation has started.
     * Allows object to perform any setup required. */
    virtual void startOfSimulation() = 0;

    /** Indicate to Statistic Output that simulation has ended.
     * Allows object to perform any shutdown required. */
    virtual void endOfSimulation() = 0;

private:
    // Start / Stop of register Fields
    virtual void registerStatistic(StatisticBase* stat) = 0;

    void registerGroup(StatisticGroup* group);
    void outputGroup(StatisticGroup* group, bool endOfSimFlag);

    virtual void startOutputGroup(StatisticGroup* group) = 0;
    virtual void stopOutputGroup()                       = 0;

    virtual void startRegisterGroup(StatisticGroup* group) = 0;
    virtual void stopRegisterGroup()                       = 0;

    void castError();

protected:
    /** Construct a base StatisticOutput
     * @param outputParameters - The parameters for the statistic Output.
     */
    StatisticOutput(Params& outputParameters);
    StatisticOutput() { ; } // For serialization only
    void setStatisticOutputName(const std::string& name) { m_statOutputName = name; }

    void lock() { m_lock.lock(); }
    void unlock() { m_lock.unlock(); }

private:
    std::string          m_statOutputName;
    Params               m_outputParameters;
    std::recursive_mutex m_lock;
};

class StatisticFieldsOutput : public StatisticOutput
{
public:
    void registerStatistic(StatisticBase* stat) override;

    // Start / Stop of output
    void output(StatisticBase* statistic, bool endOfSimFlag) override;

    void startOutputGroup(StatisticGroup* group) override;
    void stopOutputGroup() override;

    void startRegisterGroup(StatisticGroup* group) override;
    void stopRegisterGroup() override;

    // Start / Stop of output
    /** Indicate to Statistic Output that a statistic is about to send data to be output
     * Allows object to perform any initialization before output. */
    virtual void implStartOutputEntries(StatisticBase* statistic) = 0;

    /** Indicate to Statistic Output that a statistic is finished sending data to be output
     * Allows object to perform any cleanup. */
    virtual void implStopOutputEntries() = 0;

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
    template <typename T>
    StatisticFieldInfo* getRegisteredField(const char* statisticName, const char* fieldName)
    {
        StatisticFieldInfo*             NewStatFieldInfo;
        StatisticFieldInfo*             ExistingStatFieldInfo;
        StatisticFieldInfo::fieldType_t FieldType =
            StatisticFieldInfo::StatisticFieldInfo::getFieldTypeFromTemplate<T>();

        NewStatFieldInfo = new StatisticFieldInfo(statisticName, fieldName, FieldType);

        // Now search the FieldNameMap_t of type for a matching entry
        FieldNameMap_t::const_iterator found = m_outputFieldNameMap.find(NewStatFieldInfo->getFieldUniqueName());
        if ( found != m_outputFieldNameMap.end() ) {
            // We found a map entry, now get the StatFieldInfo from the m_outputFieldInfoArray at the index given by the
            // map and then delete the NewStatFieldInfo to prevent a leak
            ExistingStatFieldInfo = m_outputFieldInfoArray[found->second];
            delete NewStatFieldInfo;
            return ExistingStatFieldInfo;
        }

        delete NewStatFieldInfo;
        return nullptr;
    }

    /** Return the array of registered field infos. */
    FieldInfoArray_t& getFieldInfoArray() { return m_outputFieldInfoArray; }

    /////////////////
    // Methods for Outputting Fields  (Called by Statistic Objects)
    // Output fields (will call virtual functions of Derived Output classes)
    // These aren't really part of a generic interface - optimization purposes only
    /** Output field data.
     * @param fieldHandle - The handle of the registered field.
     * @param data - The data to be output.
     */
    virtual void outputField(fieldHandle_t fieldHandle, int32_t data);
    virtual void outputField(fieldHandle_t fieldHandle, uint32_t data);
    virtual void outputField(fieldHandle_t fieldHandle, int64_t data);
    virtual void outputField(fieldHandle_t fieldHandle, uint64_t data);
    virtual void outputField(fieldHandle_t fieldHandle, float data);
    virtual void outputField(fieldHandle_t fieldHandle, double data);

    /** Register a field to be output (templated function)
     * @param fieldName - The name of the field.
     * @return The handle of the registered field or -1 if type is not supported.
     * Note: Any field names (of the same data type) that are previously
     *       registered by a statistic will return the previously
     *       handle.
     */
    // Field Registration
    // ONLY SUPPORTED TYPES ARE int32_t, uint32_t, int64_t, uint64_t, float, double
    template <typename T>
    fieldHandle_t registerField(const char* fieldName)
    {
        StatisticFieldInfo::fieldType_t FieldType =
            StatisticFieldInfo::StatisticFieldInfo::getFieldTypeFromTemplate<T>();

        auto res = generateFieldHandle(addFieldToLists(fieldName, FieldType));
        implRegisteredField(res);
        return res;
    }

    /** Output field data.
     * @param type - The field type to get name of.
     * @return String name of the field type.
     */
    const char* getFieldTypeShortName(fieldType_t type);

protected:
    /** Construct a base StatisticOutput
     * @param outputParameters - The parameters for the statistic Output.
     */
    StatisticFieldsOutput(Params& outputParameters);

    // For Serialization
    StatisticFieldsOutput() {}

private:
    // Other support functions
    StatisticFieldInfo* addFieldToLists(const char* fieldName, fieldType_t fieldType);
    fieldHandle_t       generateFieldHandle(StatisticFieldInfo* FieldInfo);
    virtual void        implRegisteredField(fieldHandle_t UNUSED(fieldHandle)) {}

    FieldInfoArray_t m_outputFieldInfoArray;
    FieldNameMap_t   m_outputFieldNameMap;
    fieldHandle_t    m_highestFieldHandle;
    std::string      m_currentFieldStatName;

protected:
    /** These can be overriden, if necessary, but must be callable
     *  by the derived class */
    virtual void startRegisterFields(StatisticBase* statistic);
    virtual void stopRegisterFields();

    virtual void startOutputEntries(StatisticBase* statistic);
    virtual void stopOutputEntries();
};

} // namespace Statistics
} // namespace SST

#endif // SST_CORE_STATAPI_STATOUTPUT_H
