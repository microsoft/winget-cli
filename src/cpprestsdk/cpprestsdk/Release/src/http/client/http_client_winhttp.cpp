/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: Client-side APIs.
 *
 * This file contains the implementation for Windows Desktop, based on WinHTTP.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#include "../common/x509_cert_utilities.h"
#include "../common/internal_http_helpers.h"
#include "cpprest/http_headers.h"
#include "http_client_impl.h"
#ifdef WIN32
#include <Wincrypt.h>
#endif
#if defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
#include "winhttppal.h"
#endif
#include <atomic>

#if _WIN32_WINNT && (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
#include <VersionHelpers.h>
#endif

namespace
{
struct security_failure_message
{
    std::uint32_t flag;
    const char* text;
};

CPPREST_CONSTEXPR security_failure_message g_security_failure_messages[] = {
    {WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED,
     "WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED failed to check revocation status."},
    {WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT,
     "WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT SSL certificate is invalid."},
    {WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED,
     "WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED SSL certificate was revoked."},
    {WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA, "WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA SSL invalid CA."},
    {WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID,
     "WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID SSL common name does not match."},
    {WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID,
     "WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID SLL certificate is expired."},
    {WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR,
     "WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR internal error."},
};

std::string generate_security_failure_message(std::uint32_t flags)
{
    std::string result("SSL Error:");
    for (const auto& message : g_security_failure_messages)
    {
        if (flags & message.flag)
        {
            result.push_back(' ');
            result.append(message.text);
        }
    }

    return result;
}

} // unnamed namespace
namespace web
{
namespace http
{
namespace client
{
namespace details
{
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
static http::status_code parse_status_code(HINTERNET request_handle)
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
    return (unsigned short)stoi(buffer);
}

// Helper function to get the reason phrase from a WinHTTP response.
static utility::string_t parse_reason_phrase(HINTERNET request_handle)
{
    utility::string_t phrase;
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
    ::web::http::details::trim_nulls(phrase);
    return phrase;
}

/// <summary>
/// Parses a string containing HTTP headers.
/// </summary>
static void parse_winhttp_headers(HINTERNET request_handle, _In_z_ utility::char_t* headersStr, http_response& response)
{
    // Clear the header map for each new response; otherwise, the header values will be combined.
    response.headers().clear();

    // Status code and reason phrase.
    response.set_status_code(parse_status_code(request_handle));
    response.set_reason_phrase(parse_reason_phrase(request_handle));

    web::http::details::parse_headers_string(headersStr, response.headers());
}

// Helper function to build error messages.
static std::string build_error_msg(unsigned long code, const std::string& location)
{
    std::string msg(location);
    msg.append(": ");
    msg.append(std::to_string(code));
    msg.append(": ");
    msg.append(utility::details::platform_category().message(static_cast<int>(code)));
    return msg;
}

// Helper function to build an error message from a WinHTTP async result.
static std::string build_error_msg(_In_ WINHTTP_ASYNC_RESULT* error_result)
{
    switch (error_result->dwResult)
    {
        case API_RECEIVE_RESPONSE: return build_error_msg(error_result->dwError, "WinHttpReceiveResponse");
        case API_QUERY_DATA_AVAILABLE: return build_error_msg(error_result->dwError, "WinHttpQueryDataAvaliable");
        case API_READ_DATA: return build_error_msg(error_result->dwError, "WinHttpReadData");
        case API_WRITE_DATA: return build_error_msg(error_result->dwError, "WinHttpWriteData");
        case API_SEND_REQUEST: return build_error_msg(error_result->dwError, "WinHttpSendRequest");
        default: return build_error_msg(error_result->dwError, "Unknown WinHTTP Function");
    }
}


class memory_holder
{
    uint8_t* m_externalData;
    std::vector<uint8_t> m_internalData;
    size_t m_size;

public:
    memory_holder() : m_externalData(nullptr), m_size(0) {}

    void allocate_space(size_t length)
    {
        if (length > m_internalData.size())
        {
            m_internalData.resize(length);
        }
        m_externalData = nullptr;
    }

    inline void reassign_to(_In_opt_ uint8_t* block, size_t length)
    {
        assert(block != nullptr);
        m_externalData = block;
        m_size = length;
    }

    inline bool is_internally_allocated() const { return m_externalData == nullptr; }

    inline uint8_t* get() { return is_internally_allocated() ? &m_internalData[0] : m_externalData; }

    inline size_t size() const { return is_internally_allocated() ? m_internalData.size() : m_size; }
};

// Possible ways a message body can be sent/received.
enum msg_body_type
{
    no_body,
    content_length_chunked,
    transfer_encoding_chunked
};

static DWORD WinHttpDefaultProxyConstant() CPPREST_NOEXCEPT
{
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
#if _WIN32_WINNT < _WIN32_WINNT_WINBLUE
    if (!IsWindows8Point1OrGreater())
    {
        // Not Windows 8.1 or later, use the default proxy setting
        return WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    }
#endif // _WIN32_WINNT < _WIN32_WINNT_WINBLUE

    // Windows 8.1 or later, use the automatic proxy setting
    return WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
#else  // ^^^ _WIN32_WINNT >= _WIN32_WINNT_VISTA ^^^ // vvv _WIN32_WINNT < _WIN32_WINNT_VISTA vvv
    return WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
#endif // _WIN32_WINNT >= _WIN32_WINNT_VISTA
}

// Additional information necessary to track a WinHTTP request.
class winhttp_request_context final : public request_context
{
public:
    // Factory function to create requests on the heap.
    static std::shared_ptr<request_context> create_request_context(
        const std::shared_ptr<_http_client_communicator>& client, const http_request& request)
    {
        std::shared_ptr<winhttp_request_context> ret(new winhttp_request_context(client, request));
        ret->m_self_reference = ret;
        return std::move(ret);
    }

    ~winhttp_request_context() { cleanup(); }

    void allocate_request_space(_In_opt_ uint8_t* block, size_t length)
    {
        if (block == nullptr)
            m_body_data.allocate_space(length);
        else
            m_body_data.reassign_to(block, length);
    }

    void allocate_reply_space(_In_opt_ uint8_t* block, size_t length)
    {
        if (block == nullptr)
            m_body_data.allocate_space(length);
        else
            m_body_data.reassign_to(block, length);
    }

    bool is_externally_allocated() const { return !m_body_data.is_internally_allocated(); }

    HINTERNET m_request_handle;
    std::weak_ptr<winhttp_request_context>*
        m_request_handle_context; // owned by m_request_handle to be deleted by WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING

    bool m_proxy_authentication_tried;
    bool m_server_authentication_tried;

    size_t m_remaining_redirects;

    msg_body_type m_bodyType;

    utility::size64_t m_remaining_to_write;

    std::char_traits<uint8_t>::pos_type m_startingPosition;

    // If the user specified that to guarantee data buffering of request data, in case of challenged authentication
    // requests, etc... Then if the request stream buffer doesn't support seeking we need to copy the body chunks as it
    // is sent.
    concurrency::streams::istream m_readStream;
    std::unique_ptr<concurrency::streams::container_buffer<std::vector<uint8_t>>> m_readBufferCopy;
    virtual concurrency::streams::streambuf<uint8_t> _get_readbuffer() { return m_readStream.streambuf(); }

    // This self reference will keep us alive until finish() is called.
    std::shared_ptr<winhttp_request_context> m_self_reference;
    memory_holder m_body_data;

    // Compress/decompress-related processing state lives here
    class compression_state
    {
    public:
        compression_state()
            : m_acquired(nullptr)
            , m_bytes_read(0)
            , m_bytes_processed(0)
            , m_needs_flush(false)
            , m_started(false)
            , m_done(false)
            , m_chunked(false)
        {
        }

#if (defined(_MSC_VER) && _MSC_VER < 1900) || defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
        compression_state(const compression_state&) = delete;
        compression_state(compression_state&& other)
            : m_buffer(std::move(other.m_buffer))
            , m_acquired(other.m_acquired)
            , m_bytes_read(other.m_bytes_read)
            , m_bytes_processed(other.m_bytes_processed)
            , m_needs_flush(other.m_needs_flush)
            , m_started(other.m_started)
            , m_done(other.m_done)
            , m_chunked(other.m_chunked)
            , m_chunk_bytes(other.m_chunk_bytes)
            , m_chunk(std::move(other.m_chunk))
        {
        }
        compression_state& operator=(const compression_state&) = delete;
        compression_state& operator=(compression_state&& other)
        {
            m_buffer = std::move(other.m_buffer);
            m_acquired = other.m_acquired;
            m_bytes_read = other.m_bytes_read;
            m_bytes_processed = other.m_bytes_processed;
            m_needs_flush = other.m_needs_flush;
            m_started = other.m_started;
            m_done = other.m_done;
            m_chunked = other.m_chunked;
            m_chunk_bytes = other.m_chunk_bytes;
            m_chunk = std::move(other.m_chunk);
            return *this;
        }
#endif // defined(_MSC_VER) && _MSC_VER < 1900

        // Minimal state for on-the-fly decoding of "chunked" encoded data
        class _chunk_helper
        {
        public:
            _chunk_helper()
                : m_bytes_remaining(0)
                , m_chunk_size(true)
                , m_chunk_delim(false)
                , m_expect_linefeed(false)
                , m_ignore(false)
                , m_trailer(false)
            {
            }

            // Returns true if the end of chunked data has been reached, specifically whether the 0-length
            // chunk and its trailing delimiter has been processed.  Otherwise, offset and length bound the
            // portion of buffer that represents a contiguous (and possibly partial) chunk of consumable
            // data; offset+length is the total number of bytes processed from the buffer on this pass.
            bool process_buffer(uint8_t* buffer, size_t buffer_size, size_t& offset, size_t& length)
            {
                bool done = false;
                size_t n = 0;
                size_t l = 0;

                while (n < buffer_size)
                {
                    if (m_ignore)
                    {
                        if (m_expect_linefeed)
                        {
                            _ASSERTE(m_chunk_delim && m_trailer);
                            if (buffer[n] != '\n')
                            {
                                // The data stream does not conform to "chunked" encoding
                                throw http_exception(status_codes::BadRequest, "Transfer-Encoding malformed trailer");
                            }

                            // Look for further trailer fields or the end of the stream
                            m_expect_linefeed = false;
                            m_trailer = false;
                        }
                        else if (buffer[n] == '\r')
                        {
                            if (!m_trailer)
                            {
                                // We're at the end of the data we need to ignore
                                _ASSERTE(m_chunk_size || m_chunk_delim);
                                m_ignore = false;
                                m_chunk_delim = false; // this is only set if we're at the end of the message
                            }                          // else we're at the end of a trailer field
                            m_expect_linefeed = true;
                        }
                        else if (m_chunk_delim)
                        {
                            // We're processing (and ignoring) a trailer field
                            m_trailer = true;
                        }
                    }
                    else if (m_expect_linefeed)
                    {
                        // We've already seen a carriage return; confirm the linefeed
                        if (buffer[n] != '\n')
                        {
                            // The data stream does not conform to "chunked" encoding
                            throw http_exception(status_codes::BadRequest, "Transfer-Encoding malformed delimiter");
                        }
                        if (m_chunk_size)
                        {
                            if (!m_bytes_remaining)
                            {
                                // We're processing the terminating "empty" chunk; there's
                                // no data, we just need to confirm the final chunk delimiter,
                                // possibly ignoring a trailer part along the way
                                m_ignore = true;
                                m_chunk_delim = true;
                            } // else we move on to the chunk data itself
                            m_chunk_size = false;
                        }
                        else
                        {
                            // Now we move on to the next chunk size
                            _ASSERTE(!m_bytes_remaining);
                            if (m_chunk_delim)
                            {
                                // We expect a chunk size next
                                m_chunk_size = true;
                            }
                            else
                            {
                                // We just processed the end-of-input delimiter
                                done = true;
                            }
                            m_chunk_delim = false;
                        }
                        m_expect_linefeed = false;
                    }
                    else if (m_chunk_delim)
                    {
                        // We're processing a post-chunk delimiter
                        if (buffer[n] != '\r')
                        {
                            // The data stream does not conform to "chunked" encoding
                            throw http_exception(status_codes::BadRequest,
                                                 "Transfer-Encoding malformed chunk delimiter");
                        }

                        // We found the carriage return; look for the linefeed
                        m_expect_linefeed = true;
                    }
                    else if (m_chunk_size)
                    {
                        // We're processing an ASCII hexadecimal chunk size
                        if (buffer[n] >= 'a' && buffer[n] <= 'f')
                        {
                            m_bytes_remaining *= 16;
                            m_bytes_remaining += 10 + buffer[n] - 'a';
                        }
                        else if (buffer[n] >= 'A' && buffer[n] <= 'F')
                        {
                            m_bytes_remaining *= 16;
                            m_bytes_remaining += 10 + buffer[n] - 'A';
                        }
                        else if (buffer[n] >= '0' && buffer[n] <= '9')
                        {
                            m_bytes_remaining *= 16;
                            m_bytes_remaining += buffer[n] - '0';
                        }
                        else if (buffer[n] == '\r')
                        {
                            // We've reached the end of the size, and there's no chunk extension
                            m_expect_linefeed = true;
                        }
                        else if (buffer[n] == ';')
                        {
                            // We've reached the end of the size, and there's a chunk extension;
                            // we don't support extensions, so we ignore them per RFC
                            m_ignore = true;
                        }
                        else
                        {
                            // The data stream does not conform to "chunked" encoding
                            throw http_exception(status_codes::BadRequest,
                                                 "Transfer-Encoding malformed chunk size or extension");
                        }
                    }
                    else
                    {
                        if (m_bytes_remaining)
                        {
                            // We're at the offset of a chunk of consumable data; let the caller process it
                            l = (std::min)(m_bytes_remaining, buffer_size - n);
                            m_bytes_remaining -= l;
                            if (!m_bytes_remaining)
                            {
                                // We're moving on to the post-chunk delimiter
                                m_chunk_delim = true;
                            }
                        }
                        else
                        {
                            // We've previously processed the terminating empty chunk and its
                            // trailing delimiter; skip the entire buffer, and inform the caller
                            n = buffer_size;
                            done = true;
                        }

                        // Let the caller process the result
                        break;
                    }

                    // Move on to the next byte
                    n++;
                }

                offset = n;
                length = l;
                return buffer_size ? done : (!m_bytes_remaining && !m_chunk_size && !m_chunk_delim);
            }

        private:
            size_t m_bytes_remaining; // the number of bytes remaining in the chunk we're currently processing
            bool m_chunk_size;        // if true, we're processing a chunk size or its trailing delimiter
            bool m_chunk_delim;       // if true, we're processing a delimiter between a chunk and the next chunk's size
            bool m_expect_linefeed; // if true, we're processing a delimiter, and we've already seen its carriage return
            bool m_ignore;          // if true, we're processing a chunk extension or trailer, which we don't support
            bool m_trailer;         // if true, we're processing (and ignoring) a trailer field; m_ignore is also true
        };

        std::vector<uint8_t> m_buffer; // we read data from the stream into this before compressing
        uint8_t* m_acquired; // we use this in place of m_buffer if the stream has directly-accessible data available
        size_t m_bytes_read; // we most recently read this many bytes, which may be less than m_buffer.size()
        size_t m_bytes_processed; // we've compressed this many bytes of m_bytes_read so far
        bool m_needs_flush;   // we've read and compressed all bytes, but the compressor still has compressed bytes to
                              // give us
        bool m_started;       // we've sent at least some number of bytes to m_decompressor
        bool m_done;          // we've read, compressed, and consumed all bytes
        bool m_chunked;       // if true, we need to decode and decompress a transfer-encoded message
        size_t m_chunk_bytes; // un-decompressed bytes remaining in the most-recently-obtained data from m_chunk
        std::unique_ptr<_chunk_helper> m_chunk;
    } m_compression_state;

    void cleanup()
    {
        if (m_compression_state.m_acquired != nullptr)
        {
            // We may still hold a piece of the buffer if we encountered an exception; release it here
            if (m_decompressor)
            {
                _get_writebuffer().commit(0);
            }
            else
            {
                _get_readbuffer().release(m_compression_state.m_acquired, m_compression_state.m_bytes_processed);
            }
            m_compression_state.m_acquired = nullptr;
        }

        if (m_request_handle != nullptr)
        {
            WinHttpCloseHandle(m_request_handle);
        }
    }

    void install_custom_cn_check(const utility::string_t& customHost)
    {
        m_customCnCheck = customHost;
        utility::details::inplace_tolower(m_customCnCheck);
    }

    void on_send_request_validate_cn()
    {
#if defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
        // we do the validation inside curl
        return;
#else
        if (m_customCnCheck.empty())
        {
            // no custom validation selected; either we've delegated that to winhttp or
            // certificate checking is completely disabled
            return;
        }

        winhttp_cert_context certContext;
        DWORD bufferSize = sizeof(certContext.raw);
        if (!WinHttpQueryOption(m_request_handle, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &certContext.raw, &bufferSize))
        {
            auto errorCode = GetLastError();
            if (errorCode == HRESULT_CODE(WININET_E_INCORRECT_HANDLE_STATE))
            {
                // typically happens when given a custom host with an initially HTTP connection
                return;
            }

            report_error(errorCode,
                         build_error_msg(errorCode, "WinHttpQueryOption WINHTTP_OPTION_SERVER_CERT_CONTEXT"));
            cleanup();
            return;
        }

        const auto encodedFirst = certContext.raw->pbCertEncoded;
        const auto encodedLast = encodedFirst + certContext.raw->cbCertEncoded;
        if (certContext.raw->cbCertEncoded == m_cachedEncodedCert.size() &&
            std::equal(encodedFirst, encodedLast, m_cachedEncodedCert.begin()))
        {
            // already validated OK
            return;
        }

        char oidPkixKpServerAuth[] = szOID_PKIX_KP_SERVER_AUTH;
        char oidServerGatedCrypto[] = szOID_SERVER_GATED_CRYPTO;
        char oidSgcNetscape[] = szOID_SGC_NETSCAPE;
        char* chainUses[] = {
            oidPkixKpServerAuth,
            oidServerGatedCrypto,
            oidSgcNetscape,
        };

        winhttp_cert_chain_context chainContext;
        CERT_CHAIN_PARA chainPara = {sizeof(chainPara)};
        chainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
        chainPara.RequestedUsage.Usage.cUsageIdentifier = sizeof(chainUses) / sizeof(char*);
        chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = chainUses;

        // note that the following chain only checks the end certificate; we
        // assume WinHTTP already validated everything but the common name.
        if (!CertGetCertificateChain(NULL,
                                     certContext.raw,
                                     nullptr,
                                     certContext.raw->hCertStore,
                                     &chainPara,
                                     CERT_CHAIN_CACHE_END_CERT,
                                     NULL,
                                     &chainContext.raw))
        {
            auto errorCode = GetLastError();
            report_error(errorCode, build_error_msg(errorCode, "CertGetCertificateChain"));
            cleanup();
            return;
        }

        HTTPSPolicyCallbackData policyData = {
            {sizeof(policyData)},
            AUTHTYPE_SERVER,
            // we assume WinHTTP already checked these:
            0x00000080       /* SECURITY_FLAG_IGNORE_REVOCATION */
                | 0x00000100 /* SECURITY_FLAG_IGNORE_UNKNOWN_CA */
                | 0x00000200 /* SECURITY_FLAG_IGNORE_WRONG_USAGE */
                | 0x00002000 /* SECURITY_FLAG_IGNORE_CERT_DATE_INVALID */,
            &m_customCnCheck[0],
        };
        CERT_CHAIN_POLICY_PARA policyPara = {sizeof(policyPara)};
        policyPara.pvExtraPolicyPara = &policyData;

        CERT_CHAIN_POLICY_STATUS policyStatus = {sizeof(policyStatus)};
        if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chainContext.raw, &policyPara, &policyStatus))
        {
            auto errorCode = GetLastError();
            report_error(errorCode, build_error_msg(errorCode, "CertVerifyCertificateChainPolicy"));
            cleanup();
            return;
        }

        if (policyStatus.dwError)
        {
            report_error(policyStatus.dwError, build_error_msg(policyStatus.dwError, "Incorrect common name"));
            cleanup();
            return;
        }

        m_cachedEncodedCert.assign(encodedFirst, encodedLast);
#endif
    }

protected:
    virtual void finish() override
    {
        request_context::finish();
        assert(m_self_reference != nullptr);
        auto dereference_self = std::move(m_self_reference);
        // As the stack frame cleans up, this will be deleted if no other references exist.
    }

private:
    utility::string_t m_customCnCheck;
    std::vector<unsigned char> m_cachedEncodedCert;

    // Can only create on the heap using factory function.
    winhttp_request_context(const std::shared_ptr<_http_client_communicator>& client, const http_request& request)
        : request_context(client, request)
        , m_request_handle(nullptr)
        , m_proxy_authentication_tried(false)
        , m_server_authentication_tried(false)
        , m_remaining_redirects(0)
        , m_bodyType(no_body)
        , m_remaining_to_write(0)
        , m_startingPosition(std::char_traits<uint8_t>::eof())
        , m_readStream(request.body())
        , m_body_data()
    {
    }
};

static DWORD ChooseAuthScheme(DWORD dwSupportedSchemes)
{
    //  It is the server's responsibility only to accept
    //  authentication schemes that provide a sufficient
    //  level of security to protect the servers resources.
    //
    //  The client is also obligated only to use an authentication
    //  scheme that adequately protects its username and password.
    //
    if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NEGOTIATE)
        return WINHTTP_AUTH_SCHEME_NEGOTIATE;
    else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NTLM)
        return WINHTTP_AUTH_SCHEME_NTLM;
    else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_PASSPORT)
        return WINHTTP_AUTH_SCHEME_PASSPORT;
    else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_DIGEST)
        return WINHTTP_AUTH_SCHEME_DIGEST;
    else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_BASIC)
        return WINHTTP_AUTH_SCHEME_BASIC;
    else
        return 0;
}

// Small RAII helper to ensure that the fields of this struct are always
// properly freed.
struct proxy_info : WINHTTP_PROXY_INFO
{
    proxy_info() { memset(this, 0, sizeof(WINHTTP_PROXY_INFO)); }

    ~proxy_info()
    {
        if (lpszProxy) ::GlobalFree(lpszProxy);
        if (lpszProxyBypass) ::GlobalFree(lpszProxyBypass);
    }
};

struct ie_proxy_config : WINHTTP_CURRENT_USER_IE_PROXY_CONFIG
{
    ie_proxy_config() { memset(this, 0, sizeof(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG)); }

    ~ie_proxy_config()
    {
        if (lpszAutoConfigUrl) ::GlobalFree(lpszAutoConfigUrl);
        if (lpszProxy) ::GlobalFree(lpszProxy);
        if (lpszProxyBypass) ::GlobalFree(lpszProxyBypass);
    }
};

// WinHTTP client.
class winhttp_client final : public _http_client_communicator
{
public:
    winhttp_client(http::uri address, http_client_config client_config)
        : _http_client_communicator(std::move(address), std::move(client_config))
        , m_opened(false)
        , m_hSession(nullptr)
        , m_hConnection(nullptr)
        , m_secure(m_uri.scheme() == _XPLATSTR("https"))
    {
    }

    winhttp_client(const winhttp_client&) = delete;
    winhttp_client& operator=(const winhttp_client&) = delete;

    // Closes session.
    ~winhttp_client()
    {
        if (m_hConnection != nullptr)
        {
            WinHttpCloseHandle(m_hConnection);
        }

        if (m_hSession != nullptr)
        {
            // Unregister the callback.
            WinHttpSetStatusCallback(m_hSession, nullptr, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);

            WinHttpCloseHandle(m_hSession);
        }
    }

    virtual pplx::task<http_response> propagate(http_request request) override
    {
        auto self = std::static_pointer_cast<_http_client_communicator>(shared_from_this());
        auto context = details::winhttp_request_context::create_request_context(self, request);

        // Use a task to externally signal the final result and completion of the task.
        auto result_task = pplx::create_task(context->m_request_completion);

        // Asynchronously send the response with the HTTP client implementation.
        this->async_send_request(context);

        return result_task;
    }

protected:
    // Open session and connection with the server.
    unsigned long open()
    {
        if (m_opened)
        {
            return 0;
        }

        pplx::extensibility::scoped_critical_section_t l(m_client_lock);
        if (m_opened)
        {
            return 0;
        }

        // This object have lifetime greater than proxy_name and proxy_bypass
        // which may point to its elements.
        ie_proxy_config proxyIE;

        DWORD access_type;
        LPCTSTR proxy_name = WINHTTP_NO_PROXY_NAME;
        LPCTSTR proxy_bypass = WINHTTP_NO_PROXY_BYPASS;
        m_proxy_auto_config = false;
        utility::string_t proxy_str;
        http::uri uri;

        const auto& config = client_config();
        const auto& proxy = config.proxy();
        if (proxy.is_default())
        {
            access_type = WinHttpDefaultProxyConstant();
        }
        else if (proxy.is_disabled())
        {
            access_type = WINHTTP_ACCESS_TYPE_NO_PROXY;
        }
        else if (proxy.is_auto_discovery())
        {
            access_type = WinHttpDefaultProxyConstant();
            if (access_type != WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY)
            {
                // Windows 8 or earlier, do proxy autodetection ourselves
                m_proxy_auto_config = true;

                proxy_info proxyDefault;
                if (!WinHttpGetDefaultProxyConfiguration(&proxyDefault) ||
                    proxyDefault.dwAccessType == WINHTTP_ACCESS_TYPE_NO_PROXY)
                {
                    // ... then try to fall back on the default WinINET proxy, as
                    // recommended for the desktop applications (if we're not
                    // running under a user account, the function below will just
                    // fail, so there is no real need to check for this explicitly)
                    if (WinHttpGetIEProxyConfigForCurrentUser(&proxyIE))
                    {
                        if (proxyIE.fAutoDetect)
                        {
                            m_proxy_auto_config = true;
                        }
                        else if (proxyIE.lpszAutoConfigUrl)
                        {
                            m_proxy_auto_config = true;
                            m_proxy_auto_config_url = proxyIE.lpszAutoConfigUrl;
                        }
                        else if (proxyIE.lpszProxy)
                        {
                            access_type = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                            proxy_name = proxyIE.lpszProxy;

                            if (proxyIE.lpszProxyBypass)
                            {
                                proxy_bypass = proxyIE.lpszProxyBypass;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            _ASSERTE(config.proxy().is_specified());
            access_type = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
            // WinHttpOpen cannot handle trailing slash in the name, so here is some string gymnastics to keep
            // WinHttpOpen happy proxy_str is intentionally declared at the function level to avoid pointing to the
            // string in the destructed object
            uri = config.proxy().address();
            if (uri.is_port_default())
            {
                proxy_name = uri.host().c_str();
            }
            else
            {
                proxy_str = uri.host();
                if (uri.port() > 0)
                {
                    proxy_str.push_back(_XPLATSTR(':'));
                    proxy_str.append(::utility::conversions::details::to_string_t(uri.port()));
                }

                proxy_name = proxy_str.c_str();
            }
        }

        // Open session.
        m_hSession = WinHttpOpen(NULL, access_type, proxy_name, proxy_bypass, WINHTTP_FLAG_ASYNC);
        if (!m_hSession)
        {
            return GetLastError();
        }

        {
            // Set timeouts.
            const int milliseconds =
                (std::max)(static_cast<int>(config.timeout<std::chrono::milliseconds>().count()), 1);
            if (!WinHttpSetTimeouts(m_hSession, milliseconds, milliseconds, milliseconds, milliseconds))
            {
                return GetLastError();
            }
        }

        if (config.guarantee_order())
        {
            // Set max connection to use per server to 1.
            DWORD maxConnections = 1;
            if (!WinHttpSetOption(
                    m_hSession, WINHTTP_OPTION_MAX_CONNS_PER_SERVER, &maxConnections, sizeof(maxConnections)))
            {
                return GetLastError();
            }
        }

        {
            // Enable TLS 1.1 and 1.2
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA) || defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
            DWORD secure_protocols(WINHTTP_FLAG_SECURE_PROTOCOL_SSL3 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 |
                                   WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2);
            if (!WinHttpSetOption(
                m_hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols)))
            {
                return GetLastError();
            }
#endif
        }

        config._invoke_nativesessionhandle_options(m_hSession);

        // Register asynchronous callback.
        if (WINHTTP_INVALID_STATUS_CALLBACK ==
            WinHttpSetStatusCallback(m_hSession,
                                     &winhttp_client::completion_callback,
                                     WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_HANDLES |
                                         WINHTTP_CALLBACK_FLAG_SECURE_FAILURE | WINHTTP_CALLBACK_FLAG_SEND_REQUEST |
                                         WINHTTP_CALLBACK_STATUS_REDIRECT,
                                     0))
        {
            return GetLastError();
        }

        // Open connection.
        unsigned int port = m_uri.is_port_default()
                                ? (m_secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT)
                                : m_uri.port();
        m_hConnection = WinHttpConnect(m_hSession, m_uri.host().c_str(), (INTERNET_PORT)port, 0);

        if (m_hConnection == nullptr)
        {
            return GetLastError();
        }

        m_opened = true;
        return S_OK;
    }

    // Start sending request.
    void send_request(_In_ const std::shared_ptr<request_context>& request)
    {
        // First see if we need to be opened.
        unsigned long error = open();
        if (error != 0)
        {
            // DO NOT TOUCH the this pointer after completing the request
            // This object could be freed along with the request as it could
            // be the last reference to this object
            request->report_error(error, _XPLATSTR("Open failed"));
            return;
        }

        http_request& msg = request->m_request;
        http_headers& headers = msg.headers();
        std::shared_ptr<winhttp_request_context> winhttp_context =
            std::static_pointer_cast<winhttp_request_context>(request);
        std::weak_ptr<winhttp_request_context> weak_winhttp_context = winhttp_context;

        proxy_info info;
        bool proxy_info_required = false;

        const auto& method = msg.method();

        // stop injection of headers via method
        // resource should be ok, since it's been encoded
        // and host won't resolve
        if (!::web::http::details::validate_method(method))
        {
            request->report_exception(http_exception("The method string is invalid."));
            return;
        }

        if (m_proxy_auto_config)
        {
            WINHTTP_AUTOPROXY_OPTIONS autoproxy_options {};
            if (m_proxy_auto_config_url.empty())
            {
                autoproxy_options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
                autoproxy_options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
            }
            else
            {
                autoproxy_options.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
                autoproxy_options.lpszAutoConfigUrl = m_proxy_auto_config_url.c_str();
            }

            autoproxy_options.fAutoLogonIfChallenged = TRUE;

            auto result = WinHttpGetProxyForUrl(m_hSession, m_uri.to_string().c_str(), &autoproxy_options, &info);
            if (result)
            {
                proxy_info_required = true;
            }
            else
            {
                // Failure to download the auto-configuration script is not fatal. Fall back to the default proxy.
            }
        }

        // Need to form uri path, query, and fragment for this request.
        // Make sure to keep any path that was specified with the uri when the http_client was created.
        const utility::string_t encoded_resource =
            http::uri_builder(m_uri).append(msg.relative_uri()).to_uri().resource().to_string();

        // Open the request.
        winhttp_context->m_request_handle_context = new std::weak_ptr<winhttp_request_context>(winhttp_context);

        winhttp_context->m_request_handle =
            WinHttpOpenRequest(m_hConnection,
                               msg.method().c_str(),
                               encoded_resource.c_str(),
                               nullptr,
                               WINHTTP_NO_REFERER,
                               WINHTTP_DEFAULT_ACCEPT_TYPES,
                               WINHTTP_FLAG_ESCAPE_DISABLE | (m_secure ? WINHTTP_FLAG_SECURE : 0));
        if (winhttp_context->m_request_handle == nullptr)
        {
            auto errorCode = GetLastError();
            delete winhttp_context->m_request_handle_context;
            winhttp_context->m_request_handle_context = 0;
            request->report_error(errorCode, build_error_msg(errorCode, "WinHttpOpenRequest"));
            return;
        }

        if (!WinHttpSetOption(winhttp_context->m_request_handle,
                              WINHTTP_OPTION_CONTEXT_VALUE,
                              &winhttp_context->m_request_handle_context,
                              sizeof(void*)))
        {
            auto errorCode = GetLastError();
            delete winhttp_context->m_request_handle_context;
            winhttp_context->m_request_handle_context = 0;
            request->report_error(errorCode, build_error_msg(errorCode, "WinHttpSetOption request context"));
            return;
        }

        if (proxy_info_required)
        {
            auto result = WinHttpSetOption(
                winhttp_context->m_request_handle, WINHTTP_OPTION_PROXY, &info, sizeof(WINHTTP_PROXY_INFO));
            if (!result)
            {
                auto errorCode = GetLastError();
                request->report_error(errorCode, build_error_msg(errorCode, "Setting proxy options"));
                return;
            }
        }

        // If credentials are specified, use autologon policy: WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH
        //    => default credentials are not used.
        // Else, the default autologon policy WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM will be used.
        if (client_config().credentials().is_set())
        {
            DWORD data = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;

            auto result = WinHttpSetOption(
                winhttp_context->m_request_handle, WINHTTP_OPTION_AUTOLOGON_POLICY, &data, sizeof(data));
            if (!result)
            {
                auto errorCode = GetLastError();
                request->report_error(
                    errorCode,
                    build_error_msg(errorCode, "Setting autologon policy to WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH"));
                return;
            }
        }

        // Check to turn off server certificate verification.
        DWORD ignoredCertificateValidationSteps = 0;
        if (client_config().validate_certificates())
        {
            // if we are validating certificates, also turn on revocation checking
            DWORD dwEnableSSLRevocationOpt = WINHTTP_ENABLE_SSL_REVOCATION;
            if (!WinHttpSetOption(winhttp_context->m_request_handle,
                                  WINHTTP_OPTION_ENABLE_FEATURE,
                                  &dwEnableSSLRevocationOpt,
                                  sizeof(dwEnableSSLRevocationOpt)))
            {
                auto errorCode = GetLastError();
                request->report_error(errorCode, build_error_msg(errorCode, "Error enabling SSL revocation check"));
                return;
            }

            // check if the user has overridden the desired Common Name with the host header
            const auto hostHeader = headers.find(_XPLATSTR("Host"));
            if (hostHeader != headers.end())
            {
                const auto& requestHost = hostHeader->second;
                if (!utility::details::str_iequal(requestHost, m_uri.host()))
                {
                    winhttp_context->install_custom_cn_check(requestHost);
                    ignoredCertificateValidationSteps = SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
                }
            }
        }
        else
        {
            ignoredCertificateValidationSteps =
                SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        }

        if (ignoredCertificateValidationSteps && !WinHttpSetOption(winhttp_context->m_request_handle,
                                                                   WINHTTP_OPTION_SECURITY_FLAGS,
                                                                   &ignoredCertificateValidationSteps,
                                                                   sizeof(ignoredCertificateValidationSteps)))
        {
            auto errorCode = GetLastError();
            request->report_error(errorCode,
                                  build_error_msg(errorCode, "Setting ignore server certificate verification"));
            return;
        }

// WinHttpPAL does not currently provide these options
// See https://github.com/microsoft/WinHttpPAL/issues/1
#if !defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
        if (client_config().max_redirects() == 0)
        {
            // Disable auto redirects.
            DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
            if (!WinHttpSetOption(winhttp_context->m_request_handle,
                                  WINHTTP_OPTION_REDIRECT_POLICY,
                                  &redirectPolicy,
                                  sizeof(redirectPolicy)))
            {
                auto errorCode = GetLastError();
                request->report_error(errorCode, build_error_msg(errorCode, "Setting redirect policy"));
                return;
            }
            // Note, using WINHTTP_OPTION_DISABLE_FEATURE with WINHTTP_DISABLE_REDIRECTS here doesn't seem to work.
        }
        else
        {
            // Set max auto redirects.

            // Add 1 to config value because WinHttp option counts the original request.
            // And another 1 to enable the response (headers) of the rejected automatic redirect to be returned
            // rather than reporting an error "WinHttpReceiveResponse: 12156: The HTTP redirect request failed".
            DWORD maxRedirects = client_config().max_redirects() < MAXDWORD - 2
                ? static_cast<DWORD>(client_config().max_redirects() + 2)
                : MAXDWORD;
            // Therefore, effective max redirects
            winhttp_context->m_remaining_redirects = maxRedirects - 2;

            if (!WinHttpSetOption(winhttp_context->m_request_handle,
                                  WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS,
                                  &maxRedirects,
                                  sizeof(maxRedirects)))
            {
                auto errorCode = GetLastError();
                request->report_error(errorCode, build_error_msg(errorCode, "Setting max automatic redirects"));
                return;
            }

            // (Dis)allow HTTPS to HTTP redirects.
            DWORD redirectPolicy = client_config().https_to_http_redirects()
                ? WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS
                : WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
            if (!WinHttpSetOption(winhttp_context->m_request_handle,
                                  WINHTTP_OPTION_REDIRECT_POLICY,
                                  &redirectPolicy,
                                  sizeof(redirectPolicy)))
            {
                auto errorCode = GetLastError();
                request->report_error(errorCode, build_error_msg(errorCode, "Setting redirect policy"));
                return;
            }
        }
#endif

        size_t content_length;
        try
        {
            content_length = msg._get_impl()->_get_content_length_and_set_compression();
        }
        catch (...)
        {
            request->report_exception(std::current_exception());
            return;
        }
        if (content_length > 0)
        {
            if (msg.method() == http::methods::GET || msg.method() == http::methods::HEAD)
            {
                request->report_exception(http_exception(get_with_body_err_msg));
                return;
            }

            // There is a request body that needs to be transferred.
            if (content_length == (std::numeric_limits<size_t>::max)())
            {
                // The content length is not set and the application set a stream. This is an
                // indication that we will use transfer encoding chunked.  We still want to
                // know that stream's effective length if possible for memory efficiency.
                winhttp_context->m_bodyType = transfer_encoding_chunked;
                winhttp_context->m_remaining_to_write = msg._get_impl()->_get_stream_length();
            }
            else
            {
                // While we won't be transfer-encoding the data, we will write it in portions.
                winhttp_context->m_bodyType = content_length_chunked;
                winhttp_context->m_remaining_to_write = content_length;
            }
        }

        utility::string_t flattened_headers = web::http::details::flatten_http_headers(headers);
        if (winhttp_context->m_request.method() == http::methods::GET)
        {
            // Prepare to request a compressed response from the server if necessary.
            flattened_headers += winhttp_context->get_compression_header();
        }

        // Add headers.
        if (!flattened_headers.empty())
        {
            if (!WinHttpAddRequestHeaders(winhttp_context->m_request_handle,
                                          flattened_headers.c_str(),
                                          static_cast<DWORD>(flattened_headers.length()),
                                          WINHTTP_ADDREQ_FLAG_ADD))
            {
                auto errorCode = GetLastError();
                request->report_error(errorCode, build_error_msg(errorCode, "WinHttpAddRequestHeaders"));
                return;
            }
        }

        // Register for notification on cancellation to abort this request.
        if (msg._cancellation_token() != pplx::cancellation_token::none())
        {
            // cancellation callback is unregistered when request is completed.
            winhttp_context->m_cancellationRegistration =
                msg._cancellation_token().register_callback([weak_winhttp_context]() {
                    // Call the WinHttpSendRequest API after WinHttpCloseHandle will give invalid handle error and we
                    // throw this exception. Call the cleanup to make the m_request_handle as nullptr, otherwise,
                    // Application Verifier will give AV exception on m_request_handle.
                    auto lock = weak_winhttp_context.lock();
                    if (!lock) return;
                    lock->cleanup();
                });
        }

        // Call the callback function of user customized options.
        try
        {
            client_config().invoke_nativehandle_options(winhttp_context->m_request_handle);
        }
        catch (...)
        {
            request->report_exception(std::current_exception());
            return;
        }

        // Only need to cache the request body if user specified and the request stream doesn't support seeking.
        if (winhttp_context->m_bodyType != no_body && client_config().buffer_request() &&
            !winhttp_context->_get_readbuffer().can_seek())
        {
            winhttp_context->m_readBufferCopy =
                ::utility::details::make_unique<::concurrency::streams::container_buffer<std::vector<uint8_t>>>();
        }

        _start_request_send(winhttp_context, content_length);

        return;
    }

private:
    void _start_request_send(const std::shared_ptr<winhttp_request_context>& winhttp_context, size_t content_length)
    {
        DWORD totalLength;
        if (winhttp_context->m_bodyType == no_body)
        {
            totalLength = 0;
        }
        else
        {
            // Capture the current read position of the stream.
            auto rbuf = winhttp_context->_get_readbuffer();

            // Record starting position in case request is challenged for authorization
            // and needs to seek back to where reading is started from.
            winhttp_context->m_startingPosition = rbuf.getpos(std::ios_base::in);

            // If we find ourselves here, we either don't know how large the message
            totalLength = winhttp_context->m_bodyType == content_length_chunked ? (DWORD)content_length
                                                                                : WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH;
        }

        const auto requestSuccess = WinHttpSendRequest(winhttp_context->m_request_handle,
                                                       WINHTTP_NO_ADDITIONAL_HEADERS,
                                                       0,
                                                       nullptr,
                                                       0,
                                                       totalLength,
                                                       (DWORD_PTR)winhttp_context->m_request_handle_context);
        if (!requestSuccess)
        {
            auto errorCode = GetLastError();
            winhttp_context->report_error(errorCode, build_error_msg(errorCode, "WinHttpSendRequest"));
        }
    }

    // Helper function to query/read next part of response data from winhttp.
    static void read_next_response_chunk(winhttp_request_context* pContext, DWORD bytesRead, bool firstRead = false)
    {
        const bool defaultChunkSize = pContext->m_http_client->client_config().is_default_chunksize();

        // If user specified a chunk size then read in chunks instead of using query data available.
        if (defaultChunkSize)
        {
            if (!WinHttpQueryDataAvailable(pContext->m_request_handle, nullptr))
            {
                auto errorCode = GetLastError();
                pContext->report_error(errorCode, build_error_msg(errorCode, "WinHttpQueryDataAvaliable"));
            }
        }
        else
        {
            // If bytes read is less than the chunk size this request is done.
            // Is it really, though?  The WinHttpReadData docs suggest that less can be returned regardless...
            const size_t chunkSize = pContext->m_http_client->client_config().chunksize();
            std::unique_ptr<compression::decompress_provider>& decompressor = pContext->m_decompressor;
            if (!decompressor && bytesRead < chunkSize && !firstRead)
            {
                pContext->complete_request(pContext->m_downloaded);
            }
            else
            {
                uint8_t* buffer;

                if (decompressor)
                {
                    // m_buffer holds the compressed data; we'll decompress into the caller's buffer later
                    if (pContext->m_compression_state.m_buffer.capacity() < chunkSize)
                    {
                        pContext->m_compression_state.m_buffer.reserve(chunkSize);
                    }
                    buffer = pContext->m_compression_state.m_buffer.data();
                }
                else
                {
                    auto writebuf = pContext->_get_writebuffer();
                    pContext->allocate_reply_space(writebuf.alloc(chunkSize), chunkSize);
                    buffer = pContext->m_body_data.get();
                }

                if (!WinHttpReadData(pContext->m_request_handle, buffer, static_cast<DWORD>(chunkSize), nullptr))
                {
                    auto errorCode = GetLastError();
                    pContext->report_error(errorCode, build_error_msg(errorCode, "WinHttpReadData"));
                }
            }
        }
    }

    static void _transfer_encoding_chunked_write_data(_In_ winhttp_request_context* p_request_context)
    {
        size_t chunk_size;
        std::unique_ptr<compression::compress_provider>& compressor = p_request_context->m_request.compressor();

        // Set the chunk size up front; we need it before the lambda functions come into scope
        if (compressor)
        {
            // We could allocate less than a chunk for the compressed data here, though that
            // would result in more trips through this path for not-so-compressible data...
            if (p_request_context->m_body_data.size() > http::details::chunked_encoding::additional_encoding_space)
            {
                // If we've previously allocated space for the compressed data, don't reduce it
                chunk_size =
                    p_request_context->m_body_data.size() - http::details::chunked_encoding::additional_encoding_space;
            }
            else if (p_request_context->m_remaining_to_write != (std::numeric_limits<size_t>::max)())
            {
                // Choose a semi-intelligent size based on how much total data is left to compress
                chunk_size = (std::min)(static_cast<size_t>(p_request_context->m_remaining_to_write) + 128,
                                        p_request_context->m_http_client->client_config().chunksize());
            }
            else
            {
                // Just base our allocation on the chunk size, since we don't have any other data available
                chunk_size = p_request_context->m_http_client->client_config().chunksize();
            }
        }
        else
        {
            // We're not compressing; use the smaller of the remaining data (if known) and the configured (or default)
            // chunk size
            chunk_size = (std::min)(static_cast<size_t>(p_request_context->m_remaining_to_write),
                                    p_request_context->m_http_client->client_config().chunksize());
        }
        p_request_context->allocate_request_space(
            nullptr, chunk_size + http::details::chunked_encoding::additional_encoding_space);

        auto after_read = [p_request_context, chunk_size, &compressor](pplx::task<size_t> op) {
            size_t bytes_read;
            try
            {
                bytes_read = op.get();
                // If the read buffer for copying exists then write to it.
                if (p_request_context->m_readBufferCopy)
                {
                    // We have raw memory here writing to a memory stream so it is safe to wait
                    // since it will always be non-blocking.
                    if (!compressor)
                    {
                        p_request_context->m_readBufferCopy
                            ->putn_nocopy(
                                &p_request_context->m_body_data.get()[http::details::chunked_encoding::data_offset],
                                bytes_read)
                            .wait();
                    }
                }
            }
            catch (...)
            {
                p_request_context->report_exception(std::current_exception());
                return;
            }

            _ASSERTE(bytes_read != static_cast<size_t>(-1));

            size_t offset = http::details::chunked_encoding::add_chunked_delimiters(
                p_request_context->m_body_data.get(),
                chunk_size + http::details::chunked_encoding::additional_encoding_space,
                bytes_read);

            if (!compressor && p_request_context->m_remaining_to_write != (std::numeric_limits<size_t>::max)())
            {
                if (bytes_read == 0 && p_request_context->m_remaining_to_write)
                {
                    // The stream ended earlier than we detected it should
                    http_exception ex(
                        U("Unexpected end of request body stream encountered before expected length met."));
                    p_request_context->report_exception(ex);
                    return;
                }
                p_request_context->m_remaining_to_write -= bytes_read;
            }

            // Stop writing chunks if we reached the end of the stream.
            // Note that we could detect end-of-stream based on !m_remaining_to_write, and insert
            // the last (0) chunk if we have enough extra space... though we currently don't.
            if (bytes_read == 0)
            {
                p_request_context->m_bodyType = no_body;
                if (p_request_context->m_readBufferCopy)
                {
                    // Move the saved buffer into the read buffer, which now supports seeking.
                    p_request_context->m_readStream =
                        concurrency::streams::container_stream<std::vector<uint8_t>>::open_istream(
                            std::move(p_request_context->m_readBufferCopy->collection()));
                    p_request_context->m_readBufferCopy.reset();
                }
            }

            const auto length = bytes_read + (http::details::chunked_encoding::additional_encoding_space - offset);

            if (!WinHttpWriteData(p_request_context->m_request_handle,
                                  &p_request_context->m_body_data.get()[offset],
                                  static_cast<DWORD>(length),
                                  nullptr))
            {
                auto errorCode = GetLastError();
                p_request_context->report_error(errorCode, build_error_msg(errorCode, "WinHttpWriteData"));
            }
        };

        if (compressor)
        {
            auto do_compress =
                [p_request_context, chunk_size, &compressor](pplx::task<size_t> op) -> pplx::task<size_t> {
                size_t bytes_read;

                try
                {
                    bytes_read = op.get();
                }
                catch (...)
                {
                    return pplx::task_from_exception<size_t>(std::current_exception());
                }

                uint8_t* buffer = p_request_context->m_compression_state.m_acquired;
                if (buffer == nullptr)
                {
                    buffer = p_request_context->m_compression_state.m_buffer.data();
                }

                web::http::compression::operation_hint hint = web::http::compression::operation_hint::has_more;

                if (bytes_read)
                {
                    // An actual read always resets compression state for the next chunk
                    _ASSERTE(p_request_context->m_compression_state.m_bytes_processed ==
                             p_request_context->m_compression_state.m_bytes_read);
                    _ASSERTE(!p_request_context->m_compression_state.m_needs_flush);
                    p_request_context->m_compression_state.m_bytes_read = bytes_read;
                    p_request_context->m_compression_state.m_bytes_processed = 0;
                    if (p_request_context->m_readBufferCopy)
                    {
                        // If we've been asked to keep a copy of the raw data for restarts, do so here, pre-compression
                        p_request_context->m_readBufferCopy->putn_nocopy(buffer, bytes_read).wait();
                    }
                    if (p_request_context->m_remaining_to_write == bytes_read)
                    {
                        // We've read to the end of the stream; finalize here if possible.  We'll
                        // decrement the remaining count as we actually process the read buffer.
                        hint = web::http::compression::operation_hint::is_last;
                    }
                }
                else if (p_request_context->m_compression_state.m_needs_flush)
                {
                    // All input has been consumed, but we still need to collect additional compressed output;
                    // this is done (in theory it can be multiple times) as a finalizing operation
                    hint = web::http::compression::operation_hint::is_last;
                }
                else if (p_request_context->m_compression_state.m_bytes_processed ==
                         p_request_context->m_compression_state.m_bytes_read)
                {
                    if (p_request_context->m_remaining_to_write &&
                        p_request_context->m_remaining_to_write != (std::numeric_limits<size_t>::max)())
                    {
                        // The stream ended earlier than we detected it should
                        return pplx::task_from_exception<size_t>(http_exception(
                            U("Unexpected end of request body stream encountered before expected length met.")));
                    }

                    // We think we're done; inform the compression library so it can finalize and/or give us any pending
                    // compressed bytes. Note that we may end up here multiple times if m_needs_flush is set, until all
                    // compressed bytes are drained.
                    hint = web::http::compression::operation_hint::is_last;
                }
                // else we're still compressing bytes from the previous read

                _ASSERTE(p_request_context->m_compression_state.m_bytes_processed <=
                         p_request_context->m_compression_state.m_bytes_read);

                uint8_t* in = buffer + p_request_context->m_compression_state.m_bytes_processed;
                size_t inbytes = p_request_context->m_compression_state.m_bytes_read -
                                 p_request_context->m_compression_state.m_bytes_processed;
                return compressor
                    ->compress(in,
                               inbytes,
                               &p_request_context->m_body_data.get()[http::details::chunked_encoding::data_offset],
                               chunk_size,
                               hint)
                    .then([p_request_context, bytes_read, hint, chunk_size](
                              pplx::task<http::compression::operation_result> op) -> pplx::task<size_t> {
                        http::compression::operation_result r;

                        try
                        {
                            r = op.get();
                        }
                        catch (...)
                        {
                            return pplx::task_from_exception<size_t>(std::current_exception());
                        }

                        if (hint == web::http::compression::operation_hint::is_last)
                        {
                            // We're done reading all chunks, but the compressor may still have compressed bytes to
                            // drain from previous reads
                            _ASSERTE(r.done || r.output_bytes_produced == chunk_size);
                            p_request_context->m_compression_state.m_needs_flush = !r.done;
                            p_request_context->m_compression_state.m_done = r.done;
                        }

                        // Update the number of bytes compressed in this read chunk; if it's been fully compressed,
                        // we'll reset m_bytes_processed and m_bytes_read after reading the next chunk
                        p_request_context->m_compression_state.m_bytes_processed += r.input_bytes_processed;
                        _ASSERTE(p_request_context->m_compression_state.m_bytes_processed <=
                                 p_request_context->m_compression_state.m_bytes_read);
                        if (p_request_context->m_remaining_to_write != (std::numeric_limits<size_t>::max)())
                        {
                            _ASSERTE(p_request_context->m_remaining_to_write >= r.input_bytes_processed);
                            p_request_context->m_remaining_to_write -= r.input_bytes_processed;
                        }

                        if (p_request_context->m_compression_state.m_acquired != nullptr &&
                            p_request_context->m_compression_state.m_bytes_processed ==
                                p_request_context->m_compression_state.m_bytes_read)
                        {
                            // Release the acquired buffer back to the streambuf at the earliest possible point
                            p_request_context->_get_readbuffer().release(
                                p_request_context->m_compression_state.m_acquired,
                                p_request_context->m_compression_state.m_bytes_processed);
                            p_request_context->m_compression_state.m_acquired = nullptr;
                        }

                        return pplx::task_from_result<size_t>(r.output_bytes_produced);
                    });
            };

            if (p_request_context->m_compression_state.m_bytes_processed <
                    p_request_context->m_compression_state.m_bytes_read ||
                p_request_context->m_compression_state.m_needs_flush)
            {
                // We're still working on data from a previous read; continue compression without reading new data
                do_compress(pplx::task_from_result<size_t>(0)).then(after_read);
            }
            else if (p_request_context->m_compression_state.m_done)
            {
                // We just need to send the last (zero-length) chunk; there's no sense in going through the compression
                // path
                after_read(pplx::task_from_result<size_t>(0));
            }
            else
            {
                size_t length;

                // We need to read from the input stream, then compress before sending
                if (p_request_context->_get_readbuffer().acquire(p_request_context->m_compression_state.m_acquired,
                                                                 length))
                {
                    if (length == 0)
                    {
                        if (p_request_context->_get_readbuffer().exception())
                        {
                            p_request_context->report_exception(p_request_context->_get_readbuffer().exception());
                            return;
                        }
                        else if (p_request_context->m_remaining_to_write &&
                                 p_request_context->m_remaining_to_write != (std::numeric_limits<size_t>::max)())
                        {
                            // Unexpected end-of-stream.
                            p_request_context->report_error(GetLastError(),
                                                            _XPLATSTR("Outgoing HTTP body stream ended early."));
                            return;
                        }
                    }
                    else if (length > p_request_context->m_remaining_to_write)
                    {
                        // The stream grew, but we won't
                        length = static_cast<size_t>(p_request_context->m_remaining_to_write);
                    }

                    do_compress(pplx::task_from_result<size_t>(length)).then(after_read);
                }
                else
                {
                    length = (std::min)(static_cast<size_t>(p_request_context->m_remaining_to_write),
                                        p_request_context->m_http_client->client_config().chunksize());
                    if (p_request_context->m_compression_state.m_buffer.capacity() < length)
                    {
                        p_request_context->m_compression_state.m_buffer.reserve(length);
                    }
                    p_request_context->_get_readbuffer()
                        .getn(p_request_context->m_compression_state.m_buffer.data(), length)
                        .then(do_compress)
                        .then(after_read);
                }
            }
        }
        else
        {
            // We're not compressing; just read and chunk
            p_request_context->_get_readbuffer()
                .getn(&p_request_context->m_body_data.get()[http::details::chunked_encoding::data_offset], chunk_size)
                .then(after_read);
        }
    }

    static void _multiple_segment_write_data(_In_ winhttp_request_context* p_request_context)
    {
        auto rbuf = p_request_context->_get_readbuffer();
        msl::safeint3::SafeInt<utility::size64_t> safeCount = p_request_context->m_remaining_to_write;
        safeCount = safeCount.Min(p_request_context->m_http_client->client_config().chunksize());

        uint8_t* block = nullptr;
        size_t length = 0;
        if (rbuf.acquire(block, length))
        {
            if (length == 0)
            {
                // Unexpected end-of-stream.
                if (rbuf.exception() == nullptr)
                {
                    p_request_context->report_error(GetLastError(),
                                                    _XPLATSTR("Error reading outgoing HTTP body from its stream."));
                }
                else
                {
                    p_request_context->report_exception(rbuf.exception());
                }
                return;
            }

            p_request_context->allocate_request_space(block, length);

            const size_t to_write = safeCount.Min(length);

            // Stop writing chunks after this one if no more data.
            p_request_context->m_remaining_to_write -= to_write;
            if (p_request_context->m_remaining_to_write == 0)
            {
                p_request_context->m_bodyType = no_body;
            }

            if (!WinHttpWriteData(p_request_context->m_request_handle,
                                  p_request_context->m_body_data.get(),
                                  static_cast<DWORD>(to_write),
                                  nullptr))
            {
                auto errorCode = GetLastError();
                p_request_context->report_error(errorCode, build_error_msg(errorCode, "WinHttpWriteData"));
            }
        }
        else
        {
            p_request_context->allocate_request_space(nullptr, safeCount);

            rbuf.getn(p_request_context->m_body_data.get(), safeCount)
                .then([p_request_context, rbuf](pplx::task<size_t> op) {
                    size_t read;
                    try
                    {
                        read = op.get();
                    }
                    catch (...)
                    {
                        p_request_context->report_exception(std::current_exception());
                        return;
                    }
                    _ASSERTE(read != static_cast<size_t>(-1));

                    if (read == 0)
                    {
                        p_request_context->report_exception(http_exception(
                            U("Unexpected end of request body stream encountered before Content-Length met.")));
                        return;
                    }

                    p_request_context->m_remaining_to_write -= read;

                    // Stop writing chunks after this one if no more data.
                    if (p_request_context->m_remaining_to_write == 0)
                    {
                        p_request_context->m_bodyType = no_body;
                    }

                    if (!WinHttpWriteData(p_request_context->m_request_handle,
                                          p_request_context->m_body_data.get(),
                                          static_cast<DWORD>(read),
                                          nullptr))
                    {
                        auto errorCode = GetLastError();
                        p_request_context->report_error(errorCode, build_error_msg(errorCode, "WinHttpWriteData"));
                    }
                });
        }
    }

    static utility::string_t get_request_url(HINTERNET hRequestHandle)
    {
        utility::string_t url;
        auto urlSize = static_cast<unsigned long>(url.capacity()) * 2; // use initial small string optimization capacity
        for (;;)
        {
            url.resize(urlSize / sizeof(utility::char_t));
            if (WinHttpQueryOption(hRequestHandle, WINHTTP_OPTION_URL, &url[0], (LPDWORD)&urlSize))
            {
                url.resize(url.length());
                return url;
            }

            const auto lastError = GetLastError();
            if (lastError != ERROR_INSUFFICIENT_BUFFER || urlSize == 0)
            {
                url.clear();
                url.shrink_to_fit();
                return url;
            }
        }
    }

    // Returns true if we handle successfully and resending the request
    // or false if we fail to handle.
    static bool handle_authentication_failure(HINTERNET hRequestHandle,
                                              const std::shared_ptr<winhttp_request_context>& p_request_context,
                                              _In_ DWORD error = 0)
    {
        http_request& request = p_request_context->m_request;

        _ASSERTE(p_request_context->m_response.status_code() == status_codes::Unauthorized ||
                 p_request_context->m_response.status_code() == status_codes::ProxyAuthRequired ||
                 error == ERROR_WINHTTP_RESEND_REQUEST);

        // Check if the saved read position is valid
        auto rdpos = p_request_context->m_startingPosition;
        if (rdpos != static_cast<std::char_traits<uint8_t>::pos_type>(std::char_traits<uint8_t>::eof()))
        {
            // Try to seek back to the saved read position
            auto rbuf = p_request_context->_get_readbuffer();
            if (rbuf.seekpos(rdpos, std::ios::ios_base::in) != rdpos)
            {
                return false;
            }

            // We successfully seeked back; now reset the compression state, if any, to match
            if (p_request_context->m_request.compressor())
            {
                try
                {
                    p_request_context->m_request.compressor()->reset();
                }
                catch (...)
                {
                    return false;
                }
            }
        }
        p_request_context->m_compression_state = winhttp_request_context::compression_state();

        //  If we got ERROR_WINHTTP_RESEND_REQUEST, the response header is not available,
        //  we cannot call WinHttpQueryAuthSchemes and WinHttpSetCredentials.
        if (error != ERROR_WINHTTP_RESEND_REQUEST)
        {
            // Obtain the supported and preferred schemes.
            DWORD dwSupportedSchemes;
            DWORD dwFirstScheme;
            DWORD dwAuthTarget;
            if (!WinHttpQueryAuthSchemes(hRequestHandle, &dwSupportedSchemes, &dwFirstScheme, &dwAuthTarget))
            {
                // This will return the authentication failure to the user, without reporting fatal errors
                return false;
            }

            DWORD dwSelectedScheme = ChooseAuthScheme(dwSupportedSchemes);
            if (dwSelectedScheme == 0)
            {
                // This will return the authentication failure to the user, without reporting fatal errors
                return false;
            }

            credentials cred;
            if (dwAuthTarget == WINHTTP_AUTH_TARGET_SERVER && !p_request_context->m_server_authentication_tried)
            {
                cred = p_request_context->m_http_client->client_config().credentials();
                p_request_context->m_server_authentication_tried = true;
            }
            else if (dwAuthTarget == WINHTTP_AUTH_TARGET_PROXY)
            {
                bool is_redirect = false;
                try
                {
                    web::uri current_uri(get_request_url(hRequestHandle));
                    is_redirect = p_request_context->m_request.absolute_uri().to_string() != current_uri.to_string();
                }
                catch (const std::exception&)
                {
                }

                // If we have been redirected, then WinHttp needs the proxy credentials again to make the next request
                // leg (which may be on a different server)
                if (is_redirect || !p_request_context->m_proxy_authentication_tried)
                {
                    cred = p_request_context->m_http_client->client_config().proxy().credentials();
                    p_request_context->m_proxy_authentication_tried = true;
                }
            }

            // No credentials found so can't resend.
            if (!cred.is_set())
            {
                return false;
            }

            // New scope to ensure plaintext password is cleared as soon as possible.
            {
                auto password = cred._internal_decrypt();
                if (!WinHttpSetCredentials(hRequestHandle,
                                           dwAuthTarget,
                                           dwSelectedScheme,
                                           cred.username().c_str(),
                                           password->c_str(),
                                           nullptr))
                {
                    return false;
                }
            }
        }

        // Reset the request body type since it might have already started sending.
        size_t content_length;
        try
        {
            content_length = request._get_impl()->_get_content_length_and_set_compression();
        }
        catch (...)
        {
            return false;
        }

        if (content_length > 0)
        {
            // There is a request body that needs to be transferred.
            if (content_length == (std::numeric_limits<size_t>::max)())
            {
                // The content length is unknown and the application set a stream. This is an
                // indication that we will need to chunk the data.
                p_request_context->m_bodyType = transfer_encoding_chunked;
                p_request_context->m_remaining_to_write = request._get_impl()->_get_stream_length();
            }
            else
            {
                // While we won't be transfer-encoding the data, we will write it in portions.
                p_request_context->m_bodyType = content_length_chunked;
                p_request_context->m_remaining_to_write = content_length;
            }
        }
        else
        {
            p_request_context->m_bodyType = no_body;
        }

        // We're good.
        winhttp_client* winclnt = reinterpret_cast<winhttp_client*>(p_request_context->m_http_client.get());
        winclnt->_start_request_send(p_request_context, content_length);

        // We will not complete the request. Instead wait for the response to the request that was resent
        return true;
    }

    // Callback used with WinHTTP to listen for async completions.
    static void CALLBACK completion_callback(
        HINTERNET hRequestHandle, DWORD_PTR context, DWORD statusCode, _In_ void* statusInfo, DWORD statusInfoLength)
    {
        (void)statusInfoLength;

        std::weak_ptr<winhttp_request_context>* p_weak_request_context =
            reinterpret_cast<std::weak_ptr<winhttp_request_context>*>(context);

        if (p_weak_request_context == nullptr)
        {
            return;
        }

        if (statusCode == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)
        {
            // This callback is responsible for freeing the type-erased context.
            // This particular status code indicates that this is the final callback call, suitable for context
            // destruction.
            delete p_weak_request_context;
            return;
        }

        auto p_request_context = p_weak_request_context->lock();
        if (!p_request_context)
        {
            // The request context was already released, probably due to cancellation
            return;
        }

        switch (statusCode)
        {
            case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
            {
                WINHTTP_ASYNC_RESULT* error_result = reinterpret_cast<WINHTTP_ASYNC_RESULT*>(statusInfo);
                const DWORD errorCode = error_result->dwError;

                //  Some authentication schemes require multiple transactions.
                //  When ERROR_WINHTTP_RESEND_REQUEST is encountered,
                //  we should continue to resend the request until a response is received that does not contain a 401 or
                //  407 status code.
                if (errorCode == ERROR_WINHTTP_RESEND_REQUEST)
                {
                    bool resending = handle_authentication_failure(hRequestHandle, p_request_context, errorCode);
                    if (resending)
                    {
                        // The request is resending. Wait until we get a new response.
                        return;
                    }
                }

                p_request_context->report_error(errorCode, build_error_msg(error_result));
                return;
            }
            case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
            {
                if (!p_request_context->m_request.body())
                {
                    // Report progress finished uploading with no message body.
                    auto progress = p_request_context->m_request._get_impl()->_progress_handler();
                    if (progress)
                    {
                        try
                        {
                            (*progress)(message_direction::upload, 0);
                        }
                        catch (...)
                        {
                            p_request_context->report_exception(std::current_exception());
                            return;
                        }
                    }
                }

                if (p_request_context->m_bodyType == transfer_encoding_chunked)
                {
                    _transfer_encoding_chunked_write_data(p_request_context.get());
                }
                else if (p_request_context->m_bodyType == content_length_chunked)
                {
                    _multiple_segment_write_data(p_request_context.get());
                }
                else
                {
                    if (!WinHttpReceiveResponse(hRequestHandle, nullptr))
                    {
                        auto errorCode = GetLastError();
                        p_request_context->report_error(errorCode,
                                                        build_error_msg(errorCode, "WinHttpReceiveResponse"));
                    }
                }
                return;
            }
            case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
            {
                p_request_context->on_send_request_validate_cn();
                return;
            }
            case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
            {
                p_request_context->report_exception(web::http::http_exception(
                    generate_security_failure_message(*reinterpret_cast<std::uint32_t*>(statusInfo))));
                return;
            }
            case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
            {
                DWORD bytesWritten = *((DWORD*)statusInfo);
                _ASSERTE(statusInfoLength == sizeof(DWORD));

                if (bytesWritten > 0)
                {
                    auto progress = p_request_context->m_request._get_impl()->_progress_handler();
                    if (progress)
                    {
                        p_request_context->m_uploaded += bytesWritten;
                        try
                        {
                            (*progress)(message_direction::upload, p_request_context->m_uploaded);
                        }
                        catch (...)
                        {
                            p_request_context->report_exception(std::current_exception());
                            return;
                        }
                    }
                }

                if (p_request_context->is_externally_allocated())
                {
                    p_request_context->_get_readbuffer().release(p_request_context->m_body_data.get(), bytesWritten);
                }

                if (p_request_context->m_bodyType == transfer_encoding_chunked)
                {
                    _transfer_encoding_chunked_write_data(p_request_context.get());
                }
                else if (p_request_context->m_bodyType == content_length_chunked)
                {
                    _multiple_segment_write_data(p_request_context.get());
                }
                else
                {
                    if (!WinHttpReceiveResponse(hRequestHandle, nullptr))
                    {
                        auto errorCode = GetLastError();
                        p_request_context->report_error(errorCode,
                                                        build_error_msg(errorCode, "WinHttpReceiveResponse"));
                    }
                }
                return;
            }
            case WINHTTP_CALLBACK_STATUS_REDIRECT:
            {
                // Return and continue unless that's too many automatic redirects.
                if (p_request_context->m_remaining_redirects > 0)
                {
                    --p_request_context->m_remaining_redirects;
                    return;
                }

                // First need to query to see what the headers size is.
                DWORD headerBufferLength = 0;
                query_header_length(hRequestHandle, WINHTTP_QUERY_RAW_HEADERS_CRLF, headerBufferLength);

                // Now allocate buffer for headers and query for them.
                std::vector<unsigned char> header_raw_buffer;
                header_raw_buffer.resize(headerBufferLength);
                utility::char_t* header_buffer = reinterpret_cast<utility::char_t*>(&header_raw_buffer[0]);
                if (!WinHttpQueryHeaders(hRequestHandle,
                                         WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                         WINHTTP_HEADER_NAME_BY_INDEX,
                                         header_buffer,
                                         &headerBufferLength,
                                         WINHTTP_NO_HEADER_INDEX))
                {
                    auto errorCode = GetLastError();
                    p_request_context->report_error(errorCode, build_error_msg(errorCode, "WinHttpQueryHeaders"));
                    return;
                }

                http_response& response = p_request_context->m_response;
                parse_winhttp_headers(hRequestHandle, header_buffer, response);

                // Signal that the headers are available.
                p_request_context->complete_headers();

                // The body of the message is unavailable in WINHTTP_CALLBACK_STATUS_REDIRECT.
                p_request_context->allocate_request_space(nullptr, 0);
                p_request_context->complete_request(0);

                // Cancel the WinHTTP operation by closing the handle.
                p_request_context->cleanup();
                return;
            }
            case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
            {
                // First need to query to see what the headers size is.
                DWORD headerBufferLength = 0;
                query_header_length(hRequestHandle, WINHTTP_QUERY_RAW_HEADERS_CRLF, headerBufferLength);

                // Now allocate buffer for headers and query for them.
                std::vector<unsigned char> header_raw_buffer;
                header_raw_buffer.resize(headerBufferLength);
                utility::char_t* header_buffer = reinterpret_cast<utility::char_t*>(&header_raw_buffer[0]);
                if (!WinHttpQueryHeaders(hRequestHandle,
                                         WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                         WINHTTP_HEADER_NAME_BY_INDEX,
                                         header_buffer,
                                         &headerBufferLength,
                                         WINHTTP_NO_HEADER_INDEX))
                {
                    auto errorCode = GetLastError();
                    p_request_context->report_error(errorCode, build_error_msg(errorCode, "WinHttpQueryHeaders"));
                    return;
                }

                http_response& response = p_request_context->m_response;
                parse_winhttp_headers(hRequestHandle, header_buffer, response);

                if (response.status_code() == status_codes::Unauthorized /*401*/ ||
                    response.status_code() == status_codes::ProxyAuthRequired /*407*/)
                {
                    bool resending = handle_authentication_failure(hRequestHandle, p_request_context);
                    if (resending)
                    {
                        // The request was not completed but resent with credentials. Wait until we get a new response
                        return;
                    }
                }

                // Check whether the request is compressed, and if so, whether we're handling it.
                if (!p_request_context->handle_compression())
                {
                    // false indicates report_exception was called
                    return;
                }
                if (p_request_context->m_decompressor &&
                    !p_request_context->m_http_client->client_config().request_compressed_response())
                {
                    p_request_context->m_compression_state.m_chunk =
                        ::utility::details::make_unique<winhttp_request_context::compression_state::_chunk_helper>();
                    p_request_context->m_compression_state.m_chunked = true;
                }

                // Signal that the headers are available.
                p_request_context->complete_headers();

                // If the method was 'HEAD,' the body of the message is by definition empty. No need to
                // read it. Any headers that suggest the presence of a body can safely be ignored.
                if (p_request_context->m_request.method() == methods::HEAD)
                {
                    p_request_context->allocate_request_space(nullptr, 0);
                    p_request_context->complete_request(0);
                    return;
                }

                // HTTP Specification states:
                // If a message is received with both a Transfer-Encoding header field
                // and a Content-Length header field, the latter MUST be ignored.
                // If none of them is specified, the message length should be determined by the server closing the
                // connection. http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.4

                read_next_response_chunk(p_request_context.get(), 0, true);
                return;
            }
            case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
            {
                // Status information contains pointer to DWORD containing number of bytes available.
                const DWORD num_bytes = *(PDWORD)statusInfo;
                uint8_t* buffer;

                if (num_bytes > 0)
                {
                    if (p_request_context->m_decompressor)
                    {
                        // Allocate space for the compressed data; we'll decompress it into the caller stream once it's
                        // been filled in
                        if (p_request_context->m_compression_state.m_buffer.capacity() < num_bytes)
                        {
                            p_request_context->m_compression_state.m_buffer.reserve(num_bytes);
                        }
                        buffer = p_request_context->m_compression_state.m_buffer.data();
                    }
                    else
                    {
                        auto writebuf = p_request_context->_get_writebuffer();
                        p_request_context->allocate_reply_space(writebuf.alloc(num_bytes), num_bytes);
                        buffer = p_request_context->m_body_data.get();
                    }

                    // Read in available body data all at once.
                    if (!WinHttpReadData(hRequestHandle, buffer, num_bytes, nullptr))
                    {
                        auto errorCode = GetLastError();
                        p_request_context->report_error(errorCode, build_error_msg(errorCode, "WinHttpReadData"));
                    }
                }
                else
                {
                    if (p_request_context->m_decompressor)
                    {
                        if (p_request_context->m_compression_state.m_chunked)
                        {
                            // We haven't seen the 0-length chunk and/or trailing delimiter that indicate the end of
                            // chunked input
                            p_request_context->report_exception(
                                http_exception("Chunked response stream ended unexpectedly"));
                            return;
                        }
                        if (p_request_context->m_compression_state.m_started &&
                            !p_request_context->m_compression_state.m_done)
                        {
                            p_request_context->report_exception(
                                http_exception("Received incomplete compressed stream"));
                            return;
                        }
                    }

                    // No more data available, complete the request.
                    auto progress = p_request_context->m_request._get_impl()->_progress_handler();
                    if (progress)
                    {
                        try
                        {
                            (*progress)(message_direction::download, p_request_context->m_downloaded);
                        }
                        catch (...)
                        {
                            p_request_context->report_exception(std::current_exception());
                            return;
                        }
                    }

                    p_request_context->complete_request(p_request_context->m_downloaded);
                }
                return;
            }
            case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
            {
                // Status information length contains the number of bytes read.
                DWORD bytesRead = statusInfoLength;

                // Report progress about downloaded bytes.
                auto progress = p_request_context->m_request._get_impl()->_progress_handler();
                p_request_context->m_downloaded += statusInfoLength;
                if (progress)
                {
                    try
                    {
                        (*progress)(message_direction::download, p_request_context->m_downloaded);
                    }
                    catch (...)
                    {
                        p_request_context->report_exception(std::current_exception());
                        return;
                    }
                }

                // If no bytes have been read, then this is the end of the response.
                if (bytesRead == 0)
                {
                    if (p_request_context->m_decompressor)
                    {
                        if (p_request_context->m_compression_state.m_chunked)
                        {
                            // We haven't seen the 0-length chunk and/or trailing delimiter that indicate the end of
                            // chunked input
                            p_request_context->report_exception(
                                http_exception("Chunked response stream ended unexpectedly"));
                            return;
                        }
                        if (p_request_context->m_compression_state.m_started &&
                            !p_request_context->m_compression_state.m_done)
                        {
                            p_request_context->report_exception(
                                http_exception("Received incomplete compressed stream"));
                            return;
                        }
                    }
                    p_request_context->complete_request(p_request_context->m_downloaded);
                    return;
                }

                auto writebuf = p_request_context->_get_writebuffer();

                if (p_request_context->m_decompressor)
                {
                    size_t chunk_size = (std::max)(static_cast<size_t>(bytesRead),
                                                   p_request_context->m_http_client->client_config().chunksize());
                    p_request_context->m_compression_state.m_bytes_read = static_cast<size_t>(bytesRead);
                    p_request_context->m_compression_state.m_chunk_bytes = 0;

                    // Note, some servers seem to send a first chunk of body data that decompresses to nothing, but
                    // initializes the decompression state; this produces no decompressed output.  Subsequent chunks
                    // will then begin emitting decompressed body data.

                    // Oddly enough, WinHttp doesn't de-chunk for us if "chunked" isn't the only
                    // encoding, so we need to do so on the fly as we process the received data
                    auto process_buffer =
                        [chunk_size](winhttp_request_context* c, size_t bytes_produced, bool outer) -> bool {
                        if (!c->m_compression_state.m_chunk_bytes)
                        {
                            if (c->m_compression_state.m_chunked)
                            {
                                size_t offset;
                                bool done;

                                // Process the next portion of this piece of the transfer-encoded message
                                done = c->m_compression_state.m_chunk->process_buffer(
                                    c->m_compression_state.m_buffer.data() + c->m_compression_state.m_bytes_processed,
                                    c->m_compression_state.m_bytes_read - c->m_compression_state.m_bytes_processed,
                                    offset,
                                    c->m_compression_state.m_chunk_bytes);

                                // Skip chunk-related metadata; it isn't relevant to decompression
                                _ASSERTE(c->m_compression_state.m_bytes_processed + offset <=
                                         c->m_compression_state.m_bytes_read);
                                c->m_compression_state.m_bytes_processed += offset;

                                if (!c->m_compression_state.m_chunk_bytes)
                                {
                                    if (done)
                                    {
                                        // We've processed/validated all bytes in this transfer-encoded message.
                                        // Note that we currently ignore "extra" trailing bytes, i.e.
                                        // c->m_compression_state.m_bytes_processed <
                                        // c->m_compression_state.m_bytes_read
                                        if (c->m_compression_state.m_done)
                                        {
                                            c->complete_request(c->m_downloaded);
                                            return false;
                                        }
                                        else if (!outer && bytes_produced != chunk_size)
                                        {
                                            throw http_exception("Transfer ended before decompression completed");
                                        }
                                    }
                                    else if (!outer && bytes_produced != chunk_size)
                                    {
                                        // There should be more data to receive; look for it
                                        c->m_compression_state.m_bytes_processed = 0;
                                        read_next_response_chunk(
                                            c, static_cast<DWORD>(c->m_compression_state.m_bytes_read));
                                        return false;
                                    }
                                }
                            }
                            else
                            {
                                _ASSERTE(!c->m_compression_state.m_bytes_processed ||
                                         c->m_compression_state.m_bytes_processed ==
                                             c->m_compression_state.m_bytes_read);
                                if (c->m_compression_state.m_done)
                                {
                                    // Decompression is done; complete the request
                                    c->complete_request(c->m_downloaded);
                                    return false;
                                }
                                else if (c->m_compression_state.m_bytes_processed !=
                                         c->m_compression_state.m_bytes_read)
                                {
                                    // We still have more data to process in the current buffer
                                    c->m_compression_state.m_chunk_bytes =
                                        c->m_compression_state.m_bytes_read - c->m_compression_state.m_bytes_processed;
                                }
                                else if (!outer && bytes_produced != chunk_size)
                                {
                                    // There should be more data to receive; look for it
                                    c->m_compression_state.m_bytes_processed = 0;
                                    read_next_response_chunk(c,
                                                             static_cast<DWORD>(c->m_compression_state.m_bytes_read));
                                    return false;
                                }
                                // Otherwise, we've processed all bytes in the input buffer, but there's a good chance
                                // that there are still decompressed bytes to emit; we'll do so before reading the next
                                // chunk
                            }
                        }

                        // We're still processing the current message chunk
                        return true;
                    };

                    pplx::details::_do_while([p_request_context, chunk_size, process_buffer]() -> pplx::task<bool> {
                        uint8_t* buffer;

                        try
                        {
                            if (!process_buffer(p_request_context.get(), 0, true))
                            {
                                // The chunked request has been completely processed (or contains no data in the first
                                // place)
                                return pplx::task_from_result<bool>(false);
                            }
                        }
                        catch (...)
                        {
                            // The outer do-while requires an explicit task return to activate the then() clause
                            return pplx::task_from_exception<bool>(std::current_exception());
                        }

                        // If it's possible to know how much post-compression data we're expecting (for instance if we
                        // can discern how much total data the ostream can support, we could allocate (or at least
                        // attempt to acquire) based on that
                        p_request_context->m_compression_state.m_acquired =
                            p_request_context->_get_writebuffer().alloc(chunk_size);
                        if (p_request_context->m_compression_state.m_acquired)
                        {
                            buffer = p_request_context->m_compression_state.m_acquired;
                        }
                        else
                        {
                            // The streambuf couldn't accommodate our request; we'll use m_body_data's
                            // internal vector as temporary storage, then putn() to the caller's stream
                            p_request_context->allocate_reply_space(nullptr, chunk_size);
                            buffer = p_request_context->m_body_data.get();
                        }

                        uint8_t* in = p_request_context->m_compression_state.m_buffer.data() +
                                      p_request_context->m_compression_state.m_bytes_processed;
                        size_t inbytes = p_request_context->m_compression_state.m_chunk_bytes;
                        if (inbytes)
                        {
                            p_request_context->m_compression_state.m_started = true;
                        }
                        return p_request_context->m_decompressor
                            ->decompress(
                                in, inbytes, buffer, chunk_size, web::http::compression::operation_hint::has_more)
                            .then([p_request_context, buffer, chunk_size, process_buffer](
                                      pplx::task<web::http::compression::operation_result> op) {
                                auto r = op.get();
                                auto keep_going = [&r, process_buffer](winhttp_request_context* c) -> pplx::task<bool> {
                                    _ASSERTE(r.input_bytes_processed <= c->m_compression_state.m_chunk_bytes);
                                    c->m_compression_state.m_chunk_bytes -= r.input_bytes_processed;
                                    c->m_compression_state.m_bytes_processed += r.input_bytes_processed;
                                    c->m_compression_state.m_done = r.done;

                                    try
                                    {
                                        // See if we still have more work to do for this section and/or for the response
                                        // in general
                                        return pplx::task_from_result<bool>(
                                            process_buffer(c, r.output_bytes_produced, false));
                                    }
                                    catch (...)
                                    {
                                        return pplx::task_from_exception<bool>(std::current_exception());
                                    }
                                };

                                _ASSERTE(p_request_context->m_compression_state.m_bytes_processed +
                                             r.input_bytes_processed <=
                                         p_request_context->m_compression_state.m_bytes_read);

                                if (p_request_context->m_compression_state.m_acquired != nullptr)
                                {
                                    // We decompressed directly into the output stream
                                    p_request_context->m_compression_state.m_acquired = nullptr;
                                    p_request_context->_get_writebuffer().commit(r.output_bytes_produced);
                                    return keep_going(p_request_context.get());
                                }

                                // We decompressed into our own buffer; let the stream copy the data
                                return p_request_context->_get_writebuffer()
                                    .putn_nocopy(buffer, r.output_bytes_produced)
                                    .then([p_request_context, r, keep_going](pplx::task<size_t> op) {
                                        if (op.get() != r.output_bytes_produced)
                                        {
                                            return pplx::task_from_exception<bool>(
                                                std::runtime_error("Response stream unexpectedly failed to write the "
                                                                   "requested number of bytes"));
                                        }
                                        return keep_going(p_request_context.get());
                                    });
                            });
                    }).then([p_request_context](pplx::task<bool> op) {
                        try
                        {
                            op.get();
                        }
                        catch (...)
                        {
                            // We're only here to pick up any exception that may have been thrown, and to clean up
                            // if needed
                            if (p_request_context->m_compression_state.m_acquired)
                            {
                                p_request_context->_get_writebuffer().commit(0);
                                p_request_context->m_compression_state.m_acquired = nullptr;
                            }
                            p_request_context->report_exception(std::current_exception());
                        }
                    });
                }
                else
                {
                    // If the data was allocated directly from the buffer then commit, otherwise we still
                    // need to write to the response stream buffer.
                    if (p_request_context->is_externally_allocated())
                    {
                        writebuf.commit(bytesRead);
                        read_next_response_chunk(p_request_context.get(), bytesRead);
                    }
                    else
                    {
                        writebuf.putn_nocopy(p_request_context->m_body_data.get(), bytesRead)
                            .then([hRequestHandle, p_request_context, bytesRead](pplx::task<size_t> op) {
                                size_t written = 0;
                                try
                                {
                                    written = op.get();
                                }
                                catch (...)
                                {
                                    p_request_context->report_exception(std::current_exception());
                                    return;
                                }

                                // If we couldn't write everything, it's time to exit.
                                if (written != bytesRead)
                                {
                                    p_request_context->report_exception(std::runtime_error(
                                        "response stream unexpectedly failed to write the requested number of bytes"));
                                    return;
                                }

                                read_next_response_chunk(p_request_context.get(), bytesRead);
                            });
                    }
                }
                return;
            }
        }
    }

    std::atomic<bool> m_opened;

    // WinHTTP session and connection
    HINTERNET m_hSession;
    HINTERNET m_hConnection;
    bool m_secure;

    // If auto config is true, dynamically find the proxy for each URL using
    // the proxy configuration script at the given URL if it's not empty or
    // using WPAD otherwise.
    bool m_proxy_auto_config {false};
    utility::string_t m_proxy_auto_config_url;
};

std::shared_ptr<_http_client_communicator> create_platform_final_pipeline_stage(uri&& base_uri,
                                                                                http_client_config&& client_config)
{
    return std::make_shared<details::winhttp_client>(std::move(base_uri), std::move(client_config));
}


} // namespace details
} // namespace client
} // namespace http
} // namespace web
