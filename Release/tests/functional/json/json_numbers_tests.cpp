/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * json_numbers_tests.cpp
 *
 * Tests parsing numbers and json::number class
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "cpprest/json.h"
#include "unittestpp.h"
#include <clocale>
#include <iomanip>

using namespace web;
using namespace utility;

namespace tests
{
namespace functional
{
namespace json_tests
{
SUITE(json_numbers_tests)
{
    TEST(numbers)
    {
        json::value num = json::value::parse(U("-22"));
        VERIFY_ARE_EQUAL(-22, num.as_double());
        VERIFY_ARE_EQUAL(-22, num.as_integer());

        num = json::value::parse(U("-1.45E2"));
        VERIFY_IS_TRUE(num.is_number());

        num = json::value::parse(U("-1.45E+1"));
        VERIFY_IS_TRUE(num.is_number());

        num = json::value::parse(U("-1.45E-10"));
        VERIFY_IS_TRUE(num.is_number());

        num = json::value::parse(U("1e01"));
        VERIFY_IS_TRUE(num.is_number());
    }

    // Test both positive and negative number
    void test_int64(int64_t number)
    {
        stringstream_t ss;
        ss << number;
        json::value num = json::value::parse(ss);
        VERIFY_ARE_EQUAL(number, num.as_number().to_int64());
        VERIFY_IS_TRUE(num.is_integer());
        num = json::value::number(number);
        VERIFY_ARE_EQUAL(number, num.as_number().to_int64());
        VERIFY_IS_TRUE(num.is_integer());

        // Check that the number is convertible to signed int64
        VERIFY_IS_TRUE(num.as_number().is_int64());

        // Check for other integral conversions
        VERIFY_ARE_EQUAL(number >= INT_MIN && number <= INT_MAX, num.as_number().is_int32());
        VERIFY_ARE_EQUAL(number >= 0 && number <= UINT_MAX, num.as_number().is_uint32());
        VERIFY_ARE_EQUAL(number >= 0, num.as_number().is_uint64());
    }

    TEST(parse_int64)
    {
        // Negative limits
        test_int64(int64_t(LLONG_MIN));
        test_int64(int64_t(LLONG_MIN) + 1);
        test_int64(int64_t(INT_MIN) - 1);
        test_int64(int64_t(INT_MIN));
        test_int64(int64_t(INT_MIN) + 1);

        // Around zero
        test_int64(int64_t(-1));
        test_int64(int64_t(0));
        test_int64(int64_t(1));

        // Positive limits
        test_int64(int64_t(INT_MAX));
        test_int64(int64_t(INT_MAX) + 1);
        test_int64(int64_t(UINT_MAX));
        test_int64(int64_t(UINT_MAX) + 1);

        // Outside 32-bits limits
        test_int64(int64_t(INT_MAX) * 13 + 5); // a number out of the int32 range
        test_int64(uint64_t(LLONG_MAX / 2));
    }

    void test_int64(uint64_t number)
    {
        stringstream_t ss;
        ss << number;
        json::value num = json::value::parse(ss);
        VERIFY_ARE_EQUAL(number, num.as_number().to_uint64());
        VERIFY_IS_TRUE(num.is_integer());
        num = json::value::number(number);
        VERIFY_ARE_EQUAL(number, num.as_number().to_uint64());
        VERIFY_IS_TRUE(num.is_integer());

        // Check that the number is convertible to unsigned int64
        VERIFY_IS_TRUE(num.as_number().is_uint64());

        // Check for other integral conversions
        VERIFY_ARE_EQUAL(number <= INT_MAX, num.as_number().is_int32());
        VERIFY_ARE_EQUAL(number <= UINT_MAX, num.as_number().is_uint32());
        VERIFY_ARE_EQUAL(number <= LLONG_MAX, num.as_number().is_int64());
    }

    TEST(parse_uint64)
    {
        test_int64(int64_t(0));
        test_int64(int64_t(1));

        test_int64(uint64_t(LLONG_MAX) - 1);
        test_int64(uint64_t(LLONG_MAX));
        test_int64(uint64_t(LLONG_MAX) + 1);
        test_int64(uint64_t(ULLONG_MAX));
        test_int64(uint64_t(ULLONG_MAX) - 1);
    }

    const int DOUBLE_DIGITS =
        std::numeric_limits<double>::digits10 +
        7; // 7 = length of "1." and "e+123" which is the begining and the end of the double representation

    void test_double(double number, string_t str_rep)
    {
        stringstream_t ss;
        ss << str_rep;

        json::value num = json::value::parse(ss);
        VERIFY_ARE_EQUAL(number, num.as_double());
        VERIFY_ARE_EQUAL(number, num.as_number().to_double());

        // If the number is within integral types limit and not decimal, it should be stored as one of the integral
        // types
        VERIFY_ARE_EQUAL(number > LLONG_MIN && number < ULLONG_MAX && number == floor(number), num.is_integer());

        // If it is outside the range, these methods should return false.
        // Note that at this point there is no guarantee that the number was stored as double.

        if (number < INT_MIN || number > INT_MAX || number != floor(number))
            VERIFY_IS_FALSE(num.as_number().is_int32());

        if (number < 0 || number > UINT_MAX || number != floor(number)) VERIFY_IS_FALSE(num.as_number().is_uint32());

        if (number < LLONG_MIN || number > LLONG_MAX || number != floor(number))
            VERIFY_IS_FALSE(num.as_number().is_int64());

        if (number < 0 || number > ULLONG_MAX || number != floor(number)) VERIFY_IS_FALSE(num.as_number().is_uint64());
    }

    void test_double(double d)
    {
        ::std::basic_stringstream<string_t::value_type> ss;
        ss << ::std::setprecision(DOUBLE_DIGITS);
        ss << d;
        test_double(d, ss.str());
    }

    TEST(parsing_doubles_into_longs)
    {
        test_double(2.0);
        test_double(pow(2.0, 10.0));
        test_double(pow(2.0, 20.0));
        test_double(pow(2.0, 60.0));
        test_double(pow(2.0, 63.0));
    }

    TEST(parsing_doubles)
    {
        test_double(3.14);
        test_double(-9.81);

        // Note: this should not parse to a ullong because of rounding
        test_double(static_cast<double>(ULLONG_MAX));

        test_double(0 - static_cast<double>(ULLONG_MAX));
        test_double(static_cast<double>(ULLONG_MAX) +
                    (2 << (64 - 52))); // the lowest number that will be represented as double due to overflowing
                                       // unsigned int64 (52bits fraction in double-precision)
        test_double(0 - pow(2.0, 63.0) * 1.5); // between 0-ULLONG_MAX and LLONGMIN
    }

    TEST(parsing_doubles_setlocale,
         "Ignore:Android",
         "Locale not supported on Android",
         "Ignore:Linux",
         "Fails due to double conversion issues",
         "Ignore:Apple",
         "Fails due to double conversion issues")
    {
        // JSON uses the C locale always and should therefore not be impacted by the process locale
#ifdef _WIN32
        std::string changedLocale("fr-FR");
#else
        std::string changedLocale("fr_FR.UTF-8");
#endif

        // If locale isn't installed on system just silently pass.
        if (setlocale(LC_ALL, changedLocale.c_str()) != nullptr)
        {
            test_double(1.91563);
            test_double(2.0e93);
            setlocale(LC_ALL, "C");
        }
    }

    TEST(parsing_very_large_doubles)
    {
        test_double(pow(2.0, 64.0));
        test_double(pow(2.0, 70.0));
        test_double(pow(2.0, 80.0));
        test_double(pow(2.0, 120.0));
        test_double(pow(2.0, 240.0));
        test_double(pow(2.0, 300.0));
    }

    TEST(parsing_very_small_doubles)
    {
        test_double(2.34e-308);
        test_double(1e-308);
    }

    void test_integral(int number)
    {
        stringstream_t ss;
        ss << number;
        json::value num = json::value::parse(ss);
        VERIFY_IS_TRUE(num.as_number().is_int32());
        VERIFY_IS_TRUE(num.as_number().is_uint32());
        VERIFY_IS_TRUE(num.as_number().is_int64());
        VERIFY_IS_TRUE(num.as_number().is_uint64());

        VERIFY_ARE_EQUAL(number, num.as_number().to_int32());
        VERIFY_ARE_EQUAL(number, num.as_number().to_uint32());
        VERIFY_ARE_EQUAL(number, num.as_number().to_int64());
        VERIFY_ARE_EQUAL(number, num.as_number().to_uint64());
    }

    TEST(parsing_integral_types)
    {
        test_integral(0);
        test_integral(1);
        test_integral(INT_MAX / 2);
        test_integral(INT_MAX);
    }

    TEST(int_double_limits)
    {
        utility::stringstream_t stream(utility::stringstream_t::in | utility::stringstream_t::out);
        utility::stringstream_t oracleStream(utility::stringstream_t::in | utility::stringstream_t::out);

        // unsigned int64 max
        oracleStream.precision(std::numeric_limits<uint64_t>::digits10 + 2);
        oracleStream << (std::numeric_limits<uint64_t>::max)();
        json::value iMax((std::numeric_limits<uint64_t>::max)());
        VERIFY_ARE_EQUAL(oracleStream.str(), iMax.serialize());
        iMax.serialize(stream);
        VERIFY_ARE_EQUAL(oracleStream.str(), stream.str());

        // signed int64 min
        stream.str(U(""));
        oracleStream.str(U(""));
        oracleStream.clear();
        oracleStream << (std::numeric_limits<int64_t>::min)();
        json::value iMin((std::numeric_limits<int64_t>::min)());
        VERIFY_ARE_EQUAL(oracleStream.str(), iMin.serialize());
        iMin.serialize(stream);
        VERIFY_ARE_EQUAL(oracleStream.str(), stream.str());

        // double max
        stream.str(U(""));
        oracleStream.str(U(""));
        oracleStream.precision(std::numeric_limits<double>::digits10 + 2);
        oracleStream << (std::numeric_limits<double>::max)();
        json::value dMax((std::numeric_limits<double>::max)());
        VERIFY_ARE_EQUAL(oracleStream.str(), dMax.serialize());
        dMax.serialize(stream);
        VERIFY_ARE_EQUAL(oracleStream.str(), stream.str());

        // double min
        stream.str(U(""));
        oracleStream.str(U(""));
        oracleStream << (std::numeric_limits<double>::min)();
        json::value dMin((std::numeric_limits<double>::min)());
        VERIFY_ARE_EQUAL(oracleStream.str(), dMin.serialize());
        dMin.serialize(stream);
        VERIFY_ARE_EQUAL(oracleStream.str(), stream.str());
    }

    TEST(compare_numbers)
    {
        // Make sure these are equal
        VERIFY_ARE_EQUAL(json::value(3.14), json::value::parse(U("3.14")));
        VERIFY_ARE_EQUAL(json::value(uint64_t(1234)), json::value::parse(U("1234")));
        VERIFY_ARE_EQUAL(json::value(uint32_t(10)), json::value::parse(U("10")));

        // These two are to verify that explicitly stated signed int was stored as unsigned int as we store all
        // non-negative numbers as unsigned int
        VERIFY_ARE_EQUAL(json::value(int32_t(10)), json::value::parse(U("10")));
        VERIFY_ARE_EQUAL(json::value(int64_t(1234)), json::value::parse(U("1234")));

        // These numbers would be equal if converted to double first. That is how we compared them before we had int64
        // support.
        VERIFY_ARE_NOT_EQUAL(json::value(int64_t(LLONG_MIN)), json::value(int64_t(LLONG_MIN + 1)));
        VERIFY_ARE_NOT_EQUAL(json::value(uint64_t(ULLONG_MAX)), json::value(uint64_t(ULLONG_MAX - 1)));

        // Checking boundary condition - zero
        VERIFY_ARE_EQUAL(json::value(int32_t(0)), json::value::parse(U("-0")));
        VERIFY_ARE_EQUAL(json::value(int64_t(0)), json::value::parse(U("-0")));
        VERIFY_ARE_EQUAL(json::value::parse(U("0")), json::value::parse(U("-0")));
    }

} // SUITE(json_numbers_tests)

} // namespace json_tests
} // namespace functional
} // namespace tests
