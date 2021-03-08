/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests cases for HTTP methods.
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
SUITE(http_methods_tests)
{
    // Tests the defined methods and custom methods.
    TEST_FIXTURE(uri_address, http_methods)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        // Don't include 'CONNECT' it has a special meaning.
        utility::string_t send_methods[] = {methods::GET,
                                            U("GET"),
                                            methods::DEL,
                                            methods::HEAD,
#ifdef _WIN32 // -  this is never passed to the listener with http_listener
                                            methods::OPTIONS,
#endif
                                            methods::POST,
                                            methods::PUT,
                                            methods::PATCH,
#ifndef __cplusplus_winrt
#ifdef _WIN32 // - ditto
                                            methods::TRCE,
#endif
#endif

                                            U("CUstomMETHOD")};
        utility::string_t recv_methods[] = {U("GET"),
                                            U("GET"),
                                            U("DELETE"),
                                            U("HEAD"),
#ifdef _WIN32
                                            U("OPTIONS"),
#endif
                                            U("POST"),
                                            U("PUT"),
                                            U("PATCH"),
#ifndef __cplusplus_winrt
#ifdef _WIN32
                                            U("TRACE"),
#endif
#endif

                                            U("CUstomMETHOD")};
        const size_t num_methods = sizeof(send_methods) / sizeof(send_methods[0]);

        for (int i = 0; i < num_methods; ++i)
        {
            p_server->next_request().then([i, &recv_methods](test_request* p_request) {
                http_asserts::assert_test_request_equals(p_request, recv_methods[i], U("/"));
                VERIFY_ARE_EQUAL(0u, p_request->reply(200));
            });
            http_asserts::assert_response_equals(client.request(send_methods[i]).get(), status_codes::OK);
        }
    }

#ifdef __cplusplus_winrt
    TEST_FIXTURE(uri_address, http_trace_fails_on_winrt)
    {
        http_client client(m_uri);
        VERIFY_THROWS(client.request(methods::TRCE).get(), http_exception);
    }
#endif

    TEST(http_request_empty_method) { VERIFY_THROWS(http_request(U("")), std::invalid_argument); }

    TEST_FIXTURE(uri_address, empty_method)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);
        VERIFY_THROWS(client.request(U("")), std::invalid_argument);
    }
}

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
