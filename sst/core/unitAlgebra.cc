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

#include "sst_config.h"

#include <string>
#include <iostream>
#include <algorithm>

#include "unitAlgebra.h"

#include <sst/core/output.h>
#include <sst/core/simulation.h>

#include <boost/assign.hpp>


using namespace boost::multiprecision;
using namespace std;
using namespace SST;

// Helper functions and data structures used only in this file
static void split(string input, string delims, vector<string>& tokens) {
    if ( input.length() == 0 ) return;
    size_t start = 0;
    size_t stop = 0;;
    vector<string> ret;
    
    do {
        stop = input.find_first_of(delims,start);
        tokens.push_back(input.substr(start,stop-start));
        start = stop + 1;
    } while (stop != string::npos);
}

static map<string,sst_dec_float> si_unit_map =
    boost::assign::map_list_of
    ("a",sst_dec_float("1e-18"))
    ("f",sst_dec_float("1e-15"))
    ("p",sst_dec_float("1e-12"))
    ("n",sst_dec_float("1e-9"))
    ("u",sst_dec_float("1e-6"))
    ("m",sst_dec_float("1e-3"))
    ("k",sst_dec_float("1e3"))
    ("K",sst_dec_float("1e3"))
    ("ki",sst_dec_float(1024l))
    ("Ki",sst_dec_float(1024l))
    ("M",sst_dec_float("1e6"))
    ("Mi",sst_dec_float(1024l*1024l))
    ("G",sst_dec_float("1e9"))
    ("Gi",sst_dec_float(1024l*1024l*1024l))
    ("T",sst_dec_float("1e12"))
    ("Ti",sst_dec_float(1024l*1024l*1024l*1024l))
    ("P",sst_dec_float("1e15"))
    ("Pi",sst_dec_float(1024l*1024l*1024l*1024l*1024l))
    ("E",sst_dec_float("1e18")) 
    ("Ei",sst_dec_float(1024l*1024l*1024l*1024l*1024l*1024l)) ;



// Class Units

map<string,Units::unit_id_t> Units::valid_base_units;
map<string,pair<Units,sst_dec_float> > Units::valid_compound_units;
map<Units::unit_id_t,string> Units::unit_strings;
Units::unit_id_t Units::count;
bool Units::initialized = Units::initialize();

bool
Units::initialize()
{
    count = 1;
    registerBaseUnit("s");
    registerBaseUnit("B");
    registerBaseUnit("b");
    
    registerCompoundUnit("Hz","1/s");
    // Yes, I know it's wrong, but other people don't always realize
    // that.
    registerCompoundUnit("hz","1/s");
    registerCompoundUnit("Bps","B/s");
    registerCompoundUnit("bps","b/s");

    return true;
}

void
Units::reduce()
{
    // Sort both vectors
    sort(numerator.begin(),numerator.end());
    sort(denominator.begin(),denominator.end());
    
    vector<unit_id_t>::iterator n, d;
    n = numerator.begin();
    d = denominator.begin();

    while ( n != numerator.end() && d != denominator.end() ) {
        // cout << *n << "," << *d << endl;
        if ( *n == *d ) {
            n = numerator.erase(n);
            d = denominator.erase(d);
        }
        else {
            if ( *n > *d ) ++d;
            else ++n;
        }
    }
}

void
Units::addUnit(std::string units, sst_dec_float& multiplier, bool invert)
{
    // Check to see if the unit matches one of the registered unit
    // names.  If not, check for SI units and strip them, then check
    // again.
    int si_length = 0;
    if ( valid_base_units.find(units) == valid_base_units.end() &&
         valid_compound_units.find(units) == valid_compound_units.end() ) {
        // Now get the si prefix
        switch ( units[0] ) {
        case 'a':
        case 'f':
        case 'p':
        case 'n':
        case 'u':
        case 'm':
            si_length = 1;
            break;
        case 'k':
        case 'K':
        case 'M':
        case 'G':
        case 'T':
        case 'P':
        case 'E':
            if ( units[1] == 'i' ) si_length = 2;
            else si_length = 1;
            break;
        default:
            si_length = 0;
            break;
        }
    }
    
    if ( si_length > 0 ) {
        string si_unit = units.substr(0,si_length);
        multiplier *= si_unit_map[si_unit];
    }

    // Check to see if the unit is valid and get it's ID
    string type = units.substr(si_length);
    if ( valid_base_units.find(type) != valid_base_units.end() ) {
        if ( !invert ) {
            numerator.push_back(valid_base_units[type]);
        }
        else {
            denominator.push_back(valid_base_units[type]);
        }
    }
    // Check to see if this is a compound unit
    else if ( valid_compound_units.find(type) != valid_compound_units.end() ) {
        pair<Units,sst_dec_float> units = valid_compound_units[type];
        if ( !invert ) {
            *this *= units.first;
            multiplier *= units.second;
        }
        else {
            *this /= units.first;
            multiplier /= units.second;
        }
    }
    // Special case:
    else if ( type == "1" ) {
        return;
    }
    else {
        Output abort = Simulation::getSimulation()->getSimulationOutput();
        abort.fatal(CALL_INFO,1,"Invalid unit type: %s\n",type.c_str());
    }        
}

void
Units::registerBaseUnit(string u)
{
    if ( valid_base_units.find(u) != valid_base_units.end() ) return;
    valid_base_units[u] = count;
    unit_strings[count] = u;
    count++;
    return;
}

void
Units::registerCompoundUnit(string u, string v)
{
    if ( valid_compound_units.find(u) != valid_compound_units.end() ) return;
    sst_dec_float multiplier = 1;
    Units unit(v,multiplier);
    valid_compound_units[u] = std::pair<Units,sst_dec_float>(unit,multiplier);
    return;
}

Units::Units(std::string units, sst_dec_float& multiplier)
{   
    // Get the numerator and the denominator
    string s_numerator;
    string s_denominator;
    
    size_t slash_index = units.find_first_of('/');

    s_numerator = units.substr(0,slash_index);
    if ( slash_index != string::npos ) s_denominator = units.substr(slash_index+1);

    // Have numerator and denominator, now split each of those into
    // individual units, which will be seperated with '-'
    // cout << "Numerator: " << s_numerator << endl;
    vector<string> tokens;
    split(s_numerator, "-", tokens);

    // Add all the numerators
    for ( unsigned int i = 0; i < tokens.size(); i++ ) {
        addUnit(tokens[i], multiplier, false);
    }
    tokens.clear();
    split(s_denominator, "-", tokens);

    // Add all the denominators
    for ( unsigned int i = 0; i < tokens.size(); i++ ) {
        addUnit(tokens[i], multiplier, true);
    }
    reduce();

}

Units&
Units::operator= (const Units& v) {
    numerator = v.numerator;
    denominator = v.denominator;
    return *this;
}

Units&
Units::operator*= (const Units& v) {

    // Simply combine the two numerators and denominators, then reduce.
    numerator.insert(numerator.end(),v.numerator.begin(),v.numerator.end());
    denominator.insert(denominator.end(),v.denominator.begin(),v.denominator.end()); 
    reduce();
    return *this;
}

Units&
Units::operator/= (const Units& v)
{
    numerator.insert(numerator.end(),v.denominator.begin(),v.denominator.end()); 
    denominator.insert(denominator.end(),v.numerator.begin(),v.numerator.end());
    reduce();
    return *this;
}

bool
Units::operator== (const Units &lhs) const
{
    if ( numerator.size() != lhs.numerator.size() ) return false;
    if ( denominator.size() != lhs.denominator.size() ) return false;
    for ( unsigned int i = 0; i < numerator.size(); i++ ) {
        if ( numerator[i] != lhs.numerator[i] ) return false;
    }
    for ( unsigned int i = 0; i < denominator.size(); i++ ) {
        if ( denominator[i] != lhs.denominator[i] ) return false;
    }
    return true;
}

Units&
Units::invert()
{
    std::vector<unit_id_t> temp = denominator;
    denominator = numerator;
    numerator = temp;
    return *this;
}

string
Units::toString() const
{
    if ( numerator.size() == 0 && denominator.size() == 0 ) return "";
    
    // Special case Hz
    if ( valid_compound_units["Hz"].first == *this ) return "Hz";
    
    std::string ret;
    
    if ( numerator.size() == 0 ) ret.append("1");
    else {
        ret.append(unit_strings[numerator[0]]);
        for ( unsigned int i = 1; i < numerator.size(); i++ ) {
            ret.append("-");
            ret.append(unit_strings[numerator[i]]);
        }
    }
    
    if ( denominator.size() != 0 ) {
        ret.append("/");
        ret.append(unit_strings[denominator[0]]);
        for ( unsigned int i = 1; i < denominator.size(); i++ ) {
            ret.append("-");
            ret.append(unit_strings[denominator[i]]);
        }
    }
    
    return ret;
}

// UnitAlgebra

string
UnitAlgebra::trim(std::string str)
{
    // Find whitespace in front
    int front_index = 0;
    while ( isspace(str[front_index]) ) front_index++;
    
    // Find whitespace in back
    int back_index = str.length() - 1;
    while ( isspace(str[back_index]) ) back_index--;
    
    return str.substr(front_index,back_index-front_index+1);
}

void
UnitAlgebra::init(std::string val)
{
    //Trim off all whitespace on front and back
    string parse = trim(val);

    // Start from the back and find the first digit.  Split the string
    // at that point.
    int split = 0;
    for (int i = parse.length() - 1; i >= 0; i--) {
        if ( isdigit(parse[i]) ) {
            split = i+1;
            break;
        }
    }

    string number = trim(parse.substr(0,split));
    string units = trim(parse.substr(split));

    sst_dec_float multiplier(1);
    unit = Units(units,multiplier);

    try {
        value = sst_dec_float(number);
    }
    catch (runtime_error e) {
        Output abort = Simulation::getSimulation()->getSimulationOutput();
        abort.fatal(CALL_INFO,1,"Error: invalud number string: %s\n",number.c_str());
    }
    
    value *= multiplier;
}
    

UnitAlgebra::UnitAlgebra(std::string val)
{
    init(val);
}

void
UnitAlgebra::print(ostream& stream) {
    stream << value << " " << unit.toString() << endl;
}

void
UnitAlgebra::printWithBestSI(std::ostream& stream)
{
    stream << toStringBestSI() << endl;
}

string
UnitAlgebra::toString() const
{
    stringstream s;
    s << value << " " << unit.toString();
    return s.str();
}


string
UnitAlgebra::toStringBestSI() const
{
    stringstream s;
    sst_dec_float temp;
    string si;
    bool found = false;
    for ( map<string,sst_dec_float>::iterator it = si_unit_map.begin();
         it != si_unit_map.end(); ++it ) {
        // Divide by the value, if it's between 1 and 1000, we have
        // the most natual SI unit
        if ( it->first.length() == 2 ) continue;  // Don't do power of 2 units
        temp = value / it->second;
        if ( temp >= 1 && temp < 1000 ) {
            found = true;
            si = it->first;
            break;
        }
    }
    s << (found ? temp : value) << " " << si << unit.toString();
    return s.str();
}

UnitAlgebra&
UnitAlgebra::operator= (const std::string& v)
{
    init(v);
    return *this;
}

UnitAlgebra&
UnitAlgebra::operator*= (const UnitAlgebra& v)
{
    value *= v.value;
    unit *= v.unit;
    return *this;
}

UnitAlgebra&
UnitAlgebra::operator/= (const UnitAlgebra& v)
{
    value /= v.value;
    unit /= v.unit;
    return *this;
}

bool
UnitAlgebra::operator> (const UnitAlgebra& v)
{
    if ( unit != v.unit ) {
        Output abort = Simulation::getSimulation()->getSimulationOutput();
        abort.fatal(CALL_INFO,1,"Error: Attempting to compare UnitAlgebra values "
                    "with non-matching units: %s, %s\n",
                    toString().c_str(), v.toString().c_str());
    }
    return value > v.value;
}

bool
UnitAlgebra::operator>= (const UnitAlgebra& v)
{
    if ( unit != v.unit ) {
        Output abort = Simulation::getSimulation()->getSimulationOutput();
        abort.fatal(CALL_INFO,1,"Error: Attempting to compare UnitAlgebra values "
                    "with non-matching units: %s, %s\n",
                    toString().c_str(), v.toString().c_str());
    }
    return value >= v.value;
}

bool
UnitAlgebra::operator< (const UnitAlgebra& v)
{
    if ( unit != v.unit ) {
        Output abort = Simulation::getSimulation()->getSimulationOutput();
        abort.fatal(CALL_INFO,1,"Error: Attempting to compare UnitAlgebra values "
                    "with non-matching units: %s, %s\n",
                    toString().c_str(), v.toString().c_str());
    }
    return value < v.value;
}

bool
UnitAlgebra::operator<= (const UnitAlgebra& v)
{
    if ( unit != v.unit ) {
        Output abort = Simulation::getSimulation()->getSimulationOutput();
        abort.fatal(CALL_INFO,1,"Error: Attempting to compare UnitAlgebra values "
                    "with non-matching units: %s, %s\n",
                    toString().c_str(), v.toString().c_str());
    }
    return value <= v.value;
}


UnitAlgebra&
UnitAlgebra::invert()
{
    unit.invert();
    value = 1 / value;
    return *this;
}

bool
UnitAlgebra::hasUnits(std::string u) const
{
    sst_dec_float multiplier = 1;
    Units check_units(u,multiplier);
    return unit == check_units;
}

UnitAlgebra::~UnitAlgebra()
{
}

int64_t
UnitAlgebra::getRoundedValue() const
{
    // stringstream ss;
    // int64_t ret;

    // ss << round(value);
    // ss >> ret;    
    // return ret;
    return llround(value);
}


// template <typename T>
// UnitAlgebra operator* (UnitAlgebra lhs, const T& rhs)
// {
//     lhs *= rhs;
//     return lhs;
// }

// template <typename T>
// UnitAlgebra operator* (const T& lhs, UnitAlgebra rhs)
// {
//     rhs *= lhs;
//     return rhs;
// }

// UnitAlgebra operator* (UnitAlgebra& lhs, const UnitAlgebra rhs)
// {
//     lhs *= rhs;
//     return lhs;
// }

// template <typename T>
// UnitAlgebra operator/ (UnitAlgebra lhs, const T& rhs)
// {
//     lhs /= rhs;
//     return lhs;
// }

// std::ostream& operator<< (std::ostream& os, const UnitAlgebra& r)
// {
//     os << r.toString();
//     return os;
// }

// std::ostream& operator<< (std::ostream& os, const Units& r)
// {
//     os << r.toString();
//     return os;
// }
//

SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Units::serialize)
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::UnitAlgebra::serialize)

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Units)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::UnitAlgebra)

