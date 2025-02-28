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

#ifndef SST_CORE_STATAPI_STATNULL_H
#define SST_CORE_STATAPI_STATNULL_H

#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/warnmacros.h"

namespace SST::Statistics {

// NOTE: When calling base class members of classes derived from
//       a templated base class.  The user must use "this->" in
//       order to call base class members (to avoid a compiler
//       error) because they are "nondependant named" and the
//       templated base class is a "dependant named".  The
//       compiler will not look in dependant named base classes
//       when looking up indepenedent names.
// See: http://www.parashift.com/c++-faq-lite/nondependent-name-lookup-members.html

/**
    \class NullStatistic

    An empty statistic place holder.

    @tparam T A template for holding the main data type of this statistic
*/
template <class T, bool = std::is_fundamental_v<T>>
class NullStatisticBase
{};

template <class T>
class NullStatisticBase<T, true> : public Statistic<T>
{
public:
    NullStatisticBase(
        BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        Statistic<T>(comp, statName, statSubId, statParams, true)
    {}

    void addData_impl(T UNUSED(data)) override {}

    void addData_impl_Ntimes(uint64_t UNUSED(N), T UNUSED(data)) override {}

    virtual const std::string& getStatTypeName() const { return stat_type_; }

private:
    inline static const std::string stat_type_ = "NULL";
};

template <class... Args>
class NullStatisticBase<std::tuple<Args...>, false> : public Statistic<std::tuple<Args...>>
{
public:
    NullStatisticBase(
        BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        Statistic<std::tuple<Args...>>(comp, statName, statSubId, statParams, true)
    {}

    void addData_impl(Args... UNUSED(data)) override {}

    void addData_impl_Ntimes(uint64_t UNUSED(N), Args... UNUSED(data)) override {}

    virtual const std::string& getStatTypeName() const { return stat_type_; }

private:
    inline static const std::string stat_type_ = "NULL";
};

template <class T>
class NullStatisticBase<T, false> : public Statistic<T>
{
public:
    NullStatisticBase(
        BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        Statistic<T>(comp, statName, statSubId, statParams, true)
    {}

    void addData_impl(T&& UNUSED(data)) override {}
    void addData_impl(const T& UNUSED(data)) override {}

    void addData_impl_Ntimes(uint64_t UNUSED(N), T&& UNUSED(data)) override {}
    void addData_impl_Ntimes(uint64_t UNUSED(N), const T& UNUSED(data)) override {}

    virtual const std::string& getStatTypeName() const { return stat_type_; }

private:
    inline static const std::string stat_type_ = "NULL";
};

template <class T>
class NullStatistic : public NullStatisticBase<T>
{
public:
    SST_ELI_DECLARE_STATISTIC_TEMPLATE(
      NullStatistic,
      "sst",
      "NullStatistic",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Null object that ignores all collections",
      "SST::Statistic<T>")

    NullStatistic(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParam) :
        NullStatisticBase<T>(comp, statName, statSubId, statParam)
    {}

    ~NullStatistic() {}

    void clearStatisticData() override
    {
        // Do Nothing
    }

    void registerOutputFields(StatisticFieldsOutput* UNUSED(statOutput)) override
    {
        // Do Nothing
    }

    void outputStatisticFields(StatisticFieldsOutput* UNUSED(statOutput), bool UNUSED(EndOfSimFlag)) override
    {
        // Do Nothing
    }

    bool isReady() const override { return true; }

    bool isNullStatistic() const override { return true; }

    static bool isLoaded() { return loaded_; }

private:
    static bool loaded_;
};

template <class T>
bool NullStatistic<T>::loaded_ = true;

template <>
class NullStatistic<void> : public Statistic<void>
{
public:
    SST_ELI_REGISTER_DERIVED(
    Statistic<void>,
    NullStatistic<void>,
    "sst",
    "NullStatistic",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "Null statistic for custom (void) stats"
  )

    SST_ELI_INTERFACE_INFO("Statistic<void>")

    NullStatistic(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        Statistic<void>(comp, statName, statSubId, statParams, true)
    {}

    virtual std::string getELIName() const override { return "sst.NullStatistic"; }

    virtual const std::string& getStatTypeName() const { return stat_type_; }

private:
    inline static const std::string stat_type_ = "NULL";
};

} // namespace SST::Statistics

#endif // SST_CORE_STATAPI_STATNULL_H
