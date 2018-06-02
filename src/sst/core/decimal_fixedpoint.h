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

#ifndef SST_CORE_DECIMAL_FIXEDPOINT_H
#define SST_CORE_DECIMAL_FIXEDPOINT_H

#include <iomanip>
#include <sstream>
#include <type_traits>

#include <sst/core/from_string.h>

namespace SST {
/**
   Class that implements a decimal fixed-point number.

   Fixed point class that stores digits in radix-10.  Size is
   specified in words, and each word represents 8 digits.
   
   @tparam whole_words Number of words used to represent the digits to
   the left of the decimal point.  Each word represents 8 decimal
   digits.

   @tparam fraction_words Number of words used to represent the digits
   to the right of the decimal point.  Each word represents 8 decimal
   digits.
*/
template <int whole_words, int fraction_words>
class decimal_fixedpoint {

public:
    static constexpr uint32_t storage_radix = 100000000; 
    static constexpr uint64_t storage_radix_long = 100000000l; 
    static constexpr int32_t  digits_per_word = 8;
    
    /**
       Get the value of whole_words template parameter.
     */
    constexpr int getWholeWords() const {
        return whole_words;
    }

    /**
       Get the value of fraction_words template parameter.
     */
    constexpr int getFractionWords() const {
        return fraction_words;
    }

private:

    // I should be a friend of all versions of this class
    template <int A, int B>
    friend class sst_dec_fixed;

    /**
       Data representing the digits of the number.
       
       Represents 8 decimal digits per 32-bits.  Each digit will not
       be represented independently, but rather all 8 digits will be
       represented as a number of parts out of 100,000,000.
       Essentially, we're storing as radix 100,000,000.  Index 0 holds
       the least significant digits
    */
    uint32_t data[whole_words + fraction_words];

    /**
       Represents if the number is negative or not.
     */
    bool negative;


    /**
       initialize a decimal_fixedpoint using a string initializer

       @param init Initialization string.  The format is similar to
       the c++ double precision strings.  For example 1.234, -1.234,
       0.234, 1.234e14, 1.234e14, etc.
     */
    void from_string(std::string init) {
        negative = false;
        for ( int i = 0; i < whole_words + fraction_words; ++i ) {
            data[i] = 0;
        }
        
        // Look for a negative sign
        if ( init[0] == '-' ) {
            negative = true;
            init = init.substr(1,init.npos);
        }

        // See if we have an exponent
        size_t exponent_pos = init.find_last_of("eE");
        int32_t exponent = 0;
        if ( exponent_pos != init.npos ) {
            exponent = static_cast<int32_t>(SST::Core::from_string<double>(init.substr(exponent_pos+1,init.npos)));
            init = init.substr(0,exponent_pos);
        }

        int dp = init.length();
        for ( size_t i = 0; i < init.length(); ++i ) {
            if ( init[i] == '.' ) {
                dp = i;
            }
        }

        // get rid of the decimal point
        init.erase(dp,1);
        
        //                     pos of decimal pt       digits after dec pt
        int start_of_digits = (fraction_words * digits_per_word) - (init.length() - dp) + exponent;
        // Convert digits to numbers

        // Compute the first mult
        int start_pos_word = start_of_digits % digits_per_word;
        uint32_t mult = 1;
        for ( int i = 0; i < start_pos_word; i++ ) {
            mult *= 10;
        }

        for ( int i = init.length() - 1; i >= 0; --i ) {
            int digit = start_of_digits + ( init.length() - 1 - i );
            int word = ( digit / digits_per_word );

            data[word] += (SST::Core::from_string<uint32_t>(init.substr(i,1)) * mult);
            mult *= 10;
            if (mult == storage_radix) mult = 1;
        }
        
    }

    /**
       Initialize a decimal_fixedpoint using a 64-bit unsigned number.

       @param init Initialization value.
     */
    void from_uint64(uint64_t init) {
        negative = false;

        for ( int i = 0; i < whole_words + fraction_words; ++i ) {
            data[i] = 0;
        }

        // A 64-bit integer can use at most two of the words
        // uint64_t factor = ( storage_radix_long * storage_radix_long );
        // for ( int i = 2; i > 0; --i ) {
        //     data[i+fraction_words] = init / factor;
        //     init -= data[i+fraction_words] * factor;
        //     factor /= storage_radix_long;
        // }
        // data[fraction_words] = init;

        for ( int i = fraction_words; i < whole_words + fraction_words; ++i ) {
            data[i] = init % storage_radix_long;
            init /= storage_radix_long;
        }
    }

    /**
       Initialize a decimal_fixedpoint using a double.

       @param init Initialization value.
     */
    void from_double(double init) {
        negative = false;

        for ( int i = 0; i < whole_words + fraction_words; ++i ) {
            data[i] = 0;
        }
        double factor = 1;
        
        for ( int i = 0; i < whole_words - 1; ++i ) {
            factor *= storage_radix;
        }
        
        for ( int i = whole_words + fraction_words - 1; i >= 0; --i ) {
            data[i] = static_cast<uint32_t>(init / factor);
            init -= (data[i] * factor);
            factor /= storage_radix;
        }
    }

public:

    /**
       Default constructor.

       Builds a decimal_fixedpoint with the value 0;
     */
    decimal_fixedpoint() {
        negative = false;
        for ( int i = 0; i < whole_words + fraction_words; ++i ) {
            data[i] = 0;
        }        
    }

    /**
       Build a decimal_fixedpoint using a string initializer.

       @param init Initialization string.  The format is similar to
       the c++ double precision strings.  For example 1.234, -1.234,
       0.234, 1.234e14, 1.234e14, etc.
     */
    decimal_fixedpoint(std::string init) {
        from_string(init);
    }


    /**
       Build a decimal_fixedpoint using a 64-bit unsigned number.

       @param init Initialization value.
     */
    template <class T>
    decimal_fixedpoint(T init, typename std::enable_if<std::is_unsigned<T>::value >::type* = 0) {
        from_uint64(init);
    }


    /**
       Build a decimal_fixedpoint using a 64-bit signed number.

       @param init Initialization value.
     */
    template <class T>
    decimal_fixedpoint(T init, typename std::enable_if<std::is_signed<T>::value &&
                       std::is_integral<T>::value >::type* = 0) {
        if ( init < 0 ) {
            from_uint64(-init);
            negative = true;
        }
        else {
            from_uint64(init);
        }
    }

    /**
       Build a decimal_fixedpoint using a double.

       @param init Initialization value.
     */
    template <class T>
    decimal_fixedpoint(const T init, typename std::enable_if<std::is_floating_point<T>::value >::type* = 0) {
        from_double(init);
    }

    /**
       Build a decimal_fixedpoint using another decimal_fixedpoint.

       @param init Initialization value.
     */
    decimal_fixedpoint(const decimal_fixedpoint& init) {
        negative = init.negative;
        for (int i = 0; i < whole_words + fraction_words; ++i ) {
            data[i] = init.data[i];
        }
    }

    /**
       Equal operator for other decimal_fixedpoint objects.
     */
    decimal_fixedpoint& operator=(const decimal_fixedpoint& v) {
        negative = v.negative;
        for (int i = 0; i < whole_words + fraction_words; ++i ) {
            data[i] = v.data[i];
        }
        return *this;
    }
    
    /**
       Equal operator for 64-bit unsigned int.
     */
    decimal_fixedpoint& operator=(uint64_t v) {
        from_uint64(v);
        return *this;
    }
    
    /**
       Equal operator for 64-bit signed int.
     */
    decimal_fixedpoint& operator=(int64_t v) {
        if ( v < 0 ) {
            from_uint64(-v);
            negative = true;
        }
        else {
            from_uint64(v);
        }
        return *this;
    }
    
    /**
       Equal operator for double.
     */
    decimal_fixedpoint& operator=(double v) {
        from_double(v);
        return *this;
    }
    
    /**
       Equal operator for string.
     */
    decimal_fixedpoint& operator=(std::string v) {
        from_string(v);
        return *this;
    }
    
    
    /**
       Negate the value (change the sign bit).
     */
    void negate() {
        negative = negative ^ 0x1;
    }

    /**
       Return a double precision version of the decimal_fixedpoint.
       There is possible precision loss in this conversion.
     */
    double toDouble() const {
        double ret = 0;
        double factor = 1;
        for ( int i = 0; i < fraction_words; ++i ) {
            factor /= storage_radix;
        }

        for ( int i = 0; i <whole_words + fraction_words; ++i ) {
            ret += ( static_cast<double>(data[i]) * factor );
            factor *= storage_radix;
        }

        return ret;
    }
    
    /**
       Return a int64_t version of the decimal_fixedpoint.  There is
       possible precision loss in this conversion.
     */
    int64_t toLong() const {
        int64_t ret = 0;
        int64_t factor = 1;
        for ( int i = 0; i < whole_words; ++i ) {
            ret += ( static_cast<int64_t>(data[fraction_words + i]) * factor );
            factor *= storage_radix;
        }

        // Check to see if we need to round
        bool round = false;
        if ( data[fraction_words - 1] > ( storage_radix / 2 ) ) {
            round = true;
        }
        else if ( data[fraction_words - 1] == ( storage_radix / 2 ) ) {
            for ( int i = fraction_words - 2; i >= 0; --i ) {
                if ( data[i] != 0 ) {
                    round = true;
                    break;
                }
            }

            if ( !round ) {
                // There were no extra digits, so we need to do a
                // round to nearest even.
                if ( ret % 2 == 1 ) round = true;
            }
        }
        if ( round ) ++ret;
        if ( negative ) ret = -ret;
        return ret;
    }

    /**
       Return a uint64_t version of the decimal_fixedpoint.  There is
       possible precision loss in this conversion.
     */
    uint64_t toUnsignedLong() const {
        uint64_t ret = 0;
        uint64_t factor = 1;
        for ( int i = 0; i < whole_words; ++i ) {
            ret += ( static_cast<int64_t>(data[i]) * factor );
            factor *= storage_radix;
        }
        return ret;
    }

    /**
       Templated conversion function for unsigned types.
     */
    template<typename T>
    T convert_to(typename std::enable_if<std::is_unsigned<T>::value>::type* = 0) const {
        return static_cast<T>(toUnsignedLong());
    }
    
    /**
       Templated conversion function for signed integral types.
     */
    template<typename T>
    T convert_to(typename std::enable_if<std::is_signed<T>::value &&
                 std::is_integral<T>::value >::type* = 0)  const {
        return static_cast<T>(toLong());
    }
    
    /**
       Templated conversion function for floating point types.
     */
    template<typename T>
    T convert_to(typename std::enable_if<std::is_floating_point<T>::value >::type* = 0) const {
        return static_cast<T>(toDouble());
    }
    
    /**
       Create a string representation of this decimal_fixedpoint.

       @param precision Precision to use when printing number

    */
    std::string toString(int32_t precision = 6) const {

        std::stringstream stream;
        if ( precision <= 0 || precision > ((whole_words + fraction_words) * digits_per_word ) )
            precision = (whole_words + fraction_words) * digits_per_word;

        // First create a digit by digit representation so we can
        // reason about what to print based on the precision.
        constexpr int num_digits = (whole_words + fraction_words) * digits_per_word;
        
        unsigned char digits[num_digits];
        for ( int i = 0; i < whole_words + fraction_words; ++i ) {
            uint32_t value = data[i];
            for ( int j = 0; j < digits_per_word; ++j ) {
                digits[i*digits_per_word+j] = value % 10;
                value /= 10;
            }
        }

        // Find the first non-zero
        int first_non_zero = -1;
        for ( int i = num_digits - 1; i >= 0; --i ) {
            if ( digits[i] != 0 ) {
                first_non_zero = i;
                break;
            }
        }

        // If no non-zeros, return 0
        if ( first_non_zero == -1 ) return "0";

        // Now, we will round to the precision asked for
        int round_position = first_non_zero - precision;
        bool round = false;

        // If round_position < 0 then we have already gotten all the
        // digits that exist, so no need to round
        if ( round_position >= 0 ) {
            if ( digits[round_position] > 5 ) round = true;
            else if ( digits[round_position] < 5 ) round = false;
            else { // Round if the rest aren't zeros
            
                for ( int i = round_position - 1; i >= 0; --i ) {
                    if ( digits[i] != 0 ) {
                        round = true;
                        break;
                    }
                }
                if ( !round ) {
                    // There were no extra zeros, so we need to do round
                    // to nearest even.
                    if ( digits[round_position+1] % 2 == 1 ) round = true;
                }
            }

            if ( round ) {
                // Do the round
                unsigned char carry = 1;
                for ( int i = round_position + 1; i < num_digits; ++i ) {
                    digits[i] += carry;
                    carry = digits[i] / 10;
                    digits[i] = digits[i] % 10;
                }
            }

            // Zero out all the bits beyond the precision
            for ( int i = 0; i <= round_position; ++i ) {
                digits[i] = 0;
            }
        }

        // print possible negative sign
        if ( negative ) stream << '-';
        

        // Find the first non-zero
        first_non_zero = -1;
        for ( int i = num_digits - 1; i >= 0; --i ) {
            if ( digits[i] != 0 ) {
                first_non_zero = i;
                break;
            }
        }

        // Check for overflow in the round
        if ( first_non_zero == -1 ) {
            // This means we rounded to a number bigger than we can
            // support
            stream << "1e+" << (whole_words*digits_per_word);
            return stream.str();
        }
        
        // There are several cases to cover:

        // Need to switch to exponent notation for numbers > 1
        if ( first_non_zero >= ((fraction_words*digits_per_word) + precision) ) {
            // Need to use exponent notation
            int exponent = first_non_zero - (fraction_words * digits_per_word );
            stream << static_cast<uint32_t>(digits[first_non_zero]) << ".";
            int zeros = 0;
            for ( int i = first_non_zero - 1; i >= first_non_zero - precision; --i ) {
                // Avoid printing trailing zeros.  Keep track of runs
                // of zeros and only print them if we hit another
                // non-xero
                if ( digits[i] == 0 ) zeros++;
                else {
                    for ( int j = 0; j < zeros; ++j ) {
                        stream << "0";
                    }
                    stream << static_cast<uint32_t>(digits[i]);
                    zeros = 0;
                }
            }
            std::string ret = stream.str();
            if ( ret[ret.length()-1] == '.' ) {
                ret = ret.substr(0,ret.length()-1);
                stream.str(std::string(""));
                stream << ret;
            }
            stream << "e+" << std::setfill('0') << std::setw(2) << exponent;
            // return stream.str();
        }
        
        // Decimal point is within the string of digits to print
        else if ( first_non_zero >= (fraction_words * digits_per_word)) {
            int zeros = 0;
            for ( int i = first_non_zero; i >= (fraction_words * digits_per_word); --i ) {
                // Digits before the decimal point
                stream << static_cast<uint32_t>(digits[i]);
            }
            
            stream << ".";
                
            for ( int i = (fraction_words * digits_per_word) - 1;
                  i >= first_non_zero - precision && (i >= 0);
                  --i ) {
                // Avoid printing trailing zeros.  Keep track of runs
                // of zeros and only print them if we hit another
                // non-xero
                if ( digits[i] == 0 ) zeros++;
                else {
                    for ( int j = 0; j < zeros; ++j ) {
                        stream << "0";
                    }
                    stream << static_cast<uint32_t>(digits[i]);
                    zeros = 0;
                }
            }
            std::string ret = stream.str();
            if ( ret[ret.length()-1] == '.' ) {
                ret = ret.substr(0,ret.length()-1);
                stream.str(std::string(""));
                stream << ret;
            }
        }

        // No whole digits, but not switching to exponent notation
        // yet.  We are willing to print three leading zeros before
        // switching to exponent notation
        else if ( first_non_zero > (fraction_words * digits_per_word) - 5 ) {
            stream << "0.";
            for ( int i = (fraction_words * digits_per_word) - 1; i > first_non_zero; --i ) {
                stream << "0";
            }
            int zeros = 0;
            for ( int i = first_non_zero;
                  (i >= first_non_zero - precision) && (i >= 0);
                  --i ) {
                // Avoid printing trailing zeros.  Keep track of runs
                // of zeros and only print them if we hit another
                // non-xero
                if ( digits[i] == 0 ) zeros++;
                else {
                    for ( int j = 0; j < zeros; ++j ) {
                        stream << "0";
                    }
                    stream << static_cast<uint32_t>(digits[i]);
                    zeros = 0;
                }
            }
        }
        // Switch to exponent notation
        else {
            // Need to use exponent notation
            int exponent = first_non_zero - (fraction_words * digits_per_word );
            exponent = -exponent;
            stream << static_cast<uint32_t>(digits[first_non_zero]) << ".";
            int zeros = 0;
            for ( int i = first_non_zero - 1;
                  (i >= first_non_zero - precision) && (i >= 0);
                  --i ) {
                // Avoid printing trailing zeros.  Keep track of runs
                // of zeros and only print them if we hit another
                // non-xero
                if ( digits[i] == 0 ) zeros++;
                else {
                    for ( int j = 0; j < zeros; ++j ) {
                        stream << "0";
                    }
                    stream << static_cast<uint32_t>(digits[i]);
                    zeros = 0;
                }
            }
            std::string ret = stream.str();
            if ( ret[ret.length()-1] == '.' ) {
                ret = ret.substr(0,ret.length()-1);
                stream.str(std::string(""));
                stream << ret;
            }
            stream << "e-" << std::setfill('0') << std::setw(2) << exponent;
        }
        
        return stream.str();
    }
    
    /**
       Adds another number to this one and sets it equal to the
       result.

       @param v Number to add to this one
    */
    decimal_fixedpoint& operator+= (const decimal_fixedpoint& v) {
        // Depending on the signs, this may be a subtract
        if ( (negative ^ v.negative) == 0 ) {
            // Signs match, just do an add
            // Calculate the result for each digit.
            uint64_t carry_over = 0;
            for ( int i = 0; i < whole_words + fraction_words; i++ ) {
                uint64_t value = static_cast<uint64_t>(data[i])
                    + static_cast<uint64_t>(v.data[i])
                    + carry_over;
                carry_over = value / storage_radix;
                // data[i] = static_cast<uint32_t>(value % storage_radix);
                data[i] = value % storage_radix;
            }
            return *this;
        }

        // Signs don't match, so we need to sort
        if ( operator>=(v) ) {
            // Calculate the result for each digit.  Need to negate the
            // second operand and pass a 1 in as carry-in (carry-over).
            uint64_t carry_over = 1;
            for ( int i = 0; i < whole_words + fraction_words; i++ ) {
                uint64_t negate = static_cast<uint64_t>(storage_radix - 1) - static_cast<uint64_t>(v.data[i]);
                
                uint64_t value = static_cast<uint64_t>(data[i])
                    + negate
                    + carry_over;
                carry_over = value / storage_radix;
                data[i] = static_cast<uint32_t>(value % storage_radix);
            }
            return *this;
        }
        else {
            // Calculate the result for each digit.  Need to negate the
            // second operand and pass a 1 in as carry-in (carry-over).
            uint64_t carry_over = 1;
            for ( int i = 0; i < whole_words + fraction_words; i++ ) {
                uint64_t negate = static_cast<uint64_t>(storage_radix - 1) - static_cast<uint64_t>(data[i]);
                
                uint64_t value = static_cast<uint64_t>(v.data[i])
                    + negate
                    + carry_over;
                carry_over = value / storage_radix;
                data[i] = static_cast<uint32_t>(value % storage_radix);
            }
            // Since the other one is bigger, I take his sign
            negative = v.negative;
            return *this;
        }
    }


    /**
       Subtracts another number from this one and sets it equal to the
       result.

       @param v Number to subtract from this one
     */
    decimal_fixedpoint& operator-= (const decimal_fixedpoint& v) {
        decimal_fixedpoint ret(v);
        ret.negate();
        return operator+=(ret);
    }


    /**
       Multiplies another number to this one and sets it equal to the
       result.

       @param v Number to multiply to this one
     */
    decimal_fixedpoint& operator*= (const decimal_fixedpoint& v) {
        // Need to do the multiply accumulate for each digit.
        decimal_fixedpoint<whole_words,fraction_words> me(*this);
        
        // The first "fraction_words" digits only matter as far as
        // their carries go.  They get dropped in the final output
        // because they are less than the least significant digit.
        uint64_t carry_over = 0;
        for ( int i = 0; i < fraction_words; ++i ) {
            uint64_t sum = carry_over;
            for ( int j = 0; j <= i; ++j ) {
                sum += static_cast<uint64_t>(me.data[j]) * static_cast<uint64_t>(v.data[i-j]);
            }
            carry_over = sum / storage_radix_long;
        }

        // Calculate the digits that we'll keep
        for ( int i = fraction_words; i < whole_words + fraction_words; ++i ) {
            uint64_t sum = carry_over;
            for ( int j = 0; j <= i; ++j ) {
                sum += static_cast<uint64_t>(me.data[j]) * static_cast<uint64_t>(v.data[i-j]);
            }
            carry_over = sum / storage_radix_long;
            data[i-fraction_words] = static_cast<uint32_t>(sum % storage_radix_long);
        }
        
        for ( int i = 0; i < fraction_words; ++i ) {
            uint64_t sum = carry_over;
            for ( int j = i+1; j < whole_words + fraction_words; ++j ) {
                sum += static_cast<uint64_t>(me.data[j])
                    * static_cast<uint64_t>(v.data[whole_words+fraction_words+i-j]);
            }
            carry_over = sum / storage_radix_long;
            data[i+whole_words] = static_cast<uint32_t>(sum % storage_radix_long);
        }
        return *this;
    }

    /**
       Divides another number from this one and sets it equal to the
       result.

       @param v Number to divide from this one
     */
    decimal_fixedpoint& operator/= (const decimal_fixedpoint& v) {
        decimal_fixedpoint inv(v);
        inv.inverse();
        operator*=(inv);
        return *this;
    }

    /**
       Inverts the number (1 divided by this number)
     */

    decimal_fixedpoint& inverse() {
        // We will use the Newton-Raphson method to compute the
        // inverse

        // First, get an estimate of the inverse by converting to a
        // double and inverting
        decimal_fixedpoint me(*this);
        decimal_fixedpoint<whole_words,fraction_words> inv(1.0 / toDouble());
        // decimal_fixedpoint<whole_words,fraction_words> inv("400");
        
        decimal_fixedpoint<whole_words,fraction_words> two(2.0);

        // Since we converted to double to get an estimate of the
        // inverse, we have approximated a double's worth of precision
        // in our answer.  We'll divide that by 2 for now, just to be
        // save.
        int digits_of_prec = std::numeric_limits<double>::digits10 / 2;
        
        // Number of digits of precision doubles with each iteration
        for ( int i = digits_of_prec; i <= (whole_words + fraction_words) * digits_per_word; i *= 2 ) {
            decimal_fixedpoint temp(inv);
            temp *= me;
            temp -= two;
            temp.negate();
            inv *= temp;
        }

        *this = inv;
        return *this;
    }

    /**
       Checks to see if two numbers are equal

       @param v Number to check equality against
     */
    bool operator== (const decimal_fixedpoint& v) const {
        for ( int i = whole_words + fraction_words - 1; i >= 0; --i ) {
            if ( data[i] != v.data[i] ) return false;
        }
        return true;
    }

    /**
       Checks to see if two numbers are not equal

       @param v Number to check equality against
     */
    bool operator!= (const decimal_fixedpoint& v) const {
        for ( int i = whole_words + fraction_words - 1; i >= 0; --i ) {
            if ( data[i] != v.data[i] ) return true;
        }
        return false;
    }

    /**
       Checks to see if this number is greater than another number

       @param v Number to compare to
     */
    bool operator> (const decimal_fixedpoint& v) const {
        for ( int i = whole_words +fraction_words - 1; i >= 0; --i ) {
            if ( data[i] > v.data[i] ) return true;
            if ( data[i] < v.data[i] ) return false;
        }
        return false;
    }

    /**
       Checks to see if this number is greater than or equal to
       another number

       @param v Number to compare to
     */
    bool operator>= (const decimal_fixedpoint& v) const {
        for ( int i = whole_words + fraction_words - 1; i >= 0; --i ) {
            if ( data[i] > v.data[i] ) return true;
            if ( data[i] < v.data[i] ) return false;
        }
        return true;
    }

    /**
       Checks to see if this number is less than another number

       @param v Number to compare to
     */
    bool operator< (const decimal_fixedpoint& v) const {
        for ( int i = whole_words + fraction_words - 1; i >= 0; --i ) {
            if ( data[i] < v.data[i] ) return true;
            if ( data[i] > v.data[i] ) return false;
        }
        return false;
    }

    /**
       Checks to see if this number is less than or equal to another
       number

       @param v Number to compare to
     */
    bool operator<= (const decimal_fixedpoint& v) const {
        for ( int i = whole_words + fraction_words - 1; i >= 0; --i ) {
            if ( data[i] < v.data[i] ) return true;
            if ( data[i] > v.data[i] ) return false;
        }
        return true;
    }

};

template <int whole_words, int fraction_words>
decimal_fixedpoint<whole_words,fraction_words> operator+(decimal_fixedpoint<whole_words,fraction_words> lhs,
                                                    decimal_fixedpoint<whole_words,fraction_words> rhs) {
    decimal_fixedpoint<whole_words,fraction_words> ret(lhs);
    return ret += rhs;
}

template <int whole_words, int fraction_words>
decimal_fixedpoint<whole_words,fraction_words> operator-(decimal_fixedpoint<whole_words,fraction_words> lhs,
                                                    decimal_fixedpoint<whole_words,fraction_words> rhs) {
    decimal_fixedpoint<whole_words,fraction_words> ret(lhs);
    return ret -= rhs;
}

template <int whole_words, int fraction_words>
decimal_fixedpoint<whole_words,fraction_words> operator*(decimal_fixedpoint<whole_words,fraction_words> lhs,
                                                    decimal_fixedpoint<whole_words,fraction_words> rhs) {
    decimal_fixedpoint<whole_words,fraction_words> ret(lhs);
    return ret *= rhs;
}

template <int whole_words, int fraction_words>
decimal_fixedpoint<whole_words,fraction_words> operator/(decimal_fixedpoint<whole_words,fraction_words> lhs,
                                                    decimal_fixedpoint<whole_words,fraction_words> rhs) {
    decimal_fixedpoint<whole_words,fraction_words> ret(lhs);
    return ret /= rhs;
}

template <int whole_words, int fraction_words, typename T>
bool operator==(const T& lhs, const decimal_fixedpoint<whole_words,fraction_words>& rhs) {
    return rhs == decimal_fixedpoint<whole_words,fraction_words>(lhs);
}

template <int whole_words, int fraction_words, typename T>
bool operator!=(const T& lhs, const decimal_fixedpoint<whole_words,fraction_words>& rhs) {
    return rhs != decimal_fixedpoint<whole_words,fraction_words>(lhs);
}

template <int whole_words, int fraction_words>
std::ostream& operator <<(std::ostream& os, const decimal_fixedpoint<whole_words,fraction_words>& rhs) {
    os << rhs.toString(os.precision());
    return os;
}

} // namespace SST

#endif
