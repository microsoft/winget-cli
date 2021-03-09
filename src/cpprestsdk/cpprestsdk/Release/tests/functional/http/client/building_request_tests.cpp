/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * building_request_tests.cpp
 *
 * Tests cases manually building up HTTP requests.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#ifdef _WIN32
#include <WinError.h>
#endif

#include <locale_guard.h>

using namespace web::http;
using namespace web::http::client;

using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace client
{
SUITE(building_request_tests)
{
    TEST_FIXTURE(uri_address, simple_values)
    {
        test_http_server::scoped_server scoped(m_uri);
        pplx::task<void> t1, t2;
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        // Set a method.
        const method method = methods::OPTIONS;
        http_request msg(method);
        VERIFY_ARE_EQUAL(method, msg.method());

        // Set a path once.
        const utility::string_t custom_path1 = U("/hey/custom/path");
        msg.set_request_uri(custom_path1);
        VERIFY_ARE_EQUAL(custom_path1, msg.relative_uri().to_string());
        t1 = p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, method, custom_path1);
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // Set the path twice.
        msg = http_request(method);
        msg.set_request_uri(custom_path1);
        VERIFY_ARE_EQUAL(custom_path1, msg.relative_uri().to_string());
        const utility::string_t custom_path2 = U("/yes/you/there");
        msg.set_request_uri(custom_path2);
        VERIFY_ARE_EQUAL(custom_path2, msg.relative_uri().to_string());
        t2 = p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, method, custom_path2);
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
        p_server->close();
        try
        {
            t1.get();
        }
        catch (...)
        {
            VERIFY_ARE_EQUAL(0, 1, "t1 failed");
        }
        try
        {
            t2.get();
        }
        catch (...)
        {
            VERIFY_ARE_EQUAL(0, 2, "t2 failed");
        }
    }

    TEST_FIXTURE(uri_address, body_types)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        // Body data types.
        const method method(U("CUSTOMmethod"));
        utility::string_t str_body(U("YES_BASIC_STRING BODY"));
        utility::string_t str_move_body(str_body);
        std::vector<unsigned char> vector_body;
        vector_body.resize(str_body.size() * sizeof(utility::char_t));
        memcpy(&vector_body[0], &str_body[0], str_body.size() * sizeof(utility::char_t));
        std::vector<unsigned char> vector_move_body(vector_body);
        utility::string_t custom_content = U("YESNOW!");

        // vector - no content type.
        http_request msg(method);
        msg.set_body(std::move(vector_move_body));
        VERIFY_ARE_EQUAL(U("application/octet-stream"), msg.headers()[U("Content-Type")]);
        p_server->next_request().then([&](test_request* p_request) {
            auto received = p_request->m_body;
            auto sent = vector_body;
            VERIFY_IS_TRUE(received == sent);
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // vector - with content type.
        msg = http_request(method);
        vector_move_body = vector_body;
        msg.headers().add(U("Content-Type"), custom_content);
        msg.set_body(std::move(vector_move_body));
        VERIFY_ARE_EQUAL(custom_content, msg.headers()[U("Content-Type")]);
        p_server->next_request().then([&](test_request* p_request) {
            auto received = p_request->m_body;
            auto sent = vector_body;
            VERIFY_IS_TRUE(received == sent);
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // string - no content type.
        msg = http_request(method);
        msg.set_body(std::move(str_move_body));
        VERIFY_ARE_EQUAL(U("text/plain; charset=utf-8"), msg.headers()[U("Content-Type")]);
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(
                p_request, method, U("/"), U("text/plain; charset=utf-8"), str_body);
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // string - with content type.
        msg = http_request(method);
        str_move_body = str_body;
        msg.headers().add(U("Content-Type"), custom_content);
        msg.set_body(std::move(str_move_body));
        VERIFY_ARE_EQUAL(custom_content, msg.headers()[U("Content-Type")]);
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, method, U("/"), custom_content, str_body);
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
    }

    TEST(set_body_string_with_charset)
    {
        http_request request;
        VERIFY_THROWS(request.set_body(::utility::conversions::to_utf16string("body_data"),
                                       ::utility::conversions::to_utf16string("text/plain;charset=utf-16")),
                      std::invalid_argument);
    }

    TEST_FIXTURE(uri_address, empty_bodies)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        // Body data.
        std::string empty_str;
        std::vector<unsigned char> vector_body;
        utility::string_t str_body;
        utility::string_t wstr_body;

        // empty vector.
        const method method(methods::PUT);
        http_request msg(method);
        msg.set_body(std::move(vector_body));
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, method, U("/"), U("application/octet-stream"));
            VERIFY_ARE_EQUAL(0u, p_request->m_body.size());
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // empty string.
        msg = http_request(method);
        msg.set_body(std::move(str_body));
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, method, U("/"), U("text/plain; charset=utf-8"));
            VERIFY_ARE_EQUAL(0u, p_request->m_body.size());
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // empty wstring.
        msg = http_request(method);
        msg.set_body(std::move(wstr_body));
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, method, U("/"), U("text/plain; charset=utf-8"));
            VERIFY_ARE_EQUAL(0u, p_request->m_body.size());
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
    }

    TEST_FIXTURE(uri_address, set_body)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);
        const method mtd = methods::POST;
        utility::string_t data(U("YOU KNOW~!!!!!"));
        utility::string_t content_type = U("text/plain; charset=utf-8");

        // without content type
        http_request msg(mtd);
        msg.set_body(data);
        VERIFY_ARE_EQUAL(content_type, msg.headers()[U("Content-Type")]);
        scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, mtd, U("/"), content_type, data);
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // with content type
        content_type = U("YESYES");
#ifdef _UTF16_STRINGS
        const utility::string_t expected_content_type = U("YESYES; charset=utf-8");
#else
        const utility::string_t expected_content_type = U("YESYES");
#endif
        msg = http_request(mtd);
        msg.set_body(data, content_type);
        VERIFY_ARE_EQUAL(expected_content_type, msg.headers()[U("Content-Type")]);
        scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, mtd, U("/"), expected_content_type, data);
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
    }

    TEST_FIXTURE(uri_address, set_body_with_charset)
    {
        http_request msg(methods::PUT);
        msg.set_body("datadatadata", "text/plain;charset=us-ascii");
        VERIFY_THROWS(msg.set_body(::utility::conversions::to_utf16string("datadatadata"),
                                   ::utility::conversions::to_utf16string("text/plain;charset=us-ascii")),
                      std::invalid_argument);
    }

    TEST_FIXTURE(uri_address, set_content_length_locale, "Ignore:Android", "Locale unsupported on Android")
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
            // Silently pass if locale isn't installed on the machine.
            return;
        }

        tests::common::utilities::locale_guard loc(changedLocale);

        http_request req(methods::PUT);
        req.headers().set_content_length(1000);
        VERIFY_ARE_EQUAL(U("1000"), req.headers()[web::http::header_names::content_length]); // fr_RF would have 1 000
    }

    TEST_FIXTURE(uri_address, set_port_locale, "Ignore:Android", "Locale unsupported on Android")
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

        test_http_server::scoped_server scoped(m_uri);
        pplx::task<void> t;
        http_client client(m_uri);

        utility::string_t data(U("STRING data 1000"));
        t = scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(
                p_request, methods::PUT, U("/"), U("text/plain; charset=utf-8"), data);
            p_request->reply(200);
        });

        http_request msg(methods::PUT);
        msg.set_body(data);
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        scoped.server()->close();
        t.get();
    }

    TEST_FIXTURE(uri_address, reuse_request)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        http_request msg(methods::GET);
        for (int i = 0; i < 3; ++i)
        {
            p_server->next_request().then([](test_request* p_request) {
                http_asserts::assert_test_request_equals(p_request, methods::GET, U("/"));
                p_request->reply(200);
            });
            http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
        }
    }
}

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
