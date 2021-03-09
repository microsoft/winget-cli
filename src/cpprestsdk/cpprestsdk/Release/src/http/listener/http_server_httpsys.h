/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: implementation of HTTP server API built on Windows HTTP Server APIs.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
#error "Error: http server APIs are not supported in XP"
#endif //_WIN32_WINNT < _WIN32_WINNT_VISTA

// Windows Sockets are not code analysis clean.
#pragma warning(push)
#pragma warning(disable : 6386)
#include <http.h>
#pragma warning(pop)

#include "cpprest/details/http_server.h"
#include <atomic>
#include <mutex>

namespace web
{
namespace http
{
namespace experimental
{
namespace details
{
class http_windows_server;
struct windows_request_context;

/// <summary>
/// Class used to wrap OVERLAPPED I/O with any HTTP I/O.
/// </summary>
class http_overlapped : public OVERLAPPED
{
public:
    void set_http_io_completion(std::function<void(DWORD, DWORD)> http_io_completion)
    {
        ZeroMemory(this, sizeof(OVERLAPPED));
        m_http_io_completion = http_io_completion;
    }

    /// <summary>
    /// Callback for all I/O completions.
    /// </summary>
    static void CALLBACK io_completion_callback(PTP_CALLBACK_INSTANCE instance,
                                                PVOID context,
                                                PVOID pOverlapped,
                                                ULONG result,
                                                ULONG_PTR numberOfBytesTransferred,
                                                PTP_IO io)
    {
        (void)io;
        (void)context;
        (void)instance;

        http_overlapped* p_http_overlapped = (http_overlapped*)pOverlapped;
        p_http_overlapped->m_http_io_completion(result, (DWORD)numberOfBytesTransferred);
    }

private:
    std::function<void(DWORD, DWORD)> m_http_io_completion;
};

/// <summary>
/// Context for http request through Windows HTTP Server API.
/// </summary>
struct windows_request_context : http::details::_http_server_context
{
    windows_request_context();
    virtual ~windows_request_context();

    // Asynchronously starts processing the current request.
    void async_process_request(HTTP_REQUEST_ID request_id, http::http_request msg, const unsigned long headers_size);

    // Dispatch request to the provided http_listener.
    void dispatch_request_to_listener(_In_ web::http::experimental::listener::details::http_listener_impl* pListener);

    enum class ShouldWaitForBody
    {
        Wait,
        DontWait
    };
    // Initialise the response task callbacks. If the body has been requested, we should wait for it to avoid race
    // conditions.
    void init_response_callbacks(ShouldWaitForBody shouldWait);

    // Read in a portion of the request body.
    void read_request_body_chunk();

    // Start processing the response.
    void async_process_response();

    void transmit_body();

    // Read request headers io completion callback function .
    void read_headers_io_completion(DWORD error_code, DWORD bytes_read);

    // Read request body io completion callback function.
    void read_body_io_completion(DWORD error_code, DWORD bytes_read);

    // Send response io completion callback function .
    void send_response_io_completion(DWORD error_code, DWORD bytes_read);

    // Send response body io completion callback function.
    void send_response_body_io_completion(DWORD error_code, DWORD bytes_read);

    // Cancel request io completion callback function.
    void cancel_request_io_completion(DWORD error_code, DWORD bytes_read);

    // TCE that indicates the completion of response
    // Workaround for ppl task_completion_event bug.
    std::mutex m_responseCompletedLock;
    pplx::task_completion_event<void> m_response_completed;

    // Id of the currently processed request on this connection.
    HTTP_REQUEST_ID m_request_id;

    bool m_sending_in_chunks;
    bool m_transfer_encoding;

    size_t m_remaining_to_write;

    HTTP_REQUEST* m_request;
    std::unique_ptr<unsigned char[]> m_request_buffer;

    std::unique_ptr<HTTP_UNKNOWN_HEADER[]> m_headers;
    std::vector<std::string> m_headers_buffer;

    http_overlapped m_overlapped;

    http_request m_msg;
    http_response m_response;

    std::exception_ptr m_except_ptr;

    std::vector<uint8_t> m_compress_buffer;
    std::unique_ptr<web::http::compression::compress_provider> m_compressor;
    std::unique_ptr<web::http::compression::decompress_provider> m_decompressor;
    utility::string_t m_decompress_header;
    http::compression::details::header_types m_decompress_header_type;

private:
    windows_request_context(const windows_request_context&);
    windows_request_context& operator=(const windows_request_context&);

    // Sends entity body chunk.
    void send_entity_body(_In_reads_(data_length) unsigned char* data, _In_ size_t data_length);

    // Cancels this request.
    void cancel_request(std::exception_ptr except_ptr);

    std::vector<unsigned char> m_body_data;
};

/// <summary>
/// Class to implement HTTP server API on Windows.
/// </summary>
class http_windows_server : public http_server
{
public:
    /// <summary>
    /// Constructs a http_windows_server.
    /// </summary>
    http_windows_server();

    /// <summary>
    /// Releases resources held.
    /// </summary>
    ~http_windows_server();

    /// <summary>
    /// Start listening for incoming requests.
    /// </summary>
    virtual pplx::task<void> start();

    /// <summary>
    /// Registers an http listener.
    /// </summary>
    virtual pplx::task<void> register_listener(
        _In_ web::http::experimental::listener::details::http_listener_impl* pListener);

    /// <summary>
    /// Unregisters an http listener.
    /// </summary>
    virtual pplx::task<void> unregister_listener(
        _In_ web::http::experimental::listener::details::http_listener_impl* pListener);

    /// <summary>
    /// Stop processing and listening for incoming requests.
    /// </summary>
    virtual pplx::task<void> stop();

    /// <summary>
    /// Asynchronously sends the specified http response.
    /// </summary>
    /// <param name="response">The http_response to send.</param>
    /// <returns>A operation which is completed once the response has been sent.</returns>
    virtual pplx::task<void> respond(http::http_response response);

private:
    friend struct details::windows_request_context;

    // Structure to hold each registered listener.
    class listener_registration
    {
    public:
        listener_registration(HTTP_URL_GROUP_ID urlGroupId) : m_urlGroupId(urlGroupId) {}

        // URL group id for this listener. Each listener needs it own URL group
        // because configuration like timeouts, authentication, etc...
        HTTP_URL_GROUP_ID m_urlGroupId;

        // Request handler lock to guard against removing a listener while in user code.
        pplx::extensibility::reader_writer_lock_t m_requestHandlerLock;
    };

    // Registered listeners
    pplx::extensibility::reader_writer_lock_t _M_listenersLock;
    std::unordered_map<web::http::experimental::listener::details::http_listener_impl*,
                       std::unique_ptr<listener_registration>>
        _M_registeredListeners;

    // HTTP Server API server session id.
    HTTP_SERVER_SESSION_ID m_serverSessionId;

    // Tracks the number of outstanding requests being processed.
    std::atomic<int> m_numOutstandingRequests;
    pplx::extensibility::event_t m_zeroOutstandingRequests;

    // Handle to HTTP Server API request queue.
    HANDLE m_hRequestQueue;

    // Threadpool I/O structure for overlapped I/O.
    TP_IO* m_threadpool_io;

    // Task which actually handles receiving requests from HTTP Server API request queue.
    pplx::task<void> m_receivingTask;
    void receive_requests();
};

} // namespace details
} // namespace experimental
} // namespace http
} // namespace web
