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

#ifndef SST_CORE_STATAPI_STATBASE_H
#define SST_CORE_STATAPI_STATBASE_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/oneshot.h"
#include "sst/core/params.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statfieldinfo.h"
#include "sst/core/warnmacros.h"

#include <string>

namespace SST {
class BaseComponent;
class Factory;

namespace Statistics {
class StatisticOutput;
class StatisticFieldsOutput;
class StatisticProcessingEngine;
class StatisticGroup;

class StatisticInfo : public SST::Core::Serialization::serializable
{
public:
    std::string name;
    Params      params;

    StatisticInfo(const std::string& name) : name(name) {}
    StatisticInfo(const std::string& name, const Params& params) : name(name), params(params) {}
    StatisticInfo() {} /* DO NOT USE:  For serialization */

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& name;
        ser& params;
    }

    ImplementSerializable(SST::Statistics::StatisticInfo)
};

/**
    \class StatisticBase

    Forms the base class for statistics gathering within SST. Statistics are
    gathered and processed into various (extensible) output forms. Statistics
    are expected to be named so that they can be located in the simulation
    output files.
*/

class StatisticBase
{
public:
    /** Statistic collection mode */
    typedef enum { STAT_MODE_UNDEFINED, STAT_MODE_COUNT, STAT_MODE_PERIODIC, STAT_MODE_DUMP_AT_END } StatMode_t;

    // Enable/Disable of Statistic
    /** Enable Statistic for collections */
    void enable() { m_statEnabled = true; }

    /** Disable Statistic for collections */
    void disable() { m_statEnabled = false; }

    // Handling of Collection Counts and Data
    /** Inform the Statistic to clear its data */
    virtual void clearStatisticData() {}

    /** Set the current collection count to 0 */
    virtual void resetCollectionCount();

    /** Increment current collection count */
    virtual void incrementCollectionCount(uint64_t increment);

    /** Set the current collection count to a defined value */
    virtual void setCollectionCount(uint64_t newCount);

    /** Set the collection count limit to a defined value */
    virtual void setCollectionCountLimit(uint64_t newLimit);

    // Control Statistic Operation Flags
    /** Set the Reset Count On Output flag.
     *  If Set, the collection count will be reset when statistic is output.
     */
    void setFlagResetCountOnOutput(bool flag) { m_resetCountOnOutput = flag; }

    static const std::vector<ElementInfoParam>& ELI_getParams();

    /** Set the Clear Data On Output flag.
     *  If Set, the data in the statistic will be cleared by calling
     *  clearStatisticData().
     */
    void setFlagClearDataOnOutput(bool flag) { m_clearDataOnOutput = flag; }

    /** Set the Output At End Of Sim flag.
     *  If Set, the statistic will perform an output at the end of simulation.
     */
    void setFlagOutputAtEndOfSim(bool flag) { m_outputAtEndOfSim = flag; }

    // Get Data & Information on Statistic
    /** Return the Component Name */
    const std::string& getCompName() const;

    /** Return the Statistic Name */
    inline const std::string& getStatName() const { return m_statName; }

    /** Return the Statistic SubId */
    inline const std::string& getStatSubId() const { return m_statSubId; }

    /** Return the full Statistic name of Component.StatName.SubId */
    inline const std::string& getFullStatName() const { return m_statFullName; } // Return compName.statName.subId

    /** Return the Statistic type name */
    inline const std::string& getStatTypeName() const { return m_statTypeName; }

    /** Return the Statistic data type */
    inline const StatisticFieldInfo::fieldType_t& getStatDataType() const { return m_statDataType; }

    /** Return the Statistic data type */
    inline const char* getStatDataTypeShortName() const
    {
        return StatisticFieldInfo::getFieldTypeShortName(m_statDataType);
    }

    /** Return the Statistic data type */
    inline const char* getStatDataTypeFullName() const
    {
        return StatisticFieldInfo::getFieldTypeFullName(m_statDataType);
    }

    /** Return a pointer to the parent Component */
    BaseComponent* getComponent() const { return m_component; }

    /** Return the enable status of the Statistic */
    bool isEnabled() const { return m_statEnabled; }

    /** Return the enable status of the Statistic's ability to output data */
    bool isOutputEnabled() const { return m_outputEnabled; }

    /** Return the collection count limit */
    uint64_t getCollectionCountLimit() const { return m_collectionCountLimit; }

    /** Return the current collection count */
    uint64_t getCollectionCount() const { return m_currentCollectionCount; }

    /** Return the ResetCountOnOutput flag value */
    bool getFlagResetCountOnOutput() const { return m_resetCountOnOutput; }

    /** Return the ClearDataOnOutput flag value */
    bool getFlagClearDataOnOutput() const { return m_clearDataOnOutput; }

    /** Return the OutputAtEndOfSim flag value */
    bool getFlagOutputAtEndOfSim() const { return m_outputAtEndOfSim; }

    /** Return the collection mode that is registered */
    StatMode_t getRegisteredCollectionMode() const { return m_registeredCollectionMode; }

    // Delay Methods (Uses OneShot to disable Statistic or Collection)
    /** Delay the statistic from outputting data for a specified delay time
     * @param delayTime - Value in UnitAlgebra format for delay (i.e. 10ns).
     */
    void delayOutput(const char* delayTime);

    /** Delay the statistic from collecting data for a specified delay time.
     * @param delayTime - Value in UnitAlgebra format for delay (i.e. 10ns).
     */
    void delayCollection(const char* delayTime);

    // Status of Statistic
    /** Indicate that the Statistic is Ready to be used */
    virtual bool isReady() const { return true; }

    /** Indicate if the Statistic is a NullStatistic */
    virtual bool isNullStatistic() const { return false; }

protected:
    friend class SST::Statistics::StatisticProcessingEngine;
    friend class SST::Statistics::StatisticOutput;
    friend class SST::Statistics::StatisticGroup;
    friend class SST::Statistics::StatisticFieldsOutput;

    /** Construct a StatisticBase
     * @param comp - Pointer to the parent constructor.
     * @param statName - Name of the statistic to be registered.  This name must
     * match the name in the ElementInfoStatistic.
     * @param statSubId - Additional name of the statistic
     * @param statParams - The parameters for this statistic
     */
    // Constructors:
    StatisticBase(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams);

    // Destructor
    virtual ~StatisticBase() {}

    /** Return the Statistic Parameters */
    Params& getParams() { return m_statParams; }

    /** Set the Statistic Data Type */
    void setStatisticDataType(const StatisticFieldInfo::fieldType_t dataType) { m_statDataType = dataType; }

    /** Set an optional Statistic Type Name */
    void setStatisticTypeName(const char* typeName) { m_statTypeName = typeName; }

private:
    friend class SST::BaseComponent;

    /** Set the Registered Collection Mode */
    void setRegisteredCollectionMode(StatMode_t mode) { m_registeredCollectionMode = mode; }

    /** Construct a full name of the statistic */
    static std::string buildStatisticFullName(const char* compName, const char* statName, const char* statSubId);
    static std::string
    buildStatisticFullName(const std::string& compName, const std::string& statName, const std::string& statSubId);

    // Required Virtual Methods:
    /** Called by the system to tell the Statistic to register its output fields.
     * by calling statOutput->registerField(...)
     * @param statOutput - Pointer to the statistic output
     */
    virtual void registerOutputFields(StatisticFieldsOutput* statOutput) = 0;

    /** Called by the system to tell the Statistic to send its data to the
     * StatisticOutput to be output.
     * @param statOutput - Pointer to the statistic output
     * @param EndOfSimFlag - Indicates that the output is occurring at the end of simulation.
     */
    virtual void outputStatisticFields(StatisticFieldsOutput* statOutput, bool EndOfSimFlag) = 0;

    /** Indicate if the Statistic Mode is supported.
     * This allows Statistics to support STAT_MODE_COUNT and STAT_MODE_PERIODIC modes.
     * by default, both modes are supported.
     * @param mode - Mode to test
     */
    virtual bool isStatModeSupported(StatMode_t UNUSED(mode)) const { return true; } // Default is to accept all modes

    /** Verify that the statistic names match */
    bool operator==(StatisticBase& checkStat);

    // Support Routines
    void initializeStatName(const char* compName, const char* statName, const char* statSubId);
    void initializeStatName(const std::string& compName, const std::string& statName, const std::string& statSubId);

    void initializeProperties();
    void checkEventForOutput();

    // OneShot Callbacks:
    void delayOutputExpiredHandler();     // Enable Output in handler
    void delayCollectionExpiredHandler(); // Enable Collection in Handler

    const StatisticGroup* getGroup() const { return m_group; }
    void                  setGroup(const StatisticGroup* group) { m_group = group; }

protected:
    StatisticBase(); // For serialization only

private:
    BaseComponent*                  m_component;
    std::string                     m_statName;
    std::string                     m_statSubId;
    std::string                     m_statFullName;
    std::string                     m_statTypeName;
    Params                          m_statParams;
    StatMode_t                      m_registeredCollectionMode;
    uint64_t                        m_currentCollectionCount;
    uint64_t                        m_collectionCountLimit;
    StatisticFieldInfo::fieldType_t m_statDataType;

    bool m_statEnabled;
    bool m_outputEnabled;
    bool m_resetCountOnOutput;
    bool m_clearDataOnOutput;
    bool m_outputAtEndOfSim;

    bool                  m_outputDelayed;
    bool                  m_collectionDelayed;
    bool                  m_savedStatEnabled;
    bool                  m_savedOutputEnabled;
    OneShot::HandlerBase* m_outputDelayedHandler;
    OneShot::HandlerBase* m_collectionDelayedHandler;
    const StatisticGroup* m_group;
};

/**
 \class StatisticCollector
 * Base type that creates the virtual addData(...) interface
 * Used for distinguishing fundamental types (collected by value)
 * and composite struct types (collected by reference)
 */
template <class T, bool F = std::is_fundamental<T>::value>
struct StatisticCollector
{};

template <class T>
struct StatisticCollector<T, true>
{
    virtual void addData_impl(T data) = 0;

    /**
     * @brief addData_impl_Ntimes Add the same data N times in a row
     *        By default, this just calls the addData function N times
     * @param N    The number of consecutive times
     * @param args The arguments to pass in
     */
    virtual void addData_impl_Ntimes(uint64_t N, T data)
    {
        for ( uint64_t i = 0; i < N; ++i ) {
            addData_impl(data);
        }
    }
};

template <class... Args>
struct StatisticCollector<std::tuple<Args...>, false>
{
    virtual void addData_impl(Args... args) = 0;

    /**
     * @brief addData_impl_Ntimes Add the same data N times in a row
     *        By default, this just calls the addData function N times
     * @param N    The number of consecutive times
     * @param args The arguments to pass in
     */
    virtual void addData_impl_Ntimes(uint64_t N, Args... args)
    {
        for ( uint64_t i = 0; i < N; ++i ) {
            addData_impl(args...);
        }
    }

    template <class... InArgs>
    void addData(InArgs&&... args)
    {
        addData_impl(std::make_tuple(std::forward<InArgs>(args)...));
    }
};

////////////////////////////////////////////////////////////////////////////////

/**
    \class Statistic

    Forms the template defined base class for statistics gathering within SST.

    @tparam T A template for the basic numerical data stored by this Statistic
*/

template <typename T>
class Statistic : public StatisticBase, public StatisticCollector<T>
{
public:
    SST_ELI_DECLARE_BASE(Statistic)
    SST_ELI_DECLARE_INFO(
    ELI::ProvidesInterface,
    ELI::ProvidesParams)
    SST_ELI_DECLARE_CTOR(BaseComponent*, const std::string&, const std::string&, SST::Params&)

    using Datum = T;
    using StatisticCollector<T>::addData_impl;
    // The main method to add data to the statistic
    /** Add data to the Statistic
     * This will call the addData_impl() routine in the derived Statistic.
     */
    template <class... InArgs> // use a universal reference here
    void addData(InArgs&&... args)
    {
        // Call the Derived Statistic's implementation
        //  of addData and increment the count
        if ( isEnabled() ) {
            addData_impl(std::forward<InArgs>(args)...);
            incrementCollectionCount(1);
        }
    }

    template <class... InArgs> // use a universal reference here
    void addDataNTimes(uint64_t N, InArgs&&... args)
    {
        // Call the Derived Statistic's implementation
        //  of addData and increment the count
        if ( isEnabled() ) {
            addData_impl_Ntimes(N, std::forward<InArgs>(args)...);
            incrementCollectionCount(N);
        }
    }

    static fieldType_t fieldId() { return StatisticFieldType<T>::id(); }

protected:
    friend class SST::Factory;
    friend class SST::BaseComponent;
    /** Construct a Statistic
     * @param comp - Pointer to the parent constructor.
     * @param statName - Name of the statistic to be registered.  This name must
     * match the name in the ElementInfoStatistic.
     * @param statSubId - Additional name of the statistic
     * @param statParams - The parameters for this statistic
     */

    Statistic(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        StatisticBase(comp, statName, statSubId, statParams)
    {
        setStatisticDataType(StatisticFieldInfo::getFieldTypeFromTemplate<T>());
    }

    virtual ~Statistic() {}

private:
    Statistic() {} // For serialization only
};

/**
 * void Statistic has special meaning in that it does not collect fields
 * in the usual way through the addData function. It has to use custom functions.
 */
template <>
class Statistic<void> : public StatisticBase
{
public:
    SST_ELI_DECLARE_BASE(Statistic)
    SST_ELI_DECLARE_INFO(
    ELI::ProvidesInterface,
    ELI::ProvidesParams)
    SST_ELI_DECLARE_CTOR(BaseComponent*, const std::string&, const std::string&, SST::Params&)

    void registerOutputFields(StatisticFieldsOutput* statOutput) override;

    void outputStatisticFields(StatisticFieldsOutput* statOutput, bool EndOfSimFlag) override;

protected:
    friend class SST::Factory;
    friend class SST::BaseComponent;
    /** Construct a Statistic
     * @param comp - Pointer to the parent constructor.
     * @param statName - Name of the statistic to be registered.  This name must
     * match the name in the ElementInfoStatistic.
     * @param statSubId - Additional name of the statistic
     * @param statParams - The parameters for this statistic
     */

    Statistic(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        StatisticBase(comp, statName, statSubId, statParams)
    {}

    virtual ~Statistic() {}

private:
    Statistic() {} // For serialization only
};

using CustomStatistic = Statistic<void>;

template <class... Args>
using MultiStatistic = Statistic<std::tuple<Args...>>;

struct ImplementsStatFields
{
public:
    std::string toString() const;

    Statistics::fieldType_t fieldId() const { return field_; }

    const char* fieldName() const { return field_name_; }

    const char* fieldShortName() const { return short_name_; }

protected:
    template <class T>
    ImplementsStatFields(T* UNUSED(t)) :
        field_name_(T::ELI_fieldName()),
        short_name_(T::ELI_fieldShortName()),
        field_(T::ELI_registerField(T::ELI_fieldName(), T::ELI_fieldShortName()))
    {}

private:
    const char*             field_name_;
    const char*             short_name_;
    Statistics::fieldType_t field_;
};

#define SST_ELI_DECLARE_STATISTIC_TEMPLATE(cls, lib, name, version, desc, interface) \
    SST_ELI_DEFAULT_INFO(lib, name, ELI_FORWARD_AS_ONE(version), desc)               \
    SST_ELI_INTERFACE_INFO(interface)

#define SST_ELI_REGISTER_CUSTOM_STATISTIC(cls, lib, name, version, desc)                                     \
    SST_ELI_REGISTER_DERIVED(SST::Statistics::CustomStatistic,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
    SST_ELI_INTERFACE_INFO("CustomStatistic")

#define SST_ELI_DECLARE_STATISTIC(cls, field, lib, name, version, desc, interface)                                   \
    static bool ELI_isLoaded()                                                                                       \
    {                                                                                                                \
        return SST::Statistics::Statistic<field>::template addDerivedInfo<cls>(lib, name)                            \
               && SST::Statistics::Statistic<field>::template addDerivedBuilder<cls>(lib, name)                      \
               && SST::Statistics::Statistic<field>::template addDerivedInfo<SST::Statistics::NullStatistic<field>>( \
                   lib, name)                                                                                        \
               && SST::Statistics::Statistic<field>::template addDerivedBuilder<                                     \
                   SST::Statistics::NullStatistic<field>>(lib, name);                                                \
    }                                                                                                                \
    SST_ELI_DEFAULT_INFO(lib, name, ELI_FORWARD_AS_ONE(version), desc)                                               \
    SST_ELI_INTERFACE_INFO(interface)                                                                                \
    static const char* ELI_fieldName() { return #field; }                                                            \
    static const char* ELI_fieldShortName() { return #field; }

#ifdef __INTEL_COMPILER
#define SST_ELI_INSTANTIATE_STATISTIC(cls, field)                                                     \
    bool force_instantiate_##cls##_##field                                                            \
        = SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<field>, cls<field>>::isLoaded() \
          && SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<field>, cls<field>>::isLoaded()  \
          && SST::ELI::InstantiateBuilderInfo<                                                        \
              SST::Statistics::Statistic<field>, SST::Statistics::NullStatistic<field>>::isLoaded()   \
          && SST::ELI::InstantiateBuilder<                                                            \
              SST::Statistics::Statistic<field>, SST::Statistics::NullStatistic<field>>::isLoaded();
#else
#define SST_ELI_INSTANTIATE_STATISTIC(cls, field)                                                             \
    struct cls##_##field##_##shortName : public cls<field>                                                    \
    {                                                                                                         \
        cls##_##field##_##shortName(                                                                          \
            SST::BaseComponent* bc, const std::string& sn, const std::string& si, SST::Params& p) :           \
            cls<field>(bc, sn, si, p)                                                                         \
        {}                                                                                                    \
        static bool ELI_isLoaded()                                                                            \
        {                                                                                                     \
            return SST::ELI::InstantiateBuilderInfo<                                                          \
                       SST::Statistics::Statistic<field>, cls##_##field##_##shortName>::isLoaded()            \
                   && SST::ELI::InstantiateBuilder<                                                           \
                       SST::Statistics::Statistic<field>, cls##_##field##_##shortName>::isLoaded()            \
                   && SST::ELI::InstantiateBuilderInfo<                                                       \
                       SST::Statistics::Statistic<field>, SST::Statistics::NullStatistic<field>>::isLoaded()  \
                   && SST::ELI::InstantiateBuilder<                                                           \
                       SST::Statistics::Statistic<field>, SST::Statistics::NullStatistic<field>>::isLoaded(); \
        }                                                                                                     \
    };
#endif

#define PP_NARG(...)                         PP_NARG_(__VA_ARGS__, PP_NSEQ())
#define PP_NARG_(...)                        PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1, _2, _3, _4, _5, N, ...) N
#define PP_NSEQ()                            5, 4, 3, 2, 1, 0

#define PP_GLUE(X, Y)   PP_GLUE_I(X, Y)
#define PP_GLUE_I(X, Y) X##Y

#define STAT_NAME1(base, a)          base##a
#define STAT_NAME2(base, a, b)       base##a##b
#define STAT_NAME3(base, a, b, c)    base##a##b##c
#define STAT_NAME4(base, a, b, c, d) base##a##b##c##d

#define STAT_GLUE_NAME(base, ...) PP_GLUE(STAT_NAME, PP_NARG(__VA_ARGS__))(base, __VA_ARGS__)
#define STAT_TUPLE(...)           std::tuple<__VA_ARGS__>

#ifdef __INTEL_COMPILER
#define MAKE_MULTI_STATISTIC(cls, name, tuple, ...)                                                         \
    bool force_instantiate_stat_name                                                                        \
        = SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<tuple>, cls<__VA_ARGS__>>::isLoaded() \
          && SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<tuple>, cls<__VA_ARGS__>>::isLoaded()  \
          && SST::ELI::InstantiateBuilderInfo<                                                              \
              SST::Statistics::Statistic<tuple>, SST::Statistics::NullStatistic<tuple>>::isLoaded()         \
          && SST::ELI::InstantiateBuilder<                                                                  \
              SST::Statistics::Statistic<tuple>, SST::Statistics::NullStatistic<tuple>>::isLoaded();        \
    }                                                                                                       \
    }                                                                                                       \
    ;
#else
#define MAKE_MULTI_STATISTIC(cls, name, tuple, ...)                                                           \
    struct name : public cls<__VA_ARGS__>                                                                     \
    {                                                                                                         \
        name(SST::BaseComponent* bc, const std::string& sn, const std::string& si, SST::Params& p) :          \
            cls<__VA_ARGS__>(bc, sn, si, p)                                                                   \
        {}                                                                                                    \
        bool ELI_isLoaded() const                                                                             \
        {                                                                                                     \
            return SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<tuple>, name>::isLoaded()      \
                   && SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<tuple>, name>::isLoaded()       \
                   && SST::ELI::InstantiateBuilderInfo<                                                       \
                       SST::Statistics::Statistic<tuple>, SST::Statistics::NullStatistic<tuple>>::isLoaded()  \
                   && SST::ELI::InstantiateBuilder<                                                           \
                       SST::Statistics::Statistic<tuple>, SST::Statistics::NullStatistic<tuple>>::isLoaded(); \
        }                                                                                                     \
    };
#endif

#define SST_ELI_INSTANTIATE_MULTI_STATISTIC(cls, ...) \
    MAKE_MULTI_STATISTIC(cls, STAT_GLUE_NAME(cls, __VA_ARGS__), STAT_TUPLE(__VA_ARGS__), __VA_ARGS__)

} // namespace Statistics
} // namespace SST

// we need to make sure null stats are instantiated for whatever types we use
#include "sst/core/statapi/statnull.h"

#endif // SST_CORE_STATAPI_STATBASE_H
