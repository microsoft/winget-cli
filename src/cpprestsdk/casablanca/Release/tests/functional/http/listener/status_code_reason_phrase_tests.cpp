/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * status_code_reason_phrase_tests.cpp
 *
 * Tests cases for using HTTP status codes and reason phrases with http_listener.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

using namespace web::http;
using namespace web::http::experimental::listener;

using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace listener
{
SUITE(status_code_reason_phrase_tests)
{
    TEST_FIXTURE(uri_address, status_codes)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // known status code
        listener.support([&](http_request request) { request.reply(status_codes::Conflict).wait(); });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::Conflict);
            })
            .wait();

        // user defined status code
        listener.support([&](http_request request) { request.reply(867).wait(); });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::PUT, U("")));
        p_client->next_response()
            .then([](test_response* p_response) { http_asserts::assert_test_response_equals(p_response, 867); })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, reason_phrase)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // standard status code, no reason phrase
        listener.support([](http_request request) { request.reply(status_codes::NotModified).wait(); });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::PUT, U("")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::NotModified);
                VERIFY_ARE_EQUAL(U("Not Modified"), p_response->m_reason_phrase);
            })
            .wait();

        // standard status code, with reason phrase
        listener.support([](http_request request) {
            http_response response(status_codes::NotModified);
            response.set_reason_phrase(U("Custom"));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::PUT, U("")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::NotModified);
                VERIFY_ARE_EQUAL(U("Custom"), p_response->m_reason_phrase);
            })
            .wait();

        // non standard status code, no reason phrase
        listener.support([](http_request request) { request.reply(987); });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::PUT, U("")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, 987);
                VERIFY_ARE_EQUAL(U(""), p_response->m_reason_phrase);
            })
            .wait();

        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
