/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Defines a test client to handle requests and sending responses.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "test_http_client.h"

#include "cpprest/details/http_helpers.h"
#include "cpprest/uri.h"
#ifdef _WIN32
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#pragma warning(push)
#pragma warning(disable : 4457)
#include <agents.h>
#pragma warning(pop)
#endif

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
// Flatten the http_headers into a name:value pairs separated by a carriage return and line feed.
utility::string_t flatten_http_headers(const std::map<utility::string_t, utility::string_t>& headers)
{
    utility::string_t flattened_headers;
    for (auto iter = headers.begin(); iter != headers.end(); ++iter)
    {
        utility::string_t temp((*iter).first + U(":") + (*iter).second + U("\r\n"));
        flattened_headers.append(utility::string_t(temp.begin(), temp.end()));
    }
    return flattened_headers;
}

#ifdef _WIN32

// Helper function to query for the size of header values.
static void query_header_length(HINTERNET request_handle, DWORD header, DWORD& length)
{
    WinHttpQueryHeaders(request_handle,
                        header,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        WINHTTP_NO_OUTPUT_BUFFER,
                        &length,
                        WINHTTP_NO_HEADER_INDEX);
}

// Helper function to get the status code from a WinHTTP response.
static void parse_status_code(HINTERNET request_handle, unsigned short& code)
{
    DWORD length = 0;
    query_header_length(request_handle, WINHTTP_QUERY_STATUS_CODE, length);
    utility::string_t buffer;
    buffer.resize(length);
    WinHttpQueryHeaders(request_handle,
                        WINHTTP_QUERY_STATUS_CODE,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &buffer[0],
                        &length,
                        WINHTTP_NO_HEADER_INDEX);
    code = (unsigned short)_wtoi(buffer.c_str());
}

// Helper function to trim leading and trailing null characters from a string.
static void trim_nulls(utility::string_t& str)
{
    size_t index;
    for (index = 0; index < str.size() && str[index] == 0; ++index)
        ;
    str.erase(0, index);
    index;
    for (index = str.size(); index > 0 && str[index - 1] == 0; --index)
        ;
    str.erase(index);
}

// Helper function to get the reason phrase from a WinHTTP response.
static void parse_reason_phrase(HINTERNET request_handle, utility::string_t& phrase)
{
    DWORD length = 0;
    query_header_length(request_handle, WINHTTP_QUERY_STATUS_TEXT, length);
    phrase.resize(length);
    WinHttpQueryHeaders(request_handle,
                        WINHTTP_QUERY_STATUS_TEXT,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &phrase[0],
                        &length,
                        WINHTTP_NO_HEADER_INDEX);
    // WinHTTP reports back the wrong length, trim any null characters.
    trim_nulls(phrase);
}

/// <summary>
/// Parses a string containing Http headers.
/// </summary>
static void parse_winhttp_headers(HINTERNET request_handle, utf16char* headersStr, test_response* p_response)
{
    // Status code and reason phrase.
    parse_status_code(request_handle, p_response->m_status_code);
    parse_reason_phrase(request_handle, p_response->m_reason_phrase);

    utf16char* context = nullptr;
    utf16char* line = wcstok_s(headersStr, U("\r\n"), &context);
    while (line != nullptr)
    {
        const utility::string_t header_line(line);
        const size_t colonIndex = header_line.find_first_of(U(":"));
        if (colonIndex != utility::string_t::npos)
        {
            utility::string_t key = header_line.substr(0, colonIndex);
            utility::string_t value = header_line.substr(colonIndex + 1, header_line.length() - colonIndex - 1);
            tests::functional::http::utilities::trim_whitespace(key);
            tests::functional::http::utilities::trim_whitespace(value);
            p_response->m_headers[key] = value;
        }
        line = wcstok_s(nullptr, U("\r\n"), &context);
    }
}

class _test_http_client
{
public:
    _test_http_client(const utility::string_t& uri) : m_uri(uri), m_hSession(nullptr), m_hConnection(nullptr) {}

    unsigned long open()
    {
        // Open session.
        m_hSession = WinHttpOpen(U("test_http_client"),
                                 WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                 WINHTTP_NO_PROXY_NAME,
                                 WINHTTP_NO_PROXY_BYPASS,
                                 WINHTTP_FLAG_ASYNC);
        if (!m_hSession)
        {
            return GetLastError();
        }

        // Set timeouts.
        int multiplier = 10;
        if (!WinHttpSetTimeouts(
                m_hSession, 60000 * multiplier, 60000 * multiplier, 30000 * multiplier, 30000 * multiplier))
        {
            return GetLastError();
        }

        // Set max connection to use per server to 1.
        DWORD maxConnections = 1;
        if (!WinHttpSetOption(m_hSession, WINHTTP_OPTION_MAX_CONNS_PER_SERVER, &maxConnections, sizeof(maxConnections)))
        {
            return GetLastError();
        }

        // Register asynchronous callback.
        if (WINHTTP_INVALID_STATUS_CALLBACK ==
            WinHttpSetStatusCallback(
                m_hSession, &_test_http_client::completion_callback, WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS, 0))
        {
            return GetLastError();
        }

        // Open connection.
        ::http::uri u(m_uri);
        unsigned int port = u.is_port_default() ? INTERNET_DEFAULT_PORT : u.port();
        m_hConnection = WinHttpConnect(m_hSession, u.host().c_str(), (INTERNET_PORT)port, 0);
        if (m_hConnection == nullptr)
        {
            return GetLastError();
        }
        return 0;
    }

    unsigned long close()
    {
        // Release memory for each request.
        std::for_each(
            m_responses_memory.begin(), m_responses_memory.end(), [](test_response* p_response) { delete p_response; });

        if (m_hConnection != nullptr)
        {
            if (WinHttpCloseHandle(m_hConnection) == NULL)
            {
                return GetLastError();
            }
        }

        if (m_hSession != nullptr)
        {
            // Unregister the callback.
            if (!WinHttpSetStatusCallback(m_hSession, NULL, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, NULL))
            {
                return GetLastError();
            }

            if (WinHttpCloseHandle(m_hSession) == NULL)
            {
                return GetLastError();
            }
        }
        return 0;
    }

    unsigned long request(const utility::string_t& method,
                          const utility::string_t& path,
                          const std::map<utility::string_t, utility::string_t> headers,
                          void* data,
                          size_t data_length)
    {
        HINTERNET request_handle = WinHttpOpenRequest(
            m_hConnection, method.c_str(), path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (request_handle == nullptr)
        {
            return GetLastError();
        }

        // Add headers.
        if (!headers.empty())
        {
            utility::string_t flattened_headers = flatten_http_headers(headers);
            if (!WinHttpAddRequestHeaders(request_handle,
                                          flattened_headers.c_str(),
                                          (DWORD)flattened_headers.length(),
                                          WINHTTP_ADDREQ_FLAG_ADD))
            {
                return GetLastError();
            }
        }

        if (!WinHttpSendRequest(request_handle,
                                WINHTTP_NO_ADDITIONAL_HEADERS,
                                0,
                                data,
                                (DWORD)data_length,
                                (DWORD)data_length,
                                (DWORD_PTR) new test_response(this)))
        {
            return GetLastError();
        }
        return 0;
    }

    test_response* wait_for_response() { return wait_for_responses(1)[0]; }

    pplx::task<test_response*> next_response()
    {
        return pplx::create_task([this]() -> test_response* { return wait_for_response(); });
    }

    std::vector<test_response*> wait_for_responses(const size_t count)
    {
        std::vector<test_response*> m_test_responses;
        for (size_t i = 0; i < count; ++i)
        {
            m_test_responses.push_back(Concurrency::receive(m_responses));
        }
        return m_test_responses;
    }

    std::vector<pplx::task<test_response*>> next_responses(const size_t count)
    {
        std::vector<pplx::task_completion_event<test_response*>> events;
        std::vector<pplx::task<test_response*>> responses;
        for (size_t i = 0; i < count; ++i)
        {
            events.push_back(pplx::task_completion_event<test_response*>());
            responses.push_back(pplx::create_task(events[i]));
        }
        pplx::create_task([this, count, events]() {
            for (size_t i = 0; i < count; ++i)
            {
                events[i].set(wait_for_response());
            }
        });
        return responses;
    }

private:
    // WinHTTP callback.
    static void CALLBACK
    completion_callback(HINTERNET hRequestHandle, DWORD_PTR context, DWORD statusCode, void* statusInfo, DWORD)
    {
        test_response* p_response = reinterpret_cast<test_response*>(context);
        if (p_response != nullptr)
        {
            if (statusCode == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
            {
                WINHTTP_ASYNC_RESULT* pStatusInfo = static_cast<WINHTTP_ASYNC_RESULT*>(statusInfo);
                pStatusInfo;
                throw std::exception("Error in WinHTTP callback");
            }
            else if (statusCode == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)
            {
                if (!WinHttpReceiveResponse(hRequestHandle, NULL))
                {
                    throw std::exception("Error receiving response");
                }
            }
            else if (statusCode == WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE)
            {
                DWORD headers_length;
                WinHttpQueryHeaders(hRequestHandle,
                                    WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                    WINHTTP_HEADER_NAME_BY_INDEX,
                                    WINHTTP_NO_OUTPUT_BUFFER,
                                    &headers_length,
                                    WINHTTP_NO_HEADER_INDEX);

                // Now allocate buffer for headers and query for them.
                std::vector<unsigned char> header_raw_buffer;
                header_raw_buffer.resize(headers_length);
                utf16char* header_buffer = reinterpret_cast<utf16char*>(&header_raw_buffer[0]);
                if (!WinHttpQueryHeaders(hRequestHandle,
                                         WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                         WINHTTP_HEADER_NAME_BY_INDEX,
                                         header_buffer,
                                         &headers_length,
                                         WINHTTP_NO_HEADER_INDEX))
                {
                    throw std::exception("Error querying for headers");
                }
                parse_winhttp_headers(hRequestHandle, header_buffer, p_response);

                // Check to see if the response has a body or not.
                if (!WinHttpQueryDataAvailable(hRequestHandle, nullptr))
                {
                    throw std::exception("Error reading response body");
                }
            }
            else if (statusCode == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE)
            {
                DWORD num_bytes = *(PDWORD)statusInfo;

                if (num_bytes > 0)
                {
                    size_t current_size = p_response->m_data.size();
                    p_response->m_data.resize(current_size + (size_t)num_bytes);

                    // Actual WinHTTP call to read in body.
                    if (!WinHttpReadData(hRequestHandle, &p_response->m_data[current_size], (DWORD)num_bytes, NULL))
                    {
                        throw std::exception("Error reading response body");
                    }
                }
                else
                {
                    WinHttpCloseHandle(hRequestHandle);
                    p_response->m_client->m_responses_memory.push_back(p_response);
                    Concurrency::asend(p_response->m_client->m_responses, p_response);
                }
            }
            else if (statusCode == WINHTTP_CALLBACK_STATUS_READ_COMPLETE)
            {
                if (!WinHttpQueryDataAvailable(hRequestHandle, nullptr))
                {
                    throw std::exception("Error reading response body");
                }
            }
        }
    }

    Concurrency::unbounded_buffer<test_response*> m_responses;

    // Used to store all requests to simplify memory management.
    std::vector<test_response*> m_responses_memory;

    const utility::string_t m_uri;
    HINTERNET m_hSession;
    HINTERNET m_hConnection;
};
#else
class _test_http_client
{
private:
    const web::http::uri m_uri;
    typename web::http::client::http_client m_client;
    std::vector<pplx::task<web::http::http_response>> m_responses;
    std::vector<test_response*> m_test_responses;

public:
    _test_http_client(utility::string_t uri) : m_uri(web::http::uri::encode_uri(uri)), m_client(m_uri.authority()) {}

    unsigned long open() { return 0; }
    unsigned long close() { return 0; }

    unsigned long request(const utility::string_t& method,
                          const utility::string_t& path,
                          const std::map<utility::string_t, utility::string_t>& headers,
                          void* data,
                          size_t data_length)
    {
        auto localHeaders = headers;
        localHeaders["User-Agent"] = "test_http_client";
        web::http::http_request request;
        request.set_method(method);
        request.set_request_uri(web::http::uri_builder(m_uri).append_path(path).to_uri());
        auto& hDest = request.headers();
        for (auto it = localHeaders.begin(); it != localHeaders.end(); ++it)
        {
            auto& currentValue = hDest[it->first];
            if (currentValue.empty())
                currentValue = it->second;
            else
                currentValue = currentValue + U(", ") + it->second;
        }
        request.set_body(utility::string_t(reinterpret_cast<const char*>(data), data_length));

        m_responses.push_back(m_client.request(request));
        return 0;
    }

    test_response* wait_for_response() { return wait_for_responses(1)[0]; }

    pplx::task<test_response*> next_response()
    {
        return pplx::create_task([this]() -> test_response* { return wait_for_response(); });
    }

    std::vector<test_response*> wait_for_responses(const size_t count)
    {
        if (count > m_responses.size()) throw std::logic_error("count too big");

        std::vector<test_response*> m_test_responses;
        for (size_t i = 0; i < count; ++i)
        {
            auto response = m_responses[0].get();

            auto tr = new test_response(this);
            tr->m_status_code = response.status_code();
            tr->m_reason_phrase = response.reason_phrase();
            for (auto it = response.headers().begin(); it != response.headers().end(); ++it)
            {
                tr->m_headers[it->first] = it->second;
            }
            tr->m_data = response.extract_vector().get();

            m_test_responses.push_back(tr);
            m_responses.erase(m_responses.begin());
        }
        return m_test_responses;
    }

    std::vector<pplx::task<test_response*>> next_responses(const size_t count)
    {
        std::vector<pplx::task<test_response*>> result;
        for (size_t i = 0; i < count; ++i)
        {
            result.push_back(next_response());
        }
        return result;
    }
};
#endif

test_http_client::test_http_client(const web::http::uri& uri)
{
    m_impl = std::unique_ptr<_test_http_client>(new _test_http_client(uri.to_string()));
}

test_http_client::~test_http_client() {}

test_http_client::test_http_client(test_http_client&& other) : m_impl(std::move(other.m_impl)) {}

test_http_client& test_http_client::operator=(test_http_client&& other)
{
    if (this != &other)
    {
        this->m_impl = std::move(other.m_impl);
    }
    return *this;
}

unsigned long test_http_client::open() { return m_impl->open(); }
unsigned long test_http_client::close() { return m_impl->close(); }

unsigned long test_http_client::request(const utility::string_t& method, const utility::string_t& path)
{
    return request(method, path, std::map<utility::string_t, utility::string_t>());
}
unsigned long test_http_client::request(const utility::string_t& method,
                                        const utility::string_t& path,
                                        const std::map<utility::string_t, utility::string_t>& headers)
{
    return request(method, path, headers, std::string());
}
unsigned long test_http_client::request(const utility::string_t& method,
                                        const utility::string_t& path,
                                        const std::string& data)
{
    return request(method, path, std::map<utility::string_t, utility::string_t>(), data);
}
unsigned long test_http_client::request(const utility::string_t& method,
                                        const utility::string_t& path,
                                        const utility::string_t& content_type,
                                        const std::string& data)
{
    std::map<utility::string_t, utility::string_t> headers;
    headers[U("Content-Type")] = content_type;
    return request(method, path, headers, data);
}

unsigned long test_http_client::request(const utility::string_t& method,
                                        const utility::string_t& path,
                                        const std::map<utility::string_t, utility::string_t>& headers,
                                        const std::string& data)
{
    return m_impl->request(method, path, headers, (void*)&data[0], data.size());
}

test_response* test_http_client::wait_for_response() { return m_impl->wait_for_response(); }
pplx::task<test_response*> test_http_client::next_response() { return m_impl->next_response(); }
std::vector<test_response*> test_http_client::wait_for_responses(const size_t count)
{
    return m_impl->wait_for_responses(count);
}
std::vector<pplx::task<test_response*>> test_http_client::next_responses(const size_t count)
{
    return m_impl->next_responses(count);
}

} // namespace utilities
} // namespace http
} // namespace functional
} // namespace tests
