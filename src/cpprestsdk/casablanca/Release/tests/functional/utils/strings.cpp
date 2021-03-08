/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * base64.cpp
 *
 * Tests for base64-related utility functions and classes.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if !defined(__GLIBCXX__)
#include <codecvt>
#endif

#include <locale_guard.h>

using namespace utility;

namespace tests
{
namespace functional
{
namespace utils_tests
{
SUITE(strings)
{
    TEST(usascii_to_utf16)
    {
        std::string str_ascii("This is a test");
        utf16string str_utf16 = utility::conversions::usascii_to_utf16(str_ascii);

        for (size_t i = 0; i < str_ascii.size(); ++i)
        {
            VERIFY_ARE_EQUAL((utf16char)str_ascii[i], str_utf16[i]);
        }
    }

#ifdef _WIN32
#define UTF16(x) L##x
#else
#define UTF16(x) u##x
#endif

    TEST(utf16_to_utf8)
    {
#if !defined(__GLIBCXX__)
        std::wstring_convert<std::codecvt_utf8_utf16<utf16char>, utf16char> conversion;
#endif

        // encodes to single byte character
        VERIFY_ARE_EQUAL("ABC987", utility::conversions::utf16_to_utf8(UTF16("ABC987")));
        utf16string input;
        input.push_back(0x7F); // last ASCII character
        auto result = utility::conversions::utf16_to_utf8(input);
        VERIFY_ARE_EQUAL(0x7F, result[0]);

        // encodes to 2 byte character
        input.clear();
        input.push_back(0x80);
        input.push_back(0x14D);
        input.push_back(0x7FF);
        result = utility::conversions::utf16_to_utf8(input);
#if defined(__GLIBCXX__)
        VERIFY_ARE_EQUAL(194u, static_cast<unsigned char>(result[0]));
        VERIFY_ARE_EQUAL(128u, static_cast<unsigned char>(result[1]));
        VERIFY_ARE_EQUAL(197u, static_cast<unsigned char>(result[2]));
        VERIFY_ARE_EQUAL(141u, static_cast<unsigned char>(result[3]));
        VERIFY_ARE_EQUAL(223u, static_cast<unsigned char>(result[4]));
        VERIFY_ARE_EQUAL(191u, static_cast<unsigned char>(result[5]));
#else
        VERIFY_ARE_EQUAL(conversion.to_bytes(input), result);
#endif

        // encodes to 3 byte character
        input.clear();
        input.push_back(0x800);
        input.push_back(0x14AB);
        input.push_back(0xFFFF);
        result = utility::conversions::utf16_to_utf8(input);
#if defined(__GLIBCXX__)
        VERIFY_ARE_EQUAL(224u, static_cast<unsigned char>(result[0]));
        VERIFY_ARE_EQUAL(160u, static_cast<unsigned char>(result[1]));
        VERIFY_ARE_EQUAL(128u, static_cast<unsigned char>(result[2]));
        VERIFY_ARE_EQUAL(225u, static_cast<unsigned char>(result[3]));
        VERIFY_ARE_EQUAL(146u, static_cast<unsigned char>(result[4]));
        VERIFY_ARE_EQUAL(171u, static_cast<unsigned char>(result[5]));
        VERIFY_ARE_EQUAL(239u, static_cast<unsigned char>(result[6]));
        VERIFY_ARE_EQUAL(191u, static_cast<unsigned char>(result[7]));
        VERIFY_ARE_EQUAL(191u, static_cast<unsigned char>(result[8]));
#else
        VERIFY_ARE_EQUAL(conversion.to_bytes(input), result);
#endif

        // surrogate pair - encodes to 4 byte character
        input.clear();
        // U+10000
        input.push_back(0xD800);
        input.push_back(0xDC00);
        // U+12345
        input.push_back(0xD802);
        input.push_back(0xDD29);
        // U+10FFFF
        input.push_back(0xDA3F);
        input.push_back(0xDFFF);
        result = utility::conversions::utf16_to_utf8(input);
#if defined(__GLIBCXX__)
        VERIFY_ARE_EQUAL(240u, static_cast<unsigned char>(result[0]));
        VERIFY_ARE_EQUAL(144u, static_cast<unsigned char>(result[1]));
        VERIFY_ARE_EQUAL(128u, static_cast<unsigned char>(result[2]));
        VERIFY_ARE_EQUAL(128u, static_cast<unsigned char>(result[3]));
        VERIFY_ARE_EQUAL(240u, static_cast<unsigned char>(result[4]));
        VERIFY_ARE_EQUAL(144u, static_cast<unsigned char>(result[5]));
        VERIFY_ARE_EQUAL(164u, static_cast<unsigned char>(result[6]));
        VERIFY_ARE_EQUAL(169u, static_cast<unsigned char>(result[7]));
        VERIFY_ARE_EQUAL(242u, static_cast<unsigned char>(result[8]));
        VERIFY_ARE_EQUAL(159u, static_cast<unsigned char>(result[9]));
        VERIFY_ARE_EQUAL(191u, static_cast<unsigned char>(result[10]));
        VERIFY_ARE_EQUAL(191u, static_cast<unsigned char>(result[11]));
#else
        VERIFY_ARE_EQUAL(conversion.to_bytes(input), result);
#endif

        // surrogate pair - covering regression bug where 0x10000 was accidentally bitwise OR'ed instead of added.
        input.clear();
        input.push_back(0xD840);
        input.push_back(0xDC00);
        result = utility::conversions::utf16_to_utf8(input);
#if defined(__GLIBCXX__)
        VERIFY_ARE_EQUAL(240u, static_cast<unsigned char>(result[0]));
        VERIFY_ARE_EQUAL(160u, static_cast<unsigned char>(result[1]));
        VERIFY_ARE_EQUAL(128u, static_cast<unsigned char>(result[2]));
        VERIFY_ARE_EQUAL(128u, static_cast<unsigned char>(result[3]));
#else
        VERIFY_ARE_EQUAL(conversion.to_bytes(input), result);
#endif
    }

    TEST(utf8_to_utf16)
    {
#if !defined(__GLIBCXX__)
        std::wstring_convert<std::codecvt_utf8_utf16<utf16char>, utf16char> conversion;
#endif

        // single byte character
        VERIFY_ARE_EQUAL(UTF16("ABC123"), utility::conversions::utf8_to_utf16("ABC123"));
        std::string input;
        input.push_back(0x7F); // last ASCII character
        auto result = utility::conversions::utf8_to_utf16(input);
        VERIFY_ARE_EQUAL(0x7F, result[0]);

        // null byte
        input.clear();
        input.push_back(0);
        input.push_back(0);
        result = utility::conversions::utf8_to_utf16(input);
        VERIFY_ARE_EQUAL(0, result[0]);
        VERIFY_ARE_EQUAL(0, result[1]);

        // 2 byte character
        input.clear();
        // U+80
        input.push_back(208u); // 11010000
        input.push_back(128u); // 10000000
        // U+7FF
        input.push_back(223u); // 11011111
        input.push_back(191u); // 10111111
        result = utility::conversions::utf8_to_utf16(input);
#if defined(__GLIBCXX__)
        VERIFY_ARE_EQUAL(1024, result[0]);
        VERIFY_ARE_EQUAL(2047, result[1]);
#else
        VERIFY_ARE_EQUAL(conversion.from_bytes(input), result);
#endif

        // 3 byte character
        input.clear();
        // U+800
        input.push_back(232u); // 11101000
        input.push_back(128u); // 10000000
        input.push_back(128u); // 10000000
        // U+FFFF
        input.push_back(239u); // 11101111
        input.push_back(191u); // 10111111
        input.push_back(191u); // 10111111
        result = utility::conversions::utf8_to_utf16(input);
#if defined(__GLIBCXX__)
        VERIFY_ARE_EQUAL(32768, result[0]);
        VERIFY_ARE_EQUAL(65535, result[1]);
#else
        VERIFY_ARE_EQUAL(conversion.from_bytes(input), result);
#endif

        // 4 byte character
        input.clear();
        // U+10000
        input.push_back(244u); // 11110100
        input.push_back(128u); // 10000000
        input.push_back(128u); // 10000000
        input.push_back(128u); // 10000000
        // U+10FFFF
        input.push_back(244u); // 11110100
        input.push_back(143u); // 10001111
        input.push_back(191u); // 10111111
        input.push_back(191u); // 10111111
        result = utility::conversions::utf8_to_utf16(input);
#if defined(__GLIBCXX__)
        VERIFY_ARE_EQUAL(56256, result[0]);
        VERIFY_ARE_EQUAL(56320, result[1]);
        VERIFY_ARE_EQUAL(56319, result[2]);
        VERIFY_ARE_EQUAL(57343, result[3]);
#else
        VERIFY_ARE_EQUAL(conversion.from_bytes(input), result);
#endif

        // 1 byte character followed by 4 byte character
        input.clear();
        input.push_back(51u); // 00110011
        // U+10000
        input.push_back(244u); // 11110100
        input.push_back(128u); // 10000000
        input.push_back(128u); // 10000000
        input.push_back(128u); // 10000000
        // U+10FFFF
        input.push_back(244u); // 11110100
        input.push_back(143u); // 10001111
        input.push_back(191u); // 10111111
        input.push_back(191u); // 10111111
        result = utility::conversions::utf8_to_utf16(input);
#if defined(__GLIBCXX__)
        VERIFY_ARE_EQUAL(51, result[0]);
        VERIFY_ARE_EQUAL(56256, result[1]);
        VERIFY_ARE_EQUAL(56320, result[2]);
        VERIFY_ARE_EQUAL(56319, result[3]);
        VERIFY_ARE_EQUAL(57343, result[4]);
#else
        VERIFY_ARE_EQUAL(conversion.from_bytes(input), result);
#endif
    }

    TEST(utf16_to_utf8_errors)
    {
        VERIFY_ARE_EQUAL("ABC987", utility::conversions::utf16_to_utf8(UTF16("ABC987")));
        utf16string input;

        // high surrogate with missing low surrogate.
        input.push_back(0xD800);
        input.push_back(0x0);
        VERIFY_THROWS(utility::conversions::utf16_to_utf8(input), std::range_error);

        // high surrogate with no more characters
        input.clear();
        input.push_back(0xD800);
        VERIFY_THROWS(utility::conversions::utf16_to_utf8(input), std::range_error);
    }

    TEST(utf8_to_utf16_errors)
    {
        // missing second continuation byte
        std::string input;
        input.push_back(207u); // 11001111
        VERIFY_THROWS(utility::conversions::utf8_to_utf16(input), std::range_error);

        // missing third continuation byte
        input.clear();
        input.push_back(230u); // 11100110
        input.push_back(141u); // 10001101
        VERIFY_THROWS(utility::conversions::utf8_to_utf16(input), std::range_error);

        // missing fourth continuation byte
        input.clear();
        input.push_back(240u); // 11110000
        input.push_back(173u); // 10101101
        input.push_back(157u); // 10011101
        VERIFY_THROWS(utility::conversions::utf8_to_utf16(input), std::range_error);

        // continuation byte missing leading 10xxxxxx
        input.clear();
        input.push_back(230u); // 11100110
        input.push_back(141u); // 00001101
        VERIFY_THROWS(utility::conversions::utf8_to_utf16(input), std::range_error);
        input.clear();
        input.push_back(230u); // 11100110
        input.push_back(141u); // 11001101
        VERIFY_THROWS(utility::conversions::utf8_to_utf16(input), std::range_error);

        // invalid for a first character to start with 1xxxxxxx
        input.clear();
        input.push_back(128u); // 10000000
        input.push_back(128u); // 10000000
        VERIFY_THROWS(utility::conversions::utf8_to_utf16(input), std::range_error);
        input.clear();
        input.push_back(191u); // 10111111
        input.push_back(128u); // 10000000
        VERIFY_THROWS(utility::conversions::utf8_to_utf16(input), std::range_error);
    }

    TEST(latin1_to_utf16)
    {
        char in[256] = {0};
        char16_t expectedResult[256] = {0};
        for (size_t i = 0; i < 256; ++i)
        {
            in[i] = static_cast<char>(i);
            expectedResult[i] = static_cast<char16_t>(i);
        }

        std::string str_latin1(in, 256);

        auto actualResult = utility::conversions::latin1_to_utf16(str_latin1);

        VERIFY_ARE_EQUAL(str_latin1.size(), actualResult.size());
        for (size_t i = 0; i < actualResult.size(); ++i)
        {
            VERIFY_ARE_EQUAL(expectedResult[i], actualResult[i]);
        }
    }

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

    TEST(print_string_locale, "Ignore:Android", "Locale unsupported on Android")
    {
        std::locale changedLocale;
        try
        {
#ifdef _WIN32
            changedLocale = std::locale("fr-FR");
#else
            changedLocale = std::locale("fr_FR.UTF-8");
#endif
        }
        catch (const std::exception&)
        {
            // Silently pass if locale isn't installed on machine.
            return;
        }

        tests::common::utilities::locale_guard loc(changedLocale);

        utility::ostringstream_t oss;
        oss << 1000;
        VERIFY_ARE_EQUAL(oss.str(), utility::conversions::print_string(1000));
        VERIFY_ARE_EQUAL(_XPLATSTR("1000"), utility::conversions::print_string(1000, std::locale::classic()));
    }

    TEST(scan_string_locale, "Ignore:Android", "Locale unsupported on Android")
    {
        std::locale changedLocale;
        try
        {
#ifdef _WIN32
            changedLocale = std::locale("fr-FR");
#else
            changedLocale = std::locale("fr_FR.UTF-8");
#endif
        }
        catch (const std::exception&)
        {
            // Silently pass if locale isn't installed on machine.
            return;
        }

        VERIFY_ARE_EQUAL(_XPLATSTR("1000"),
                         utility::conversions::scan_string<utility::string_t>(utility::string_t(_XPLATSTR("1000"))));
        VERIFY_ARE_EQUAL(_XPLATSTR("1,000"),
                         utility::conversions::scan_string<utility::string_t>(utility::string_t(_XPLATSTR("1,000"))));

        VERIFY_ARE_EQUAL(
            _XPLATSTR("1000"),
            utility::conversions::scan_string<utility::string_t>(utility::string_t(_XPLATSTR("1000")), changedLocale));
        VERIFY_ARE_EQUAL(
            _XPLATSTR("1,000"),
            utility::conversions::scan_string<utility::string_t>(utility::string_t(_XPLATSTR("1,000")), changedLocale));

        {
            tests::common::utilities::locale_guard loc(changedLocale);
            VERIFY_ARE_EQUAL(_XPLATSTR("1000"),
                             utility::conversions::scan_string<utility::string_t>(utility::string_t(_XPLATSTR("1000")),
                                                                                  std::locale::classic()));
            VERIFY_ARE_EQUAL(_XPLATSTR("1,000"),
                             utility::conversions::scan_string<utility::string_t>(utility::string_t(_XPLATSTR("1,000")),
                                                                                  std::locale::classic()));
        }
    }

#ifdef _WIN32
    TEST(windows_category_message)
    {
        // Ensure the error message string returned by windows_category doesn't contain trailing zeros.
        std::string error_message = utility::details::windows_category().message(0);
        std::string zero_terminated_copy = error_message.c_str();
        VERIFY_ARE_EQUAL(zero_terminated_copy, error_message);
    }
#endif // _WIN32
}

} // namespace utils_tests
} // namespace functional
} // namespace tests
