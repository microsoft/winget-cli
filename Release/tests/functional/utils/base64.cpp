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

using namespace utility;

namespace tests
{
namespace functional
{
namespace utils_tests
{
SUITE(base64)
{
    // Note: base64 works by encoding any 3 bytes as a four-byte string. Each triple is encoded independently of
    // previous and subsequent triples. If, for a given set of input bytes, the number is not an even multiple of 3,
    // the remaining 1 or two bytes are encoded and padded using '=' characters at the end. The encoding format is
    // defined by IETF RFC 4648. Such padding is only allowed at the end of a encoded string, which makes it impossible
    // to generally concatenate encoded strings and wind up with a string that is a valid base64 encoding.
    //
    // Since each triple of bytes is independent of others, we don't have to test particularly large sets if input data,
    // validating that the algorithm can process at least two triples should be sufficient.
    //
    TEST(rfc_4648_tests_encode)
    {
        // These tests are what base64 RFC 4648 proposes.
        {
            std::vector<unsigned char> str1;
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("")), utility::conversions::to_base64(str1));
        }
        {
            std::vector<unsigned char> str1;
            str1.push_back('f');
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("Zg==")), utility::conversions::to_base64(str1));
        }
        {
            std::vector<unsigned char> str1;
            str1.push_back('f');
            str1.push_back('o');
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("Zm8=")), utility::conversions::to_base64(str1));
        }
        {
            std::vector<unsigned char> str1;
            str1.push_back('f');
            str1.push_back('o');
            str1.push_back('o');
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("Zm9v")), utility::conversions::to_base64(str1));
        }
        {
            std::vector<unsigned char> str1;
            str1.push_back('f');
            str1.push_back('o');
            str1.push_back('o');
            str1.push_back('b');
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("Zm9vYg==")), utility::conversions::to_base64(str1));
        }
        {
            std::vector<unsigned char> str1;
            str1.push_back('f');
            str1.push_back('o');
            str1.push_back('o');
            str1.push_back('b');
            str1.push_back('a');
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("Zm9vYmE=")), utility::conversions::to_base64(str1));
        }
        {
            std::vector<unsigned char> str1;
            str1.push_back('f');
            str1.push_back('o');
            str1.push_back('o');
            str1.push_back('b');
            str1.push_back('a');
            str1.push_back('r');
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("Zm9vYmFy")), utility::conversions::to_base64(str1));
        }
    }

    TEST(rfc_4648_tests_decode)
    {
        // These tests are what base64 RFC 4648 proposes.
        {
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR(""));
            VERIFY_ARE_EQUAL(0u, str1.size());
        }
        {
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("Zg=="));
            VERIFY_ARE_EQUAL(1u, str1.size());
            VERIFY_ARE_EQUAL('f', str1[0]);
        }
        {
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("Zm8="));
            VERIFY_ARE_EQUAL(2u, str1.size());
            VERIFY_ARE_EQUAL('f', str1[0]);
            VERIFY_ARE_EQUAL('o', str1[1]);
        }
        {
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("Zm9v"));
            VERIFY_ARE_EQUAL(3u, str1.size());
            VERIFY_ARE_EQUAL('f', str1[0]);
            VERIFY_ARE_EQUAL('o', str1[1]);
            VERIFY_ARE_EQUAL('o', str1[2]);
        }
        {
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("Zm9vYg=="));
            VERIFY_ARE_EQUAL(4u, str1.size());
            VERIFY_ARE_EQUAL('f', str1[0]);
            VERIFY_ARE_EQUAL('o', str1[1]);
            VERIFY_ARE_EQUAL('o', str1[2]);
            VERIFY_ARE_EQUAL('b', str1[3]);
        }
        {
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("Zm9vYmE="));
            VERIFY_ARE_EQUAL(5u, str1.size());
            VERIFY_ARE_EQUAL('f', str1[0]);
            VERIFY_ARE_EQUAL('o', str1[1]);
            VERIFY_ARE_EQUAL('o', str1[2]);
            VERIFY_ARE_EQUAL('b', str1[3]);
            VERIFY_ARE_EQUAL('a', str1[4]);
        }
        {
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("Zm9vYmFy"));
            VERIFY_ARE_EQUAL(6u, str1.size());
            VERIFY_ARE_EQUAL('f', str1[0]);
            VERIFY_ARE_EQUAL('o', str1[1]);
            VERIFY_ARE_EQUAL('o', str1[2]);
            VERIFY_ARE_EQUAL('b', str1[3]);
            VERIFY_ARE_EQUAL('a', str1[4]);
            VERIFY_ARE_EQUAL('r', str1[5]);
        }
    }

    TEST(additional_encode)
    {
        {
            // Check '/' encoding
            std::vector<unsigned char> str1;
            str1.push_back(254);
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("/g==")), utility::conversions::to_base64(str1));
        }
        {
            // Check '+' encoding
            std::vector<unsigned char> str1;
            str1.push_back(250);
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("+g==")), utility::conversions::to_base64(str1));
        }
        {
            std::vector<unsigned char> str1;
            str1.push_back('f');
            str1.push_back('o');
            str1.push_back(239);
            str1.push_back('b');
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("Zm/vYg==")), utility::conversions::to_base64(str1));
        }
        {
            std::vector<unsigned char> str1;
            str1.push_back('g');
            str1.push_back(239);
            str1.push_back('o');
            str1.push_back('b');
            VERIFY_ARE_EQUAL(string_t(_XPLATSTR("Z+9vYg==")), utility::conversions::to_base64(str1));
        }
    }

    TEST(additional_decode)
    {
        // Tests beyond what the RFC recommends.
        {
            // Check '/' decoding
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("/g=="));
            VERIFY_ARE_EQUAL(1u, str1.size());
            VERIFY_ARE_EQUAL(254u, str1[0]);
        }
        {
            // Check '+' decoding
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("+g=="));
            VERIFY_ARE_EQUAL(1u, str1.size());
            VERIFY_ARE_EQUAL(250u, str1[0]);
        }
        {
            // Check '/' decoding
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("Zm/vYg=="));
            VERIFY_ARE_EQUAL(4u, str1.size());
            VERIFY_ARE_EQUAL('f', str1[0]);
            VERIFY_ARE_EQUAL('o', str1[1]);
            VERIFY_ARE_EQUAL(239, str1[2]);
            VERIFY_ARE_EQUAL('b', str1[3]);
        }
        {
            // Check '+' decoding
            std::vector<unsigned char> str1 = utility::conversions::from_base64(_XPLATSTR("Z+9vYg=="));
            VERIFY_ARE_EQUAL(4u, str1.size());
            VERIFY_ARE_EQUAL('g', str1[0]);
            VERIFY_ARE_EQUAL(239, str1[1]);
            VERIFY_ARE_EQUAL('o', str1[2]);
            VERIFY_ARE_EQUAL('b', str1[3]);
        }
        {
            // Check the whole base64 alphabet
            std::vector<unsigned char> str1 = utility::conversions::from_base64(
                _XPLATSTR("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"));
            VERIFY_ARE_EQUAL(48u, str1.size());
        }
    }

    TEST(bad_decode)
    {
        // These tests are for input that should be disallowed by a very strict decoder, but
        // the available APIs on Windows accept them, as does glib, which is used on Linux.

        // Invalid character before padding, unused ones.
        VERIFY_THROWS(utility::conversions::from_base64(_XPLATSTR("/q==")), std::runtime_error);
        VERIFY_THROWS(utility::conversions::from_base64(_XPLATSTR("Zm9vYmD=")), std::runtime_error);

        // CRLF in the middle.
        VERIFY_THROWS(utility::conversions::from_base64(
                          _XPLATSTR("ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nabcdefghijklmnopqrstuvwxyz\r\n0123456789+/")),
                      std::runtime_error);

        // Not the right length.
        VERIFY_THROWS(utility::conversions::from_base64(_XPLATSTR("/q")), std::runtime_error);
        // Characters not in the alphabet
        VERIFY_THROWS(utility::conversions::from_base64(_XPLATSTR("$%#@")), std::runtime_error);
        // Too much padding at the end.
        VERIFY_THROWS(utility::conversions::from_base64(_XPLATSTR("/q=========")), std::runtime_error);
        // Valid strings, concatenated
        VERIFY_THROWS(utility::conversions::from_base64(_XPLATSTR("Z+9vYg==Z+9vYg==")), std::runtime_error);
    }

    TEST(large_string)
    {
        const size_t size = 64 * 1024;

        std::vector<unsigned char> data(size);
        for (auto i = 0u; i < size; ++i)
        {
            data[i] = (unsigned char)(rand() & 0xFF);
        }

        auto string = utility::conversions::to_base64(data);
        auto data2 = utility::conversions::from_base64(string);

        VERIFY_ARE_EQUAL(data, data2);
    }

} // SUITE(base64)

} // namespace utils_tests
} // namespace functional
} // namespace tests
