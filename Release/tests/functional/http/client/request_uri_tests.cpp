/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * request_uri_tests.cpp
 *
 * Tests cases covering various kinds of request URIs with http_client.
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
SUITE(request_uri_tests)
{
    // Tests path specified in requests with non-empty base path in client constructor.
    TEST_FIXTURE(uri_address, path_non_empty_ctor)
    {
        uri address(U("http://localhost:45678/base_path/"));

        // Path not starting with '/'.
        {
            test_http_server::scoped_server scoped(address);
            http_client client(address);
            test_connection(scoped.server(), &client, U("next_level"), U("/base_path/next_level"));
        }

        // Path starting with '/'.
        {
            test_http_server::scoped_server scoped(address);
            http_client client(address);
            test_connection(scoped.server(), &client, U("/next_level"), U("/base_path/next_level"));
        }
    }

    // Tests path specified in requests with empty base path in client constructor.
    TEST_FIXTURE(uri_address, path_empty_ctor)
    {
        uri address(U("http://localhost:45678"));

        // NON empty path.
        {
            test_http_server::scoped_server scoped(address);
            http_client client(address);
            test_connection(scoped.server(), &client, U("next_level"), U("/next_level"));
        }

        // Request path of '*'
        {
            test_http_server::scoped_server scoped(address);
            http_client client(address);
            test_connection(scoped.server(), &client, U("*"), U("/*"));
        }

        // Empty base of '/' with request path starting with '/'.
        address = uri(U("http://localhost:45678/"));
        {
            test_http_server::scoped_server scoped(address);
            http_client client(address);
            test_connection(scoped.server(), &client, U("/hehehe"), U("/hehehe"));
        }
    }

    TEST_FIXTURE(uri_address, with_query_fragment)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // query
        test_connection(scoped.server(), &client, U("/hehehe?key1=value2&"), U("/hehehe?key1=value2&"));

        // fragment

        // WinRT implementation percent encodes the '#'.
        utility::string_t expected_value = U("/heheh?key1=value2#fragment");
#if defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
        expected_value = percent_encode_pound(expected_value);
#endif

        test_connection(scoped.server(), &client, U("/heheh?key1=value2#fragment"), expected_value);
    }

    TEST_FIXTURE(uri_address, uri_encoding)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        // try with encoding string.
        http_request msg(methods::GET);
        msg.set_request_uri(U("/path1!!alreadyencoded"));
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, methods::GET, U("/path1!!alreadyencoded"));
            http_asserts::assert_test_request_contains_headers(p_request, msg.headers());
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // verify encoding actual happens with plain.
        msg = http_request(methods::GET);
        msg.set_request_uri(web::http::uri::encode_uri(U("/path1 /encode")));
        VERIFY_ARE_EQUAL(U("/path1%20/encode"), msg.relative_uri().to_string());
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, methods::GET, U("/path1%20/encode"));
            http_asserts::assert_test_request_contains_headers(p_request, msg.headers());
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
    }

    // Tests combining case URI query/fragments with relative URI query/fragments.
    TEST_FIXTURE(uri_address, append_query_fragment)
    {
        // Try with query.
        const utility::string_t base_uri_with_query =
            web::http::uri_builder(m_uri).append(U("/path1?key1=value1")).to_string();
        {
            test_http_server::scoped_server scoped(m_uri);
            test_http_server* p_server = scoped.server();
            http_client client(base_uri_with_query);

            p_server->next_request().then([&](test_request* p_request) {
                // WinRT implementation percent encodes the '#'.
                utility::string_t expected_value = U("/path1?key1=value1&key2=value2#frag");
#if defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
                expected_value = percent_encode_pound(expected_value);
#endif
                http_asserts::assert_test_request_equals(p_request, methods::GET, expected_value);
                p_request->reply(200);
            });
            http_asserts::assert_response_equals(client.request(methods::GET, U("?key2=value2#frag")).get(),
                                                 status_codes::OK);
        }

        // Try with fragment.
        const utility::string_t base_uri_with_frag(m_uri.to_string() + U("path1#fragment"));
        {
            test_http_server::scoped_server scoped(m_uri);
            test_http_server* p_server = scoped.server();
            http_client client(base_uri_with_frag);

            p_server->next_request().then([&](test_request* p_request) {
                // WinRT implementation percent encodes the '#'.
                utility::string_t expected_value = U("/path1/path2?key2=value2#fragmentfg2");
#if defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
                expected_value = percent_encode_pound(expected_value);
#endif
                http_asserts::assert_test_request_equals(p_request, methods::GET, expected_value);
                p_request->reply(200);
            });
            http_asserts::assert_response_equals(client.request(methods::GET, U("path2?key2=value2#fg2")).get(),
                                                 status_codes::OK);
        }
    }

} // SUITE(request_uri_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
