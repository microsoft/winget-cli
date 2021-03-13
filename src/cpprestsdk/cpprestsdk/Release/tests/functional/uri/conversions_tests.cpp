/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * conversion_tests.cpp
 *
 * Tests to string functions and implicit conversions of the http::uri class.
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
SUITE(conversions_tests)
{
    TEST(to_string_conversion)
    {
        utility::string_t encoded = uri::encode_uri(U("http://testname.com/%%?qstring"));
        uri u1(U("http://testname.com/%25%25?qstring"));

        VERIFY_ARE_EQUAL(uri::decode(encoded), uri::decode(u1.to_string()));
    }

    TEST(to_encoded_string)
    {
        utility::string_t encoded = uri::encode_uri(U("http://testname.com/%%?qstring"));
        uri u(U("http://testname.com/%25%25?qstring"));

        VERIFY_ARE_EQUAL(encoded, u.to_string());
    }

    TEST(empty_to_string)
    {
        uri u;
        VERIFY_ARE_EQUAL(U("/"), u.to_string());
    }

} // SUITE(conversions_tests)

} // namespace uri_tests
} // namespace functional
} // namespace tests
