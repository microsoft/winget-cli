/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * http_client_tests.h
 *
 * Common declarations and helper functions for http_client test cases.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "cpprest/http_client.h"
#include "http_test_utilities.h"
#include "unittestpp.h"

namespace tests
{
namespace functional
{
namespace http
{
namespace client
{
class uri_address
{
public:
    uri_address() : m_uri(U("http://localhost:34568/")) {}
    web::http::uri m_uri;
};

// Helper function to send a simple request to a server to test
// the connection.
void test_connection(tests::functional::http::utilities::test_http_server* p_server,
                     web::http::client::http_client* p_client,
                     const utility::string_t& path);

// Helper function send a simple request to test the connection.
// Take in the path to request and what path should be received in the server.
void test_connection(tests::functional::http::utilities::test_http_server* p_server,
                     web::http::client::http_client* p_client,
                     const utility::string_t& request_path,
                     const utility::string_t& expected_path);

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
