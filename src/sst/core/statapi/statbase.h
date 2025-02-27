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
#include "sst/core/oneshot.h"
#include "sst/core/params.h"
#include "sst/core/serialization/serialize_impl_fwd.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statfieldinfo.h"
#include "sst/core/unitAlgebra.h"
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
    /** Statistic collection mode
     * STAT_MODE_UNDEFINED - unknown mode
     * STAT_MODE_COUNT - generating statistic output when the statistic has been added to a certain number of times
     * STAT_MODE_PERIODIC - generating statistic output on a periodic time basis
     * STAT_MODE_DUMP_AT_END - generating statistic output only at end of simulation
     */
    enum StatMode_t { STAT_MODE_UNDEFINED, STAT_MODE_COUNT, STAT_MODE_PERIODIC, STAT_MODE_DUMP_AT_END };

    // Enable/Disable of Statistic
    /** Enable Statistic for collections */
    void enable() { info_->stat_enabled_ = true; }

    /** Disable Statistic for collections */
    void disable() { info_->stat_enabled_ = false; }

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
    void setFlagResetCountOnOutput(bool flag) { info_->reset_count_on_output_ = flag; }

    static const std::vector<ElementInfoParam>& ELI_getParams();

    /** Set the Clear Data On Output flag.
     *  If Set, the data in the statistic will be cleared by calling
     *  clearStatisticData().
     */
    void setFlagClearDataOnOutput(bool flag) { info_->clear_data_on_output_ = flag; }

    /** Set the Output At End Of Sim flag.
     *  If Set, the statistic will perform an output at the end of simulation.
     */
    void setFlagOutputAtEndOfSim(bool flag) { info_->output_at_end_of_sim_ = flag; }

    // Get Data & Information on Statistic
    /** Return the Component Name */
    const std::string& getCompName() const;

    /** Return the Statistic Name */
    inline const std::string& getStatName() const { return info_->stat_name_; }

    /** Return the Statistic SubId */
    inline const std::string& getStatSubId() const { return info_->stat_sub_id_; }

    /** Return the full Statistic name of component.stat_name.sub_id */
    inline const std::string& getFullStatName() const
    {
        return info_->stat_full_name_;
    } // Return comp_name.stat_name.sub_id

    /** Return the ELI type of the statistic
     *  The ELI registration macro creates this function automatically for child classes.
     */
    virtual std::string getELIName() const = 0;

    /** Return the Statistic type name */
    virtual const std::string& getStatTypeName() const { return info_->stat_type_name_; }

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
    bool isEnabled() const { return info_->stat_enabled_; }

    /** Return the enable status of the Statistic's ability to output data */
    bool isOutputEnabled() const { return info_->output_enabled_; }

    /** Return the rate at which the statistic should be output */
    UnitAlgebra& getCollectionRate() const { return info_->collection_rate_; }

    /** Return the time at which the statistic should be enabled */
    UnitAlgebra& getStartAtTime() const { return info_->start_at_time_; }

    /** Return the time at which the statistic should be disabled */
    UnitAlgebra& getStopAtTime() const { return info_->stop_at_time_; }

    /** Return the collection count limit */
    uint64_t getCollectionCountLimit() const { return info_->collection_count_limit_; }

    /** Return the current collection count */
    uint64_t getCollectionCount() const { return info_->current_collection_count_; }

    /** Return the ResetCountOnOutput flag value */
    bool getFlagResetCountOnOutput() const { return info_->reset_count_on_output_; }

    /** Return the ClearDataOnOutput flag value */
    bool getFlagClearDataOnOutput() const { return info_->clear_data_on_output_; }

    /** Return the OutputAtEndOfSim flag value */
    bool getFlagOutputAtEndOfSim() const { return info_->output_at_end_of_sim_; }

    /** Return the collection mode that is registered */
    StatMode_t getRegisteredCollectionMode() const { return info_->registered_collection_mode_; }

    // Delay Methods (Uses OneShot to disable Statistic or Collection)
    /** Delay the statistic from outputting data for a specified delay time
     * @param delay_time - Value in UnitAlgebra format for delay (i.e. 10ns).
     */
    void delayOutput(const char* delay_time);

    /** Delay the statistic from collecting data for a specified delay time.
     * @param delayTime - Value in UnitAlgebra format for delay (i.e. 10ns).
     */
    void delayCollection(const char* delay_time);

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
     * @param null_stat - True if the statistic is null (not enabled). Defaults to False.
     */
    // Constructors:
    StatisticBase(
        BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params,
        bool null_stat);

    // Destructor
    virtual ~StatisticBase() {}

protected:
    /** Set the Statistic Data Type */
    void setStatisticDataType(const StatisticFieldInfo::fieldType_t data_type) { stat_data_type_ = data_type; }

    /** Set an optional Statistic Type Name (for output) */
    void setStatisticTypeName(const char* type_name) { info_->stat_type_name_ = type_name; }

private:
    friend class SST::BaseComponent;

    /** Set the Registered Collection Mode */
    void setRegisteredCollectionMode(StatMode_t mode) { info_->registered_collection_mode_ = mode; }

    /** Construct a full name of the statistic */
    static std::string buildStatisticFullName(const char* comp_name, const char* stat_name, const char* stat_sub_id);
    static std::string
    buildStatisticFullName(const std::string& comp_name, const std::string& stat_name, const std::string& stat_sub_id);

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
    virtual bool isStatModeSupported(StatMode_t UNUSED(mode)) const { return true; } // Default is to accept all modes

    /** Verify that the statistic names match */
    bool operator==(StatisticBase& check_stat);

    void checkEventForOutput();

    // OneShot Callbacks:
    void delayOutputExpiredHandler();     // Enable Output in handler
    void delayCollectionExpiredHandler(); // Enable Collection in Handler

    const StatisticGroup* getGroup() const { return info_->group_; }
    void                  setGroup(const StatisticGroup* group) { info_->group_ = group; }

protected:
    StatisticBase(); // For serialization only

private:
    /* Class to hold information about a statistic
     * This limits the size of StatisticBase for NullStatistic
     */
    class StatisticInfo
    {
    public:
        StatisticInfo();

        void serialize_order(SST::Core::Serialization::serializer& ser);

        std::string stat_name_;      // Name of stat, matches ELI
        std::string stat_sub_id_;    // Sub ID for this instance of the stat (default="")
        std::string stat_type_name_; // DEPRECATED - override 'getStatTypeName()' instead

        std::string stat_full_name_; // Fully-qualified name of the stat
        bool        stat_enabled_;   // Whether stat is currently collecting data
        StatMode_t  registered_collection_mode_;
        uint64_t    current_collection_count_;
        uint64_t    output_collection_count_;
        uint64_t    collection_count_limit_;

        const StatisticGroup* group_; // Group that stat belongs to

        SST::UnitAlgebra start_at_time_;
        SST::UnitAlgebra stop_at_time_;
        SST::UnitAlgebra collection_rate_;

        bool output_enabled_;
        bool reset_count_on_output_;
        bool clear_data_on_output_;
        bool output_at_end_of_sim_;
        bool output_delayed_;
        bool collection_delayed_;
        bool saved_stat_enabled_;
        bool saved_output_enabled_;
    };

    BaseComponent*                  component_;
    StatisticFieldInfo::fieldType_t stat_data_type_;

    StatisticInfo* info_;

    // Instance of StatisticInfo for null stats
    static StatisticInfo null_info_;
};

/**
 \class StatisticCollector
 * Base type that creates the virtual addData(...) interface
 * Used for distinguishing fundamental types (collected by value)
 * and composite struct types (collected by reference)
 */
template <class T, bool = std::is_fundamental_v<T>>
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
     * @param null_stat - True if the statistic is null (not enabled). Defaults to False.
     */

    Statistic(
        BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params,
        bool null_stat = false) :
        StatisticBase(comp, stat_name, stat_sub_id, stat_params, null_stat)
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
     * @param null_stat - True if the statistic is null (not enabled). Defaults to False.
     */

    Statistic(
        BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params,
        bool null_stat = false) :
        StatisticBase(comp, stat_name, stat_sub_id, stat_params, null_stat)
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
    SST_ELI_INTERFACE_INFO(interface)                                                \
    virtual std::string getELIName() const override { return std::string(lib) + "." + name; }

#define SST_ELI_REGISTER_CUSTOM_STATISTIC(cls, lib, name, version, desc)                                     \
    SST_ELI_REGISTER_DERIVED(SST::Statistics::CustomStatistic,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
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
    SST_ELI_INTERFACE_INFO(interface)                                                                                \
    static const char* ELI_fieldName() { return #field; }                                                            \
    static const char* ELI_fieldShortName() { return #field; }

#ifdef __INTEL_COMPILER
#define SST_ELI_INSTANTIATE_STATISTIC(cls, field)                                                      \
    bool force_instantiate_##cls##_##field =                                                           \
        SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<field>, cls<field>>::isLoaded() && \
        SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<field>, cls<field>>::isLoaded() &&     \
        SST::ELI::InstantiateBuilderInfo<                                                              \
            SST::Statistics::Statistic<field>, SST::Statistics::NullStatistic<field>>::isLoaded() &&   \
        SST::ELI::InstantiateBuilder<                                                                  \
            SST::Statistics::Statistic<field>, SST::Statistics::NullStatistic<field>>::isLoaded();
#else
#define SST_ELI_INSTANTIATE_STATISTIC(cls, field)                                                               \
    struct cls##_##field##_##shortName : public cls<field>                                                      \
    {                                                                                                           \
        cls##_##field##_##shortName(                                                                            \
            SST::BaseComponent* bc, const std::string& sn, const std::string& si, SST::Params& p) :             \
            cls<field>(bc, sn, si, p)                                                                           \
        {}                                                                                                      \
        static bool ELI_isLoaded()                                                                              \
        {                                                                                                       \
            return SST::ELI::InstantiateBuilderInfo<                                                            \
                       SST::Statistics::Statistic<field>, cls##_##field##_##shortName>::isLoaded() &&           \
                   SST::ELI::InstantiateBuilder<                                                                \
                       SST::Statistics::Statistic<field>, cls##_##field##_##shortName>::isLoaded() &&           \
                   SST::ELI::InstantiateBuilderInfo<                                                            \
                       SST::Statistics::Statistic<field>, SST::Statistics::NullStatistic<field>>::isLoaded() && \
                   SST::ELI::InstantiateBuilder<                                                                \
                       SST::Statistics::Statistic<field>, SST::Statistics::NullStatistic<field>>::isLoaded();   \
        }                                                                                                       \
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
        SST::ELI::InstantiateBuilderInfo<                                                                    \
            SST::Statistics::Statistic<tuple>, SST::Statistics::NullStatistic<tuple>>::isLoaded() &&         \
        SST::ELI::InstantiateBuilder<                                                                        \
            SST::Statistics::Statistic<tuple>, SST::Statistics::NullStatistic<tuple>>::isLoaded();           \
    }                                                                                                        \
    }                                                                                                        \
    ;
#else
#define MAKE_MULTI_STATISTIC(cls, name, tuple, ...)                                                             \
    struct name : public cls<__VA_ARGS__>                                                                       \
    {                                                                                                           \
        name(SST::BaseComponent* bc, const std::string& sn, const std::string& si, SST::Params& p) :            \
            cls<__VA_ARGS__>(bc, sn, si, p)                                                                     \
        {}                                                                                                      \
        bool ELI_isLoaded() const                                                                               \
        {                                                                                                       \
            return SST::ELI::InstantiateBuilderInfo<SST::Statistics::Statistic<tuple>, name>::isLoaded() &&     \
                   SST::ELI::InstantiateBuilder<SST::Statistics::Statistic<tuple>, name>::isLoaded() &&         \
                   SST::ELI::InstantiateBuilderInfo<                                                            \
                       SST::Statistics::Statistic<tuple>, SST::Statistics::NullStatistic<tuple>>::isLoaded() && \
                   SST::ELI::InstantiateBuilder<                                                                \
                       SST::Statistics::Statistic<tuple>, SST::Statistics::NullStatistic<tuple>>::isLoaded();   \
        }                                                                                                       \
    };
#endif

#define SST_ELI_INSTANTIATE_MULTI_STATISTIC(cls, ...) \
    MAKE_MULTI_STATISTIC(cls, STAT_GLUE_NAME(cls, __VA_ARGS__), STAT_TUPLE(__VA_ARGS__), __VA_ARGS__)

} // namespace Statistics

namespace Stat::pvt {

/** Helper function for re-registering statistics during simulation restart */
void registerStatWithEngineOnRestart(SST::Statistics::StatisticBase* s);

} // namespace Stat::pvt

namespace Core::Serialization {

template <class T>
class serialize_impl<Statistics::Statistic<T>*>
{
    template <class A>
    friend class serialize;
    void operator()(Statistics::Statistic<T>*& s, serializer& ser, const char* UNUSED(name) = nullptr)
    {
        // For sizer and pack, need to get the information needed
        // to create a new statistic of the correct type on unpack.
        switch ( ser.mode() ) {
        case serializer::SIZER:
        case serializer::PACK:
        {
            std::string    stat_eli_type = s->getELIName();
            std::string    stat_name     = s->getStatName();
            std::string    stat_id       = s->getStatSubId();
            BaseComponent* comp          = s->getComponent();
            ser&           stat_eli_type;
            ser&           comp;
            ser&           stat_name;
            ser&           stat_id;
            s->serialize_order(ser);
            break;
        }
        case serializer::UNPACK:
        {
            std::string    stat_eli_type;
            BaseComponent* comp;
            std::string    stat_name;
            std::string    stat_id;
            ser&           stat_eli_type;
            ser&           comp;
            ser&           stat_name;
            ser&           stat_id;
            Params         params;
            params.insert("type", stat_eli_type);
            s = Factory::getFactory()->CreateWithParams<Statistics::Statistic<T>>(
                stat_eli_type, params, comp, stat_name, stat_id, params);
            s->serialize_order(ser);
            if ( stat_eli_type != "sst.NullStatistic" ) { SST::Stat::pvt::registerStatWithEngineOnRestart(s); }
            break;
        }
        case serializer::MAP:
            // Mapping mode not supported for stats
            break;
        }
    }

    // void operator()(Statistics::Statistic<T>*& UNUSED(s), serializer& UNUSED(ser), const char* UNUSED(name))
    // {
    //     // Mapping mode not supported for stats
    // }
};

} // namespace Core::Serialization

} // namespace SST

// we need to make sure null stats are instantiated for whatever types we use
#include "sst/core/statapi/statnull.h"

#endif // SST_CORE_STATAPI_STATBASE_H
