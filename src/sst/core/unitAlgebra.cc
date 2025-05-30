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

#include "sst_config.h"

#include "unitAlgebra.h"

#include "sst/core/output.h"

#include <algorithm>
#include <iostream>
#include <string>

using namespace SST;

// Helper functions and data structures used only in this file
static void
split(const std::string& input, const std::string& delims, std::vector<std::string>& tokens)
{
    if ( input.length() == 0 ) return;
    size_t start = 0;
    size_t stop  = 0;
    ;
    std::vector<std::string> ret;

    do {
        stop = input.find_first_of(delims, start);
        tokens.push_back(input.substr(start, stop - start));
        start = stop + 1;
    } while ( stop != std::string::npos );
}

static std::map<std::string, sst_big_num> si_unit_map = { { "a", sst_big_num("1e-18") }, { "f", sst_big_num("1e-15") },
    { "p", sst_big_num("1e-12") }, { "n", sst_big_num("1e-9") }, { "u", sst_big_num("1e-6") },
    { "m", sst_big_num("1e-3") }, { "k", sst_big_num("1e3") }, { "K", sst_big_num("1e3") },
    { "ki", sst_big_num(1024l) }, { "Ki", sst_big_num(1024l) }, { "M", sst_big_num("1e6") },
    { "Mi", sst_big_num(1024l * 1024l) }, { "G", sst_big_num("1e9") }, { "Gi", sst_big_num(1024l * 1024l * 1024l) },
    { "T", sst_big_num("1e12") }, { "Ti", sst_big_num(1024l * 1024l * 1024l * 1024l) }, { "P", sst_big_num("1e15") },
    { "Pi", sst_big_num(1024l * 1024l * 1024l * 1024l * 1024l) }, { "E", sst_big_num("1e18") },
    { "Ei", sst_big_num(1024l * 1024l * 1024l * 1024l * 1024l * 1024l) } };

// Class Units

std::recursive_mutex                                 Units::unit_lock;
std::map<std::string, Units::unit_id_t>              Units::valid_base_units;
std::map<std::string, std::pair<Units, sst_big_num>> Units::valid_compound_units;
std::map<Units::unit_id_t, std::string>              Units::unit_strings;
Units::unit_id_t                                     Units::count;
bool                                                 Units::initialized = Units::initialize();

bool
Units::initialize()
{
    count = 1;
    registerBaseUnit("s");
    registerBaseUnit("B");
    registerBaseUnit("b");
    registerBaseUnit("events");

    registerCompoundUnit("Hz", "1/s");
    // Yes, I know it's wrong, but other people don't always realize
    // that.
    registerCompoundUnit("hz", "1/s");
    registerCompoundUnit("Bps", "B/s");
    registerCompoundUnit("bps", "b/s");
    registerCompoundUnit("event", "events");

    return true;
}

void
Units::reduce()
{
    // Sort both vectors
    sort(numerator.begin(), numerator.end());
    sort(denominator.begin(), denominator.end());

    std::vector<unit_id_t>::iterator n, d;
    n = numerator.begin();
    d = denominator.begin();

    while ( n != numerator.end() && d != denominator.end() ) {
        // cout << *n << "," << *d << endl;
        if ( *n == *d ) {
            n = numerator.erase(n);
            d = denominator.erase(d);
        }
        else {
            if ( *n > *d )
                ++d;
            else
                ++n;
        }
    }
}

void
Units::addUnit(const std::string& units, sst_big_num& multiplier, bool invert)
{
    std::lock_guard<std::recursive_mutex> lock(unit_lock);
    // Check to see if the unit matches one of the registered unit
    // names.  If not, check for SI units and strip them, then check
    // again.
    int                                   si_length = 0;
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
            if ( units[1] == 'i' )
                si_length = 2;
            else
                si_length = 1;
            break;
        default:
            si_length = 0;
            break;
        }
    }

    if ( si_length > 0 ) {
        std::string si_unit = units.substr(0, si_length);
        multiplier *= si_unit_map[si_unit];
    }

    // Check to see if the unit is valid and get its ID
    std::string type = units.substr(si_length);
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
        auto units = valid_compound_units[type];
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
        throw UnitAlgebra::InvalidUnitType(type);
    }
}

void
Units::registerBaseUnit(const std::string& u)
{
    std::lock_guard<std::recursive_mutex> lock(unit_lock);
    if ( valid_base_units.find(u) != valid_base_units.end() ) return;
    valid_base_units[u] = count;
    unit_strings[count] = u;
    count++;
    return;
}

void
Units::registerCompoundUnit(const std::string& u, const std::string& v)
{
    std::lock_guard<std::recursive_mutex> lock(unit_lock);
    if ( valid_compound_units.find(u) != valid_compound_units.end() ) return;
    sst_big_num multiplier = 1;
    Units       unit(v, multiplier);
    valid_compound_units[u] = std::pair<Units, sst_big_num>(unit, multiplier);
    return;
}

Units::Units(const std::string& units, sst_big_num& multiplier)
{
    // Get the numerator and the denominator
    std::string s_numerator;
    std::string s_denominator;

    size_t slash_index = units.find_first_of('/');

    s_numerator = units.substr(0, slash_index);
    if ( slash_index != std::string::npos ) s_denominator = units.substr(slash_index + 1);

    // Have numerator and denominator, now split each of those into
    // individual units, which will be separated with '-'
    // cout << "Numerator: " << s_numerator << endl;
    std::vector<std::string> tokens;
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
Units::operator=(const Units& v)
{
    numerator   = v.numerator;
    denominator = v.denominator;
    return *this;
}

Units&
Units::operator*=(const Units& v)
{

    // Simply combine the two numerators and denominators, then reduce.
    numerator.insert(numerator.end(), v.numerator.begin(), v.numerator.end());
    denominator.insert(denominator.end(), v.denominator.begin(), v.denominator.end());
    reduce();
    return *this;
}

Units&
Units::operator/=(const Units& v)
{
    numerator.insert(numerator.end(), v.denominator.begin(), v.denominator.end());
    denominator.insert(denominator.end(), v.numerator.begin(), v.numerator.end());
    reduce();
    return *this;
}

bool
Units::operator==(const Units& lhs) const
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
    denominator                 = numerator;
    numerator                   = temp;
    return *this;
}

std::string
Units::toString() const
{
    std::lock_guard<std::recursive_mutex> lock(unit_lock);
    if ( numerator.size() == 0 && denominator.size() == 0 ) return "";

    // Special case Hz
    if ( valid_compound_units["Hz"].first == *this ) return "Hz";

    std::string ret;

    if ( numerator.size() == 0 )
        ret.append("1");
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

std::string
UnitAlgebra::trim(const std::string& str)
{
    if ( !str.size() ) return str;

    // Find whitespace in front
    int front_index = 0;
    while ( isspace(str[front_index]) )
        front_index++;

    // Find whitespace in back
    int back_index = str.length() - 1;
    while ( isspace(str[back_index]) )
        back_index--;

    return str.substr(front_index, back_index - front_index + 1);
}

void
UnitAlgebra::init(const std::string& val)
{
    // Trim off all whitespace on front and back
    const std::string parse = trim(val);

    // Start from the back and find the first digit.  Split the string
    // at that point.
    int split = 0;
    for ( int i = parse.length() - 1; i >= 0; i-- ) {
        if ( isdigit(parse[i]) ) {
            split = i + 1;
            break;
        }
    }

    const std::string number = trim(parse.substr(0, split));
    std::string       units  = trim(parse.substr(split));

    sst_big_num multiplier(1);
    unit = Units(units, multiplier);

    try {
        value = sst_big_num(number);
    }
    catch ( std::runtime_error& e ) {
        throw InvalidNumberString(number);
    }

    value *= multiplier;
}

UnitAlgebra::UnitAlgebra(const std::string& val)
{
    init(val);
}

void
UnitAlgebra::print(std::ostream& stream, int32_t precision)
{
    stream << value.toString(precision) << " " << unit.toString() << std::endl;
}

void
UnitAlgebra::printWithBestSI(std::ostream& stream, int32_t precision)
{
    stream << toStringBestSI(precision) << std::endl;
}

std::string
UnitAlgebra::toString(int32_t precision) const
{
    std::stringstream s;
    s << value.toString(precision) << " " << unit.toString();
    return s.str();
}

std::string
UnitAlgebra::toStringBestSI(int32_t precision) const
{
    std::stringstream s;
    sst_big_num       temp;
    std::string       si;
    bool              found = false;
    for ( auto it = si_unit_map.begin(); it != si_unit_map.end(); ++it ) {
        // Divide by the value, if it's between 1 and 1000, we have
        // the most natural SI unit
        if ( it->first.length() == 2 ) continue; // Don't do power of 2 units
        temp = value / it->second;
        if ( temp >= 1 && temp < 1000 ) {
            found = true;
            si    = it->first;
            break;
        }
    }
    s << (found ? temp.toString(precision) : value.toString(precision)) << " " << si << unit.toString();
    return s.str();
}

UnitAlgebra&
UnitAlgebra::operator=(const std::string& v)
{
    init(v);
    return *this;
}

UnitAlgebra&
UnitAlgebra::operator*=(const UnitAlgebra& v)
{
    value *= v.value;
    unit *= v.unit;
    return *this;
}

UnitAlgebra&
UnitAlgebra::operator/=(const UnitAlgebra& v)
{
    value /= v.value;
    unit /= v.unit;
    return *this;
}

UnitAlgebra&
UnitAlgebra::operator+=(const UnitAlgebra& v)
{
    if ( unit != v.unit ) {
        throw NonMatchingUnits(unit.toString(), v.unit.toString(), "add");
    }
    value += v.value;
    return *this;
}

UnitAlgebra&
UnitAlgebra::operator-=(const UnitAlgebra& v)
{
    if ( unit != v.unit ) {
        throw NonMatchingUnits(unit.toString(), v.unit.toString(), "subtract");
    }
    value -= v.value;
    return *this;
}

bool
UnitAlgebra::operator>(const UnitAlgebra& v) const
{
    if ( unit != v.unit ) {
        throw NonMatchingUnits(unit.toString(), v.unit.toString(), "compare");
    }
    return value > v.value;
}

bool
UnitAlgebra::operator>=(const UnitAlgebra& v) const
{
    if ( unit != v.unit ) {
        Output abort = Output::getDefaultObject();
        throw NonMatchingUnits(unit.toString(), v.unit.toString(), "compare");
    }
    return value >= v.value;
}

bool
UnitAlgebra::operator<(const UnitAlgebra& v) const
{
    if ( unit != v.unit ) {
        throw NonMatchingUnits(unit.toString(), v.unit.toString(), "compare");
    }
    return value < v.value;
}

bool
UnitAlgebra::operator<=(const UnitAlgebra& v) const
{
    if ( unit != v.unit ) {
        throw NonMatchingUnits(unit.toString(), v.unit.toString(), "compare");
    }
    return value <= v.value;
}

bool
UnitAlgebra::operator==(const UnitAlgebra& v) const
{
    if ( unit != v.unit ) return false;
    return value == v.value;
}

bool
UnitAlgebra::operator!=(const UnitAlgebra& v) const
{
    if ( unit != v.unit ) return false;
    return value != v.value;
}

UnitAlgebra&
UnitAlgebra::invert()
{
    unit.invert();
    // value = 1 / value;
    value.inverse();
    return *this;
}

bool
UnitAlgebra::hasUnits(const std::string& u) const
{
    sst_big_num multiplier = 1;
    Units       check_units(u, multiplier);
    return unit == check_units;
}

int64_t
UnitAlgebra::getRoundedValue() const
{
    // stringstream ss;
    // int64_t ret;

    // ss << round(value);
    // ss >> ret;
    // return ret;
    // return llround(value);
    return value.toLong();
}

double
UnitAlgebra::getDoubleValue() const
{
    return value.toDouble();
}

bool
UnitAlgebra::isValueZero() const
{
    return value.isZero();
}

UnitAlgebra::UnitAlgebraException::UnitAlgebraException(const std::string& msg) :
    std::logic_error(msg)
{}

UnitAlgebra::InvalidUnitType::InvalidUnitType(const std::string& type) :
    UnitAlgebra::UnitAlgebraException(std::string("Invalid unit type: ") + type)
{}

UnitAlgebra::InvalidNumberString::InvalidNumberString(const std::string& number) :
    UnitAlgebra::UnitAlgebraException(std::string("Invalid number string: ") + number)
{}

UnitAlgebra::NonMatchingUnits::NonMatchingUnits(
    const std::string& lhs, const std::string& rhs, const std::string& operation) :
    UnitAlgebraException(
        std::string("Attempting to ") + operation + " UnitAlgebra values with non-matching units: " + lhs + ", " + rhs)
{}
