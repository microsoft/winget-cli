/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * test_server_utilities.h - Utility class to send and verify requests and responses working with the http_test_server.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "test_server_utilities.h"

#include "http_asserts.h"

using namespace web;
using namespace utility;

namespace tests
{
namespace functional
{
namespace http
{
namespace utilities
{
void test_server_utilities::verify_request(::http::client::http_client* p_client,
                                           const utility::string_t& method,
                                           const utility::string_t& path,
                                           test_http_server* p_server,
                                           unsigned short code)
{
    p_server->next_request().then([&](test_request* p_request) {
        http_asserts::assert_test_request_equals(p_request, method, path);
        VERIFY_ARE_EQUAL(0, p_request->reply(code));
    });
    http_asserts::assert_response_equals(p_client->request(method, path).get(), code);
}

void test_server_utilities::verify_request(::http::client::http_client* p_client,
                                           const utility::string_t& method,
                                           const utility::string_t& path,
                                           test_http_server* p_server,
                                           unsigned short code,
                                           const utility::string_t& reason)
{
    p_server->next_request().then([&](test_request* p_request) {
        http_asserts::assert_test_request_equals(p_request, method, path);
        VERIFY_ARE_EQUAL(0, p_request->reply(code, reason));
    });
    http_asserts::assert_response_equals(p_client->request(method, path).get(), code, reason);
}

void test_server_utilities::verify_request(::http::client::http_client* p_client,
                                           const utility::string_t& method,
                                           const utility::string_t& path,
                                           const utility::string_t& request_content_type,
                                           const utility::string_t& request_data,
                                           test_http_server* p_server,
                                           unsigned short code,
                                           const utility::string_t& reason)
{
    p_server->next_request().then([&](test_request* p_request) {
        http_asserts::assert_test_request_equals(p_request, method, path, request_content_type, request_data);
        VERIFY_ARE_EQUAL(0, p_request->reply(code, reason));
    });
    http_asserts::assert_response_equals(
        p_client->request(method, path, request_data, request_content_type).get(), code, reason);
}

void test_server_utilities::verify_request(::http::client::http_client* p_client,
                                           const utility::string_t& method,
                                           const utility::string_t& path,
                                           test_http_server* p_server,
                                           unsigned short code,
                                           const std::map<utility::string_t, utility::string_t>& response_headers)
{
    p_server->next_request().then([&](test_request* p_request) {
        http_asserts::assert_test_request_equals(p_request, method, path);
        VERIFY_ARE_EQUAL(0, p_request->reply(code, U(""), response_headers));
    });
    http_asserts::assert_response_equals(p_client->request(method, path).get(), code, response_headers);
}

} // namespace utilities
} // namespace http
} // namespace functional
} // namespace tests
