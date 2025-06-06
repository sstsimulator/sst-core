// -*- c++ -*-

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

#ifndef SST_CORE_UNITALGEBRA_H
#define SST_CORE_UNITALGEBRA_H

#include "sst/core/decimal_fixedpoint.h"
#include "sst/core/serialization/objectMap.h"
#include "sst/core/serialization/serialize.h"
#include "sst/core/serialization/serializer.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace SST {

using sst_big_num = decimal_fixedpoint<3, 3>;

/**
 * Helper class internal to UnitAlgebra.
 *
 * Contains information on valid units
 */
class Units
{

    using unit_id_t = uint8_t;

private:
    friend class UnitAlgebra;

    // Static data members and functions
    static std::recursive_mutex                                 unit_lock;
    static std::map<std::string, unit_id_t>                     valid_base_units;
    static std::map<std::string, std::pair<Units, sst_big_num>> valid_compound_units;
    static std::map<unit_id_t, std::string>                     unit_strings;
    static unit_id_t                                            count;
    static bool                                                 initialized;

    static bool initialize();

    // Non-static data members and functions
    std::vector<unit_id_t> numerator;
    std::vector<unit_id_t> denominator;

    void reduce();
    // Used in constructor to incrementally build up unit from string
    void addUnit(const std::string&, sst_big_num& multiplier, bool invert);

public:
    // Static data members and functions
    /** Create a new Base Unit type */
    static void registerBaseUnit(const std::string& u);
    /** Create a new Compound Unit type */
    static void registerCompoundUnit(const std::string& u, const std::string& v);

    // Non-static data members and functions
    /** Create a new instantiation of a Units with a base unit string, and multiplier
     * @param units String representing the new unit
     * @param multiplier Value by which to multiply to get to this unit
     */
    Units(const std::string& units, sst_big_num& multiplier);
    Units() {}
    virtual ~Units() {}

    /** Copy constructor */
    Units(const Units&) = default;

    /** Assignment operator */
    Units& operator=(const Units& v);
    /** Self-multiplication operator */
    Units& operator*=(const Units& v);
    /** Self-division operator */
    Units& operator/=(const Units& v);
    /** Equality Operator */
    bool   operator==(const Units& lhs) const;
    /** Inequality Operator */
    bool   operator!=(const Units& lhs) const { return !(*this == lhs); }
    /** Perform a reciprocal operation.  Numerator and Denominator swap. */
    Units& invert();

    /** Return a String representation if this Unit */
    std::string toString() const;
};

/**
 * Performs Unit math in full precision
 *
 * Allows operations such as multiplying a frequency by 2.
 *
 */
class UnitAlgebra
{
private:
    Units       unit;
    sst_big_num value;

    static std::string trim(const std::string& str);

public:
    void init(const std::string& val);

    UnitAlgebra() {}
    /**
     Create a new UnitAlgebra instance, and pre-populate with a parsed value.

     @param val  Value to parse.  It is of the following format:
     @code
     val        := NUMBER( )?UNITS
     NUMBER     := (-)?[0-9]+(.[0-9]+)?
     UNITS      := UNITGROUP(/UNITGROUP)
     UNITGROUP  := UNIT(-UNIT)*
     UNIT       := (SIPREFIX)?(BASEUNIT|COMPUNIT)
     SIPREFIX   := {a,f,p,n,u,m,[kKMGTPE]i?}
     BASEUNIT   := {s,B,b,events}
     COMPUNIT   := {Hz,hz,Bps,bps,event}
     @endcode
     */
    UnitAlgebra(const std::string& val);
    virtual ~UnitAlgebra() = default;

    /** Copy constructor */
    UnitAlgebra(const UnitAlgebra&) = default;

    /** Print to an ostream the value
     * @param stream Output stream
     * @param precision Number of digits to print. Default is 6. <= 0 is full precision.
     */
    void        print(std::ostream& stream, int32_t precision = 6);
    /** Print to an ostream the value
     * Formats the number using SI-prefixes
     * @param stream Output stream
     * @param precision Number of digits to print. Default is 6. <= 0 is full precision.
     */
    void        printWithBestSI(std::ostream& stream, int32_t precision = 6);
    /** Return a string representation of this value
     * @param precision Number of digits to print. Default is 6. <= 0 is full precision.
     */
    std::string toString(int32_t precision = 6) const;
    /** Return a string representation of this value
     * Formats the number using SI-prefixes
     * @param precision Number of digits to print. Default is 6. <= 0 is full precision.
     */
    std::string toStringBestSI(int32_t precision = 6) const;

    UnitAlgebra& operator=(const std::string& v);

    /** Multiply by an argument; */
    UnitAlgebra& operator*=(const UnitAlgebra& v);
    /** Multiply by an argument; */
    template <typename T>
    UnitAlgebra& operator*=(const T& v)
    {
        value *= v;
        return *this;
    }

    /** Divide by an argument; */
    UnitAlgebra& operator/=(const UnitAlgebra& v);
    /** Divide by an argument; */
    template <typename T>
    UnitAlgebra& operator/=(const T& v)
    {
        value /= v;
        return *this;
    }

    /** Add an argument; */
    UnitAlgebra& operator+=(const UnitAlgebra& v);
    /** Multiply by an argument; */
    template <typename T>
    UnitAlgebra& operator+=(const T& v)
    {
        value += v;
        return *this;
    }

    /** Subtract an argument; */
    UnitAlgebra& operator-=(const UnitAlgebra& v);
    /** Divide by an argument; */
    template <typename T>
    UnitAlgebra& operator-=(const T& v)
    {
        value -= v;
        return *this;
    }

    /** Compare if this object is greater than the argument */
    bool         operator>(const UnitAlgebra& v) const;
    /** Compare if this object is greater than, or equal to, the argument */
    bool         operator>=(const UnitAlgebra& v) const;
    /** Compare if this object is less than the argument */
    bool         operator<(const UnitAlgebra& v) const;
    /** Compare if this object is less than, or equal to, the argument */
    bool         operator<=(const UnitAlgebra& v) const;
    /** Compare if this object is equal to, the argument */
    bool         operator==(const UnitAlgebra& v) const;
    /** Compare if this object is not equal to, the argument */
    bool         operator!=(const UnitAlgebra& v) const;
    /** Apply a reciprocal operation to the object */
    UnitAlgebra& invert();

    /** Returns true if the units in the parameter string are found
     * in this object.
     */
    bool        hasUnits(const std::string& u) const;
    /** Return the raw value */
    sst_big_num getValue() const { return value; }
    /** @return Rounded value as a 64bit integer */
    int64_t     getRoundedValue() const;
    double      getDoubleValue() const;
    bool        isValueZero() const;

    void serialize_order(SST::Core::Serialization::serializer& ser) /* override */
    {
        // Do the unit
        SST_SER(unit.numerator);
        SST_SER(unit.denominator);

        // For value, need to convert cpp_dec_float to string and
        // reinit from string
        switch ( ser.mode() ) {
        case SST::Core::Serialization::serializer::SIZER:
        case SST::Core::Serialization::serializer::PACK:
        {
            // std::string s = value.str(40, std::ios_base::fixed);
            std::string s = value.toString(0);
            SST_SER(s);
            break;
        }
        case SST::Core::Serialization::serializer::UNPACK:
        {
            std::string s;
            SST_SER(s);
            value = sst_big_num(s);
            break;
        }
        case SST::Core::Serialization::serializer::MAP:
            // Add your code here
            break;
        }
    }

public:
    /** Base exception for all exception classes in UnitAlgebra
     *
     * This exception inherits from std::logic_error, as all exceptions
     * thrown in UnitAlgebra are considered configuration errors occurring
     * prior to simulation start, rather than runtime errors.
     */
    class UnitAlgebraException : public std::logic_error
    {
    public:
        /**
         * @param msg exception message displayed as-is without modification
         */
        explicit UnitAlgebraException(const std::string& msg);
    };

    /** Exception for when units are not recognized or are invalid
     */
    class InvalidUnitType : public UnitAlgebraException
    {
    public:
        /**
         * @param type string containing invalid type
         */
        explicit InvalidUnitType(const std::string& type);
    };

    /** Exception for when number couldn't be parsed
     */
    class InvalidNumberString : public UnitAlgebraException
    {
    public:
        /**
         * @param number string containing invalid number
         */
        explicit InvalidNumberString(const std::string& number);
    };

    /** Exception for when attempting operations between objects that do not have matching base units
     */
    class NonMatchingUnits : public UnitAlgebraException
    {
    public:
        /**
         * @param lhs units for UnitAlgebra on left-hand side of operation
         * @param rhs units for UnitAlgebra on right-hand side of operation
         * @param operation representation of operation attempted between units
         */
        NonMatchingUnits(const std::string& lhs, const std::string& rhs, const std::string& operation);
    };
};

// template <typename T>
// UnitAlgebra operator* (UnitAlgebra lhs, const T& rhs);

// template <typename T>
// UnitAlgebra operator* (const T& lhs, UnitAlgebra rhs);

//     UnitAlgebra operator* (UnitAlgebra& lhs, const UnitAlgebra rhs);

// template <typename T>
// UnitAlgebra operator/ (UnitAlgebra lhs, const T& rhs);

// std::ostream& operator<< (std::ostream& os, const UnitAlgebra& r);

// std::ostream& operator<< (std::ostream& os, const Units& r);

template <typename T>
UnitAlgebra
operator*(UnitAlgebra lhs, const T& rhs)
{
    lhs *= rhs;
    return lhs;
}

// template <typename T>
// UnitAlgebra operator* (const T& lhs, UnitAlgebra rhs)
// {
//     rhs *= lhs;
//     return rhs;
// }

inline UnitAlgebra
operator*(UnitAlgebra lhs, const UnitAlgebra& rhs)
{
    lhs *= rhs;
    return lhs;
}

template <typename T>
UnitAlgebra
operator/(UnitAlgebra lhs, const T& rhs)
{
    lhs /= rhs;
    return lhs;
}

inline UnitAlgebra
operator/(UnitAlgebra lhs, const UnitAlgebra& rhs)
{
    lhs /= rhs;
    return lhs;
}

template <typename T>
UnitAlgebra
operator+(UnitAlgebra lhs, const T& rhs)
{
    lhs += rhs;
    return lhs;
}

inline UnitAlgebra
operator+(UnitAlgebra lhs, const UnitAlgebra& rhs)
{
    lhs += rhs;
    return lhs;
}

template <typename T>
UnitAlgebra
operator-(UnitAlgebra lhs, const T& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline UnitAlgebra
operator-(UnitAlgebra lhs, const UnitAlgebra& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline std::ostream&
operator<<(std::ostream& os, const UnitAlgebra& r)
{
    os << r.toString();
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const Units& r)
{
    os << r.toString();
    return os;
}


namespace Core::Serialization {

template <>
class ObjectMapFundamental<UnitAlgebra> : public ObjectMap
{
protected:
    /**
       Address of the variable for reading and writing
     */
    UnitAlgebra* addr_ = nullptr;

public:
    std::string get() override { return addr_->toStringBestSI(); }
    void        set_impl(const std::string& value) override { addr_->init(value); }

    // We'll act like we're a fundamental type
    bool isFundamental() override { return true; }

    /**
       Get the address of the variable represented by the ObjectMap

       @return Address of varaible
     */
    void* getAddr() override { return addr_; }

    explicit ObjectMapFundamental(UnitAlgebra* addr) :
        ObjectMap(),
        addr_(addr)
    {}

    std::string getType() override { return demangle_name(typeid(UnitAlgebra).name()); }
};

template <>
class serialize_impl<UnitAlgebra>
{
    void operator()(UnitAlgebra& ua, serializer& ser, ser_opt_t options)
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        case serializer::PACK:
        case serializer::UNPACK:
            ua.serialize_order(ser);
            break;
        case serializer::MAP:
        {
            ObjectMap* obj_map = new ObjectMapFundamental<UnitAlgebra>(&ua);
            if ( options & SerOption::map_read_only ) {
                ser.mapper().setNextObjectReadOnly();
            }
            ser.mapper().map_primitive(ser.getMapName(), obj_map);
            break;
        }
        }
    }

    SST_FRIEND_SERIALIZE();
};


} // namespace Core::Serialization

} // namespace SST

#endif // SST_CORE_UNITALGEBRA_H
