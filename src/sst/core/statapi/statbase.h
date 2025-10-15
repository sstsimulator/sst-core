// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_STATAPI_STATBASE_H
#define SST_CORE_STATAPI_STATBASE_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/factory.h"
#include "sst/core/params.h"
#include "sst/core/serialization/serialize_impl_fwd.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statfieldinfo.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/warnmacros.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace SST {
class BaseComponent;
class Factory;

namespace Statistics {
class StatisticOutput;
class StatisticFieldsOutput;
class StatisticProcessingEngine;
class StatisticGroup;

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
    // Enable/Disable of Statistic
    /** Enable Statistic for collections */
    void enable() { stat_enabled_ = true; }

    /** Disable Statistic for collections */
    void disable() { stat_enabled_ = false; }

    // Handling of Collection Counts and Data
    /** Inform the Statistic to clear its data */
    virtual void clearStatisticData() {}

    /** Set the current collection count to 0 */
    virtual void resetCollectionCount();

    /** Increment current collection count */
    virtual void incrementCollectionCount(uint64_t increment);

    /** Set the current collection count to a defined value */
    virtual void setCollectionCount(uint64_t new_count);

    /** Set the collection count limit to a defined value */
    virtual void setCollectionCountLimit(uint64_t new_limit);

    // Control Statistic Operation Flags
    /** Set the Reset Count On Output flag.
     *  If Set, the collection count will be reset when statistic is output.
     */
    void setFlagResetCountOnOutput(bool flag) { reset_count_on_output_ = flag; }

    static const std::vector<ElementInfoParam>& ELI_getParams();

    /** Set the Clear Data On Output flag.
     *  If Set, the data in the statistic will be cleared by calling
     *  clearStatisticData().
     */
    void setFlagClearDataOnOutput(bool flag) { clear_data_on_output_ = flag; }

    /** Set the Output At End Of Sim flag.
     *  If Set, the statistic will perform an output at the end of simulation.
     */
    void setFlagOutputAtEndOfSim(bool flag) { output_at_end_of_sim_ = flag; }

    // Get Data & Information on Statistic
    /** Return the Component Name */
    const std::string& getCompName() const;

    /** Return the Statistic Name */
    inline const std::string& getStatName() const { return stat_name_; }

    /** Return the Statistic SubId */
    inline const std::string& getStatSubId() const { return stat_sub_id_; }

    /** Return the full Statistic name of component.stat_name.sub_id */
    inline const std::string getFullStatName() const
    {
        std::string stat_full_name_rtn = getCompName() + "." + stat_name_;
        if ( stat_sub_id_ != "" ) stat_full_name_rtn += "." + stat_sub_id_;
        return stat_full_name_rtn;
    } // Return comp_name.stat_name.sub_id or comp_name.stat_name

    /** Return the ELI type of the statistic
     *  The ELI registration macro creates this function automatically for child classes.
     */
    virtual std::string getELIName() const = 0;

    /** Return the Statistic type name */
    virtual const std::string& getStatTypeName() const { return stat_type_name_; }

    /** Return the Statistic data type */
    inline const StatisticFieldInfo::fieldType_t& getStatDataType() const { return stat_data_type_; }

    /** Return the Statistic data type */
    inline const char* getStatDataTypeShortName() const
    {
        return StatisticFieldInfo::getFieldTypeShortName(stat_data_type_);
    }

    /** Return the Statistic data type */
    inline const char* getStatDataTypeFullName() const
    {
        return StatisticFieldInfo::getFieldTypeFullName(stat_data_type_);
    }

    /** Return a pointer to the parent Component */
    BaseComponent* getComponent() const { return component_; }

    /** Return the enable status of the Statistic */
    bool isEnabled() const { return stat_enabled_; }

    /** Return the rate at which the statistic should be output */

    /** Manage StartAt flag */
    bool getStartAtFlag() { return (flags_ & Flag::StartAt); }
    void setStartAtFlag() { flags_ |= Flag::StartAt; }
    void unsetStartAtFlag() { flags_ &= (~Flag::StartAt); }

    /** Manage StopAt flag */
    bool getStopAtFlag() { return (flags_ & Flag::StopAt); }
    void setStopAtFlag() { flags_ |= Flag::StopAt; }
    void unsetStopAtFlag() { flags_ &= (~Flag::StopAt); }

    /** Manage OutputRate flag */
    bool getOutputRateFlag() { return (flags_ & Flag::OutputRate); }
    void setOutputRateFlag() { flags_ |= Flag::OutputRate; }
    void unsetOutputRateFlag() { flags_ &= (~Flag::OutputRate); }

    /* Query stat engine for statistic data needed for checkpoint. */
    SimTime_t getStartAtFactor();
    SimTime_t getStopAtFactor();
    SimTime_t getOutputRateFactor();

    /** Return the collection count limit */
    uint64_t getCollectionCountLimit() const { return collection_count_limit_; }

    /** Return the current collection count */
    uint64_t getCollectionCount() const { return current_collection_count_; }

    /** Return the ResetCountOnOutput flag value */
    bool getFlagResetCountOnOutput() const { return reset_count_on_output_; }

    /** Return the ClearDataOnOutput flag value */
    bool getFlagClearDataOnOutput() const { return clear_data_on_output_; }

    /** Return the OutputAtEndOfSim flag value */
    bool getFlagOutputAtEndOfSim() const { return output_at_end_of_sim_; }

    /** Return the collection mode that is registered */
    bool isOutputPeriodic() const { return registered_collection_mode_; }
    bool isOutputEventBased() const { return !registered_collection_mode_; }

    // Status of Statistic
    /** Indicate that the Statistic is Ready to be used */
    virtual bool isReady() const { return true; }

    /** Indicate if the Statistic is a NullStatistic */
    virtual bool isNullStatistic() const { return false; }

    /** Serialization */
    virtual void serialize_order(SST::Core::Serialization::serializer& ser);

protected:
    friend class SST::Statistics::StatisticProcessingEngine;
    friend class SST::Statistics::StatisticOutput;
    friend class SST::Statistics::StatisticGroup;
    friend class SST::Statistics::StatisticFieldsOutput;

    /** Construct a StatisticBase
     * @param comp - Pointer to the parent constructor.
     * @param stat_name - Name of the statistic to be registered.  This name must
     * match the name in the ElementInfoStatistic.
     * @param stat_sub_id - Additional name of the statistic
     * @param stat_params - The parameters for this statistic
     */
    // Constructors:
    StatisticBase(
        BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params);

    // Destructor
    virtual ~StatisticBase() {}

protected:
    /** Set the Statistic Data Type */
    void setStatisticDataType(const StatisticFieldInfo::fieldType_t data_type) { stat_data_type_ = data_type; }

    /** Set an optional Statistic Type Name (for output) */
    void setStatisticTypeName(const char* type_name) { stat_type_name_ = type_name; }

private:
    friend class SST::BaseComponent;

    /** Set the Registered Collection Mode */
    void setRegisteredCollectionMode(bool is_periodic) { registered_collection_mode_ = is_periodic; }

    /** Construct a full name of the statistic */
    static std::string buildStatisticFullName(const char* comp_name, const char* stat_name, const char* stat_sub_id);
    static std::string buildStatisticFullName(
        const std::string& comp_name, const std::string& stat_name, const std::string& stat_sub_id);

    // Required Virtual Methods:
    /** Called by the system to tell the Statistic to register its output fields.
     * by calling statOutput->registerField(...)
     * @param stat_output - Pointer to the statistic output
     */
    virtual void registerOutputFields(StatisticFieldsOutput* stat_output) = 0;

    /** Called by the system to tell the Statistic to send its data to the
     * StatisticOutput to be output.
     * @param stat_output - Pointer to the statistic output
     * @param end_of_sim_flag - Indicates that the output is occurring at the end of simulation.
     */
    virtual void outputStatisticFields(StatisticFieldsOutput* stat_output, bool end_of_sim_flag) = 0;

    /** Indicate if the Statistic Mode is supported.
     * This allows Statistics to support STAT_MODE_COUNT and STAT_MODE_PERIODIC modes.
     * by default, both modes are supported.
     * @param mode - Mode to test
     */
    virtual bool isStatModeSupported(bool UNUSED(periodic)) const { return true; } // Default is to accept all modes

    /** Verify that the statistic names match */
    bool operator==(StatisticBase& check_stat);

    void checkEventForOutput();

    const StatisticGroup* getGroup() const { return group_; }
    void                  setGroup(const StatisticGroup* group) { group_ = group; }


protected:
    StatisticBase() {} // For serialization only

private:
    enum Flag : uint8_t {
        StartAt    = 1, // bit 0
        StopAt     = 2, // bit 1
        OutputRate = 4, // bit 2
    };

    std::string stat_name_      = ""; // Name of stat, matches ELI
    std::string stat_sub_id_    = ""; // Sub ID for this instance of the stat (default="")
    std::string stat_type_name_ = ""; // DEPRECATED (remove SST 16.0) - override 'getStatTypeName()' instead

    uint8_t  flags_                      = 0;    // Flags for checkpoint output
    bool     stat_enabled_               = true; // Whether stat is currently collecting data
    bool     registered_collection_mode_ = true; // True if periodic, false if event-based
    uint64_t current_collection_count_   = 0;
    uint64_t output_collection_count_    = 0; // Relevant for STAT_MODE_COUNT
    uint64_t collection_count_limit_     = 0; // Relevant for STAT_MODE_COUNT

    const StatisticGroup* group_ = nullptr; // Group that stat belongs to

    bool reset_count_on_output_ = false; // Whether to clear current_collection_count_ on output
    bool clear_data_on_output_  = false; // Whether to reset data on output
    bool output_at_end_of_sim_  = true;  // Whether to output stat at end of sim

    BaseComponent*                  component_;
    StatisticFieldInfo::fieldType_t stat_data_type_;
};

/**
 \class StatisticCollector
 * Base type that creates the virtual addData(...) interface
 * Used for distinguishing arithmetic types (collected by value)
 * and composite struct types (collected by reference)
 */
template <class T, bool = std::is_arithmetic_v<T>>
struct StatisticCollector;

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

    virtual ~StatisticCollector() = default;
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

    virtual ~StatisticCollector() = default;
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
    SST_ELI_DECLARE_BASE(Statistic<T>)
    SST_ELI_DECLARE_INFO(
    ELI::ProvidesInterface,
    ELI::ProvidesParams)
    SST_ELI_DECLARE_CTOR(BaseComponent*, const std::string&, const std::string&, SST::Params&)

    SST_ELI_DOCUMENT_PARAMS(
        {"rate",    "Frequency at which to output statistic. Must include units. 0ns = output at end of simulation only.", "0ns" },
        {"startat", "Time at which to enable data collection in this statistic. Must include units. 0ns = always enabled.", "0ns"},
        {"stopat",  "Time at which to disable data collection in this statistic. 0ns = always enabled.", "0ns"},
        {"resetOnOutput", "Whether to reset the statistic's values after each output.", "False"}
    )

    using Datum = T;
    using StatisticCollector<T>::addData_impl;
    using StatisticCollector<T>::addData_impl_Ntimes;

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

    virtual void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        StatisticBase::serialize_order(ser);
    }

protected:
    friend class SST::Factory;
    friend class SST::BaseComponent;
    /** Construct a Statistic
     * @param comp - Pointer to the parent constructor.
     * @param stat_name - Name of the statistic to be registered.  This name must
     * match the name in the ElementInfoStatistic.
     * @param stat_sub_id - Additional name of the statistic
     * @param stat_params - The parameters for this statistic
     */

    Statistic(BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params) :
        StatisticBase(comp, stat_name, stat_sub_id, stat_params)
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
    SST_ELI_DECLARE_BASE(Statistic<void>)
    SST_ELI_DECLARE_INFO(
    ELI::ProvidesInterface,
    ELI::ProvidesParams)
    SST_ELI_DECLARE_CTOR(BaseComponent*, const std::string&, const std::string&, SST::Params&)

    void registerOutputFields(StatisticFieldsOutput* stat_output) override;

    void outputStatisticFields(StatisticFieldsOutput* stat_output, bool end_of_sim_flag) override;

protected:
    friend class SST::Factory;
    friend class SST::BaseComponent;
    /** Construct a Statistic
     * @param comp - Pointer to the parent constructor.
     * @param stat_name - Name of the statistic to be registered.  This name must
     * match the name in the ElementInfoStatistic.
     * @param stat_sub_id - Additional name of the statistic
     * @param stat_params - The parameters for this statistic
     */

    Statistic(BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params) :
        StatisticBase(comp, stat_name, stat_sub_id, stat_params)
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
    explicit ImplementsStatFields(T* UNUSED(t)) :
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
    SST_ELI_INTERFACE_INFO(interface)                                                           \
    virtual std::string getELIName() const override                                  \
    {                                                                                \
        return std::string(lib) + "." + name;                                        \
    }

#define SST_ELI_DECLARE_STATISTIC_TEMPLATE_DERIVED(cls, field, lib, name, version, desc, interface)          \
    using SST::Statistics::Statistic<field>::__EliBaseLevel;                                                 \
    using typename SST::Statistics::Statistic<field>::__LocalEliBase;                                        \
    using typename SST::Statistics::Statistic<field>::__ParentEliBase;                                       \
    [[maybe_unused]]                                                                                         \
    static constexpr int __EliDerivedLevel =                                                                 \
        std::is_same_v<SST::Statistics::Statistic<field>, cls<field>> ? __EliBaseLevel : __EliBaseLevel + 1; \
    SST_ELI_DEFAULT_INFO(lib, name, ELI_FORWARD_AS_ONE(version), desc)                                       \
    SST_ELI_INTERFACE_INFO(interface)                                                                                   \
    virtual std::string getELIName() const override                                                          \
    {                                                                                                        \
        return std::string(lib) + "." + name;                                                                \
    }

#define SST_ELI_REGISTER_CUSTOM_STATISTIC(cls, lib, name, version, desc) \
    SST_ELI_REGISTER_DERIVED(SST::Statistics::CustomStatistic,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)                                             \
    SST_ELI_INTERFACE_INFO("CustomStatistic")

#define SST_ELI_DECLARE_STATISTIC(cls, field, lib, name, version, desc, interface)                                   \
    static bool ELI_isLoaded()                                                                                       \
    {                                                                                                                \
        return SST::Statistics::Statistic<field>::template addDerivedInfo<cls>(lib, name) &&                         \
               SST::Statistics::Statistic<field>::template addDerivedBuilder<cls>(lib, name) &&                      \
               SST::Statistics::Statistic<field>::template addDerivedInfo<SST::Statistics::NullStatistic<field>>(    \
                   lib, name) &&                                                                                     \
               SST::Statistics::Statistic<field>::template addDerivedBuilder<SST::Statistics::NullStatistic<field>>( \
                   lib, name);                                                                                       \
    }                                                                                                                \
    SST_ELI_DEFAULT_INFO(lib, name, ELI_FORWARD_AS_ONE(version), desc)                                               \
    SST_ELI_INTERFACE_INFO(interface)                                                                                           \
    static const char* ELI_fieldName()                                                                               \
    {                                                                                                                \
        return #field;                                                                                               \
    }                                                                                                                \
    static const char* ELI_fieldShortName()                                                                          \
    {                                                                                                                \
        return #field;                                                                                               \
    }

#ifdef __INTEL_COMPILER
#define SST_ELI_INSTANTIATE_STATISTIC(cls, field)                                                      \
    bool force_instantiate_##cls##_##field =                                                           \
        SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<field>, cls<field>>::isLoaded() && \
        SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<field>, cls<field>>::isLoaded() &&     \
        SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<field>,                            \
            SST::Statistics::NullStatistic<field>>::isLoaded() &&                                      \
        SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<field>,                                \
            SST::Statistics::NullStatistic<field>>::isLoaded();
#else
#define SST_ELI_INSTANTIATE_STATISTIC(cls, field)                                                   \
    struct cls##_##field##_##shortName : public cls<field>                                          \
    {                                                                                               \
        cls##_##field##_##shortName(                                                                \
            SST::BaseComponent* bc, const std::string& sn, const std::string& si, SST::Params& p) : \
            cls<field>(bc, sn, si, p)                                                               \
        {}                                                                                          \
        static bool ELI_isLoaded()                                                                  \
        {                                                                                           \
            return SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<field>,              \
                       cls##_##field##_##shortName>::isLoaded() &&                                  \
                   SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<field>,                  \
                       cls##_##field##_##shortName>::isLoaded() &&                                  \
                   SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<field>,              \
                       SST::Statistics::NullStatistic<field>>::isLoaded() &&                        \
                   SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<field>,                  \
                       SST::Statistics::NullStatistic<field>>::isLoaded();                          \
        }                                                                                           \
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
#define MAKE_MULTI_STATISTIC(cls, name, tuple, ...)                                                          \
    bool force_instantiate_stat_name =                                                                       \
        SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<tuple>, cls<__VA_ARGS__>>::isLoaded() && \
        SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<tuple>, cls<__VA_ARGS__>>::isLoaded() &&     \
        SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<tuple>,                                  \
            SST::Statistics::NullStatistic<tuple>>::isLoaded() &&                                            \
        SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<tuple>,                                      \
            SST::Statistics::NullStatistic<tuple>>::isLoaded();                                              \
    }                                                                                                        \
    }                                                                                                        \
    ;
#else
#define MAKE_MULTI_STATISTIC(cls, name, tuple, ...)                                                         \
    struct name : public cls<__VA_ARGS__>                                                                   \
    {                                                                                                       \
        name(SST::BaseComponent* bc, const std::string& sn, const std::string& si, SST::Params& p) :        \
            cls<__VA_ARGS__>(bc, sn, si, p)                                                                 \
        {}                                                                                                  \
        bool ELI_isLoaded() const                                                                           \
        {                                                                                                   \
            return SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<tuple>, name>::isLoaded() && \
                   SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<tuple>, name>::isLoaded() &&     \
                   SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<tuple>,                      \
                       SST::Statistics::NullStatistic<tuple>>::isLoaded() &&                                \
                   SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<tuple>,                          \
                       SST::Statistics::NullStatistic<tuple>>::isLoaded();                                  \
        }                                                                                                   \
    };
#endif

#define SST_ELI_INSTANTIATE_MULTI_STATISTIC(cls, ...) \
    MAKE_MULTI_STATISTIC(cls, STAT_GLUE_NAME(cls, __VA_ARGS__), STAT_TUPLE(__VA_ARGS__), __VA_ARGS__)

} // namespace Statistics

namespace Stat::pvt {

/** Helper function for re-registering statistics during simulation restart */
void registerStatWithEngineOnRestart(
    SST::Statistics::StatisticBase* stat, SimTime_t start_factor, SimTime_t stop_factor, SimTime_t output_factor);

} // namespace Stat::pvt

namespace Core::Serialization {

template <class T>
class serialize_impl<Statistics::Statistic<T>*>
{
    void operator()(Statistics::Statistic<T>*& s, serializer& ser, ser_opt_t UNUSED(options))
    {
        // For sizer and pack, need to get the information needed
        // to create a new statistic of the correct type on unpack.
        switch ( ser.mode() ) {
        case serializer::SIZER:
        case serializer::PACK:
        {
            std::string stat_eli_type = s->getELIName();
            SST_SER(stat_eli_type);

            if ( !s->isNullStatistic() ) {
                std::string    stat_name = s->getStatName();
                std::string    stat_id   = s->getStatSubId();
                BaseComponent* comp      = s->getComponent();
                SimTime_t      factor    = s->getOutputRateFactor();
                SST_SER(comp);
                SST_SER(stat_name);
                SST_SER(stat_id);
                SST_SER(factor);
                s->serialize_order(ser);

                // Get state stored at stat engine if needed, must be after stat serialization
                // because we need to read flags first to determine if these fields exist
                if ( s->getStartAtFlag() ) {
                    factor = s->getStartAtFactor();
                    SST_SER(factor);
                }
                if ( s->getStopAtFlag() ) {
                    factor = s->getStopAtFactor();
                    SST_SER(factor);
                }
            }
            break;
        }
        case serializer::UNPACK:
        {
            std::string stat_eli_type;
            Params      params;
            params.insert("type", stat_eli_type);

            SST_SER(stat_eli_type);

            if ( stat_eli_type == "sst.NullStatistic" ) {
                static Statistics::Statistic<T>* nullstat =
                    Factory::getFactory()->CreateWithParams<Statistics::Statistic<T>>(
                        stat_eli_type, params, nullptr, "", "", params);
                s = nullstat;
            }
            else {

                BaseComponent* comp;
                std::string    stat_name;
                std::string    stat_id;
                SimTime_t      output_rate_factor;

                SimTime_t start_at_factor = 0;
                SimTime_t stop_at_factor  = 0;

                SST_SER(comp);
                SST_SER(stat_name);
                SST_SER(stat_id);
                SST_SER(output_rate_factor);

                s = Factory::getFactory()->CreateWithParams<Statistics::Statistic<T>>(
                    stat_eli_type, params, comp, stat_name, stat_id, params);

                s->serialize_order(ser);

                // Need to reconstruct time parameters for engine
                if ( s->getStartAtFlag() ) {
                    SST_SER(start_at_factor);
                }
                if ( s->getStopAtFlag() ) {
                    SST_SER(stop_at_factor);
                }
                SST::Stat::pvt::registerStatWithEngineOnRestart(s, start_at_factor, stop_at_factor, output_rate_factor);
            }
            break;
        }
        case serializer::MAP:
        {
            // Mapping mode not supported for stats
            break;
        }
        }
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace Core::Serialization

} // namespace SST

// we need to make sure null stats are instantiated for whatever types we use
#include "sst/core/statapi/statnull.h"

#endif // SST_CORE_STATAPI_STATBASE_H
