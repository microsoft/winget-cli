/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * splitting_tests.cpp
 *
 * Tests for path and query splitting features of the uri class.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

namespace tests
{
namespace functional
{
namespace uri_tests
{
using namespace web;
using namespace utility;

SUITE(splitting_tests)
{
    TEST(split_string)
    {
        std::vector<utility::string_t> s = uri::split_path(U("/first/second/third"));
        VERIFY_ARE_EQUAL(3u, s.size());
        VERIFY_ARE_EQUAL(U("first"), s[0]);
        VERIFY_ARE_EQUAL(U("second"), s[1]);
        VERIFY_ARE_EQUAL(U("third"), s[2]);
    }

    TEST(split_encoded_string)
    {
        std::vector<utility::string_t> s = uri::split_path(utility::string_t(U("heh%2Ffirst/second/third")));
        VERIFY_ARE_EQUAL(3u, s.size());
        VERIFY_ARE_EQUAL(U("heh%2Ffirst"), s[0]);
        VERIFY_ARE_EQUAL(U("second"), s[1]);
        VERIFY_ARE_EQUAL(U("third"), s[2]);
    }

    TEST(split_no_slash)
    {
        std::vector<utility::string_t> s = uri::split_path(utility::string_t(U("noslash")));
        VERIFY_ARE_EQUAL(1u, s.size());
        VERIFY_ARE_EQUAL(U("noslash"), s[0]);
    }

    TEST(split_query_basic)
    {
        {
            // Separating with '&'
            std::map<utility::string_t, utility::string_t> keyMap =
                uri::split_query(U("key1=value1&key2=value2&key3=value3"));
            VERIFY_ARE_EQUAL(3u, keyMap.size());
            auto iter = keyMap.begin();
            VERIFY_ARE_EQUAL(U("key1"), iter->first);
            VERIFY_ARE_EQUAL(U("value1"), iter->second);
            ++iter;
            VERIFY_ARE_EQUAL(U("key2"), iter->first);
            VERIFY_ARE_EQUAL(U("value2"), iter->second);
            ++iter;
            VERIFY_ARE_EQUAL(U("key3"), iter->first);
            VERIFY_ARE_EQUAL(U("value3"), iter->second);
        }
        {
            // Separating with ';'
            std::map<utility::string_t, utility::string_t> keyMap =
                uri::split_query(U("key1=value1;key2=value2;key3=value3"));
            VERIFY_ARE_EQUAL(3u, keyMap.size());
            auto iter = keyMap.begin();
            VERIFY_ARE_EQUAL(U("key1"), iter->first);
            VERIFY_ARE_EQUAL(U("value1"), iter->second);
            ++iter;
            VERIFY_ARE_EQUAL(U("key2"), iter->first);
            VERIFY_ARE_EQUAL(U("value2"), iter->second);
            ++iter;
            VERIFY_ARE_EQUAL(U("key3"), iter->first);
            VERIFY_ARE_EQUAL(U("value3"), iter->second);
        }
    }

    TEST(split_encoded_query)
    {
        {
            // Separating with '&'
            std::map<utility::string_t, utility::string_t> keyMap =
                uri::split_query(utility::string_t(U("key=value%26key1%20=value1&key2=%5Evalue2&key3=value3%20")));
            VERIFY_ARE_EQUAL(3u, keyMap.size());
            auto iter = keyMap.begin();
            VERIFY_ARE_EQUAL(U("key"), iter->first);
            VERIFY_ARE_EQUAL(U("value%26key1%20=value1"), iter->second);
            ++iter;
            VERIFY_ARE_EQUAL(U("key2"), iter->first);
            VERIFY_ARE_EQUAL(U("%5Evalue2"), iter->second);
            ++iter;
            VERIFY_ARE_EQUAL(U("key3"), iter->first);
            VERIFY_ARE_EQUAL(U("value3%20"), iter->second);
        }
        {
            // Separating with ';'
            std::map<utility::string_t, utility::string_t> keyMap =
                uri::split_query(utility::string_t(U("key=value%26key1%20=value1;key2=%5Evalue2;key3=value3%20")));
            VERIFY_ARE_EQUAL(3u, keyMap.size());
            auto iter = keyMap.begin();
            VERIFY_ARE_EQUAL(U("key"), iter->first);
            VERIFY_ARE_EQUAL(U("value%26key1%20=value1"), iter->second);
            ++iter;
            VERIFY_ARE_EQUAL(U("key2"), iter->first);
            VERIFY_ARE_EQUAL(U("%5Evalue2"), iter->second);
            ++iter;
            VERIFY_ARE_EQUAL(U("key3"), iter->first);
            VERIFY_ARE_EQUAL(U("value3%20"), iter->second);
        }
    }

    TEST(split_query_empty)
    {
        std::map<utility::string_t, utility::string_t> keyMap = uri::split_query(U(""));
        VERIFY_ARE_EQUAL(0u, keyMap.size());
    }

    TEST(split_query_single)
    {
        std::map<utility::string_t, utility::string_t> keyMap = uri::split_query(U("key1=44"));
        VERIFY_ARE_EQUAL(1u, keyMap.size());
        auto iter = keyMap.begin();
        VERIFY_ARE_EQUAL(U("key1"), iter->first);
        VERIFY_ARE_EQUAL(U("44"), iter->second);
    }

    TEST(split_query_no_value)
    {
        std::map<utility::string_t, utility::string_t> keyMap = uri::split_query(U("key1"));
        VERIFY_ARE_EQUAL(0u, keyMap.size());
        keyMap = uri::split_query(U("key1="));
        VERIFY_ARE_EQUAL(1u, keyMap.size());
        auto iter = keyMap.begin();
        VERIFY_ARE_EQUAL(U("key1"), iter->first);
        VERIFY_ARE_EQUAL(U(""), iter->second);
        keyMap = uri::split_query(U("key1&"));
        VERIFY_ARE_EQUAL(0u, keyMap.size());
    }

    TEST(split_query_no_key)
    {
        std::map<utility::string_t, utility::string_t> keyMap = uri::split_query(U("=value1"));
        VERIFY_ARE_EQUAL(1u, keyMap.size());
        auto iter = keyMap.begin();
        VERIFY_ARE_EQUAL(U(""), iter->first);
        VERIFY_ARE_EQUAL(U("value1"), iter->second);
    }

    TEST(split_query_end_with_amp)
    {
        {
            // Separating with '&'
            std::map<utility::string_t, utility::string_t> keyMap = uri::split_query(U("key1=44&"));
            VERIFY_ARE_EQUAL(1u, keyMap.size());
            auto iter = keyMap.begin();
            VERIFY_ARE_EQUAL(U("key1"), iter->first);
            VERIFY_ARE_EQUAL(U("44"), iter->second);
        }
        {
            // Separating with ';'
            std::map<utility::string_t, utility::string_t> keyMap = uri::split_query(U("key1=44;"));
            VERIFY_ARE_EQUAL(1u, keyMap.size());
            auto iter = keyMap.begin();
            VERIFY_ARE_EQUAL(U("key1"), iter->first);
            VERIFY_ARE_EQUAL(U("44"), iter->second);
        }
    }

} // SUITE(splitting_tests)

} // namespace uri_tests
} // namespace functional
} // namespace tests
