/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * request_extract_tests.cpp
 *
 * Tests cases for covering calling extract_ overloads on HTTP request.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
using namespace utility::conversions;

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
SUITE(request_extract_tests)
{
    TEST_FIXTURE(uri_address, extract_string)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();
        std::string data("HEHEHE");

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, U("PUT"), U("/"), to_string_t(data));
            VERIFY_ARE_EQUAL(U("text/plain"), request.headers().content_type());
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U(""), U("text/plain"), data));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, extract_string_force)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();
        std::string data("HEHEHE");

        listener.support([&](http_request request) {
            VERIFY_ARE_EQUAL(to_string_t(data), request.extract_string(true).get());
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U(""), U("unknown charset"), data));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, extract_json)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        json::value j(true);
        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, U("PUT"), U("/"));
            VERIFY_ARE_EQUAL(U("application/json"), request.headers().content_type());
            const json::value j_found = request.extract_json().get();
            VERIFY_ARE_EQUAL(j.serialize(), j_found.serialize());
            request.reply(status_codes::OK);
        });
        std::string data = to_utf8string(j.serialize());
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U(""), U("application/json"), data));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, extract_json_force)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        json::value j(true);
        listener.support([&](http_request request) {
            const json::value j_found = request.extract_json(true).get();
            VERIFY_ARE_EQUAL(j.serialize(), j_found.serialize());
            request.reply(status_codes::OK);
        });
        std::string data = to_utf8string(j.serialize());
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U(""), U("unknown charset"), data));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, empty_vector)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();
        std::string data("");

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, U("PUT"), U("/"));
            VERIFY_ARE_EQUAL(U("text/plain"), request.headers().content_type());
            std::vector<unsigned char> vec = request.extract_vector().get();
            VERIFY_ARE_EQUAL(vec.size(), 0);
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U(""), U("text/plain"), data));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, extract_vector)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();
        std::string data("HEHEHE");

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, U("PUT"), U("/"));
            VERIFY_ARE_EQUAL(U("text/plain"), request.headers().content_type());
            std::vector<unsigned char> vec = request.extract_vector().get();
            VERIFY_ARE_EQUAL(vec.size(), data.size());
            VERIFY_ARE_EQUAL('H', vec[0]);
            VERIFY_ARE_EQUAL('E', vec[1]);
            VERIFY_ARE_EQUAL('H', vec[2]);
            VERIFY_ARE_EQUAL('E', vec[3]);
            VERIFY_ARE_EQUAL('H', vec[4]);
            VERIFY_ARE_EQUAL('E', vec[5]);
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U(""), U("text/plain"), data));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
