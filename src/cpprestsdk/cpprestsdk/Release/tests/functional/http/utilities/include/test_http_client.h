/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * test_http_client.h -- Defines a test client to handle requests and sending responses.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "cpprest/uri.h"
#include "http_test_utilities_public.h"
#include <map>
#include <unittestpp.h>
#include <vector>

namespace tests
{
namespace functional
{
namespace http
{
namespace utilities
{
class _test_http_client;

/// <summary>
/// Structure for storing information about an HTTP response.
/// <summary>
class test_response
{
public:
    test_response(_test_http_client* client) : m_client(client) {}

    // API to check if a specific header exists and get it.
    template<typename T>
    bool match_header(const utility::string_t& header_name, T& header_value)
    {
        auto iter = m_headers.find(header_name);

        if (iter != m_headers.end())
        {
            utility::istringstream_t iss(iter->second);
            iss >> header_value;
            if (iss.fail() || !iss.eof())
            {
                return false;
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    bool match_header(const utility::string_t& header_name, utility::string_t& header_value)
    {
        auto iter = m_headers.find(header_name);
        if (iter != m_headers.end())
        {
            header_value = m_headers[header_name];
            return true;
        }
        return false;
    }

    // Response data.
    unsigned short m_status_code;
    utility::string_t m_reason_phrase;
    std::map<utility::string_t, utility::string_t> m_headers;
    std::vector<unsigned char> m_data;

    friend class _test_http_client;
    _test_http_client* m_client;
};

/// <summary>
/// Basic HTTP client for testing. Supports sending multiple requests.
///
/// NOTE: this HTTP client is not concurrency safe. I.e. only one thread at a time should use it.
/// </summary>
class test_http_client
{
public:
    TEST_UTILITY_API test_http_client(const web::http::uri& uri);
    TEST_UTILITY_API ~test_http_client();
    TEST_UTILITY_API test_http_client(test_http_client&& other);
    TEST_UTILITY_API test_http_client& operator=(test_http_client&& other);

    // APIs to open and close requests.
    TEST_UTILITY_API unsigned long open();
    TEST_UTILITY_API unsigned long close();

    // APIs to send requests.
    TEST_UTILITY_API unsigned long request(const utility::string_t& method, const utility::string_t& path);
    TEST_UTILITY_API unsigned long request(const utility::string_t& method,
                                           const utility::string_t& path,
                                           const std::map<utility::string_t, utility::string_t>& headers);
    TEST_UTILITY_API unsigned long request(const utility::string_t& method,
                                           const utility::string_t& path,
                                           const std::string& data);
    TEST_UTILITY_API unsigned long request(const utility::string_t& method,
                                           const utility::string_t& path,
                                           const utility::string_t& content_type,
                                           const std::string& data);
    TEST_UTILITY_API unsigned long request(const utility::string_t& method,
                                           const utility::string_t& path,
                                           const std::map<utility::string_t, utility::string_t>& headers,
                                           const std::string& data);

    // APIs to receive responses.
    TEST_UTILITY_API test_response* wait_for_response();
    TEST_UTILITY_API pplx::task<test_response*> next_response();
    TEST_UTILITY_API std::vector<test_response*> wait_for_responses(const size_t count);
    TEST_UTILITY_API std::vector<pplx::task<test_response*>> next_responses(const size_t count);

    // RAII pattern for test_http_client.
    class scoped_client
    {
    public:
        scoped_client(const web::http::uri& uri)
        {
            m_p_client = new test_http_client(uri);
            VERIFY_ARE_EQUAL(0u, m_p_client->open());
        }
        ~scoped_client()
        {
            VERIFY_ARE_EQUAL(0u, m_p_client->close());
            delete m_p_client;
        }
        test_http_client* client() { return m_p_client; }

    private:
        test_http_client* m_p_client;
    };

private:
    test_http_client& operator=(const test_http_client&);
    test_http_client(const test_http_client&);

    std::unique_ptr<_test_http_client> m_impl;
};

} // namespace utilities
} // namespace http
} // namespace functional
} // namespace tests
