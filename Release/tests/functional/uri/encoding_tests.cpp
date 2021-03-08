/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests for encoding features of the uri class.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

using namespace web;
using namespace utility;

namespace tests
{
namespace functional
{
namespace uri_tests
{
SUITE(encoding_tests)
{
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4428)
    TEST(encode_string)
    {
        utility::string_t result = uri::encode_uri(L"first%second\u4e2d\u56fd");
        VERIFY_ARE_EQUAL(U("first%25second%E4%B8%AD%E5%9B%BD"), result);

        result = uri::encode_uri(U("first%second"));
        VERIFY_ARE_EQUAL(U("first%25second"), result);
    }

    TEST(decode_string)
    {
        utility::string_t result = uri::decode(U("first%25second%E4%B8%AD%E5%9B%BD"));
        VERIFY_ARE_EQUAL(L"first%second\u4e2d\u56fd", result);

        result = uri::decode(U("first%25second"));
        VERIFY_ARE_EQUAL(U("first%second"), result);
    }
#pragma warning(pop)
#endif

    TEST(encode_characters_in_resource)
    {
        utility::string_t result = uri::encode_uri(U("http://path%name/%#!%"));
        VERIFY_ARE_EQUAL(U("http://path%25name/%25#!%25"), result);
    }

    // Tests trying to encode empty strings.
    TEST(encode_decode_empty_strings)
    {
        // utility::string_t
        utility::string_t result = uri::encode_uri(U(""));
        VERIFY_ARE_EQUAL(U(""), result);
        utility::string_t str = uri::decode(result);
        VERIFY_ARE_EQUAL(U(""), str);

        // std::wstring
        result = uri::encode_uri(U(""));
        VERIFY_ARE_EQUAL(U(""), result);
        auto wstr = uri::decode(result);
        VERIFY_ARE_EQUAL(U(""), wstr);
    }

    // Tests encoding in various components of the URI.
    TEST(encode_uri_multiple_components)
    {
        // only encodes characters that aren't in the unreserved and reserved set.

        // utility::string_t
        utility::string_t str(U("htt p://^localhost:80/path ?^one=two# frag"));
        utility::string_t result = uri::encode_uri(str);
        VERIFY_ARE_EQUAL(U("htt%20p://%5Elocalhost:80/path%20?%5Eone=two#%20frag"), result);
        VERIFY_ARE_EQUAL(str, uri::decode(result));
    }

    // Tests encoding individual components of a URI.
    TEST(encode_uri_component)
    {
        // encodes all characters not in the unreserved set.

        // utility::string_t
        utility::string_t str(U("path with^spaced"));
        utility::string_t result = uri::encode_uri(str);
        VERIFY_ARE_EQUAL(U("path%20with%5Espaced"), result);
        VERIFY_ARE_EQUAL(str, uri::decode(result));
    }

    // Tests trying to decode a string that doesn't have 2 hex digits after %
    TEST(decode_invalid_hex)
    {
        VERIFY_THROWS(uri::decode(U("hehe%")), uri_exception);
        VERIFY_THROWS(uri::decode(U("hehe%2")), uri_exception);
        VERIFY_THROWS(uri::decode(U("hehe%4H")), uri_exception);
        VERIFY_THROWS(uri::decode(U("he%kkhe")), uri_exception);
    }

    // Tests making sure '+' is encoded even though nonstandard, so it doesn't
    // get mistaken later by some implementations as a space.
    TEST(encode_plus_char)
    {
        const utility::string_t encodedPlus(U("%2B"));

        uri_builder builder;
        builder.set_user_info(U("+"), true);
        builder.set_path(U("+"), true);
        builder.set_query(U("+"), true);
        builder.set_fragment(U("+"), true);

        VERIFY_ARE_EQUAL(builder.user_info(), encodedPlus);
        VERIFY_ARE_EQUAL(builder.path(), encodedPlus);
        VERIFY_ARE_EQUAL(builder.query(), encodedPlus);
        VERIFY_ARE_EQUAL(builder.fragment(), encodedPlus);
    }

    TEST(bug_417601)
    {
        utility::ostringstream_t ss1;
        auto enc1 = uri::encode_data_string(U("!"));
        ss1 << enc1;

        VERIFY_ARE_EQUAL(U("%21"), ss1.str());
    }

} // SUITE(encoding_tests)

} // namespace uri_tests
} // namespace functional
} // namespace tests
