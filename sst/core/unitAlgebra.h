// -*- c++ -*-

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

#ifndef SST_CORE_UNITALGEBRA_H
#define SST_CORE_UNITALGEBRA_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <string>
#include <map>
#include <vector>

#include <boost/multiprecision/cpp_dec_float.hpp>

namespace SST {



typedef boost::multiprecision::number<boost::multiprecision::cpp_dec_float<40,boost::int16_t> > sst_dec_float;

class Units {

    typedef uint8_t unit_id_t;
    
private:

    // Static data members and functions
    static std::map<std::string,unit_id_t> valid_base_units;
    static std::map<std::string,std::pair<Units,sst_dec_float> > valid_compound_units;
    static std::map<unit_id_t,std::string> unit_strings;
    static unit_id_t count;
    static bool initialized;
    
    static bool initialize();

    // Non-static data members and functions
    std::vector<unit_id_t> numerator;
    std::vector<unit_id_t> denominator;

    void reduce();
    // Used in constructor to incrementally build up unit from string
    void addUnit(std::string, sst_dec_float& multiplier, bool invert);

    
public:
    // Static data members and functions
    static void registerBaseUnit(std::string u);
    static void registerCompoundUnit(std::string u, std::string v);
    
    // Non-static data members and functions
    Units(std::string units, sst_dec_float& multiplier);
    Units() {}
    ~Units() {}

    Units& operator= (const Units& v);
    Units& operator*= (const Units& v);
    Units& operator/= (const Units& v);
    bool operator== (const Units &lhs) const;
    bool operator!= (const Units &lhs) const {return !(*this == lhs);}
    Units& invert();
    
    std::string toString() const;
};

class UnitAlgebra {
private:
    Units unit;
    sst_dec_float value;
    
    static std::string trim(std::string str);
    
public:
    UnitAlgebra() {}
    UnitAlgebra(std::string val);
    ~UnitAlgebra();

    void print(std::ostream& stream);
    void printWithBestSI(std::ostream& stream);
    std::string toString() const;
    std::string toStringBestSI() const;

    UnitAlgebra& operator*= (const UnitAlgebra& v);
    template <typename T>
    UnitAlgebra& operator*= (const T& v) {
        value *= v;
        return *this;
    }

    UnitAlgebra& operator/= (const UnitAlgebra& v);
    template <typename T>
    UnitAlgebra& operator/= (const T& v) {
        value /= v;
        return *this;
    }

    bool operator> (const UnitAlgebra& v);
    bool operator>= (const UnitAlgebra& v);
    bool operator< (const UnitAlgebra& v);
    bool operator<= (const UnitAlgebra& v);
    UnitAlgebra& invert();
    
    bool hasUnits(std::string u) const;
    sst_dec_float getValue() const {return value;}
    int64_t getRoundedValue() const;
    
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

template <typename T>
UnitAlgebra operator* (const T& lhs, UnitAlgebra rhs)
{
    rhs *= lhs;
    return rhs;
}

inline UnitAlgebra operator* (UnitAlgebra& lhs, const UnitAlgebra rhs)
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

#endif //SST_CORE_TIMELORD_H
