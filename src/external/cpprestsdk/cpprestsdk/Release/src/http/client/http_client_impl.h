/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: Client-side APIs.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

#include "cpprest/astreambuf.h"
#include "cpprest/details/basic_types.h"
#include "cpprest/http_client.h"
#include "cpprest/http_msg.h"
#include <memory>
#include <stdexcept>
#include <string>

namespace web
{
namespace http
{
namespace details
{
/// <summary>
/// Serialize the http_headers into name:value pairs separated by a carriage return and line feed.
/// </summary>
utility::string_t flatten_http_headers(const http_headers& headers);
/// <summary>
/// Parses a string containing Http headers.
/// </summary>
void parse_headers_string(_Inout_z_ utility::char_t* headersStr, http_headers& headers);
} // namespace details
} // namespace http
} // namespace web

namespace web
{
namespace http
{
namespace client
{
namespace details
{
class _http_client_communicator;

// Request context encapsulating everything necessary for creating and responding to a request.
class request_context
{
public:
    // Destructor to clean up any held resources.
    virtual ~request_context() {}

    virtual void report_exception(std::exception_ptr exceptionPtr);

    virtual concurrency::streams::streambuf<uint8_t> _get_readbuffer();

    void complete_headers();

    /// <summary>
    /// Completes this request, setting the underlying task completion event, and cleaning up the handles
    /// </summary>
    void complete_request(utility::size64_t body_size);

    void report_error(unsigned long error_code, const std::string& errorMessage);

#ifdef _WIN32
    void report_error(unsigned long error_code, const std::wstring& errorMessage);
#endif

    template<typename _ExceptionType>
    void report_exception(const _ExceptionType& e)
    {
        report_exception(std::make_exception_ptr(e));
    }

    /// <summary>Set m_decompressor based on the response headers, or call report_exception</summary>
    /// <returns>false on failure</returns>
    bool handle_compression();

    /// <summary>Append an Accept-Encoding header if requested by the http_client settings</summary>
    utility::string_t get_compression_header() const;

    concurrency::streams::streambuf<uint8_t> _get_writebuffer();

    // Reference to the http_client implementation.
    std::shared_ptr<_http_client_communicator> m_http_client;

    // request/response pair.
    http_request m_request;
    http_response m_response;

    utility::size64_t m_uploaded;
    utility::size64_t m_downloaded;

    // task completion event to signal request is completed.
    pplx::task_completion_event<http_response> m_request_completion;

    // Registration for cancellation notification if enabled.
    pplx::cancellation_token_registration m_cancellationRegistration;

    std::unique_ptr<web::http::compression::decompress_provider> m_decompressor;

protected:
    request_context(const std::shared_ptr<_http_client_communicator>& client, const http_request& request);

    virtual void finish();
};

//
// Interface used by client implementations. Concrete implementations are responsible for
// sending HTTP requests and receiving the responses.
//
class _http_client_communicator : public http_pipeline_stage
{
public:
    virtual ~_http_client_communicator() override = default;

    // Asynchronously send a HTTP request and process the response.
    void async_send_request(const std::shared_ptr<request_context>& request);

    void finish_request();

    const http_client_config& client_config() const;

    const uri& base_uri() const;

protected:
    _http_client_communicator(http::uri&& address, http_client_config&& client_config);

    // HTTP client implementations must implement send_request.
    virtual void send_request(_In_ const std::shared_ptr<request_context>& request) = 0;

    // URI to connect to.
    const http::uri m_uri;

    pplx::extensibility::critical_section_t m_client_lock;

private:
    http_client_config m_client_config;

    // Wraps opening the client around sending a request.
    void async_send_request_impl(const std::shared_ptr<request_context>& request);

    // Queue used to guarantee ordering of requests, when applicable.
    std::queue<std::shared_ptr<request_context>> m_requests_queue;
    bool m_outstanding;
};

/// <summary>
/// Factory function implemented by the separate platforms to construct their subclasses of _http_client_communicator
/// </summary>
std::shared_ptr<_http_client_communicator> create_platform_final_pipeline_stage(uri&& base_uri,
                                                                                http_client_config&& client_config);

} // namespace details
} // namespace client
} // namespace http
} // namespace web
