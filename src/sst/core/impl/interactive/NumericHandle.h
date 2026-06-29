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

#include "sst/core/serialization/ObjTree.h"

#include <cstdint>
#include <type_traits>

namespace SST::Core::Serialization {
// A lightweight non-owning handle returned by findByName that enables
// direct comparisons against constants and other tree nodes.
//
// Usage:
//   NumericHandle a = NumericHandle::from(root->findByName("counter"));
//   NumericHandle b = NumericHandle::from(root->findByName("threshold"));
//
//   if (a < 42)    { ... }   // compare against int constant
//   if (a < 3.14)  { ... }   // compare against double constant
//   if (a < b)     { ... }   // compare two tree nodes

class NumericHandle
{
public:
    enum class Kind { None, Integer, Float, Bool, Cont };

private:
    ObjTreeCont* node_ = nullptr;
    Kind         kind_ = Kind::None;

    NumericHandle(ObjTreeCont* n, Kind k) :
        node_(n),
        kind_(k)
    {}

public:
    NumericHandle() = default;

    static NumericHandle from(ObjTreeCont* node)
    {
        if ( !node ) return {};
        if ( dynamic_cast<IntegerObj*>(node) ) return { node, Kind::Integer };
        if ( dynamic_cast<FloatObj*>(node) ) return { node, Kind::Float };
        if ( dynamic_cast<BoolObj*>(node) ) return { node, Kind::Bool };
        if ( dynamic_cast<ContainerObj*>(node) ) return { node, Kind::Cont };
        return {};
    }

    explicit     operator bool() const { return kind_ != Kind::None; }
    Kind         getKind() const { return kind_; }
    ObjTreeCont* getNode() const { return node_; }

    IntegerObj*   asInteger() const { return dynamic_cast<IntegerObj*>(node_); }
    FloatObj*     asFloat() const { return dynamic_cast<FloatObj*>(node_); }
    BoolObj*      asBool() const { return dynamic_cast<BoolObj*>(node_); }
    ContainerObj* asCont() const { return dynamic_cast<ContainerObj*>(node_); }

    // ── Compare handle <=> handle ────────────────────────────────
    bool operator<(const NumericHandle& rhs) const { return cmpHandle(rhs, std::less<> {}); }
    bool operator>(const NumericHandle& rhs) const { return rhs < *this; }
    bool operator<=(const NumericHandle& rhs) const { return !(rhs < *this); }
    bool operator>=(const NumericHandle& rhs) const { return !(*this < rhs); }
    bool operator==(const NumericHandle& rhs) const { return !(*this < rhs) && !(rhs < *this); }
    bool operator!=(const NumericHandle& rhs) const { return !(*this == rhs); }

    // ── Compare handle <=> arithmetic constant ───────────────────
    // A single template for every numeric type avoids the int64_t vs
    // double ambiguity that plain overloads cause.

    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool operator<(T rhs) const
    {
        return cmpConst(rhs, std::less<> {});
    }

    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool operator>(T rhs) const
    {
        return cmpConst(rhs, std::greater<> {});
    }

    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool operator<=(T rhs) const
    {
        return cmpConst(rhs, std::less_equal<> {});
    }

    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool operator>=(T rhs) const
    {
        return cmpConst(rhs, std::greater_equal<> {});
    }

    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool operator==(T rhs) const
    {
        return cmpConst(rhs, std::equal_to<> {});
    }

    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool operator!=(T rhs) const
    {
        return cmpConst(rhs, std::not_equal_to<> {});
    }

private:
    // Visit both sides' variants and compare directly.
    template <typename Cmp>
    bool cmpHandle(const NumericHandle& rhs, Cmp cmp) const
    {
        if ( !*this || !rhs ) return false;

        // Lambda that visits the RHS given a concrete LHS value
        auto visitRhs = [&](auto lhsVal) -> bool {
            auto inner = [&](auto rhsVal) -> bool {
                if constexpr ( std::is_integral_v<decltype(lhsVal)> && std::is_integral_v<decltype(rhsVal)> ) {
                    using Common = std::common_type_t<decltype(lhsVal), decltype(rhsVal)>;
                    return cmp(static_cast<Common>(lhsVal), static_cast<Common>(rhsVal));
                }
                else {
                    return cmp(static_cast<long double>(lhsVal), static_cast<long double>(rhsVal));
                }
            };
            if ( rhs.kind_ == Kind::Integer ) return rhs.asInteger()->visit(inner);
            if ( rhs.kind_ == Kind::Float ) return rhs.asFloat()->visit(inner);
            if ( rhs.kind_ == Kind::Bool ) return inner(static_cast<int64_t>(rhs.asBool()->getVal()));
            if ( rhs.kind_ == Kind::Cont ) return inner(static_cast<int64_t>(rhs.asCont()->getSize()));
            return false;
        };

        if ( kind_ == Kind::Integer ) return asInteger()->visit(visitRhs);
        if ( kind_ == Kind::Float ) return asFloat()->visit(visitRhs);
        if ( kind_ == Kind::Bool ) return visitRhs(static_cast<int64_t>(asBool()->getVal()));
        if ( kind_ == Kind::Cont ) return visitRhs(static_cast<int64_t>(asCont()->getSize()));
        return false;
    }

    // Visit the held variant and apply `cmp` against any arithmetic constant.
    // Integer-to-integer comparisons stay exact; anything involving a
    // floating-point side is promoted to long double to preserve precision.
    template <typename ConstT, typename Cmp>
    bool cmpConst(ConstT rhs, Cmp cmp) const
    {
        if ( kind_ == Kind::Integer ) {
            return asInteger()->visit([&](auto lhs) {
                if constexpr ( std::is_integral_v<decltype(lhs)> && std::is_integral_v<ConstT> ) {
                    // Both sides integral — compare without floating-point loss
                    using Common = std::common_type_t<decltype(lhs), ConstT>;
                    return cmp(static_cast<Common>(lhs), static_cast<Common>(rhs));
                }
                else {
                    return cmp(static_cast<long double>(lhs), static_cast<long double>(rhs));
                }
            });
        }
        if ( kind_ == Kind::Float ) {
            return asFloat()->visit(
                [&](auto lhs) { return cmp(static_cast<long double>(lhs), static_cast<long double>(rhs)); });
        }
        if ( kind_ == Kind::Bool ) {
            auto lhs = static_cast<int64_t>(asBool()->getVal());
            if constexpr ( std::is_integral_v<ConstT> ) {
                using Common = std::common_type_t<int64_t, ConstT>;
                return cmp(static_cast<Common>(lhs), static_cast<Common>(rhs));
            }
            else {
                return cmp(static_cast<long double>(lhs), static_cast<long double>(rhs));
            }
        }
        if ( kind_ == Kind::Cont ) {
            auto lhs = asCont()->getSize();
            if constexpr ( std::is_integral_v<ConstT> ) {
                using Common = std::common_type_t<int64_t, ConstT>;
                return cmp(static_cast<Common>(lhs), static_cast<Common>(rhs));
            }
            else {
                return cmp(static_cast<long double>(lhs), static_cast<long double>(rhs));
            }
        }
        return false;
    }
};

// ── Reverse comparisons (constant <=> handle) ───────────────────
template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
bool
operator<(T lhs, const NumericHandle& rhs)
{
    return rhs > lhs;
}

template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
bool
operator>(T lhs, const NumericHandle& rhs)
{
    return rhs < lhs;
}

template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
bool
operator<=(T lhs, const NumericHandle& rhs)
{
    return rhs >= lhs;
}

template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
bool
operator>=(T lhs, const NumericHandle& rhs)
{
    return rhs <= lhs;
}
}; // namespace SST::Core::Serialization
