/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * header_tests.cpp
 *
 * Tests cases for using HTTP requests/response headers with http_listener.
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
SUITE(header_tests)
{
    TEST_FIXTURE(uri_address, request_headers)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();
        const utility::string_t mtd = methods::GET;
        std::map<utility::string_t, utility::string_t> headers;

        // single header value.
        headers[U("Header1")] = U("Value1");
        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, mtd, U("/"), headers);
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(mtd, U(""), headers));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        // empty header value.
        headers.clear();
        headers[U("Key1")] = U("");
        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, mtd, U("/"), headers);
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(mtd, U(""), headers));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        // 10 headers.
        headers.clear();
        headers[U("MyHeader")] = U("hehe;blach");
        headers[U("Yo1")] = U("You, Too");
        headers[U("Yo2")] = U("You2");
        headers[U("Yo3")] = U("You3");
        headers[U("Yo4")] = U("You4");
        headers[U("Yo5")] = U("You5");
        headers[U("Yo6")] = U("You6");
        headers[U("Yo7")] = U("You7");
        headers[U("Yo8")] = U("You8");
        headers[U("Yo9")] = U("You9");
        headers[U("Yo10")] = U("You10");
        headers[U("Yo11")] = U("You11");
        headers[U("Accept")] = U("text/plain");
        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, mtd, U("/"), headers);
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(mtd, U(""), headers));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        // several headers different casings.
        headers.clear();
        headers[U("CUSTOMHEADER")] = U("value1");
        headers[U("customHEADER")] = U("value2");
        headers[U("CUSTOMheaDER")] = U("value3");
        listener.support([&](http_request request) {
            std::map<utility::string_t, utility::string_t> h;
            h[U("CUSTOMHEADER")] = U("value1, value3, value2");
            http_asserts::assert_request_equals(request, mtd, U("/"), h);
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(mtd, U(""), headers));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, request_known_headers)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();
        const utility::string_t mtd = methods::GET;
        std::map<utility::string_t, utility::string_t> headers;

        // "Date" was being incorrectly mapped to "Data"
        // see https://github.com/microsoft/cpprestsdk/issues/1208
        headers[U("Date")] = U("Mon, 29 Jul 2019 12:32:57 GMT");
        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, mtd, U("/"), headers);
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(mtd, U(""), headers));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, response_headers)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // No http_request/response classes can be around for close to complete.
        {
            // header with empty value
            http_response response(status_codes::OK);
            response.headers()[U("Key1")] = U("");
            listener.support([&](http_request request) {
                http_asserts::assert_request_equals(request, methods::POST, U("/"));
                request.reply(response).wait();
            });
            VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
            p_client->next_response()
                .then([&](test_response* p_response) {
                    http_asserts::assert_test_response_equals(p_response, status_codes::OK, response.headers());
                })
                .wait();

            // 10 headers
            response = http_response(status_codes::Accepted);
            response.headers()[U("MyHeader")] = U("hehe;blach");
            response.headers()[U("Yo1")] = U("You, Too");
            response.headers()[U("Yo2")] = U("You2");
            response.headers()[U("Yo3")] = U("You3");
            response.headers()[U("Yo4")] = U("You4");
            response.headers()[U("Yo5")] = U("You5");
            response.headers()[U("Yo6")] = U("You6");
            response.headers()[U("Yo7")] = U("You7");
            response.headers()[U("Yo8")] = U("You8");
            response.headers()[U("Yo9")] = U("You9");
            response.headers()[U("Yo10")] = U("You10");
            response.headers()[U("Yo11")] = U("You11");
            response.headers()[U("Accept")] = U("text/plain");
            listener.support([&](http_request request) {
                http_asserts::assert_request_equals(request, methods::POST, U("/"));
                request.reply(response).wait();
            });
            VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
            p_client->next_response()
                .then([&](test_response* p_response) {
                    http_asserts::assert_test_response_equals(p_response, status_codes::Accepted, response.headers());
                })
                .wait();

            // several headers in different casings
            response = http_response(status_codes::BadGateway);
            response.headers().add(U("Key1"), U("value1"));
            response.headers()[U("KEY1")] += U("value2");
            listener.support([&](http_request request) {
                http_asserts::assert_request_equals(request, methods::POST, U("/"));
                request.reply(response).wait();
            });
            VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
            p_client->next_response()
                .then([&](test_response* p_response) {
                    http_asserts::assert_test_response_equals(p_response, status_codes::BadGateway, response.headers());
                })
                .wait();

            // duplicate headers fields
            response = http_response(status_codes::BadGateway);
            response.headers().add(U("Key1"), U("value1"));
            response.headers().add(U("Key1"), U("value2"));
            listener.support([&](http_request request) {
                http_asserts::assert_request_equals(request, methods::POST, U("/"));
                request.reply(response).wait();
            });
            VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
            p_client->next_response()
                .then([&](test_response* p_response) {
                    http_asserts::assert_test_response_equals(p_response, status_codes::BadGateway, response.headers());
                })
                .wait();
        }

        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
