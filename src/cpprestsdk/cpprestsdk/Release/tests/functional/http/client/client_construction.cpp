/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * client_construction.cpp
 *
 * Tests cases for covering creating http_clients.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include <fstream>

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
SUITE(client_construction)
{
    // Tests using different types of strings to construct an http_client.
    TEST_FIXTURE(uri_address, string_types)
    {
        // The goal of this test case is to make sure we can compile,
        // if the URI class doesn't have the proper constructors it won't.
        // So we don't need to actually do a request.
        http_client c1(U("http://localhost:4567/"));
        http_client c3(utility::string_t(U("http://localhost:4567/")));
    }

    // Tests different variations on specifying the URI in http_client constructor.
    TEST_FIXTURE(uri_address, different_uris)
    {
        const utility::string_t paths[] = {U(""), U("/"), U("/toplevel/nested"), U("/toplevel/nested/")};
        const utility::string_t expected_paths[] = {U("/"), U("/"), U("/toplevel/nested"), U("/toplevel/nested/")};
        const size_t num_paths = sizeof(paths) / sizeof(paths[0]);
        for (size_t i = 0; i < num_paths; ++i)
        {
            uri address(U("http://localhost:55678") + paths[i]);
            test_http_server::scoped_server scoped(address);
            http_client client(address);
            test_connection(scoped.server(), &client, expected_paths[i]);
        }
    }

    // Helper function verifies that when constructing an http_client with given
    // URI std::invalid_argument is thrown.
    static void verify_client_invalid_argument(const uri& address)
    {
        try
        {
            http_client client(address);
            VERIFY_IS_TRUE(false);
        }
        catch (std::invalid_argument&)
        {
            // expected
        }
    }

    TEST_FIXTURE(uri_address, client_construction_error_cases)
    {
        uri address(U("nothttp://localhost:34567/"));

        // Invalid scheme.
        verify_client_invalid_argument(address);

        // empty host.
        address = uri(U("http://:34567/"));
        verify_client_invalid_argument(address);
    }

    TEST_FIXTURE(uri_address, client_construction_no_scheme)
    {
        uri address(U("//localhost:34568/p/g"));
        test_http_server::scoped_server scoped(m_uri);

        http_client client(address);
        test_connection(scoped.server(), &client, U("/p/g"));
    }

    TEST_FIXTURE(uri_address, copy_assignment)
    {
        test_http_server::scoped_server scoped(m_uri);

        // copy constructor
        http_client original(m_uri);
        http_client new_client(original);
        test_connection(scoped.server(), &new_client, U("/"));
        test_connection(scoped.server(), &original, U("/"));

        // assignment
        http_client new_client2(U("http://bad:-1"));
        new_client2 = original;
        test_connection(scoped.server(), &new_client2, U("/"));
        test_connection(scoped.server(), &original, U("/"));
    }

    TEST_FIXTURE(uri_address, move_not_init)
    {
        test_http_server::scoped_server scoped(m_uri);

        // move constructor
        http_client original(m_uri);
        http_client new_client = std::move(original);
        test_connection(scoped.server(), &new_client, U("/"));

        // move assignment
        original = http_client(m_uri);
        test_connection(scoped.server(), &original, U("/"));
    }

    TEST_FIXTURE(uri_address, move_init)
    {
        test_http_server::scoped_server scoped(m_uri);

        // move constructor
        http_client original(m_uri);
        test_connection(scoped.server(), &original, U("/"));
        http_client new_client = std::move(original);
        test_connection(scoped.server(), &new_client, U("/"));

        // move assignment
        original = http_client(m_uri);
        test_connection(scoped.server(), &original, U("/"));
    }

    // Verify that we can read the config from the http_client
    TEST_FIXTURE(uri_address, get_client_config)
    {
        test_http_server::scoped_server scoped(m_uri);

        http_client_config config;

        VERIFY_ARE_EQUAL(config.chunksize(), 64 * 1024);
        config.set_chunksize(1024);
        VERIFY_ARE_EQUAL(config.chunksize(), 1024);

        utility::seconds timeout(100);
        config.set_timeout(timeout);
        http_client client(m_uri, config);

        const http_client_config& config2 = client.client_config();
        VERIFY_ARE_EQUAL(config2.timeout().count(), timeout.count());
        std::chrono::milliseconds milli_timeout = config2.timeout();
        VERIFY_ARE_EQUAL(milli_timeout.count(), std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
        auto micro_timeout = config.timeout<std::chrono::microseconds>();
        VERIFY_ARE_EQUAL(micro_timeout.count(), std::chrono::duration_cast<std::chrono::microseconds>(timeout).count());

        VERIFY_ARE_EQUAL(config2.chunksize(), 1024);
    }

    // Verify that we can get the baseuri from http_client constructors
    TEST_FIXTURE(uri_address, BaseURI_test)
    {
        http_client baseclient1(m_uri);
        VERIFY_ARE_EQUAL(baseclient1.base_uri(), m_uri);

        http_client_config config;
        http_client baseclient2(m_uri, config);
        VERIFY_ARE_EQUAL(baseclient2.base_uri(), m_uri);
    }

#if !defined(_WIN32) && !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)

    // Verify that the callback of sslcontext is called for HTTPS
    TEST_FIXTURE(uri_address, ssl_context_callback_https)
    {
        http_client_config config;
        bool called = false;

        config.set_ssl_context_callback([&called](boost::asio::ssl::context& ctx) { called = true; });

        http_client client(U("https://www.google.com/"), config);

        try
        {
            client.request(methods::GET, U("/")).get();
        }
        catch (...)
        {
        }

        VERIFY_IS_TRUE(called, "The sslcontext options is not called for HTTPS protocol");
    }

    // Verify that the callback of sslcontext is not called for HTTP
    TEST_FIXTURE(uri_address, ssl_context_callback_http)
    {
        http_client_config config;
        bool called = false;

        config.set_ssl_context_callback([&called](boost::asio::ssl::context& ctx) { called = true; });

        http_client client(U("http://www.google.com/"), config);

        try
        {
            client.request(methods::GET, U("/")).get();
        }
        catch (...)
        {
        }

        VERIFY_IS_FALSE(called, "The sslcontext options is called for HTTP protocol");
    }

#endif

} // SUITE(client_construction)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
