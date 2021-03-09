/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * operator_tests.cpp
 *
 * Tests for operators of the uri class.
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
SUITE(operator_tests)
{
    TEST(uri_basic_equality)
    {
        VERIFY_ARE_EQUAL(uri(U("")), uri(U("")));

        uri u1(U("http://localhost:80/path1"));
        uri u2(U("http://localhost:80/path1"));
        VERIFY_ARE_EQUAL(u1, u2);
    }

    TEST(uri_decoded_equality)
    {
        uri_builder u3b(U("http://localhost:80"));
        u3b.set_path(U("path 1"), true);

        uri u3 = u3b.to_uri();
        uri u4(U("http://localhost:80/path%201"));
        VERIFY_ARE_EQUAL(u3, u4);

        uri u5(U("http://localhost:80/pat%68a1"));
        uri u6(U("http://localhost:80/patha1"));
        VERIFY_ARE_EQUAL(u5, u6);

        uri u9(U("http://localhost:80/patha1?name=first#t%65st"));
        uri u10(U("http://localhost:80/patha1?name=first#test"));
        VERIFY_ARE_EQUAL(u9, u10);
    }

    TEST(uri_basic_inequality)
    {
        VERIFY_ARE_NOT_EQUAL(uri(U("http://localhost:80/path1")), uri(U("https://localhost:80/path1")));
        VERIFY_ARE_NOT_EQUAL(uri(U("http://localhost:80/path1")), uri(U("http://localhost2:80/path1")));
        VERIFY_ARE_NOT_EQUAL(uri(U("http://localhost:80/path1")), uri(U("http://localhost:81/path1")));
        VERIFY_ARE_NOT_EQUAL(uri(U("http://localhost:80/path1")), uri(U("http://localhost:80/path2")));
        VERIFY_ARE_NOT_EQUAL(uri(U("http://localhost:80/path1?key=value")),
                             uri(U("http://localhost:80/path1?key=value2")));
        VERIFY_ARE_NOT_EQUAL(uri(U("http://localhost:80/path1?key=value#nose")),
                             uri(U("http://localhost:80/path1?key=value#nose1")));
    }

    TEST(test_empty)
    {
        VERIFY_ARE_EQUAL(uri(), uri());
        VERIFY_ARE_EQUAL(uri(U("htTp://Path")), uri(U("hTtp://pAth")));

        VERIFY_ARE_NOT_EQUAL(uri(U("http://path")), uri());
        VERIFY_ARE_NOT_EQUAL(uri(), uri(U("http://path")));
        VERIFY_ARE_NOT_EQUAL(uri(U("http://path1")), uri(U("http://path2")));
    }

} // SUITE(operator_tests)

} // namespace uri_tests
} // namespace functional
} // namespace tests
