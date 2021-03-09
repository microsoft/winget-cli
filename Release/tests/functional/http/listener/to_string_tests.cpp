/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * to_string_tests.cpp
 *
 * Tests cases for to_string on HTTP requests/responses with http_listener.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

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
SUITE(to_string_tests)
{
    TEST_FIXTURE(uri_address, response_to_string)
    {
        // to string
        http_response resp(status_codes::PartialContent);
        resp.set_body(U("data"));
        VERIFY_ARE_EQUAL(U("HTTP/1.1 206 Partial Content\r\nContent-Length: 4\r\nContent-Type: text/plain; ")
                         U("charset=utf-8\r\n\r\ndata"),
                         resp.to_string());
    }

    TEST_FIXTURE(uri_address, request_to_string)
    {
        http_listener listener(m_uri);
        listener.open().wait();

        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // to_string
        std::string data("hehehe");
        listener.support([&](http_request request) {
            std::map<utility::string_t, utility::string_t> expected_headers;
            expected_headers[U("Connection")] = U("Keep-Alive");
            expected_headers[U("Content-Length")] = U("6");
            expected_headers[U("Content-Type")] = U("text/plain");
            expected_headers[U("Host")] = U("localhost:34567");
            expected_headers[U("User-Agent")] = U("test_http_client");

            // maybe to_string() should wait for the request to complete?
            // in the mean time...
            request.content_ready().wait();

            http_asserts::assert_request_string_equals(
                request.to_string(), U("GET"), U("/pa%20th1"), U("HTTP/1.1"), expected_headers, U("hehehe"));
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("pa%20th1"), U("text/plain"), data));
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
