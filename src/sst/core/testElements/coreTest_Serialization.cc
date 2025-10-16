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

// #include <assert.h>

#include "sst_config.h"

#include "sst/core/testElements/coreTest_Serialization.h"

#include "sst/core/componentInfo.h"
#include "sst/core/link.h"
#include "sst/core/objectSerialization.h"
#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/rng.h"
#include "sst/core/serialization/impl/serialize_utility.h"
#include "sst/core/warnmacros.h"

#include <array>
#include <bitset>
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <ostream>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace SST::CoreTestSerialization {

using SST::Core::Serialization::get_size;

template <typename T>
void
serializeDeserialize(T&& input, T&& output, bool with_tracking = false)
{
    // Set up serializer and buffers
    char*                                buffer;
    SST::Core::Serialization::serializer ser;
    ser_opt_t                            options = 0;
    if ( with_tracking ) {
        ser.enable_pointer_tracking();
        if ( !std::is_pointer_v<std::remove_reference_t<T>> ) options = SerOption::as_ptr;
    }

    ser.start_sizing();
    SST_SER(input, options);

    size_t size = ser.size();

    buffer = new char[size];

    ser.start_packing(buffer, size);
    SST_SER(input, options);

    ser.start_unpacking(buffer, size);
    SST_SER(output, options);

    delete[] buffer;
}

template <typename... Ts>
void
serializeDeserialize(std::tuple<Ts...>& input, std::tuple<Ts...>& output, bool with_tracking = false)
{
    // Set up serializer and buffers
    char*                                buffer;
    SST::Core::Serialization::serializer ser;
    ser_opt_t                            options = 0;
    if ( with_tracking ) {
        ser.enable_pointer_tracking();
    }

    ser.start_sizing();
    std::apply([&](auto&&... x) { (SST_SER(x, options), ...); }, input);

    size_t size = ser.size();

    buffer = new char[size];

    ser.start_packing(buffer, size);
    std::apply([&](auto&&... x) { (SST_SER(x, options), ...); }, input);

    ser.start_unpacking(buffer, size);
    std::apply([&](auto&&... x) { (SST_SER(x, options), ...); }, output);

    ser.finalize();
    delete[] buffer;
}


template <typename T>
struct checkSimpleSerializeDeserialize
{
    using TYPE = std::remove_pointer_t<T>;

    static bool check(TYPE data)
    {
        T    input;
        T    output;
        TYPE obj [[maybe_unused]]
        ;

        if constexpr ( std::is_pointer_v<T> ) {
            obj    = data;
            input  = &obj;
            output = nullptr;
        }
        else {
            input  = data;
            output = T();
        }

        serializeDeserialize(input, output);
        if constexpr ( std::is_pointer_v<T> ) {
            return data == *output;
        }
        else {
            return data == output;
        }
    }

    // This only works for pointer types.  Non-pointer types will
    // return false
    static bool check_nullptr()
    {
        if constexpr ( std::is_pointer_v<T> ) {
            // Need a fake variable to set output to so we can make
            // sure the nullptr gets set correctly
            TYPE fake = TYPE();

            // Input is nullptr
            T input = nullptr;

            // Set output to the pointer of the fake variable
            T output = &fake;

            serializeDeserialize(input, output);

            return nullptr == output;
        }
        else {
            return false;
        }
    }

    // This will track pointer tracking for pointers and as_ptr for
    // non-pointers
    static bool check_pointer_tracking(TYPE data)
    {
        TYPE                obj = data;
        std::pair<T, TYPE*> input;
        std::pair<T, TYPE*> output;

        if constexpr ( std::is_pointer_v<T> ) {
            input.first  = &obj;
            input.second = &obj;

            output.first  = nullptr;
            output.second = nullptr;
        }
        else {
            input.first  = obj;
            input.second = &(input.first);

            output.first  = T();
            output.second = nullptr;
        }

        serializeDeserialize(input, output, true);

        if constexpr ( std::is_pointer_v<T> ) {
            if ( nullptr == output.first || nullptr == output.second ) return false;
            if ( data != *(output.first) ) return false;
            return output.first == output.second;
        }
        else {
            if ( nullptr == output.second ) return false;
            if ( data != output.first ) return false;
            return output.second == &(output.first);
        }
    }

    static void check_all(TYPE data, Output& out, const std::string& type_name)
    {
        bool passed;
        passed = check(data);
        if ( !passed ) out.output("ERROR: %s did not serialize/deserialize properly\n", type_name.c_str());

        passed = check_pointer_tracking(data);
        if ( !passed )
            out.output("ERROR: %s did not serialize/deserialize properly with pointer tracking\n", type_name.c_str());

        if ( std::is_pointer_v<T> ) {
            passed = check_nullptr();
            if ( !passed ) out.output("ERROR: %s nullptr did not serialize/deserialize properly\n", type_name.c_str());
        }
    }
};

template <typename T, typename DELETER, typename DEL = DELETER>
bool
checkUniquePtrSerializeDeserialize(std::unique_ptr<T, DELETER>& in, DEL&& del = DELETER {})
{
    std::unique_ptr<T, DELETER> out(nullptr, std::forward<decltype(del)>(del));
    serializeDeserialize(SST::Core::Serialization::unique_ptr(in, std::forward<decltype(del)>(del)),
        SST::Core::Serialization::unique_ptr(out, std::forward<decltype(del)>(del)));
    if ( !in )
        return !out;
    else if ( !out )
        return false;
    else if constexpr ( std::is_array_v<T> ) {
        for ( size_t i = 0; i < std::extent_v<T>; ++i )
            if ( !((*in.get())[i] == (*out.get())[i]) ) return false;
        return true;
    }
    else {
        return *in == *out;
    }
}

template <typename E, typename DELETER, typename DEL = DELETER>
bool
checkUniquePtrSerializeDeserialize(std::unique_ptr<E[], DELETER>& in, size_t& in_size, DEL&& del = DELETER {})
{
    std::unique_ptr<E[], DELETER> out(nullptr, std::forward<decltype(del)>(del));
    size_t                        out_size;
    serializeDeserialize(SST::Core::Serialization::unique_ptr(in, in_size, std::forward<decltype(del)>(del)),
        SST::Core::Serialization::unique_ptr(out, out_size, std::forward<decltype(del)>(del)));
    if ( !in )
        return !out;
    else if ( !out )
        return false;
    else if ( in_size != out_size )
        return false;
    else {
        for ( size_t i = 0; i < out_size; ++i )
            if ( !(in[i] == out[i]) ) return false;
        return true;
    }
}

template <typename T>
bool
checkOptionalSerializeDeserialize(std::optional<T>& data)
{
    std::optional<T> result;
    serializeDeserialize(data, result);
    if ( data.has_value() != result.has_value() ) return false;
    if ( !data.has_value() ) return true;
    return *data == *result;
}

template <typename T>
bool
checkContainerSerializeDeserialize(T& data)
{
    T result;
    serializeDeserialize(data, result);

    if ( get_size(data) != get_size(result) ) return false;

    auto data_it   = data.begin();
    auto result_it = result.begin();

    // Only need to check one iterator since we already checked to
    // make sure they are equal size
    while ( data_it != data.end() ) {
        if ( *data_it != *result_it ) return false;
        ++data_it;
        ++result_it;
    }
    return true;
};

template <typename T>
bool
checkContainerSerializeDeserialize(T*& data)
{
    T* result;
    serializeDeserialize(data, result);

    if ( get_size(*data) != get_size(*result) ) return false;

    auto data_it   = data->begin();
    auto result_it = result->begin();

    // Only need to check one iterator since we already checked to
    // make sure they are equal size
    while ( data_it != data->end() ) {
        if ( *data_it != *result_it ) return false;
        ++data_it;
        ++result_it;
    }
    return true;
};

// std::variant
auto checkVariant = [](auto& data, auto& result) {
    using T = std::decay_t<decltype(data)>;
    using R = std::decay_t<decltype(result)>;

    if constexpr ( !std::is_same_v<T, R> ) {
        // ignore the cases where T != R at compile-time, since they are excluded by index() runtime equality test
        return false;
    }
    else if constexpr ( std::is_same_v<T, std::string> || std::is_arithmetic_v<T> || std::is_enum_v<T> ) {
        return data == result;
    }
    else if constexpr ( std::is_same_v<T, std::vector<int>> ) {
        if ( data.size() != result.size() ) return false;
        for ( size_t i = 0; i < data.size(); ++i )
            if ( data[i] != result[i] ) return false;
        return true;
    }
    else {
        static_assert(sizeof(T) == 0, "Unsupported type in checkVariant()");
    }
};

template <typename... Types>
bool
checkVariantSerializeDeserialize(std::variant<Types...>& data)
{
    std::variant<Types...> result;
    serializeDeserialize(data, result);
    if ( result.index() != data.index() ) return false;
    return std::visit(checkVariant, data, result);
};

// Arrays

template <typename>
constexpr size_t array_size = 0;

template <typename T, size_t S>
constexpr size_t array_size<T[S]> = S;

template <typename T, size_t S>
constexpr size_t array_size<std::array<T, S>> = S;

template <typename T>
bool
checkFixedArraySerializeDeserialize(T& data)
{
    T result;
    serializeDeserialize(data, result);

    for ( size_t i = 0; i < array_size<std::remove_pointer_t<T>>; ++i ) {
        if constexpr ( std::is_pointer_v<T> ) {
            if ( (*data)[i] != (*result)[i] ) return false;
        }
        else {
            if ( data[i] != result[i] ) return false;
        }
    }
    if constexpr ( std::is_pointer_v<T> ) delete[] result;
    return true;
};

template <typename T>
bool
checkArraySerializeDeserialize(T* data, size_t dataSize)
{
    using SST::Core::Serialization::array;

    T*     result     = nullptr;
    size_t resultSize = ~size_t {};

    serializeDeserialize(array(data, dataSize), array(result, resultSize));

    if ( resultSize != dataSize ) return false;

    for ( size_t i = 0; i < dataSize; ++i ) {
        if ( data[i] != result[i] ) return false;
    }

    delete[] result;

    return true;
};

template <typename T>
bool
checkValArraySerializeDeserialize(std::valarray<T>& data)
{
    std::valarray<T> result;

    serializeDeserialize(data, result);

    if ( result.size() != data.size() ) return false;

    for ( size_t i = 0; i < data.size(); ++i ) {
        if ( data[i] != result[i] ) return false;
    }

    return true;
};

// For ordered but non-iterable contaienrs
template <typename T>
bool
checkNonIterableContainerSerializeDeserialize(T& data)
{
    T result;
    serializeDeserialize(data, result);

    if ( get_size(data) != get_size(result) ) return false;

    while ( !data.empty() ) {
        auto data_val   = data.top();
        auto result_val = result.top();
        if ( data_val != result_val ) return false;
        data.pop();
        result.pop();
    }
    return true;
};

// For pointers to ordered but non-iterable contaienrs
template <typename T>
bool
checkNonIterableContainerSerializeDeserialize(T*& data)
{
    T* result;
    serializeDeserialize(data, result);

    if ( get_size(*data) != get_size(*result) ) return false;

    while ( !data->empty() ) {
        auto data_val   = data->top();
        auto result_val = result->top();
        if ( data_val != result_val ) return false;
        data->pop();
        result->pop();
    }
    return true;
};

// For std::queue
template <typename T>
bool
checkNonIterableContainerSerializeDeserialize(std::queue<T>& data)
{
    std::queue<T> result;
    serializeDeserialize(data, result);

    if ( get_size(data) != get_size(result) ) return false;

    while ( !data.empty() ) {
        auto data_val   = data.front();
        auto result_val = result.front();
        if ( data_val != result_val ) return false;
        data.pop();
        result.pop();
    }
    return true;
};

// For pointer to std::queue
template <typename T>
bool
checkNonIterableContainerSerializeDeserialize(std::queue<T>*& data)
{
    std::queue<T>* result;
    serializeDeserialize(data, result);

    if ( get_size(*data) != get_size(*result) ) return false;

    while ( !data->empty() ) {
        auto data_val   = data->front();
        auto result_val = result->front();
        if ( data_val != result_val ) return false;
        data->pop();
        result->pop();
    }
    return true;
};

// For unordered containers
template <typename T>
bool
checkUContainerSerializeDeserialize(T& data)
{
    auto buffer = SST::Comms::serialize(data);
    T    result;
    SST::Comms::deserialize(buffer, result);

    if ( get_size(data) != get_size(result) ) return false;


    // Only need to check one iterator since we already checked to
    // make sure they are equal size
    for ( auto data_it = data.begin(); data_it != data.end(); ++data_it ) {
        bool match_found = false;
        for ( auto result_it = result.begin(); result_it != result.end(); ++result_it ) {
            if ( *data_it == *result_it ) {
                match_found = true;
                break;
            }
        }
        if ( !match_found ) return false;
    }
    return true;
};

// For pointers to unordered containers
template <typename T>
bool
checkUContainerSerializeDeserialize(T*& data)
{
    auto buffer = SST::Comms::serialize(data);
    T*   result;
    SST::Comms::deserialize(buffer, result);

    if ( get_size(*data) != get_size(*result) ) return false;


    // Only need to check one iterator since we already checked to
    // make sure they are equal size
    for ( auto data_it = data->begin(); data_it != data->end(); ++data_it ) {
        bool match_found = false;
        for ( auto result_it = result->begin(); result_it != result->end(); ++result_it ) {
            if ( *data_it == *result_it ) {
                match_found = true;
                break;
            }
        }
        if ( !match_found ) return false;
    }
    return true;
};

// Classes to test pointer tracking
class pointed_to_class : public SST::Core::Serialization::serializable
{
    int value = -1;

public:
    explicit pointed_to_class(int val) :
        value(val)
    {}
    pointed_to_class() {}

    int  getValue() { return value; }
    void setValue(int val) { value = val; }

    void serialize_order(SST::Core::Serialization::serializer& ser) override { SST_SER(value); }

    ImplementSerializable(SST::CoreTestSerialization::pointed_to_class);
};

class shell : public SST::Core::Serialization::serializable
{
    int               value      = -10;
    pointed_to_class* pointed_to = nullptr;

public:
    shell(int val, pointed_to_class* ptc = nullptr) :
        value(val),
        pointed_to(ptc)
    {}
    shell() {}

    int  getValue() { return value; }
    void setValue(int val) { value = val; }

    pointed_to_class* getPointedTo() { return pointed_to; }
    void              setPointedTo(pointed_to_class* p) { pointed_to = p; }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(value);
        SST_SER(pointed_to);
    }

    ImplementSerializable(SST::CoreTestSerialization::shell);
};


// Class used to test serialization of handlers
struct HandlerTest : public SST::Core::Serialization::serializable
{
    // Need 8 combinations to cover void and non-void for return type,
    // arg type and metadata type

    void call_000() { std::cout << "internal value: " << value << std::endl; }

    void call_001(float f)
    {
        std::cout << "internal value: " << value << std::endl;
        std::cout << "metadata value: " << f << std::endl;
    }

    void call_010(int in)
    {
        std::cout << "internal value: " << value << std::endl;
        std::cout << "parameter value: " << in << std::endl;
    }

    void call_011(int in, float f)
    {
        std::cout << "internal value: " << value << std::endl;
        std::cout << "parameter value: " << in << std::endl;
        std::cout << "metadata value: " << f << std::endl;
    }

    int call_100()
    {
        std::cout << "internal value: " << value << std::endl;
        return 4;
    }

    int call_101(float f)
    {
        std::cout << "internal value: " << value << std::endl;
        std::cout << "metadata value: " << f << std::endl;
        return 5;
    }

    int call_110(int in)
    {
        std::cout << "internal value: " << value << std::endl;
        std::cout << "parameter value: " << in << std::endl;
        return 6;
    }

    int call_111(int in, float f)
    {
        std::cout << "internal value: " << value << std::endl;
        std::cout << "parameter value: " << in << std::endl;
        std::cout << "metadata value: " << f << std::endl;
        return 7;
    }

    int value = -1;

    explicit HandlerTest(int in) :
        value(in)
    {}
    HandlerTest() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override { SST_SER(value); }
    ImplementSerializable(HandlerTest)
};


struct RecursiveSerializationTest : public SST::Core::Serialization::serializable
{
    template <typename classT, auto funcT, typename dataT = void>
    using Handler = SSTHandler2<int, int, classT, dataT, funcT>;

    int call(int input, float f)
    {
        std::cout << "internal value: " << value << std::endl;
        std::cout << "parameter value: " << input << std::endl;
        std::cout << "metadata value: " << f << std::endl;
        return 101;
    }

    Handler<RecursiveSerializationTest, &RecursiveSerializationTest::call, float>* handler;
    int                                                                            value;

    RecursiveSerializationTest() {}
    explicit RecursiveSerializationTest(int in) :
        value(in)
    {
        handler = new Handler<RecursiveSerializationTest, &RecursiveSerializationTest::call, float>(this, 8.9);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(value);
        SST_SER(handler);
    }

    ImplementSerializable(RecursiveSerializationTest)
};

coreTestSerialization::coreTestSerialization(ComponentId_t id, Params& params) :
    Component(id)
{
    // Test serialization for various data types

    Output& out = getSimulationOutput();

    rng = std::make_unique<SST::RNG::MersenneRNG>();

    auto shuffle = [&](auto& v) {
        using std::swap;
        for ( size_t i = 1; i < v.size(); ++i )
            swap(v[rng->generateNextUInt32() % (i + 1)], v[i]);
    };

    std::string test = params.find<std::string>("test");
    if ( test == "" ) out.fatal(CALL_INFO_LONG, 1, "ERROR: Must specify test type\n");

    // Declare here to avoid having to declare it for each test type
    bool passed;

    if ( test == "pod" ) {
        // Test the POD (plain old data) types

        // Simple Data Types
        // int8, int16, int32, int64, uint8, uint16, uint32, uint64, float, double, pair<int, int>, string

        checkSimpleSerializeDeserialize<int8_t>::check_all(rng->generateNextInt32(), out, "int8_t");
        checkSimpleSerializeDeserialize<int16_t>::check_all(rng->generateNextInt32(), out, "int16_t");
        checkSimpleSerializeDeserialize<int32_t>::check_all(rng->generateNextInt32(), out, "int32_t");
        checkSimpleSerializeDeserialize<int64_t>::check_all(rng->generateNextInt64(), out, "int64_t");

        checkSimpleSerializeDeserialize<uint8_t>::check_all(rng->generateNextUInt32(), out, "uint8_t");
        checkSimpleSerializeDeserialize<uint16_t>::check_all(rng->generateNextUInt32(), out, "uint16_t");
        checkSimpleSerializeDeserialize<uint32_t>::check_all(rng->generateNextUInt32(), out, "uint32_t");
        checkSimpleSerializeDeserialize<uint64_t>::check_all(rng->generateNextUInt64(), out, "uint64_t");

        checkSimpleSerializeDeserialize<float>::check_all(rng->nextUniform() * 1000, out, "float");
        checkSimpleSerializeDeserialize<double>::check_all(rng->nextUniform() * 1000000, out, "double");
        checkSimpleSerializeDeserialize<std::string>::check_all("test_string", out, "std::string");

        checkSimpleSerializeDeserialize<std::bitset<1>>::check_all(rng->generateNextUInt64(), out, "std::bitset<1>");
        checkSimpleSerializeDeserialize<std::bitset<10>>::check_all(rng->generateNextUInt64(), out, "std::bitset<10>");
        checkSimpleSerializeDeserialize<std::bitset<100>>::check_all(
            rng->generateNextUInt64(), out, "std::bitset<100>");

        passed = checkSimpleSerializeDeserialize<std::tuple<int32_t, int32_t, int32_t>>::check(
            std::tuple<int32_t, int32_t, int32_t>(
                rng->generateNextInt32(), rng->generateNextInt32(), rng->generateNextInt32()));
        if ( !passed ) out.output("ERROR: tuple<int32_t,int32_t,int32_t> did not serialize/deserialize properly\n");
    }
    else if ( test == "pod_ptr" ) {
        // Test pointers to POD (plain old data) types

        // Simple Data Types
        // int8, int16, int32, int64, uint8, uint16, uint32, uint64, float, double, string

        checkSimpleSerializeDeserialize<int8_t*>::check_all(rng->generateNextInt32(), out, "int8_t*");
        checkSimpleSerializeDeserialize<int16_t*>::check_all(rng->generateNextInt32(), out, "int16_t*");
        checkSimpleSerializeDeserialize<int32_t*>::check_all(rng->generateNextInt32(), out, "int32_t*");
        checkSimpleSerializeDeserialize<int64_t*>::check_all(rng->generateNextInt64(), out, "int64_t*");

        checkSimpleSerializeDeserialize<uint8_t*>::check_all(rng->generateNextUInt32(), out, "uint8_t*");
        checkSimpleSerializeDeserialize<uint16_t*>::check_all(rng->generateNextUInt32(), out, "uint16_t*");
        checkSimpleSerializeDeserialize<uint32_t*>::check_all(rng->generateNextUInt32(), out, "uint32_t*");
        checkSimpleSerializeDeserialize<uint64_t*>::check_all(rng->generateNextUInt64(), out, "uint64_t*");

        checkSimpleSerializeDeserialize<float*>::check_all(rng->nextUniform() * 1000, out, "float*");
        checkSimpleSerializeDeserialize<double*>::check_all(rng->nextUniform() * 1000000, out, "double*");
        checkSimpleSerializeDeserialize<std::string*>::check_all("test_string", out, "std::string*");
    }
    else if ( test == "array" ) {
        {
            int32_t array_in[10];
            for ( size_t i = 0; i < 10; ++i )
                array_in[i] = rng->generateNextInt32();
            passed = checkFixedArraySerializeDeserialize(array_in);
            if ( !passed ) out.output("ERROR: int32_t[10] did not serialize/deserialize properly\n");
        }
        {
            std::array<int32_t, 10> array_in;
            for ( size_t i = 0; i < 10; ++i )
                array_in[i] = rng->generateNextInt32();
            passed = checkFixedArraySerializeDeserialize(array_in);
            if ( !passed ) out.output("ERROR: std::array<int32_t, 10> did not serialize/deserialize properly\n");
        }
        {
            int32_t (*array_in)[10] = reinterpret_cast<int32_t (*)[10]>(new int32_t[10]);
            for ( size_t i = 0; i < 10; ++i )
                (*array_in)[i] = rng->generateNextInt32();
            passed = checkFixedArraySerializeDeserialize(array_in);
            if ( !passed ) out.output("ERROR: int32_t[10] did not serialize/deserialize properly\n");
            delete[] array_in;
        }
        {
            std::array<int32_t, 10>* array_in = new std::array<int32_t, 10>;
            for ( size_t i = 0; i < 10; ++i )
                (*array_in)[i] = rng->generateNextInt32();
            passed = checkFixedArraySerializeDeserialize(array_in);
            if ( !passed ) out.output("ERROR: std::array<int32_t, 10> did not serialize/deserialize properly\n");
            delete array_in;
        }
        {
            size_t   size     = 100;
            int32_t* array_in = new int32_t[size];
            for ( size_t i = 0; i < size; ++i )
                array_in[i] = rng->generateNextInt32();
            passed = checkArraySerializeDeserialize(array_in, size);
            if ( !passed ) out.output("ERROR: std::array<int32_t, %zu> did not serialize/deserialize properly\n", size);
            delete[] array_in;
        }
        {
            size_t                size = 100;
            std::valarray<double> array_in(size);
            for ( size_t i = 0; i < size; ++i )
                array_in[i] = rng->generateNextInt32();
            passed = checkValArraySerializeDeserialize(array_in);
            if ( !passed ) out.output("ERROR: std::valarray<double> did not serialize/deserialize properly\n");
        }
    }
    else if ( test == "optional" ) {
        std::optional<int32_t> optional_in;
        for ( int tries = 0; tries < 10; ++tries ) {
            if ( rng->generateNextUInt32() % 2 ) {
                optional_in.emplace(rng->generateNextInt32());
            }
            else {
                optional_in.reset();
            }
            passed = checkOptionalSerializeDeserialize(optional_in);
            if ( !passed ) out.output("ERROR: std::optional<int32_t> did not serialize/deserialize properly\n");
        }
    }
    else if ( test == "ordered_containers" ) {
        // Ordered Containers
        // map, set, vector, vector<bool>, list, deque
        std::map<int32_t, int32_t>* map_in = new std::map<int32_t, int32_t>();
        for ( int i = 0; i < 10; ++i )
            (*map_in)[rng->generateNextInt32()] = rng->generateNextInt32();
        passed = checkContainerSerializeDeserialize(*map_in);
        if ( !passed ) out.output("ERROR: map<int32_t,int32_t> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(map_in);
        if ( !passed ) out.output("ERROR: map<int32_t,int32_t>* did not serialize/deserialize properly\n");
        delete map_in;

        std::multimap<int32_t, int32_t>*         multimap_in = new std::multimap<int32_t, int32_t>();
        std::vector<std::pair<int32_t, int32_t>> multimap_in_v;
        for ( int i = 0; i < 10; ++i ) {
            int32_t key   = rng->generateNextInt32();
            int     count = rng->generateNextInt32() % 3 + 1;
            for ( int j = 0; j < count; ++j )
                multimap_in_v.emplace_back(key, rng->generateNextInt32());
        }
        shuffle(multimap_in_v);
        for ( auto& e : multimap_in_v )
            multimap_in->emplace(e);
        passed = checkContainerSerializeDeserialize(*multimap_in);
        if ( !passed ) out.output("ERROR: multimap<int32_t,int32_t> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(multimap_in);
        if ( !passed ) out.output("ERROR: multimap<int32_t,int32_t>* did not serialize/deserialize properly\n");
        delete multimap_in;

        std::set<int32_t>* set_in = new std::set<int32_t>();
        for ( int i = 0; i < 10; ++i )
            set_in->insert(rng->generateNextInt32());
        passed = checkContainerSerializeDeserialize(*set_in);
        if ( !passed ) out.output("ERROR: set<int32_t> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(set_in);
        if ( !passed ) out.output("ERROR: set<int32_t>* did not serialize/deserialize properly\n");
        delete set_in;

        std::multiset<int32_t>* multiset_in = new std::multiset<int32_t>();
        std::vector<int32_t>    multiset_in_v;
        for ( int i = 0; i < 10; ++i ) {
            int32_t key   = rng->generateNextInt32();
            int     count = rng->generateNextInt32() % 3 + 1;
            for ( int j = 0; j < count; ++j )
                multiset_in_v.emplace_back(key);
        }
        shuffle(multiset_in_v);
        for ( auto& e : multiset_in_v )
            multiset_in->emplace(e);
        passed = checkContainerSerializeDeserialize(*multiset_in);
        if ( !passed ) out.output("ERROR: multiset<int32_t> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(multiset_in);
        if ( !passed ) out.output("ERROR: multiset<int32_t>* did not serialize/deserialize properly\n");
        delete multiset_in;

        std::vector<int32_t>* vector_in = new std::vector<int32_t>();
        for ( int i = 0; i < 10; ++i )
            vector_in->push_back(rng->generateNextInt32());
        passed = checkContainerSerializeDeserialize(*vector_in);
        if ( !passed ) out.output("ERROR: vector<int32_t> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(vector_in);
        if ( !passed ) out.output("ERROR: vector<int32_t>* did not serialize/deserialize properly\n");
        delete vector_in;

        std::vector<bool>* vector_in_bool = new std::vector<bool>();
        for ( int i = 0; i < 10; ++i )
            vector_in_bool->push_back((rng->generateNextUInt32() % 2));
        passed = checkContainerSerializeDeserialize(*vector_in_bool);
        if ( !passed ) out.output("ERROR: vector<bool> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(vector_in_bool);
        if ( !passed ) out.output("ERROR: vector<bool>* did not serialize/deserialize properly\n");
        delete vector_in_bool;

        std::list<int32_t>* list_in = new std::list<int32_t>();
        for ( int i = 0; i < 10; ++i )
            list_in->push_back(rng->generateNextInt32());
        passed = checkContainerSerializeDeserialize(*list_in);
        if ( !passed ) out.output("ERROR: list<int32_t> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(list_in);
        if ( !passed ) out.output("ERROR: list<int32_t>* did not serialize/deserialize properly\n");
        delete list_in;

        std::forward_list<int32_t>* forward_list_in = new std::forward_list<int32_t>();
        auto                        after           = forward_list_in->before_begin();
        for ( int i = 0; i < 10; ++i )
            after = forward_list_in->insert_after(after, rng->generateNextInt32());
        passed = checkContainerSerializeDeserialize(*forward_list_in);
        if ( !passed ) out.output("ERROR: forward_list<int32_t> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(forward_list_in);
        if ( !passed ) out.output("ERROR: forward_list<int32_t>* did not serialize/deserialize properly\n");
        delete forward_list_in;

        std::deque<int32_t>* deque_in = new std::deque<int32_t>();
        for ( int i = 0; i < 10; ++i )
            deque_in->push_back(rng->generateNextInt32());
        passed = checkContainerSerializeDeserialize(*deque_in);
        if ( !passed ) out.output("ERROR: deque<int32_t> did not serialize/deserialize properly\n");
        passed = checkContainerSerializeDeserialize(deque_in);
        if ( !passed ) out.output("ERROR: deque<int32_t>* did not serialize/deserialize properly\n");
        delete deque_in;

        std::priority_queue<int32_t>* priority_queue_in = new std::priority_queue<int32_t>();
        for ( int i = 0; i < 10; ++i )
            priority_queue_in->push(rng->generateNextInt32());
        passed = checkNonIterableContainerSerializeDeserialize(*priority_queue_in);
        if ( !passed ) out.output("ERROR: priority_queue<int32_t> did not serialize/deserialize properly\n");
        passed = checkNonIterableContainerSerializeDeserialize(priority_queue_in);
        if ( !passed ) out.output("ERROR: priority_queue<int32_t>* did not serialize/deserialize properly\n");
        delete priority_queue_in;

        std::queue<int32_t>* queue_in = new std::queue<int32_t>();
        for ( int i = 0; i < 10; ++i )
            queue_in->push(rng->generateNextInt32());
        passed = checkNonIterableContainerSerializeDeserialize(*queue_in);
        if ( !passed ) out.output("ERROR: queue<int32_t> did not serialize/deserialize properly\n");
        passed = checkNonIterableContainerSerializeDeserialize(queue_in);
        if ( !passed ) out.output("ERROR: queue<int32_t>* did not serialize/deserialize properly\n");
        delete queue_in;

        std::stack<int32_t>* stack_in = new std::stack<int32_t>();
        for ( int i = 0; i < 10; ++i )
            stack_in->push(rng->generateNextInt32());
        passed = checkNonIterableContainerSerializeDeserialize(*stack_in);
        if ( !passed ) out.output("ERROR: stack<int32_t> did not serialize/deserialize properly\n");
        passed = checkNonIterableContainerSerializeDeserialize(stack_in);
        if ( !passed ) out.output("ERROR: stack<int32_t>* did not serialize/deserialize properly\n");
        delete stack_in;
    }
    else if ( test == "unique_ptr" ) {
        // std::unique_ptr
        {
            // Plain scalar
            auto p = std::make_unique<int32_t>(rng->generateNextInt32());
            checkUniquePtrSerializeDeserialize(p);
        }
        {
            // Bounded array
            constexpr size_t n = 1000;
            auto             p = std::unique_ptr<int32_t[n]>(reinterpret_cast<int32_t (*)[n]>(new int32_t[n]));
            for ( size_t i = 0; i < n; ++i )
                (*p.get())[i] = rng->generateNextInt32();
            checkUniquePtrSerializeDeserialize(p);
        }
        {
            // Unbounded array with runtime size
            size_t n = 100;
            auto   p = std::make_unique<int32_t[]>(n);
            for ( size_t i = 0; i < n; ++i )
                p[i] = rng->generateNextInt32();
            checkUniquePtrSerializeDeserialize(p, n);
        }
        {
            // std::vector
            size_t n = 10;
            auto   p = std::make_unique<std::vector<int32_t>>();
            for ( size_t i = 0; i < n; ++i )
                p->push_back(rng->generateNextInt32());
            checkUniquePtrSerializeDeserialize(p);
        }
        {
            // custom deleter, lambda
            int32_t                                  data    = rng->generateNextInt32();
            size_t                                   deleted = 0;
            auto                                     del     = [&](void*) { ++deleted; };
            std::unique_ptr<int32_t, decltype(del)&> ptr(&data, del);
            checkUniquePtrSerializeDeserialize(ptr, del);
            if ( deleted != 1 )
                out.output("ERROR: std::unique_ptr<int32_t, lambda deleter> did not serialize/deserialize properly");
        }
        {
            // custom deleter, function pointer
            int32_t       data = rng->generateNextInt32();
            static size_t deleted;
            void (*del)(void*) = [](void*) { ++deleted; };
            std::unique_ptr<int32_t, void (*)(void*)> ptr(&data, del);
            checkUniquePtrSerializeDeserialize(ptr, del);
            if ( deleted != 1 )
                out.output(
                    "ERROR: std::unique_ptr<int32_t, function pointer deleter> did not serialize/deserialize properly");
        }
        {
            std::tuple<std::unique_ptr<int32_t>, int32_t*> i, o;
            std::get<0>(i) = std::make_unique<int32_t>(rng->generateNextInt32());
            std::get<1>(i) = std::get<0>(i).get();
            serializeDeserialize(i, o, true);
            if ( *std::get<0>(i) != *std::get<0>(o) || *std::get<1>(i) != *std::get<1>(o) ||
                 std::get<0>(o).get() != std::get<1>(o) )
                out.output("ERROR: std::tuple<std::unique_ptr, int32_t*> did not serialize/deserialize properly");
        }
        {
            std::tuple<int32_t*, std::unique_ptr<int32_t>> i, o;
            std::get<1>(i) = std::make_unique<int32_t>(rng->generateNextInt32());
            std::get<0>(i) = std::get<1>(i).get();
            serializeDeserialize(i, o, true);
            if ( *std::get<0>(i) != *std::get<0>(o) || *std::get<1>(i) != *std::get<1>(o) ||
                 std::get<0>(o) != std::get<1>(o).get() )
                out.output("ERROR: std::tuple<int32_t*, std::unique_ptr> did not serialize/deserialize properly");
        }
        {
            // Serialize both a raw pointer and a std::unique_ptr to a variable-sized array, serializing the
            // std::unique_ptr first and the raw pointer second. This ensures that raw pointers can interoperate
            // with std::unique_ptr smart owners.

            size_t                     isize = 100;
            size_t                     osize = -1;
            std::unique_ptr<int32_t[]> i1    = std::make_unique<int32_t[]>(isize);
            std::unique_ptr<int32_t[]> o1;
            int32_t*                   i0 = i1.get();
            int32_t*                   o0 = nullptr;
            for ( size_t t = 0; t < isize; ++t )
                i1[t] = rng->generateNextInt32();

            serializeDeserialize(std::forward_as_tuple(SST::Core::Serialization::unique_ptr(i1, isize),
                                     SST::Core::Serialization::array(i0, isize)),
                std::forward_as_tuple(
                    SST::Core::Serialization::unique_ptr(o1, osize), SST::Core::Serialization::array(o0, osize)),
                true);

            if ( isize != osize )
                out.output("ERROR: std::tuple<int32_t*, std::unique_ptr> did not serialize/deserialize properly: size");
            else
                for ( size_t t = 0; t < isize; ++t ) {
                    if ( i0[t] != o0[t] || i1[t] != o1[t] ) {
                        out.output("ERROR: std::tuple<int32_t*, std::unique_ptr> did not serialize/deserialize "
                                   "properly: content");
                        break;
                    }
                }
        }
        {
            // Serialize both a raw pointer and a std::unique_ptr to a variable-sized array, serializing the raw
            // pointer first and the std::unique_ptr second. This ensures that raw pointers can interoperate
            // with std::unique_ptr smart owners.

            size_t                     isize = 100;
            size_t                     osize = -1;
            std::unique_ptr<int32_t[]> i1    = std::make_unique<int32_t[]>(isize);
            std::unique_ptr<int32_t[]> o1;
            int32_t*                   i0 = i1.get();
            int32_t*                   o0 = nullptr;
            for ( size_t t = 0; t < isize; ++t )
                i1[t] = rng->generateNextInt32();

            serializeDeserialize(std::forward_as_tuple(SST::Core::Serialization::array(i0, isize),
                                     SST::Core::Serialization::unique_ptr(i1, isize)),
                std::forward_as_tuple(
                    SST::Core::Serialization::array(o0, osize), SST::Core::Serialization::unique_ptr(o1, osize)),
                true);

            if ( isize != osize )
                out.output("ERROR: std::tuple<int32_t*, std::unique_ptr> did not serialize/deserialize properly: size");
            else
                for ( size_t t = 0; t < isize; ++t ) {
                    if ( i0[t] != o0[t] || i1[t] != o1[t] ) {
                        out.output("ERROR: std::tuple<int32_t*, std::unique_ptr> did not serialize/deserialize "
                                   "properly: content");
                        break;
                    }
                }
        }
    }
    else if ( test == "unordered_containers" ) {
        // Unordered Containers
        std::unordered_map<int32_t, int32_t>* umap_in = new std::unordered_map<int32_t, int32_t>();
        for ( int i = 0; i < 10; ++i )
            umap_in->emplace(rng->generateNextInt32(), rng->generateNextInt32());
        passed = checkUContainerSerializeDeserialize(*umap_in);
        if ( !passed ) out.output("ERROR: unordered_map<int32_t,int32_t> did not serialize/deserialize properly\n");
        passed = checkUContainerSerializeDeserialize(umap_in);
        if ( !passed ) out.output("ERROR: unordered_map<int32_t,int32_t>* did not serialize/deserialize properly\n");
        delete umap_in;

        std::unordered_multimap<int32_t, int32_t>* umultimap_in = new std::unordered_multimap<int32_t, int32_t>();
        std::vector<std::pair<int32_t, int32_t>>   umultimap_in_v;
        for ( int i = 0; i < 10; ++i ) {
            int32_t key   = rng->generateNextInt32();
            int     count = rng->generateNextInt32() % 3 + 1;
            for ( int j = 0; j < count; ++j )
                umultimap_in_v.emplace_back(key, rng->generateNextInt32());
        }
        shuffle(umultimap_in_v);
        for ( auto& e : umultimap_in_v )
            umultimap_in->emplace(e);
        passed = checkUContainerSerializeDeserialize(*umultimap_in);
        if ( !passed )
            out.output("ERROR: unordered_multimap<int32_t,int32_t> did not serialize/deserialize properly\n");
        passed = checkUContainerSerializeDeserialize(umultimap_in);
        if ( !passed )
            out.output("ERROR: unordered_multimap<int32_t,int32_t>* did not serialize/deserialize properly\n");
        delete umultimap_in;

        std::unordered_set<int32_t>* uset_in = new std::unordered_set<int32_t>();
        for ( int i = 0; i < 10; ++i )
            uset_in->emplace(rng->generateNextInt32());
        passed = checkUContainerSerializeDeserialize(*uset_in);
        if ( !passed ) out.output("ERROR: unordered_set<int32_t,int32_t> did not serialize/deserialize properly\n");
        passed = checkUContainerSerializeDeserialize(uset_in);
        if ( !passed ) out.output("ERROR: unordered_set<int32_t,int32_t>* did not serialize/deserialize properly\n");
        delete uset_in;

        std::unordered_multiset<int32_t>* umultiset_in = new std::unordered_multiset<int32_t>();
        std::vector<int32_t>              umultiset_in_v;
        for ( int i = 0; i < 10; ++i ) {
            int32_t key   = rng->generateNextInt32();
            int     count = rng->generateNextInt32() % 3 + 1;
            for ( int j = 0; j < count; ++j )
                umultiset_in_v.emplace_back(key);
        }
        shuffle(umultiset_in_v);
        for ( auto& e : umultiset_in_v )
            umultiset_in->emplace(e);
        passed = checkUContainerSerializeDeserialize(*umultiset_in);
        if ( !passed ) out.output("ERROR: unordered_multiset<int32_t> did not serialize/deserialize properly\n");
        passed = checkUContainerSerializeDeserialize(umultiset_in);
        if ( !passed ) out.output("ERROR: unordered_multiset<int32_t>* did not serialize/deserialize properly\n");
        delete umultiset_in;
    }
    else if ( test == "variant" ) {
        std::variant<std::vector<int>, double, std::string> var;
        for ( int ntry = 0; ntry < 5; ++ntry ) {
            bool passed = false;

            // Generate random variant each try
            switch ( rng->generateNextUInt32() % std::variant_size_v<decltype(var)> ) {
            case 0:
            {
                var = std::vector<int>(rng->generateNextUInt32() % 1000);
                for ( auto& e : std::get<0>(var) )
                    e = rng->generateNextInt32();
                passed = checkVariantSerializeDeserialize(var);
                break;
            }
            case 1:
            {
                var    = double(rng->generateNextInt32());
                passed = checkVariantSerializeDeserialize(var);
                break;
            }
            case 2:
            {
                std::string str;
                size_t      len = rng->generateNextUInt32() % 100;
                for ( size_t i = 0; i < len; ++i )
                    str += "0123456789"[rng->generateNextUInt32() % 10];
                var    = str;
                passed = checkVariantSerializeDeserialize(var);
            }
            }
            if ( !passed ) out.output("ERROR: std::variant<...> did not serialize/deserialize properly\n");
        }
    }
    else if ( test == "map_to_vector" ) {

        // Containers to other containers

        // There is one instance where we serialize a
        // std::map<std::string, uintptr_t> and deserialize as a
        // std::vector<std::pair<std::string, uintptr_t>>, so check that
        // here
        std::map<std::string, uintptr_t> map2vec_in = { { "s1", 1 }, { "s2", 2 }, { "s3", 3 }, { "s4", 4 },
            { "s5", 5 } };
        std::vector<std::pair<std::string, uintptr_t>> map2vec_out;

        auto buffer = SST::Comms::serialize(map2vec_in);
        SST::Comms::deserialize(buffer, map2vec_out);

        passed = true;
        // Check to see if we get the same data
        for ( auto& x : map2vec_out ) {
            if ( map2vec_in[x.first] != x.second ) passed = false;
        }
        if ( passed && map2vec_in.size() != map2vec_out.size() ) passed = false;
        if ( !passed )
            out.output("ERROR: serializing as map<string,uintptr_t> and deserializing to "
                       "vector<pair<string,uintptr_t>> did not work properly\n");
    }
    else if ( test == "pointer_tracking" ) {
        // Need to test pointer tracking
        pointed_to_class* ptc10 = new pointed_to_class(10);
        pointed_to_class* ptc50 = new pointed_to_class(50);

        // First two will share a pointed to element
        shell* s1 = new shell(25, ptc10);
        shell* s2 = new shell(100, ptc10);

        // Next two are the same pointer
        shell* s3 = new shell(150, ptc50);
        shell* s4 = s3;

        std::vector<shell*> vec = { s1, s2, s3, s4 };

        SST::Core::Serialization::serializer ser;
        ser.enable_pointer_tracking();

        // Get the size
        ser.start_sizing();
        SST_SER(vec);
        size_t size = ser.size();

        char* buffer = new char[size];

        // Serialize
        ser.start_packing(buffer, size);
        SST_SER(vec);

        // Deserialize
        std::vector<shell*> vec_out;
        ser.start_unpacking(buffer, size);
        SST_SER(vec_out);

        // Now check the results

        // 0 and 1 should have the same object pointed to, but not be the
        // same object
        if ( vec_out[0] == vec_out[1] || vec_out[0]->getPointedTo() != vec_out[1]->getPointedTo() ) {
            out.output("ERROR: serializing objects with shared data using pointer tracking did not work properly\n");
        }

        if ( vec_out[2] != vec_out[3] ) {
            out.output("ERROR: serializing two pointers to the same object did not work properly\n");
        }
    }
    else if ( test == "handler" ) {

        // Test serialization of handlers
        HandlerTest* t1 = new HandlerTest(10);
        HandlerTest* t2 = new HandlerTest(20);

        // ShellBase* sb = new Shell<test, float, &test::foo2>(t, 5.6);
        // sb->call(15);

        // HandlerBase* s = new Handler<test, float, &test::foo2>(t, 4.5);
        // Need to test all the variations of the three main template
        // parameters: returnT, argT, dataT.  We need to do each for void
        // and non-void.  That makes 8 variations to test.  We'll label by
        // using 0 for void and 1 for non-void and ordered from MSB to
        // LSB: returnT, argT, dataT.

        // args -                 returnT, argT,       dataT
        auto* h000 = new SSTHandler2<void, void, HandlerTest, void, &HandlerTest::call_000>(t1);
        (*h000)();
        std::cout << std::endl;

        // args -                 returnT, argT,       dataT
        auto* h001 = new SSTHandler2<void, void, HandlerTest, float, &HandlerTest::call_001>(t1, 1.2);
        (*h001)();
        std::cout << std::endl;

        // args -                 returnT, argT,       dataT
        auto* h010 = new SSTHandler2<void, int, HandlerTest, void, &HandlerTest::call_010>(t1);
        (*h010)(52);
        std::cout << std::endl;

        // args -                 returnT, argT,       dataT
        auto* h011 = new SSTHandler2<void, int, HandlerTest, float, &HandlerTest::call_011>(t1, 3.4);
        (*h011)(53);
        std::cout << std::endl;

        // args -                returnT, argT,       dataT
        auto* h100 = new SSTHandler2<int, void, HandlerTest, void, &HandlerTest::call_100>(t2);
        int   ret  = (*h100)();
        std::cout << "Return value: " << ret << std::endl;
        std::cout << std::endl;

        // args -                returnT, argT,       dataT
        auto* h101 = new SSTHandler2<int, void, HandlerTest, float, &HandlerTest::call_101>(t2, 5.6);
        ret        = (*h101)();
        std::cout << "Return value: " << ret << std::endl;
        std::cout << std::endl;

        // args -                returnT, argT,       dataT
        auto* h110 = new SSTHandler2<int, int, HandlerTest, void, &HandlerTest::call_110>(t2);
        ret        = (*h110)(62);
        std::cout << "Return value: " << ret << std::endl;
        std::cout << std::endl;

        // args -                returnT, argT,       dataT
        auto* h111 = new SSTHandler2<int, int, HandlerTest, float, &HandlerTest::call_111>(t2, 7.8);
        ret        = (*h111)(63);
        std::cout << "Return value: " << ret << std::endl;
        std::cout << std::endl;

        // // Serialize and deserialize
        SST::Core::Serialization::serializer ser;
        ser.enable_pointer_tracking();

        // // Get the size
        ser.start_sizing();
        ser.enable_pointer_tracking();

        // Going to serialize t1, but not t2.  It should get automatically
        // serialized when the handlers pointing to it are serialized.
        SST_SER(t1);
        SST_SER(h000);
        SST_SER(h001);
        SST_SER(h010);
        SST_SER(h011);
        SST_SER(h100);
        SST_SER(h101);
        SST_SER(h110);
        SST_SER(h111);


        size_t size   = ser.size();
        char*  buffer = new char[size + 10];

        // Serialize
        ser.start_packing(buffer, size);

        SST_SER(t1);
        SST_SER(h000);
        SST_SER(h001);
        SST_SER(h010);
        SST_SER(h011);
        SST_SER(h100);
        SST_SER(h101);
        SST_SER(h110);
        SST_SER(h111);

        // Delete the original objects
        delete t1;
        delete t2;
        delete h000;
        delete h001;
        delete h010;
        delete h011;
        delete h100;
        delete h101;
        delete h110;
        delete h111;


        // Deserialize
        HandlerTest*                t1_out;
        SSTHandlerBase<void, void>* h000_out;
        SSTHandlerBase<void, void>* h001_out;
        SSTHandlerBase<void, int>*  h010_out;
        SSTHandlerBase<void, int>*  h011_out;
        SSTHandlerBase<int, void>*  h100_out;
        SSTHandlerBase<int, void>*  h101_out;
        SSTHandlerBase<int, int>*   h110_out;
        SSTHandlerBase<int, int>*   h111_out;

        ser.start_unpacking(buffer, size);

        SST_SER(t1_out);
        SST_SER(h000_out);
        SST_SER(h001_out);
        SST_SER(h010_out);
        SST_SER(h011_out);
        SST_SER(h100_out);
        SST_SER(h101_out);
        SST_SER(h110_out);
        SST_SER(h111_out);

        std::cout << "Internal value for t1: " << t1_out->value << std::endl;
        std::cout << std::endl;
        t1_out->value = 100;

        (*h000_out)();
        std::cout << std::endl;

        (*h001_out)();
        std::cout << std::endl;

        (*h010_out)(52);
        std::cout << std::endl;

        (*h011_out)(53);
        std::cout << std::endl;

        ret = (*h100_out)();
        std::cout << "Return value: " << ret << std::endl;
        std::cout << std::endl;

        ret = (*h101_out)();
        std::cout << "Return value: " << ret << std::endl;
        std::cout << std::endl;

        ret = (*h110_out)(62);
        std::cout << "Return value: " << ret << std::endl;
        std::cout << std::endl;

        ret = (*h111_out)(63);
        std::cout << "Return value: " << ret << std::endl;
        std::cout << std::endl;


        // Test recursive serialization using handlers (i.e. the handler
        // points to the enclosing class
        RecursiveSerializationTest* rst = new RecursiveSerializationTest(73);
        (*rst->handler)(17);

        ser.start_sizing();
        SST_SER(rst);

        size   = ser.size();
        buffer = new char[size + 10];

        // Serialize
        ser.start_packing(buffer, size);

        SST_SER(rst);

        RecursiveSerializationTest* rst_out;
        ser.start_unpacking(buffer, size);
        SST_SER(rst_out);

        (*rst_out->handler)(17);
    }
    else if ( test == "componentinfo" ) {
        ComponentInfo info(0, "top_component", "NONE", getTimeConverter("2GHz"));

        ComponentInfo* rinfo = info.test_addSubComponentInfo("subcomp0_0", "slot0");
        rinfo->test_addSubComponentInfo("subcomp1_0", "slot0", getTimeConverter("1GHz"));
        rinfo->test_addSubComponentInfo("subcomp1_1", "slot1", getTimeConverter("500MHz"));

        info.test_addSubComponentInfo("subcomp0_1", "slot1");

        info.test_printComponentInfoHierarchy();

        SST::Core::Serialization::serializer ser;
        ser.enable_pointer_tracking();

        // // Get the size
        ser.start_sizing();
        SST_SER(info, SerOption::as_ptr);


        size_t size   = ser.size();
        char*  buffer = new char[size + 10];

        // Serialize
        ser.start_packing(buffer, size);
        SST_SER(info, SerOption::as_ptr);

        ComponentInfo info2;

        ser.start_unpacking(buffer, size);
        SST_SER(info2, SerOption::as_ptr);

        info2.test_printComponentInfoHierarchy();
    }
    else if ( test == "atomic" ) {
        std::atomic<int32_t> atom(12);

        auto                 buffer = SST::Comms::serialize(atom);
        std::atomic<int32_t> result;
        SST::Comms::deserialize(buffer, result);
        passed = (atom.load() == result.load()) ? true : false;
        if ( !passed ) out.output("ERROR: std::atomic<int32_t> did not serialize/deserialize properly\n");
    }
    else if ( test == "complexcontainer" ) {
        // Need to test more complex combinations of containers

        // std::map<unsigned,std::pair<unsigned,std::vector<unsigned>>>
        std::map<unsigned, std::pair<unsigned, std::vector<unsigned>>> map;
        // Put in a few entries

        SST::Core::Serialization::serializer ser;
        ser.enable_pointer_tracking();

        map[0].first = 10;
        map[0].second.push_back(37);

        map[15].first = 103;
        map[15].second.push_back(35);

        // // Get the size
        ser.start_sizing();
        SST_SER(map);

        size_t size   = ser.size();
        char*  buffer = new char[size + 10];

        // Serialize
        ser.start_packing(buffer, size);
        SST_SER(map);

        std::map<unsigned, std::pair<unsigned, std::vector<unsigned>>> map_out;
        ser.start_unpacking(buffer, size);
        SST_SER(map_out);

        if ( map[0].first != 10 && map[0].second[0] != 37 ) {
            out.output("ERROR: std::map<unsigned,std::pair<unsigned,std::vector<unsigned>>> dir not "
                       "serialize/deserialize properly\n");
        }
        if ( map[15].first != 103 && map[15].second[0] != 35 ) {
            out.output("ERROR: std::map<unsigned,std::pair<unsigned,std::vector<unsigned>>> dir not "
                       "serialize/deserialize properly\n");
        }
    }
    else {
        out.fatal(CALL_INFO_LONG, 1, "ERROR: Unknown serialization test specified: %s\n", test.c_str());
    }
}

} // namespace SST::CoreTestSerialization
