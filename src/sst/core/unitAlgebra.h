// -*- c++ -*-

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

#ifndef SST_CORE_UNITALGEBRA_H
#define SST_CORE_UNITALGEBRA_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization/serializable.h>
#include <sst/core/serialization/serializer.h>

#include <string>
#include <map>
#include <vector>
#include <mutex>

#include <sst/core/warnmacros.h>
#include <sst/core/decimal_fixedpoint.h>


namespace SST {

// typedef decimal_fixedpoint<3,3> sst_dec_float;
typedef decimal_fixedpoint<3,3> sst_big_num;

/**
 * Helper class internal to UnitAlgebra.
 *
 * Contains information on valid units
 */
class Units {

    typedef uint8_t unit_id_t;

private:
    friend class UnitAlgebra;
    
    // Static data members and functions
    static std::recursive_mutex unit_lock;
    static std::map<std::string,unit_id_t> valid_base_units;
    static std::map<std::string,std::pair<Units,sst_big_num> > valid_compound_units;
    static std::map<unit_id_t,std::string> unit_strings;
    static unit_id_t count;
    static bool initialized;

    static bool initialize();

    // Non-static data members and functions
    std::vector<unit_id_t> numerator;
    std::vector<unit_id_t> denominator;

    void reduce();
    // Used in constructor to incrementally build up unit from string
    void addUnit(std::string, sst_big_num& multiplier, bool invert);

public:
    // Static data members and functions
    /** Create a new Base Unit type */
    static void registerBaseUnit(std::string u);
    /** Create a new Compound Unit type */
    static void registerCompoundUnit(std::string u, std::string v);

    // Non-static data members and functions
    /** Create a new instantiation of a Units with a base unit string, and multiplier
     * \param units String representing the new unit
     * \param multiplier Value by which to multiply to get to this unit
     */
    Units(std::string units, sst_big_num& multiplier);
    Units() {}
    virtual ~Units() {}

    /** Assignment operator */
    Units& operator= (const Units& v);
    /** Self-multiplication operator */
    Units& operator*= (const Units& v);
    /** Self-division operator */
    Units& operator/= (const Units& v);
    /** Equality Operator */
    bool operator== (const Units &lhs) const;
    /** Inequality Operator */
    bool operator!= (const Units &lhs) const {return !(*this == lhs);}
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
class UnitAlgebra : public SST::Core::Serialization::serializable,
                       public SST::Core::Serialization::serializable_type<UnitAlgebra> {
private:
    Units unit;
    sst_big_num value;

    static std::string trim(std::string str);
    void init(std::string val);

public:
    UnitAlgebra() {}
    /**
     Create a new UnitAlgebra instance, and pre-populate with a parsed value.

     \param val  Value to parse.  It is of the following format:
     \code
     val        := NUMBER( )?UNITS
     NUMBER     := (-)?[0-9]+(.[0-9]+)?
     UNITS      := UNITGROUP(/UNITGROUP)
     UNITGROUP  := UNIT(-UNIT)*
     UNIT       := (SIPREFIX)?(BASEUNIT|COMPUNIT)
     SIPREFIX   := {a,f,p,n,u,m,[kKMGTPE]i?}
     BASEUNIT   := {s,B,b,events}
     COMPUNIT   := {Hz,hz,Bps,bps,event}
     \endcode
     */
    UnitAlgebra(std::string val);
    virtual ~UnitAlgebra();

    /** Print to an ostream the value */
    void print(std::ostream& stream);
    /** Print to an ostream the value
     * Formats the number using SI-prefixes
     */
    void printWithBestSI(std::ostream& stream);
    /** Return a string representation of this value */
    std::string toString() const;
    /** Return a string representation of this value
     * Formats the number using SI-prefixes
     */
    std::string toStringBestSI() const;

    UnitAlgebra& operator= (const std::string& v);
    
    /** Multiply by an argument; */
    UnitAlgebra& operator*= (const UnitAlgebra& v);
    /** Multiply by an argument; */
    template <typename T>
    UnitAlgebra& operator*= (const T& v) {
        value *= v;
        return *this;
    }

    /** Divide by an argument; */
    UnitAlgebra& operator/= (const UnitAlgebra& v);
    /** Divide by an argument; */
    template <typename T>
    UnitAlgebra& operator/= (const T& v) {
        value /= v;
        return *this;
    }

    /** Add an argument; */
    UnitAlgebra& operator+= (const UnitAlgebra& v);
    /** Multiply by an argument; */
    template <typename T>
    UnitAlgebra& operator+= (const T& v) {
        value += v;
        return *this;
    }

    /** Subtract an argument; */
    UnitAlgebra& operator-= (const UnitAlgebra& v);
    /** Divide by an argument; */
    template <typename T>
    UnitAlgebra& operator-= (const T& v) {
        value -= v;
        return *this;
    }

    /** Compare if this object is greater than the argument */
    bool operator> (const UnitAlgebra& v) const;
    /** Compare if this object is greater than, or equal to, the argument */
    bool operator>= (const UnitAlgebra& v) const;
    /** Compare if this object is less than the argument */
    bool operator< (const UnitAlgebra& v) const;
    /** Compare if this object is less than, or equal to, the argument */
    bool operator<= (const UnitAlgebra& v) const;
    /** Apply a reciprocal operation to the object */
    UnitAlgebra& invert();

    /** Returns true if the units in the parameter string are found
     * in this object.
     */
    bool hasUnits(std::string u) const;
    /** Return the raw value */
    sst_big_num getValue() const {return value;}
    /** Return the rounded value as a 64bit integer */
    int64_t getRoundedValue() const;

    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        // Do the unit
        ser & unit.numerator;
        ser & unit.denominator;

        // For value, need to convert cpp_dec_float to string and
        // reinit from string
        switch(ser.mode()) {
        case SST::Core::Serialization::serializer::SIZER:
        case SST::Core::Serialization::serializer::PACK: {
            // std::string s = value.str(40, std::ios_base::fixed);
            std::string s = value.toString(0);
            ser & s;
        break;
        }
        case SST::Core::Serialization::serializer::UNPACK: {
            std::string s;
            ser & s;
            value = sst_big_num(s);
            break;
        }
        }
    }
    ImplementSerializable(SST::UnitAlgebra)
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
UnitAlgebra operator* (UnitAlgebra lhs, const T& rhs)
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

inline UnitAlgebra operator* (UnitAlgebra lhs, const UnitAlgebra& rhs)
{
    lhs *= rhs;
    return lhs;
}

template <typename T>
UnitAlgebra operator/ (UnitAlgebra lhs, const T& rhs)
{
    lhs /= rhs;
    return lhs;
}

inline UnitAlgebra operator/ (UnitAlgebra lhs, const UnitAlgebra& rhs)
{
    lhs /= rhs;
    return lhs;
}

template <typename T>
UnitAlgebra operator+ (UnitAlgebra lhs, const T& rhs)
{
    lhs += rhs;
    return lhs;
}

inline UnitAlgebra operator+ (UnitAlgebra lhs, const UnitAlgebra& rhs)
{
    lhs += rhs;
    return lhs;
}

template <typename T>
UnitAlgebra operator- (UnitAlgebra lhs, const T& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline UnitAlgebra operator- (UnitAlgebra lhs, const UnitAlgebra& rhs)
{
    lhs -= rhs;
    return lhs;
}


inline std::ostream& operator<< (std::ostream& os, const UnitAlgebra& r)
{
    os << r.toString();
    return os;
}

inline std::ostream& operator<< (std::ostream& os, const Units& r)
{
    os << r.toString();
    return os;
}

} // namespace SST


#endif //SST_CORE_UNITALGEBRA_H
