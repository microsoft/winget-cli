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

#pragma once

#include "cpprest/http_client.h"
#include "http_test_utilities_public.h"
#include "test_http_server.h"

namespace tests
{
namespace functional
{
namespace http
{
namespace utilities
{
class test_server_utilities
{
public:
    /// <summary>
    /// Sends request with specified values using given http_client and verifies
    /// they are properly received by the test server.
    /// </summary>
    TEST_UTILITY_API static void __cdecl verify_request(web::http::client::http_client* p_client,
                                                        const utility::string_t& method,
                                                        const utility::string_t& path,
                                                        test_http_server* p_server,
                                                        unsigned short code);

    TEST_UTILITY_API static void __cdecl verify_request(web::http::client::http_client* p_client,
                                                        const utility::string_t& method,
                                                        const utility::string_t& path,
                                                        test_http_server* p_server,
                                                        unsigned short code,
                                                        const utility::string_t& reason);

    TEST_UTILITY_API static void __cdecl verify_request(web::http::client::http_client* p_client,
                                                        const utility::string_t& method,
                                                        const utility::string_t& path,
                                                        const utility::string_t& request_content_type,
                                                        const utility::string_t& request_data,
                                                        test_http_server* p_server,
                                                        unsigned short code,
                                                        const utility::string_t& reason);

    TEST_UTILITY_API static void __cdecl verify_request(
        web::http::client::http_client* p_client,
        const utility::string_t& method,
        const utility::string_t& path,
        test_http_server* p_server,
        unsigned short code,
        const std::map<utility::string_t, utility::string_t>& response_headers);
};

} // namespace utilities
} // namespace http
} // namespace functional
} // namespace tests
