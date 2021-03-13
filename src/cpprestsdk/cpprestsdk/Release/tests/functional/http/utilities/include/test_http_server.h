/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * test_http_server.h -- Defines a test server to handle requests and sending responses.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "cpprest/uri.h"
#include "http_test_utilities_public.h"
#include "unittestpp.h"
#include <map>
#include <sstream>

namespace tests
{
namespace functional
{
namespace http
{
namespace utilities
{
/// <summary>
/// Actual implementation of test_http_server is in this class.
/// This wrapping is done to hide the fact we are using Windows HTTP Server APIs
/// from users of this test library.
/// </summary>
class _test_http_server;

/// <summary>
/// Structure for storing HTTP request information and responding to requests.
/// </summary>
class test_request
{
    friend class _test_http_server;

public:
    test_request(unsigned long long reqid, _test_http_server* p_server) : m_request_id(reqid), m_p_server(p_server) {}

    // APIs to send responses.
    unsigned long reply(const unsigned short status_code,
                        const utility::string_t& reason_phrase = U(""),
                        const std::map<utility::string_t, utility::string_t>& headers =
                            std::map<utility::string_t, utility::string_t>(),
                        const utf8string& data = "")
    {
        return reply_impl(status_code, reason_phrase, headers, (void*)&data[0], data.size() * sizeof(utf8char));
    }

    unsigned long reply(const unsigned short status_code,
                        const utility::string_t& reason_phrase,
                        const std::map<utility::string_t, utility::string_t>& headers,
                        const std::vector<uint8_t>& data)
    {
        return reply_impl(status_code, reason_phrase, headers, (void*)&data[0], data.size());
    }

    unsigned long reply(const unsigned short status_code,
                        const utility::string_t& reason_phrase,
                        const std::map<utility::string_t, utility::string_t>& headers,
                        const utf16string& data)
    {
        return reply_impl(status_code, reason_phrase, headers, (void*)&data[0], data.size() * sizeof(utf16char));
    }

    // API to check if a specific header exists and get it.
    template<typename T>
    bool match_header(const utility::string_t& header_name, T& header_value)
    {
        auto iter = m_headers.find(header_name);
        if (iter == m_headers.end())
        {
            return false;
        }

        return web::http::details::bind_impl(iter->second, header_value) || iter->second.empty();
    }

    // Request data.
    utility::string_t m_method;
    utility::string_t m_path;
    std::map<utility::string_t, utility::string_t> m_headers;
    std::vector<unsigned char> m_body;

private:
    // This is the HTTP Server API Request Id, we don't want to bring in the header file.
    unsigned long long m_request_id;
    _test_http_server* m_p_server;

    // Helper to send replies.
    TEST_UTILITY_API unsigned long reply_impl(const unsigned short status_code,
                                              const utility::string_t& reason_phrase,
                                              const std::map<utility::string_t, utility::string_t>& headers,
                                              void* data,
                                              size_t data_length);
};

/// <summary>
/// Basic HTTP server for testing. Supports waiting and collecting together requests.
///
/// NOTE: this HTTP server is not concurrency safe. I.e. only one thread at a time should use it.
/// </summary>
class test_http_server
{
public:
    TEST_UTILITY_API test_http_server(const web::http::uri& uri);
    TEST_UTILITY_API ~test_http_server();

    // APIs to receive requests.
    TEST_UTILITY_API pplx::task<test_request*> next_request();
    TEST_UTILITY_API std::vector<pplx::task<test_request*>> next_requests(const size_t count);

    // Enable early close
    TEST_UTILITY_API void close();

    // RAII pattern for test_http_server.
    class scoped_server;

private:
    std::unique_ptr<_test_http_server> m_p_impl;
};

class test_http_server::scoped_server
{
public:
    scoped_server(const web::http::uri& uri) : m_p_server(uri) {}
    test_http_server* server() { return &m_p_server; }

private:
    test_http_server m_p_server;
};

} // namespace utilities
} // namespace http
} // namespace functional
} // namespace tests
