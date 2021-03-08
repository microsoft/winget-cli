/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * request_relative_uri_tests.cpp
 *
 * Tests cases the combinations of base uri and relative uri with incoming requests to the http_listener.
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
SUITE(request_relative_uri_tests)
{
    TEST_FIXTURE(uri_address, empty_base_uri)
    {
        // listen on empty, request /path1/path2
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client client(m_uri);
        VERIFY_ARE_EQUAL(0, client.open());
        listener.support([](http_request request) {
            VERIFY_ARE_EQUAL(U("/path1/path2"), request.request_uri().path());
            VERIFY_ARE_EQUAL(U("/path1/path2"), request.relative_uri().to_string());
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, client.request(methods::GET, U("/path1/path2")));
        client.next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, client.close());

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, nested_paths)
    {
        // listen on /path1, request /path1/path2
        http_listener listener(web::http::uri_builder(m_uri).append_path(U("/path1")).to_uri());
        listener.open().wait();
        test_http_client client(m_uri);
        VERIFY_ARE_EQUAL(0, client.open());
        listener.support([](http_request request) {
            VERIFY_ARE_EQUAL(U("/path1/path2"), request.request_uri().path());
            VERIFY_ARE_EQUAL(U("/path2"), request.relative_uri().to_string());
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, client.request(methods::GET, U("/path1/path2")));
        client.next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, client.close());

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, nested_paths_encoding)
    {
        // listen on /path1%20/path2%20, request /path1%20/path2%20/path%203
        http_listener listener(web::http::uri_builder(m_uri).append_path(U("/path1%20/path2%20")).to_uri());
        listener.open().wait();
        test_http_client client(m_uri);
        VERIFY_ARE_EQUAL(0, client.open());
        listener.support([](http_request request) {
            VERIFY_ARE_EQUAL(U("/path1%20/path2%20/path3%20"), request.request_uri().path());
            VERIFY_ARE_EQUAL(U("/path3 "), web::http::uri::decode(request.relative_uri().to_string()));
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, client.request(methods::GET, U("/path1%20/path2%20/path3%20")));
        client.next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, client.close());

        listener.close().wait();
    }

    TEST(listener_uri_empty_path)
    {
        uri address(U("http://localhost:45678"));
        http_listener listener(address);
        listener.open().wait();
        test_http_client::scoped_client client(address);
        test_http_client* p_client = client.client();

        listener.support([](http_request request) { request.reply(status_codes::OK); });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    TEST(listener_invalid_encoded_uri)
    {
        uri address(U("http://localhost:45678"));
        http_listener listener(address);
        listener.open().wait();
        test_http_client::scoped_client client(address);
        test_http_client* p_client = client.client();

        listener.support([](http_request request) { request.reply(status_codes::OK); });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/%invalid/uri")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::BadRequest);
            })
            .wait();

        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
