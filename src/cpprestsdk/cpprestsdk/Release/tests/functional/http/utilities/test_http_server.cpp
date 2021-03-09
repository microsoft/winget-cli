/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Defines a test server to handle requests and sending responses.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#ifdef _WIN32
#include <http.h>
#pragma comment(lib, "httpapi.lib")
#pragma warning(push)
#pragma warning(disable : 4457)
#include <agents.h>
#pragma warning(pop)
#else
#include "cpprest/http_listener.h"
#endif
#include "cpprest/uri.h"
#include "test_http_server.h"
#include <algorithm>
#include <mutex>
#include <os_utilities.h>
#include <thread>

using namespace web;
using namespace utility;
using namespace utility::conversions;

namespace tests
{
namespace functional
{
namespace http
{
namespace utilities
{
struct test_server_queue
{
    std::mutex m_lock;
    std::deque<pplx::task_completion_event<test_request*>> m_requests;
    std::vector<std::unique_ptr<test_request>> m_requests_memory;

    void close()
    {
        std::lock_guard<std::mutex> lk(m_lock);
        for (auto&& tce : m_requests)
            tce.set_exception(std::runtime_error("test_http_server closed."));
    }

    ~test_server_queue() { close(); }

    void on_request(std::unique_ptr<test_request> req)
    {
        std::lock_guard<std::mutex> lk(m_lock);
        VERIFY_IS_FALSE(m_requests.empty(), "There are no pending calls to next_request.");
        if (m_requests.empty()) return;
        auto tce = std::move(m_requests.front());
        m_requests.pop_front();
        m_requests_memory.push_back(std::move(req));
        tce.set(m_requests_memory.back().get());
    }

    pplx::task<test_request*> next_request()
    {
        pplx::task_completion_event<test_request*> tce;
        std::lock_guard<std::mutex> lock(m_lock);
        m_requests.push_back(tce);
        return pplx::create_task(tce);
    }
};

#if defined(_WIN32)
// Helper function to parse verb from Windows HTTP Server API.
static utility::string_t parse_verb(const HTTP_REQUEST* p_http_request)
{
    utility::string_t method;
    std::string temp;
    switch (p_http_request->Verb)
    {
        case HttpVerbGET: method = U("GET"); break;
        case HttpVerbPOST: method = U("POST"); break;
        case HttpVerbPUT: method = U("PUT"); break;
        case HttpVerbDELETE: method = U("DELETE"); break;
        case HttpVerbHEAD: method = U("HEAD"); break;
        case HttpVerbOPTIONS: method = U("OPTIONS"); break;
        case HttpVerbTRACE: method = U("TRACE"); break;
        case HttpVerbCONNECT: method = U("CONNECT"); break;
        case HttpVerbUnknown: temp = p_http_request->pUnknownVerb; method = utility::string_t(temp.begin(), temp.end());
        default: break;
    }
    return method;
}

/// <summary>
/// String values for all HTTP Server API HTTP_REQUEST_HEADERS known headers.
/// NOTE: the order here is important it is from the _HTTP_HEADER_ID enum.
/// </summary>
static utility::string_t HttpServerAPIRequestKnownHeaders[] =
{
    U("Cache-Control"),
    U("Connection"),
    U("Date"),
    U("Keep-Alive"),
    U("Pragma"),
    U("Trailer"),
    U("Transfer-Encoding"),
    U("Upgrade"),
    U("Via"),
    U("Warning"),
    U("Allow"),
    U("Content-Length"),
    U("Content-Type"),
    U("Content-Encoding"),
    U("Content-Language"),
    U("Content-Location"),
    U("Content-MD5"),
    U("Content-Range"),
    U("Expires"),
    U("Last-Modified"),
    U("Accept"),
    U("Accept-Charset"),
    U("Accept-Encoding"),
    U("Accept-Language"),
    U("Authorization"),
    U("Cookie"),
    U("Expect"),
    U("From"),
    U("Host"),
    U("If-Match"),
    U("If-Modified-Since"),
    U("If-None-Match"),
    U("If-Range"),
    U("If-Unmodified-Since"),
    U("Max-Forwards"),
    U("Proxy-Authorization"),
    U("Referer"),
    U("Range"),
    U("TE"),
    U("Translate"),
    U("User-Agent")
};

static utility::string_t char_to_wstring(const char* src)
{
    if (src == nullptr)
    {
        return utility::string_t();
    }
    std::string temp(src);
    return utility::string_t(temp.begin(), temp.end());
}

static std::map<utility::string_t, utility::string_t> parse_http_headers(const HTTP_REQUEST_HEADERS& headers)
{
    std::map<utility::string_t, utility::string_t> headers_map;
    for (USHORT i = 0; i < headers.UnknownHeaderCount; ++i)
    {
        headers_map[char_to_wstring(headers.pUnknownHeaders[i].pName)] =
            char_to_wstring(headers.pUnknownHeaders[i].pRawValue);
    }
    for (int i = 0; i < HttpHeaderMaximum; ++i)
    {
        if (headers.KnownHeaders[i].RawValueLength != 0)
        {
            headers_map[HttpServerAPIRequestKnownHeaders[i]] = char_to_wstring(headers.KnownHeaders[i].pRawValue);
        }
    }
    return headers_map;
}

struct ConcRTOversubscribe
{
    ConcRTOversubscribe()
    {
#if _MSC_VER >= 1800
        concurrency::Context::Oversubscribe(true);
#endif
    }
    ~ConcRTOversubscribe()
    {
#if _MSC_VER >= 1800
        concurrency::Context::Oversubscribe(false);
#endif
    }
};

class _test_http_server
{
    inline bool is_error_code(ULONG error_code)
    {
        return error_code == ERROR_OPERATION_ABORTED || error_code == ERROR_CONNECTION_INVALID ||
               error_code == ERROR_NETNAME_DELETED || m_closing == 1;
    }

public:
    _test_http_server(const web::uri& uri) : m_uri(uri), m_session(0), m_url_group(0), m_request_queue(nullptr)
    {
        // Open server session.
        HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;
        HttpInitialize(httpApiVersion, HTTP_INITIALIZE_SERVER, NULL);
        ULONG error_code = HttpCreateServerSession(httpApiVersion, &m_session, 0);
        if (error_code)
        {
            throw std::runtime_error("error code: " + std::to_string(error_code));
        }

        // Create Url group.
        error_code = HttpCreateUrlGroup(m_session, &m_url_group, 0);
        if (error_code)
        {
            throw std::runtime_error("error code: " + std::to_string(error_code));
        }

        // Create request queue.
        error_code = HttpCreateRequestQueue(httpApiVersion, U("test_http_server"), NULL, NULL, &m_request_queue);
        if (error_code)
        {
            throw std::runtime_error("error code: " + std::to_string(error_code));
        }

        // Windows HTTP Server API will not accept a uri with an empty path, it must have a '/'.
        auto host_uri = uri.to_string();
        if (uri.is_path_empty() && host_uri[host_uri.length() - 1] != '/' && uri.query().empty() &&
            uri.fragment().empty())
        {
            host_uri.append(U("/"));
        }

        // Add Url.
        error_code = HttpAddUrlToUrlGroup(m_url_group, host_uri.c_str(), (HTTP_URL_CONTEXT)this, 0);
        if (error_code)
        {
            throw std::runtime_error("error code: " + std::to_string(error_code));
        }

        // Associate Url group with request queue.
        HTTP_BINDING_INFO bindingInfo;
        bindingInfo.RequestQueueHandle = m_request_queue;
        bindingInfo.Flags.Present = 1;
        error_code =
            HttpSetUrlGroupProperty(m_url_group, HttpServerBindingProperty, &bindingInfo, sizeof(HTTP_BINDING_INFO));
        if (error_code)
        {
            throw std::runtime_error("error code: " + std::to_string(error_code));
        }

        // Launch listener thread
        m_thread = std::thread(
            [](_test_http_server* self) {
                for (;;)
                {
                    auto req = self->sync_get_request();
                    if (req == nullptr) break;

                    self->m_queue.on_request(std::move(req));
                }
            },
            this);
    }

    ~_test_http_server()
    {
        close();

        m_thread.join();

        HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
    }

    std::unique_ptr<test_request> sync_get_request()
    {
        ConcRTOversubscribe osubs; // Oversubscription for long running ConcRT tasks
        const ULONG buffer_length = 1024 * 4;
        char buffer[buffer_length];
        ULONG bytes_received = 0;
        HTTP_REQUEST* p_http_request = (HTTP_REQUEST*)buffer;

        // Read in everything except the body.
        ULONG error_code2 =
            HttpReceiveHttpRequest(m_request_queue, HTTP_NULL_ID, 0, p_http_request, buffer_length, &bytes_received, 0);
        if (error_code2 != 0)
        {
            return nullptr;
        }

        // Now create request structure.
        auto p_test_request = std::unique_ptr<test_request>(new test_request(p_http_request->RequestId, this));
        p_test_request->m_path = utf8_to_utf16(p_http_request->pRawUrl);
        p_test_request->m_method = parse_verb(p_http_request);
        p_test_request->m_headers = parse_http_headers(p_http_request->Headers);

        // Read in request body.
        ULONG content_length;
        const bool has_content_length = p_test_request->match_header(U("Content-Length"), content_length);
        if (has_content_length && content_length > 0)
        {
            p_test_request->m_body.resize(content_length);
            auto result = HttpReceiveRequestEntityBody(m_request_queue,
                                                       p_http_request->RequestId,
                                                       HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER,
                                                       &p_test_request->m_body[0],
                                                       content_length,
                                                       &bytes_received,
                                                       NULL);
            if (result != 0) return nullptr;
        }

        utility::string_t transfer_encoding;
        const bool has_transfer_encoding = p_test_request->match_header(U("Transfer-Encoding"), transfer_encoding);
        if (has_transfer_encoding && transfer_encoding.find(U("chunked")) != std::string::npos)
        {
            content_length = 0;
            char buf[4096];
            auto result = HttpReceiveRequestEntityBody(m_request_queue,
                                                       p_http_request->RequestId,
                                                       HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER,
                                                       (LPVOID)buf,
                                                       4096,
                                                       &bytes_received,
                                                       NULL);

            while (result == NO_ERROR)
            {
                content_length += bytes_received;
                p_test_request->m_body.resize(content_length);
                memcpy(&p_test_request->m_body[content_length - bytes_received], buf, bytes_received);

                result = HttpReceiveRequestEntityBody(m_request_queue,
                                                      p_http_request->RequestId,
                                                      HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER,
                                                      (LPVOID)buf,
                                                      4096,
                                                      &bytes_received,
                                                      NULL);
            }

            if (is_error_code(result))
                return nullptr;
            else
                VERIFY_ARE_EQUAL(ERROR_HANDLE_EOF, result);
        }

        return p_test_request;
    }

    unsigned long close()
    {
        m_closing = 1;

        // Windows HTTP Server API will not accept a uri with an empty path, it must have a '/'.
        utility::string_t host_uri = m_uri.to_string();
        if (m_uri.is_path_empty() && host_uri[host_uri.length() - 1] != '/' && m_uri.query().empty() &&
            m_uri.fragment().empty())
        {
            host_uri.append(U("/"));
        }

        // Remove Url.
        ULONG error_code = HttpRemoveUrlFromUrlGroup(m_url_group, host_uri.c_str(), 0);
        if (error_code)
        {
            return error_code;
        }

        // Stop request queue.
        error_code = HttpShutdownRequestQueue(m_request_queue);
        if (error_code)
        {
            return error_code;
        }

        // Close all resources.
        HttpCloseRequestQueue(m_request_queue);
        HttpCloseUrlGroup(m_url_group);
        HttpCloseServerSession(m_session);

        m_queue.close();

        return 0;
    }

    unsigned long send_reply(const unsigned long long request_id,
                             const unsigned short status_code,
                             const utility::string_t& reason_phrase,
                             const std::map<utility::string_t, utility::string_t>& headers,
                             void* data,
                             size_t data_length)
    {
        ConcRTOversubscribe osubs; // Oversubscription for long running ConcRT tasks
        HTTP_RESPONSE response;
        ZeroMemory(&response, sizeof(HTTP_RESPONSE));
        response.StatusCode = status_code;
#pragma warning(suppress: 4244) // intentionally narrow wchar_t -> char
        std::string reason(reason_phrase.begin(), reason_phrase.end());
        response.pReason = reason.c_str();
        response.ReasonLength = (USHORT)reason.length();

        // Add headers.
        std::vector<std::string> headers_buffer;
        response.Headers.UnknownHeaderCount = (USHORT)headers.size() + 1;
        response.Headers.pUnknownHeaders = new HTTP_UNKNOWN_HEADER[headers.size() + 1];
        headers_buffer.resize(headers.size() * 2 + 2);

        // Add the no cache header.
        headers_buffer[0] = "Cache-Control";
        headers_buffer[1] = "no-cache";
        response.Headers.pUnknownHeaders[0].NameLength = (USHORT)headers_buffer[0].size();
        response.Headers.pUnknownHeaders[0].pName = headers_buffer[0].c_str();
        response.Headers.pUnknownHeaders[0].RawValueLength = (USHORT)headers_buffer[1].size();
        response.Headers.pUnknownHeaders[0].pRawValue = headers_buffer[1].c_str();

        // Add all other headers.
        if (!headers.empty())
        {
            int headerIndex = 1;
            for (auto iter = headers.begin(); iter != headers.end(); ++iter, ++headerIndex)
            {
                headers_buffer[headerIndex * 2] = utf16_to_utf8(iter->first);
                headers_buffer[headerIndex * 2 + 1] = utf16_to_utf8(iter->second);

                // TFS 624150
#pragma warning(push)
#pragma warning(disable : 6386)
                response.Headers.pUnknownHeaders[headerIndex].NameLength =
                    (USHORT)headers_buffer[headerIndex * 2].size();
#pragma warning(pop)

                response.Headers.pUnknownHeaders[headerIndex].pName = headers_buffer[headerIndex * 2].c_str();
                response.Headers.pUnknownHeaders[headerIndex].RawValueLength =
                    (USHORT)headers_buffer[headerIndex * 2 + 1].size();
                response.Headers.pUnknownHeaders[headerIndex].pRawValue = headers_buffer[headerIndex * 2 + 1].c_str();
            }
        }

        // Add body.
        response.EntityChunkCount = 0;
        HTTP_DATA_CHUNK dataChunk;
        if (data_length != 0)
        {
            response.EntityChunkCount = 1;
            dataChunk.DataChunkType = HttpDataChunkFromMemory;
            dataChunk.FromMemory.pBuffer = (void*)data;
            dataChunk.FromMemory.BufferLength = (ULONG)data_length;
            response.pEntityChunks = &dataChunk;
        }

        // Synchronously sending the request.
        unsigned long error_code = HttpSendHttpResponse(m_request_queue,
                                                        request_id,
                                                        HTTP_SEND_RESPONSE_FLAG_DISCONNECT,
                                                        &response,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        NULL);

        // Free memory needed for headers.
        if (response.Headers.UnknownHeaderCount != 0)
        {
            delete[] response.Headers.pUnknownHeaders;
        }

        return error_code;
    }

public:
    test_server_queue m_queue;

private:
    std::atomic<int> m_closing = 0;

    web::uri m_uri;
    HTTP_SERVER_SESSION_ID m_session;
    HTTP_URL_GROUP_ID m_url_group;
    HANDLE m_request_queue;

    std::thread m_thread;
};
#else
class _test_http_server
{
public:
    test_server_queue m_queue;

private:
    web::http::experimental::listener::http_listener m_listener;

    std::atomic<unsigned long> m_last_request_id;

    std::mutex m_response_lock;
    std::unordered_map<unsigned long long, web::http::http_request> m_responding_requests;

public:
    _test_http_server(const web::uri& uri) : m_listener(uri), m_last_request_id(0)
    {
        auto handler = [this](web::http::http_request result) -> void {
            auto tr = std::unique_ptr<test_request>(new test_request(this->m_last_request_id++, this));
            tr->m_method = result.method();
            tr->m_path = result.request_uri().resource().to_string();
            if (tr->m_path.empty()) tr->m_path = U("/");

            for (auto it = result.headers().begin(); it != result.headers().end(); ++it)
                tr->m_headers[it->first] = it->second;

            tr->m_body = result.extract_vector().get();

            {
                std::lock_guard<std::mutex> lock(m_response_lock);
                m_responding_requests[tr->m_request_id] = result;
            }

            m_queue.on_request(std::move(tr));
        };
        m_listener.support(handler);
        m_listener.support(web::http::methods::OPTIONS, handler);
        m_listener.support(web::http::methods::TRCE, handler);

        m_listener.open().wait();
    }

    ~_test_http_server() { close(); }

    void close()
    {
        m_listener.close().wait();
        m_queue.close();
    }

    unsigned long send_reply(unsigned long long request_id,
                             const unsigned short status_code,
                             const utility::string_t& reason_phrase,
                             const std::map<utility::string_t, utility::string_t>& headers,
                             void* data,
                             size_t data_length)
    {
        web::http::http_request request;
        {
            std::lock_guard<std::mutex> lock(m_response_lock);
            auto it = m_responding_requests.find(request_id);
            if (it == m_responding_requests.end()) throw std::runtime_error("no such request awaiting response");
            request = it->second;
            m_responding_requests.erase(it);
        }

        web::http::http_response response;
        response.set_status_code(status_code);
        response.set_reason_phrase(reason_phrase);

        for (auto it = headers.begin(); it != headers.end(); ++it)
            response.headers().add(it->first, it->second);

        unsigned char* data_bytes = reinterpret_cast<unsigned char*>(data);
        std::vector<unsigned char> body_data(data_bytes, data_bytes + data_length);
        response.set_body(std::move(body_data));

        request.reply(response).get();

        return 0;
    }
};
#endif

unsigned long test_request::reply_impl(const unsigned short status_code,
                                       const utility::string_t& reason_phrase,
                                       const std::map<utility::string_t, utility::string_t>& headers,
                                       void* data,
                                       size_t data_length)
{
    return m_p_server->send_reply(m_request_id, status_code, reason_phrase, headers, data, data_length);
}

test_http_server::test_http_server(const web::http::uri& uri)
{
    m_p_impl = std::unique_ptr<_test_http_server>(new _test_http_server(uri));
}

test_http_server::~test_http_server() {}

pplx::task<test_request*> test_http_server::next_request() { return m_p_impl->m_queue.next_request(); }

std::vector<pplx::task<test_request*>> test_http_server::next_requests(const size_t count)
{
    std::vector<pplx::task<test_request*>> ret;
    ret.reserve(count);
    for (size_t x = 0; x < count; ++x)
        ret.push_back(next_request());
    return ret;
}

void test_http_server::close() { m_p_impl->close(); }

} // namespace utilities
} // namespace http
} // namespace functional
} // namespace tests
