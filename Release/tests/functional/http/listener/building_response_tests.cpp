/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * building_response_tests.cpp
 *
 * Tests cases for manually building up HTTP responses with http_listener.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

using namespace web;
using namespace utility;
using namespace web::http;
using namespace web::http::experimental::listener;

using namespace tests::common::utilities;
using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace listener
{
SUITE(building_response_tests)
{
    TEST_FIXTURE(uri_address, set_body_with_content_type)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            http_response response(status_codes::OK);
            response.set_body(U("test string"), U("text"));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
#ifdef _UTF16_STRINGS
                const ::utility::string_t expectedContentType(U("text; charset=utf-8"));
#else
                const ::utility::string_t expectedContentType(U("text"));
#endif
                http_asserts::assert_test_response_equals(
                    p_response, status_codes::OK, expectedContentType, U("test string"));
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, set_body_without_content_type)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        listener.support([&](http_request request) {
            http_response response(status_codes::OK);
            response.set_body(U("test string"));
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(
                    p_response, status_codes::OK, U("text/plain; charset=utf-8"), U("test string"));
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, set_body_string)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            http_response response(status_codes::OK);
            utility::string_t data(U("test data"));
            response.set_body(std::move(data));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(
                    p_response, status_codes::OK, U("text/plain; charset=utf-8"), U("test data"));
            })
            .wait();

        listener.close().wait();
    }

    TEST(set_body_string_with_charset)
    {
        http_response response;
        VERIFY_THROWS(response.set_body(::utility::conversions::to_utf16string("body_data"),
                                        ::utility::conversions::to_utf16string("text/plain;charset=utf-16")),
                      std::invalid_argument);
    }

    TEST_FIXTURE(uri_address, set_body_vector)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            http_response response(status_codes::OK);
            std::vector<unsigned char> v_body;
            v_body.push_back('A');
            v_body.push_back('B');
            v_body.push_back('C');
            response.set_body(std::move(v_body));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(
                    p_response, status_codes::OK, U("application/octet-stream"), U("ABC"));
            })
            .wait();

        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
