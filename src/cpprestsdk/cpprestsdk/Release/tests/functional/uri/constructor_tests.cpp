/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * constructor_tests.cpp
 *
 * Tests for constructors of the uri class.
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
SUITE(constructor_tests)
{
    TEST(parsing_constructor_char)
    {
        uri u(uri::encode_uri(U("net.tcp://steve:@testname.com:81/bleh%?qstring#goo")));

        VERIFY_ARE_EQUAL(U("net.tcp"), u.scheme());
        VERIFY_ARE_EQUAL(U("steve:"), u.user_info());
        VERIFY_ARE_EQUAL(U("testname.com"), u.host());
        VERIFY_ARE_EQUAL(81, u.port());
        VERIFY_ARE_EQUAL(U("/bleh%25"), u.path());
        VERIFY_ARE_EQUAL(U("qstring"), u.query());
        VERIFY_ARE_EQUAL(U("goo"), u.fragment());
    }

    TEST(parsing_constructor_encoded_string)
    {
        uri u(uri::encode_uri(U("net.tcp://testname.com:81/bleh%?qstring#goo")));

        VERIFY_ARE_EQUAL(U("net.tcp"), u.scheme());
        VERIFY_ARE_EQUAL(U("testname.com"), u.host());
        VERIFY_ARE_EQUAL(81, u.port());
        VERIFY_ARE_EQUAL(U("/bleh%25"), u.path());
        VERIFY_ARE_EQUAL(U("qstring"), u.query());
        VERIFY_ARE_EQUAL(U("goo"), u.fragment());
    }

    TEST(parsing_constructor_string_string)
    {
        uri u(uri::encode_uri(U("net.tcp://testname.com:81/bleh%?qstring#goo")));

        VERIFY_ARE_EQUAL(U("net.tcp"), u.scheme());
        VERIFY_ARE_EQUAL(U("testname.com"), u.host());
        VERIFY_ARE_EQUAL(81, u.port());
        VERIFY_ARE_EQUAL(U("/bleh%25"), u.path());
        VERIFY_ARE_EQUAL(U("qstring"), u.query());
        VERIFY_ARE_EQUAL(U("goo"), u.fragment());
    }

    TEST(empty_strings)
    {
        VERIFY_IS_TRUE(uri(U("")).is_empty());
        VERIFY_IS_TRUE(uri(U("")).is_empty());
        VERIFY_IS_TRUE(uri(uri::encode_uri(U(""))).is_empty());
    }

    TEST(default_constructor) { VERIFY_IS_TRUE(uri().is_empty()); }

    TEST(relative_ref_string)
    {
        uri u(uri::encode_uri(U("first/second#boff")));

        VERIFY_ARE_EQUAL(U(""), u.scheme());
        VERIFY_ARE_EQUAL(U(""), u.host());
        VERIFY_ARE_EQUAL(0, u.port());
        VERIFY_ARE_EQUAL(U("first/second"), u.path());
        VERIFY_ARE_EQUAL(U(""), u.query());
        VERIFY_ARE_EQUAL(U("boff"), u.fragment());
    }

    TEST(absolute_ref_string)
    {
        uri u(uri::encode_uri(U("/first/second#boff")));

        VERIFY_ARE_EQUAL(U(""), u.scheme());
        VERIFY_ARE_EQUAL(U(""), u.host());
        VERIFY_ARE_EQUAL(0, u.port());
        VERIFY_ARE_EQUAL(U("/first/second"), u.path());
        VERIFY_ARE_EQUAL(U(""), u.query());
        VERIFY_ARE_EQUAL(U("boff"), u.fragment());
    }

    TEST(copy_constructor)
    {
        uri original(U("http://st:pass@localhost:456/path1?qstring#goo"));
        uri new_uri(original);

        VERIFY_ARE_EQUAL(original, new_uri);
    }

    TEST(move_constructor)
    {
        const utility::string_t uri_str(U("http://localhost:456/path1?qstring#goo"));
        uri original(uri_str);
        uri new_uri = std::move(original);

        VERIFY_ARE_EQUAL(uri_str, new_uri.to_string());
        VERIFY_ARE_EQUAL(uri(uri_str), new_uri);
    }

    TEST(assignment_operator)
    {
        uri original(U("http://localhost:456/path?qstring#goo"));
        uri new_uri = original;

        VERIFY_ARE_EQUAL(original, new_uri);
    }

    // Tests invalid URI being passed in constructor.
    TEST(parsing_constructor_invalid)
    {
        VERIFY_THROWS(uri(U("123http://localhost:345/")), uri_exception);
        VERIFY_THROWS(uri(U("h*ttp://localhost:345/")), uri_exception);
        VERIFY_THROWS(uri(U("http://localhost:345/\"")), uri_exception);
        VERIFY_THROWS(uri(U("http://localhost:345/path?\"")), uri_exception);
        VERIFY_THROWS(uri(U("http://local\"host:345/")), uri_exception);
    }

    // Tests a variety of different URIs using the examples in RFC 2732
    TEST(RFC_2732_examples_string)
    {
        // The URI parser will make characters lower case
        uri http1(U("http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html"));
        VERIFY_ARE_EQUAL(U("http"), http1.scheme());
        VERIFY_ARE_EQUAL(U("[fedc:ba98:7654:3210:fedc:ba98:7654:3210]"), http1.host());
        VERIFY_ARE_EQUAL(80, http1.port());
        VERIFY_ARE_EQUAL(U("/index.html"), http1.path());
        VERIFY_ARE_EQUAL(U(""), http1.query());

        uri http2(U("http://[1080:0:0:0:8:800:200C:417A]/index.html"));
        VERIFY_ARE_EQUAL(U("http"), http2.scheme());
        VERIFY_ARE_EQUAL(U("[1080:0:0:0:8:800:200c:417a]"), http2.host());
        VERIFY_ARE_EQUAL(0, http2.port());
        VERIFY_ARE_EQUAL(U("/index.html"), http2.path());
        VERIFY_ARE_EQUAL(U(""), http2.query());

        uri http3(U("https://[3ffe:2a00:100:7031::1]"));
        VERIFY_ARE_EQUAL(U("https"), http3.scheme());
        VERIFY_ARE_EQUAL(U("[3ffe:2a00:100:7031::1]"), http3.host());
        VERIFY_ARE_EQUAL(0, http3.port());
        VERIFY_ARE_EQUAL(U("/"), http3.path());
        VERIFY_ARE_EQUAL(U(""), http3.query());

        uri http4(U("http://[::192.9.5.5]/ipng"));
        VERIFY_ARE_EQUAL(U("http"), http4.scheme());
        VERIFY_ARE_EQUAL(U("[::192.9.5.5]"), http4.host());
        VERIFY_ARE_EQUAL(0, http4.port());
        VERIFY_ARE_EQUAL(U("/ipng"), http4.path());
        VERIFY_ARE_EQUAL(U(""), http4.query());

        uri http5(U("http://[1080::8:800:200C:417A]/foo"));
        VERIFY_ARE_EQUAL(U("http"), http5.scheme());
        VERIFY_ARE_EQUAL(U("[1080::8:800:200c:417a]"), http5.host());
        VERIFY_ARE_EQUAL(0, http5.port());
        VERIFY_ARE_EQUAL(U("/foo"), http5.path());
        VERIFY_ARE_EQUAL(U(""), http5.query());

        uri http6(U("http://[::FFFF:129.144.52.38]:80/index.html"));
        VERIFY_ARE_EQUAL(U("http"), http6.scheme());
        VERIFY_ARE_EQUAL(U("[::ffff:129.144.52.38]"), http6.host());
        VERIFY_ARE_EQUAL(80, http6.port());
        VERIFY_ARE_EQUAL(U("/index.html"), http6.path());
        VERIFY_ARE_EQUAL(U(""), http6.query());

        uri http7(U("http://[2010:836B:4179::836B:4179]"));
        VERIFY_ARE_EQUAL(U("http"), http7.scheme());
        VERIFY_ARE_EQUAL(U("[2010:836b:4179::836b:4179]"), http7.host());
        VERIFY_ARE_EQUAL(0, http7.port());
        VERIFY_ARE_EQUAL(U("/"), http7.path());
        VERIFY_ARE_EQUAL(U(""), http7.query());
    }

    // Tests a variety of different URIs using the examples in RFC 3986.
    TEST(RFC_3968_examples_string)
    {
        uri ftp(U("ftp://ftp.is.co.za/rfc/rfc1808.txt"));
        VERIFY_ARE_EQUAL(U("ftp"), ftp.scheme());
        VERIFY_ARE_EQUAL(U(""), ftp.user_info());
        VERIFY_ARE_EQUAL(U("ftp.is.co.za"), ftp.host());
        VERIFY_ARE_EQUAL(0, ftp.port());
        VERIFY_ARE_EQUAL(U("/rfc/rfc1808.txt"), ftp.path());
        VERIFY_ARE_EQUAL(U(""), ftp.query());
        VERIFY_ARE_EQUAL(U(""), ftp.fragment());

        // TFS #371892
        // uri ldap(U("ldap://[2001:db8::7]/?c=GB#objectClass?one"));
        // VERIFY_ARE_EQUAL(U("ldap"), ldap.scheme());
        // VERIFY_ARE_EQUAL(U(""), ldap.user_info());
        // VERIFY_ARE_EQUAL(U("2001:db8::7"), ldap.host());
        // VERIFY_ARE_EQUAL(0, ldap.port());
        // VERIFY_ARE_EQUAL(U("/"), ldap.path());
        // VERIFY_ARE_EQUAL(U("c=GB"), ldap.query());
        // VERIFY_ARE_EQUAL(U("objectClass?one"), ldap.fragment());

        // We don't support anything scheme specific like in C# so
        // these common ones don't have a great experience yet.
        uri mailto(U("mailto:John.Doe@example.com"));
        VERIFY_ARE_EQUAL(U("mailto"), mailto.scheme());
        VERIFY_ARE_EQUAL(U(""), mailto.user_info());
        VERIFY_ARE_EQUAL(U(""), mailto.host());
        VERIFY_ARE_EQUAL(0, mailto.port());
        VERIFY_ARE_EQUAL(U("John.Doe@example.com"), mailto.path());
        VERIFY_ARE_EQUAL(U(""), mailto.query());
        VERIFY_ARE_EQUAL(U(""), mailto.fragment());

        uri tel(U("tel:+1-816-555-1212"));
        VERIFY_ARE_EQUAL(U("tel"), tel.scheme());
        VERIFY_ARE_EQUAL(U(""), tel.user_info());
        VERIFY_ARE_EQUAL(U(""), tel.host());
        VERIFY_ARE_EQUAL(0, tel.port());
        VERIFY_ARE_EQUAL(U("+1-816-555-1212"), tel.path());
        VERIFY_ARE_EQUAL(U(""), tel.query());
        VERIFY_ARE_EQUAL(U(""), tel.fragment());

        uri telnet(U("telnet://192.0.2.16:80/"));
        VERIFY_ARE_EQUAL(U("telnet"), telnet.scheme());
        VERIFY_ARE_EQUAL(U(""), telnet.user_info());
        VERIFY_ARE_EQUAL(U("192.0.2.16"), telnet.host());
        VERIFY_ARE_EQUAL(80, telnet.port());
        VERIFY_ARE_EQUAL(U("/"), telnet.path());
        VERIFY_ARE_EQUAL(U(""), telnet.query());
        VERIFY_ARE_EQUAL(U(""), telnet.fragment());
    }

    TEST(user_info_string)
    {
        uri ftp(U("ftp://johndoe:testname@ftp.is.co.za/rfc/rfc1808.txt"));
        VERIFY_ARE_EQUAL(U("ftp"), ftp.scheme());
        VERIFY_ARE_EQUAL(U("johndoe:testname"), ftp.user_info());
        VERIFY_ARE_EQUAL(U("ftp.is.co.za"), ftp.host());
        VERIFY_ARE_EQUAL(0, ftp.port());
        VERIFY_ARE_EQUAL(U("/rfc/rfc1808.txt"), ftp.path());
        VERIFY_ARE_EQUAL(U(""), ftp.query());
        VERIFY_ARE_EQUAL(U(""), ftp.fragment());
    }

    // Test query component can be separated with '&' or ';'.
    TEST(query_seperated_with_semi_colon)
    {
        uri u(U("http://localhost/path1?key1=val1;key2=val2"));
        VERIFY_ARE_EQUAL(U("key1=val1;key2=val2"), u.query());
    }

} // SUITE(constructor_tests)

} // namespace uri_tests
} // namespace functional
} // namespace tests
