/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * http_client_tests.cpp
 *
 * Common definitions and helper functions for http_client test cases.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

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
void test_connection(test_http_server* p_server, http_client* p_client, const utility::string_t& path)
{
    p_server->next_request().then([path](test_request* p_request) {
        http_asserts::assert_test_request_equals(p_request, methods::GET, path);
        VERIFY_ARE_EQUAL(0u, p_request->reply(200));
    });
    http_asserts::assert_response_equals(p_client->request(methods::GET).get(), status_codes::OK);
}

// Helper function send a simple request to test the connection.
// Take in the path to request and what path should be received in the server.
void test_connection(test_http_server* p_server,
                     http_client* p_client,
                     const utility::string_t& request_path,
                     const utility::string_t& expected_path)
{
    p_server->next_request().then([expected_path](test_request* p_request) {
        http_asserts::assert_test_request_equals(p_request, methods::GET, expected_path);
        VERIFY_ARE_EQUAL(0u, p_request->reply(200));
    });
    http_asserts::assert_response_equals(p_client->request(methods::GET, request_path).get(), status_codes::OK);
}

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
