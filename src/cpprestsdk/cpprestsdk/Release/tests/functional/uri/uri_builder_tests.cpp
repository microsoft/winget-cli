/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests for the URI builder class.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include <locale_guard.h>

using namespace web;
using namespace utility;

namespace tests
{
namespace functional
{
namespace uri_tests
{
// Helper functions to verify components of a builder.
static void VERIFY_URI_BUILDER(uri_builder& builder,
                               const utility::string_t& scheme,
                               const utility::string_t& user_info,
                               const utility::string_t& host,
                               const int port,
                               const utility::string_t& path,
                               const utility::string_t& query,
                               const utility::string_t& fragment)
{
    VERIFY_ARE_EQUAL(scheme, builder.scheme());
    VERIFY_ARE_EQUAL(host, builder.host());
    VERIFY_ARE_EQUAL(user_info, builder.user_info());
    VERIFY_ARE_EQUAL(port, builder.port());
    VERIFY_ARE_EQUAL(path, builder.path());
    VERIFY_ARE_EQUAL(query, builder.query());
    VERIFY_ARE_EQUAL(fragment, builder.fragment());
}
static void VERIFY_URI_BUILDER(uri_builder& builder,
                               const utility::string_t& scheme,
                               const utility::string_t& host,
                               const int port)
{
    VERIFY_URI_BUILDER(builder,
                       scheme,
                       utility::string_t(),
                       host,
                       port,
                       utility::string_t(U("/")),
                       utility::string_t(),
                       utility::string_t());
}
static void VERIFY_URI_BUILDER_IS_EMPTY(uri_builder& builder)
{
    VERIFY_URI_BUILDER(builder,
                       utility::string_t(),
                       utility::string_t(),
                       utility::string_t(),
                       -1,
                       utility::string_t(U("/")),
                       utility::string_t(),
                       utility::string_t());
}

SUITE(uri_builder_tests)
{
    TEST(constructor_tests)
    {
        // Default constructor
        uri_builder builder;
        VERIFY_URI_BUILDER_IS_EMPTY(builder);
        // scheme, user_info, host, port
        utility::string_t scheme(U("ftp"));
        utility::string_t user_info(U("steve:pass"));
        utility::string_t host(U("localhost"));
        int port = 44;
        utility::string_t path(U("/Yeshere888"));
        utility::string_t uri_str(U("ftp://steve:pass@localhost:44/Yeshere888"));

        // utility::string_t
        utility::string_t uri_wstr(U("ftp://steve:pass@localhost:44/Yeshere888?abc:123&abc2:456#nose"));
        builder = uri_builder(uri_wstr);
        VERIFY_URI_BUILDER(builder,
                           scheme,
                           user_info,
                           host,
                           port,
                           path,
                           utility::string_t(U("abc:123&abc2:456")),
                           utility::string_t(U("nose")));

        // copy constructor
        uri_builder other(builder);
        builder = uri_builder(uri_str);
        VERIFY_URI_BUILDER(other,
                           scheme,
                           user_info,
                           host,
                           port,
                           path,
                           utility::string_t(U("abc:123&abc2:456")),
                           utility::string_t(U("nose")));
        VERIFY_URI_BUILDER(builder, scheme, user_info, host, port, path, U(""), U(""));

        // move constructor
        uri_builder move_other = std::move(builder);
        VERIFY_URI_BUILDER(move_other, scheme, user_info, host, port, path, U(""), U(""));
    }

    TEST(assignment_operators)
    {
        // assignment operator
        const utility::string_t scheme = U("http"), host = U("localhost");
        const int port = 44;
        uri_builder original;
        original.set_scheme(scheme).set_host(host).set_port(port);
        uri_builder assign;
        assign = original;
        VERIFY_URI_BUILDER(assign, scheme, utility::string_t(host), port);

        // move assignment operator
        uri_builder move_assign;
        move_assign = std::move(original);
        VERIFY_URI_BUILDER(assign, scheme, utility::string_t(host), port);
    }

    TEST(set_port_as_string)
    {
        uri_builder builder;

        VERIFY_THROWS(builder.set_port(U("")), std::invalid_argument);
        VERIFY_ARE_EQUAL(-1, builder.port());

        builder.set_port(U("987"));
        VERIFY_ARE_EQUAL(987, builder.port());

        VERIFY_THROWS(builder.set_port(U("abc")), std::invalid_argument);
        VERIFY_ARE_EQUAL(987, builder.port());

        builder.set_port(U(" 44 "));
        VERIFY_ARE_EQUAL(44, builder.port());

        builder.set_port(U("99"));
        VERIFY_ARE_EQUAL(99, builder.port());
    }

    TEST(component_assignment)
    {
        uri_builder builder;
        const utility::string_t scheme(U("myscheme"));
        const utility::string_t uinfo(U("johndoe:test"));
        const utility::string_t host(U("localhost"));
        const int port = 88;
        const utility::string_t path(U("jklajsd"));
        const utility::string_t query(U("key1=val1"));
        const utility::string_t fragment(U("last"));

        builder.set_scheme(scheme);
        builder.set_user_info(uinfo);
        builder.set_host(host);
        builder.set_port(port);
        builder.set_path(path);
        builder.set_query(query);
        builder.set_fragment(fragment);

        VERIFY_URI_BUILDER(builder, scheme, uinfo, host, port, path, query, fragment);
    }

    TEST(component_assignment_encode)
    {
        {
            uri_builder builder;
            const utility::string_t scheme(U("myscheme"));
            const utility::string_t uinfo(U("johndoe:test"));
            const utility::string_t host(U("localhost"));
            const int port = 88;
            const utility::string_t path(U("jklajsd/yes no"));
            const utility::string_t query(U("key1=va%l1"));
            const utility::string_t fragment(U("las t"));

            builder.set_scheme(scheme);
            builder.set_user_info(uinfo, true);
            builder.set_host(host, true);
            builder.set_port(port);
            builder.set_path(path, true);
            builder.set_query(query, true);
            builder.set_fragment(fragment, true);

            VERIFY_URI_BUILDER(builder,
                               scheme,
                               utility::string_t(U("johndoe:test")),
                               utility::string_t(U("localhost")),
                               port,
                               utility::string_t(U("jklajsd/yes%20no")),
                               utility::string_t(U("key1=va%25l1")),
                               utility::string_t(U("las%20t")));
        }
        {
            uri_builder builder;
            const utility::string_t scheme(U("myscheme"));
            const utility::string_t uinfo(U("johndoe:test"));
            const utility::string_t host(U("localhost"));
            const int port = 88;
            const utility::string_t path(U("jklajsd/yes no"));
            const utility::string_t query(U("key1=va%l1"));
            const utility::string_t fragment(U("las t"));

            builder.set_scheme(scheme);
            builder.set_user_info(uinfo, true);
            builder.set_host(host, true);
            builder.set_port(port);
            builder.set_path(path, true);
            builder.set_query(query, true);
            builder.set_fragment(fragment, true);

            VERIFY_URI_BUILDER(builder,
                               scheme,
                               utility::string_t(U("johndoe:test")),
                               utility::string_t(U("localhost")),
                               port,
                               utility::string_t(U("jklajsd/yes%20no")),
                               utility::string_t(U("key1=va%25l1")),
                               utility::string_t(U("las%20t")));
        }
    }

    TEST(validation)
    {
        {
            // true
            uri_builder builder(U("http://localhost:4567/"));
            VERIFY_IS_TRUE(builder.is_valid());

            // false
            builder = uri_builder();
            builder.set_scheme(U("123"));
            VERIFY_IS_FALSE(builder.is_valid());
        }
        {
            // true
            uri_builder builder(U("http://localhost:4567/"));
            VERIFY_IS_TRUE(builder.is_valid());

            // false
            builder = uri_builder();
            builder.set_scheme(U("123"));
            VERIFY_IS_FALSE(builder.is_valid());
        }
    }

    TEST(uri_creation_string)
    {
        utility::string_t uri_str(U("http://steve:temp@localhost:4556/"));

        // to_string
        uri_builder builder(uri_str);
        VERIFY_ARE_EQUAL(uri_str, builder.to_string());

        // to_string
        VERIFY_ARE_EQUAL(uri_str, builder.to_string());

        // to uri
        VERIFY_ARE_EQUAL(uri_str, builder.to_uri().to_string());

        // to encoded string
        uri_builder with_space(builder);
        with_space.set_path(utility::string_t(U("path%20with%20space")));
        VERIFY_ARE_EQUAL(U("http://steve:temp@localhost:4556/path%20with%20space"), with_space.to_string());
    }

    TEST(append_path_string)
    {
        // empty uri builder path
        uri_builder builder;
        builder.append_path(U("/path1"));
        VERIFY_ARE_EQUAL(U("/path1"), builder.path());

        // empty append path
        builder.append_path(U(""));
        VERIFY_ARE_EQUAL(U("/path1"), builder.path());

        // uri builder with slash
        builder.append_path(U("/"));
        builder.append_path(U("path2"));
        VERIFY_ARE_EQUAL(U("/path1/path2"), builder.path());

        // both with slash
        builder.append_path(U("/"));
        builder.append_path(U("/path3"));
        VERIFY_ARE_EQUAL(U("/path1/path2/path3"), builder.path());

        // both without slash
        builder.append_path(U("path4"));
        VERIFY_ARE_EQUAL(U("/path1/path2/path3/path4"), builder.path());

        // encoding
        builder.clear();
        builder.append_path(U("encode%things"));
        VERIFY_ARE_EQUAL(U("/encode%things"), builder.path());

        builder.clear();
        builder.append_path(U("encode%things"), false);
        VERIFY_ARE_EQUAL(U("/encode%things"), builder.path());

        builder.clear();
        builder.append_path(U("encode%things"), true);
        VERIFY_ARE_EQUAL(U("/encode%25things"), builder.path());

        // self references
        builder.set_path(U("example"));
        builder.append_path(builder.path());
        VERIFY_ARE_EQUAL(U("example/example"), builder.path());

        builder.set_path(U("/example"));
        builder.append_path(builder.path());
        VERIFY_ARE_EQUAL(U("/example/example"), builder.path());

        builder.set_path(U("/example/"));
        builder.append_path(builder.path());
        VERIFY_ARE_EQUAL(U("/example/example/"), builder.path());
    }

    TEST(append_path_raw_string)
    {
        // empty uri builder path
        uri_builder builder;
        builder.append_path_raw(U("path1"));
        VERIFY_ARE_EQUAL(U("/path1"), builder.path());

        // empty append path
        builder.append_path_raw(U(""));
        VERIFY_ARE_EQUAL(U("/path1"), builder.path());

        // uri builder with slash
        builder.append_path_raw(U("/"));
        builder.append_path_raw(U("path2"));
        VERIFY_ARE_EQUAL(U("/path1///path2"), builder.path());

        // leading slash (should result in "//")
        builder.append_path_raw(U("/path3"));
        VERIFY_ARE_EQUAL(U("/path1///path2//path3"), builder.path());

        // trailing slash
        builder.append_path_raw(U("path4/"));
        builder.append_path_raw(U("path5"));
        VERIFY_ARE_EQUAL(U("/path1///path2//path3/path4//path5"), builder.path());

        // encoding
        builder.clear();
        builder.append_path_raw(U("encode%things"));
        VERIFY_ARE_EQUAL(U("/encode%things"), builder.path());

        builder.clear();
        builder.append_path_raw(U("encode%things"), false);
        VERIFY_ARE_EQUAL(U("/encode%things"), builder.path());

        builder.clear();
        builder.append_path_raw(U("encode%things"), true);
        VERIFY_ARE_EQUAL(U("/encode%25things"), builder.path());

        // self references
        builder.set_path(U("example"));
        builder.append_path_raw(builder.path());
        VERIFY_ARE_EQUAL(U("example/example"), builder.path());

        builder.set_path(U("/example"));
        builder.append_path_raw(builder.path());
        VERIFY_ARE_EQUAL(U("/example//example"), builder.path());

        builder.set_path(U("/example/"));
        builder.append_path_raw(builder.path());
        VERIFY_ARE_EQUAL(U("/example///example/"), builder.path());
    }

    TEST(append_query_string)
    {
        // empty uri builder query
        uri_builder builder;
        builder.append_query(U("key1=value1"));
        VERIFY_ARE_EQUAL(U("key1=value1"), builder.query());

        // empty append query
        builder.append_query(U(""));
        VERIFY_ARE_EQUAL(U("key1=value1"), builder.query());

        // uri builder with ampersand
        builder.append_query(U("&"));
        builder.append_query(U("key2=value2"));
        VERIFY_ARE_EQUAL(U("key1=value1&key2=value2"), builder.query());

        // both with ampersand
        builder.append_query(U("&"));
        builder.append_query(U("&key3=value3"));
        VERIFY_ARE_EQUAL(U("key1=value1&key2=value2&key3=value3"), builder.query());

        // both without ampersand
        builder.append_query(U("key4=value4"));
        VERIFY_ARE_EQUAL(U("key1=value1&key2=value2&key3=value3&key4=value4"), builder.query());

        // number query
        builder.append_query(U("key5"), 1);
        VERIFY_ARE_EQUAL(U("key1=value1&key2=value2&key3=value3&key4=value4&key5=1"), builder.query());

        // string query
        builder.append_query(U("key6"), U("val6"));
        VERIFY_ARE_EQUAL(U("key1=value1&key2=value2&key3=value3&key4=value4&key5=1&key6=val6"), builder.query());

        // key and value separate with '=', '&', and ';'
        builder.append_query(U("key=&;"), U("=&;value"));
        VERIFY_ARE_EQUAL(
            U("key1=value1&key2=value2&key3=value3&key4=value4&key5=1&key6=val6&key%3D%26%3B=%3D%26%3Bvalue"),
            builder.query());

        // self references
        builder.set_query(U("example"));
        builder.append_query(builder.query());
        VERIFY_ARE_EQUAL(U("example&example"), builder.query());

        builder.set_query(U("&example"));
        builder.append_query(builder.query());
        VERIFY_ARE_EQUAL(U("&example&example"), builder.query());

        builder.set_query(U("&example&"));
        builder.append_query(builder.query());
        VERIFY_ARE_EQUAL(U("&example&example&"), builder.query());
    }

    TEST(append_query_string_no_encode)
    {
        uri_builder builder;
        builder.append_query(U("key=&;"), U("=&;value"), false);
        VERIFY_ARE_EQUAL(U("key=&;==&;value"), builder.query());
    }

    TEST(append_string)
    {
        // with just path
        uri_builder builder;
        builder.append(U("/path1"));
        VERIFY_ARE_EQUAL(U("/path1"), builder.path());

        // with just query
        builder.append(U("?key1=value1"));
        VERIFY_ARE_EQUAL(U("/path1"), builder.path());
        VERIFY_ARE_EQUAL(U("key1=value1"), builder.query());
        VERIFY_ARE_EQUAL(U("/path1?key1=value1"), builder.to_string());

        // with just fragment
        builder.append(U("#fragment"));
        VERIFY_ARE_EQUAL(U("/path1"), builder.path());
        VERIFY_ARE_EQUAL(U("key1=value1"), builder.query());
        VERIFY_ARE_EQUAL(U("fragment"), builder.fragment());
        VERIFY_ARE_EQUAL(U("/path1?key1=value1#fragment"), builder.to_string());

        // with all
        builder.append(U("/path2?key2=value2#frag2"));
        VERIFY_ARE_EQUAL(U("/path1/path2"), builder.path());
        VERIFY_ARE_EQUAL(U("key1=value1&key2=value2"), builder.query());
        VERIFY_ARE_EQUAL(U("fragmentfrag2"), builder.fragment());
        VERIFY_ARE_EQUAL(U("/path1/path2?key1=value1&key2=value2#fragmentfrag2"), builder.to_string());
    }

    TEST(append_empty_string)
    {
        utility::string_t uri_str(U("http://uribuilder.com/"));
        uri_builder builder(uri_str);
        builder.append(U(""));

        VERIFY_ARE_EQUAL(builder.to_string(), uri_str);
    }

    TEST(append_path_encoding)
    {
        uri_builder builder;
        builder.append_path(U("/path space"), true);
        VERIFY_ARE_EQUAL(U("/path%20space"), builder.path());

        builder.append_path(U("path2"));
        VERIFY_ARE_EQUAL(U("/path%20space/path2"), builder.path());
    }

    TEST(append_query_encoding)
    {
        uri_builder builder;
        builder.append_query(U("key1 =value2"), true);
        VERIFY_ARE_EQUAL(U("key1%20=value2"), builder.query());

        builder.append_query(U("key2=value3"));
        VERIFY_ARE_EQUAL(U("key1%20=value2&key2=value3"), builder.query());
    }

    TEST(append_encoding)
    {
        uri_builder builder;
        builder.append(uri::encode_uri(U("path space?key =space#frag space")));
        VERIFY_ARE_EQUAL(U("/path%20space"), builder.path());
        VERIFY_ARE_EQUAL(U("key%20=space"), builder.query());
        VERIFY_ARE_EQUAL(U("frag%20space"), builder.fragment());
        VERIFY_ARE_EQUAL(U("/path%20space?key%20=space#frag%20space"), builder.to_string());

        // try with encoded_string
        builder = uri_builder();
        builder.append(U("/path2?key2=value2#frag2"));
        VERIFY_ARE_EQUAL(U("/path2"), builder.path());
        VERIFY_ARE_EQUAL(U("key2=value2"), builder.query());
        VERIFY_ARE_EQUAL(U("frag2"), builder.fragment());
        VERIFY_ARE_EQUAL(U("/path2?key2=value2#frag2"), builder.to_string());
    }

    TEST(host_encoding)
    {
        // Check that ASCII characters that are invalid in a host name
        // do not get percent-encoded.

        uri_builder ub1;
        ub1.set_scheme(U("http")).set_host(U("????dfasddsf!@#$%^&*()_+")).set_port(80);

        VERIFY_IS_FALSE(ub1.is_valid());
    }

    TEST(clear)
    {
        uri_builder ub;
        ub.clear();
        CHECK(ub.scheme() == U(""));
        CHECK(ub.user_info() == U(""));
        CHECK(ub.host() == U(""));
        CHECK(ub.port() == -1);
        CHECK(ub.path() == U("/"));
        CHECK(ub.query() == U(""));
        CHECK(ub.fragment() == U(""));

        ub = uri_builder(U("http://myhost.com/path1"));
        ub.append_path(U("path2"));
        uri u = ub.to_uri();
        ub.clear();
        CHECK(ub.scheme() == U(""));
        CHECK(ub.user_info() == U(""));
        CHECK(ub.host() == U(""));
        CHECK(ub.port() == -1);
        CHECK(ub.path() == U("/"));
        CHECK(ub.query() == U(""));
        CHECK(ub.fragment() == U(""));
        CHECK(u.to_string() == U("http://myhost.com/path1/path2"));

        ub.append_path(U("path3"));
        ub.set_host(U("hahah"));
        ub.set_fragment(U("No"));
        ub.clear();
        CHECK(ub.scheme() == U(""));
        CHECK(ub.user_info() == U(""));
        CHECK(ub.host() == U(""));
        CHECK(ub.port() == -1);
        CHECK(ub.path() == U("/"));
        CHECK(ub.query() == U(""));
        CHECK(ub.fragment() == U(""));
    }

    TEST(to_string_invalid_uri)
    {
        uri_builder builder(U("http://invaliduri.com"));
        builder.set_scheme(U("1http"));
        VERIFY_THROWS(builder.to_string(), uri_exception);
        VERIFY_THROWS(builder.to_uri(), uri_exception);

        builder.set_scheme(U("ht*ip"));
        VERIFY_THROWS(builder.to_string(), uri_exception);

        builder.set_scheme(U("htt%20p"));
        VERIFY_THROWS(builder.to_string(), uri_exception);
    }

    TEST(append_query_locale, "Ignore:Android", "Locale unsupported on Android")
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

        uri_builder builder;
        auto const& key = U("key1000");
        builder.append_query(key, 1000);
        ::utility::string_t expected(key);
        expected.append(U("=1000"));
        VERIFY_ARE_EQUAL(expected, builder.query());
    }

    TEST(github_crash_994) { web::uri uri(U("http://127.0.0.1:34568/")); }

} // SUITE(uri_builder_tests)

} // namespace uri_tests
} // namespace functional
} // namespace tests
