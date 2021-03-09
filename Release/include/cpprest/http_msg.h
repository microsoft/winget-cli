/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: Request and reply message definitions.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

#include "cpprest/asyncrt_utils.h"
#include "cpprest/containerstream.h"
#include "cpprest/details/cpprest_compat.h"
#include "cpprest/http_compression.h"
#include "cpprest/http_headers.h"
#include "cpprest/json.h"
#include "cpprest/streams.h"
#include "cpprest/uri.h"
#include "pplx/pplxtasks.h"
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace web
{
namespace http
{
// URI class has been moved from web::http namespace to web namespace.
// The below using declarations ensure we don't break existing code.
// Please use the web::uri class going forward.
using web::uri;
using web::uri_builder;

namespace client
{
class http_client;
}

/// <summary>
/// Represents the HTTP protocol version of a message, as {major, minor}.
/// </summary>
struct http_version
{
    uint8_t major;
    uint8_t minor;

    inline bool operator==(const http_version& other) const { return major == other.major && minor == other.minor; }
    inline bool operator<(const http_version& other) const
    {
        return major < other.major || (major == other.major && minor < other.minor);
    }

    inline bool operator!=(const http_version& other) const { return !(*this == other); }
    inline bool operator>=(const http_version& other) const { return !(*this < other); }
    inline bool operator>(const http_version& other) const { return !(*this < other || *this == other); }
    inline bool operator<=(const http_version& other) const { return *this < other || *this == other; }

    /// <summary>
    /// Creates <c>http_version</c> from an HTTP-Version string, "HTTP" "/" 1*DIGIT "." 1*DIGIT.
    /// </summary>
    /// <returns>Returns a <c>http_version</c> of {0, 0} if not successful.</returns>
    static _ASYNCRTIMP http_version __cdecl from_string(const std::string& http_version_string);

    /// <summary>
    /// Returns the string representation of the <c>http_version</c>.
    /// </summary>
    _ASYNCRTIMP std::string to_utf8string() const;
};

/// <summary>
/// Predefined HTTP protocol versions.
/// </summary>
class http_versions
{
public:
    _ASYNCRTIMP static const http_version HTTP_0_9;
    _ASYNCRTIMP static const http_version HTTP_1_0;
    _ASYNCRTIMP static const http_version HTTP_1_1;
};

/// <summary>
/// Predefined method strings for the standard HTTP methods mentioned in the
/// HTTP 1.1 specification.
/// </summary>
typedef utility::string_t method;

/// <summary>
/// Common HTTP methods.
/// </summary>
class methods
{
public:
#define _METHODS
#define DAT(a, b) _ASYNCRTIMP const static method a;
#include "cpprest/details/http_constants.dat"
#undef _METHODS
#undef DAT
};

typedef unsigned short status_code;

/// <summary>
/// Predefined values for all of the standard HTTP 1.1 response status codes.
/// </summary>
class status_codes
{
public:
#define _PHRASES
#define DAT(a, b, c) const static status_code a = b;
#include "cpprest/details/http_constants.dat"
#undef _PHRASES
#undef DAT
};

namespace details
{
/// <summary>
/// Constants for MIME types.
/// </summary>
class mime_types
{
public:
#define _MIME_TYPES
#define DAT(a, b) _ASYNCRTIMP const static utility::string_t a;
#include "cpprest/details/http_constants.dat"
#undef _MIME_TYPES
#undef DAT
};

/// <summary>
/// Constants for charset types.
/// </summary>
class charset_types
{
public:
#define _CHARSET_TYPES
#define DAT(a, b) _ASYNCRTIMP const static utility::string_t a;
#include "cpprest/details/http_constants.dat"
#undef _CHARSET_TYPES
#undef DAT
};

} // namespace details

/// Message direction
namespace message_direction
{
/// <summary>
/// Enumeration used to denote the direction of a message: a request with a body is
/// an upload, a response with a body is a download.
/// </summary>
enum direction
{
    upload,
    download
};
} // namespace message_direction

typedef utility::string_t reason_phrase;
typedef std::function<void(message_direction::direction, utility::size64_t)> progress_handler;

struct http_status_to_phrase
{
    unsigned short id;
    reason_phrase phrase;
};

/// <summary>
/// Constants for the HTTP headers mentioned in RFC 2616.
/// </summary>
class header_names
{
public:
#define _HEADER_NAMES
#define DAT(a, b) _ASYNCRTIMP const static utility::string_t a;
#include "cpprest/details/http_constants.dat"
#undef _HEADER_NAMES
#undef DAT
};

/// <summary>
/// Represents an HTTP error. This class holds an error message and an optional error code.
/// </summary>
class http_exception : public std::exception
{
public:
    /// <summary>
    /// Creates an <c>http_exception</c> with just a string message and no error code.
    /// </summary>
    /// <param name="whatArg">Error message string.</param>
    http_exception(const utility::string_t& whatArg) : m_msg(utility::conversions::to_utf8string(whatArg)) {}

#ifdef _WIN32
    /// <summary>
    /// Creates an <c>http_exception</c> with just a string message and no error code.
    /// </summary>
    /// <param name="whatArg">Error message string.</param>
    http_exception(std::string whatArg) : m_msg(std::move(whatArg)) {}
#endif

    /// <summary>
    /// Creates an <c>http_exception</c> with from a error code using the current platform error category.
    /// The message of the error code will be used as the what() string message.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    http_exception(int errorCode) : m_errorCode(utility::details::create_error_code(errorCode))
    {
        m_msg = m_errorCode.message();
    }

    /// <summary>
    /// Creates an <c>http_exception</c> with from a error code using the current platform error category.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    /// <param name="whatArg">Message to use in what() string.</param>
    http_exception(int errorCode, const utility::string_t& whatArg)
        : m_errorCode(utility::details::create_error_code(errorCode))
        , m_msg(utility::conversions::to_utf8string(whatArg))
    {
    }

#ifdef _WIN32
    /// <summary>
    /// Creates an <c>http_exception</c> with from a error code using the current platform error category.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    /// <param name="whatArg">Message to use in what() string.</param>
    http_exception(int errorCode, std::string whatArg)
        : m_errorCode(utility::details::create_error_code(errorCode)), m_msg(std::move(whatArg))
    {
    }
#endif

    /// <summary>
    /// Creates an <c>http_exception</c> with from a error code and category. The message of the error code will be used
    /// as the <c>what</c> string message.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    /// <param name="cat">Error category for the code.</param>
    http_exception(int errorCode, const std::error_category& cat) : m_errorCode(std::error_code(errorCode, cat))
    {
        m_msg = m_errorCode.message();
    }

    /// <summary>
    /// Creates an <c>http_exception</c> with from a error code with a category, and a string message.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    /// <param name="whatArg">Error message string.</param>
    http_exception(std::error_code errorCode, const utility::string_t& whatArg)
        : m_errorCode(std::move(errorCode)), m_msg(utility::conversions::to_utf8string(whatArg))
    {
    }

#ifdef _WIN32
    /// <summary>
    /// Creates an <c>http_exception</c> with from a error code with a category, and a string message.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    /// <param name="whatArg">Error message string.</param>
    http_exception(std::error_code errorCode, std::string whatArg)
        : m_errorCode(std::move(errorCode)), m_msg(std::move(whatArg))
    {
    }
#endif

    /// <summary>
    /// Gets a string identifying the cause of the exception.
    /// </summary>
    /// <returns>A null terminated character string.</returns>
    const char* what() const CPPREST_NOEXCEPT { return m_msg.c_str(); }

    /// <summary>
    /// Retrieves the underlying error code causing this exception.
    /// </summary>
    /// <returns>A std::error_code.</returns>
    const std::error_code& error_code() const { return m_errorCode; }

private:
    std::error_code m_errorCode;
    std::string m_msg;
};

namespace details
{
/// <summary>
/// Base class for HTTP messages.
/// This class is to store common functionality so it isn't duplicated on
/// both the request and response side.
/// </summary>
class http_msg_base
{
public:
    friend class http::client::http_client;

    _ASYNCRTIMP http_msg_base();

    virtual ~http_msg_base() {}

    http::http_version http_version() const { return m_http_version; }

    http_headers& headers() { return m_headers; }

    _ASYNCRTIMP void set_body(const concurrency::streams::istream& instream, const utf8string& contentType);
    _ASYNCRTIMP void set_body(const concurrency::streams::istream& instream, const utf16string& contentType);
    _ASYNCRTIMP void set_body(const concurrency::streams::istream& instream,
                              utility::size64_t contentLength,
                              const utf8string& contentType);
    _ASYNCRTIMP void set_body(const concurrency::streams::istream& instream,
                              utility::size64_t contentLength,
                              const utf16string& contentType);

    /// <summary>
    /// Helper function for extract functions. Parses the Content-Type header and check to make sure it matches,
    /// throws an exception if not.
    /// </summary>
    /// <param name="ignore_content_type">If true ignores the Content-Type header value.</param>
    /// <param name="check_content_type">Function to verify additional information on Content-Type.</param>
    /// <returns>A string containing the charset, an empty string if no Content-Type header is empty.</returns>
    utility::string_t parse_and_check_content_type(
        bool ignore_content_type, const std::function<bool(const utility::string_t&)>& check_content_type);

    _ASYNCRTIMP utf8string extract_utf8string(bool ignore_content_type = false);
    _ASYNCRTIMP utf16string extract_utf16string(bool ignore_content_type = false);
    _ASYNCRTIMP utility::string_t extract_string(bool ignore_content_type = false);

    _ASYNCRTIMP json::value _extract_json(bool ignore_content_type = false);
    _ASYNCRTIMP std::vector<unsigned char> _extract_vector();

    virtual _ASYNCRTIMP utility::string_t to_string() const;

    /// <summary>
    /// Completes this message
    /// </summary>
    virtual _ASYNCRTIMP void _complete(utility::size64_t bodySize,
                                       const std::exception_ptr& exceptionPtr = std::exception_ptr());

    /// <summary>
    /// Set the stream through which the message body could be read
    /// </summary>
    void set_instream(const concurrency::streams::istream& instream) { m_inStream = instream; }

    /// <summary>
    /// Get the stream through which the message body could be read
    /// </summary>
    const concurrency::streams::istream& instream() const { return m_inStream; }

    /// <summary>
    /// Set the stream through which the message body could be written
    /// </summary>
    void set_outstream(const concurrency::streams::ostream& outstream, bool is_default)
    {
        m_outStream = outstream;
        m_default_outstream = is_default;
    }

    /// <summary>
    /// Get the stream through which the message body could be written
    /// </summary>
    const concurrency::streams::ostream& outstream() const { return m_outStream; }

    /// <summary>
    /// Sets the compressor for the message body
    /// </summary>
    void set_compressor(std::unique_ptr<http::compression::compress_provider> compressor)
    {
        m_compressor = std::move(compressor);
    }

    /// <summary>
    /// Gets the compressor for the message body, if any
    /// </summary>
    std::unique_ptr<http::compression::compress_provider>& compressor() { return m_compressor; }

    /// <summary>
    /// Sets the collection of factory classes for decompressors for use with the message body
    /// </summary>
    void set_decompress_factories(const std::vector<std::shared_ptr<http::compression::decompress_factory>>& factories)
    {
        m_decompressors = factories;
    }

    /// <summary>
    /// Gets the collection of factory classes for decompressors to be used to decompress the message body, if any
    /// </summary>
    const std::vector<std::shared_ptr<http::compression::decompress_factory>>& decompress_factories()
    {
        return m_decompressors;
    }

    const pplx::task_completion_event<utility::size64_t>& _get_data_available() const { return m_data_available; }

    /// <summary>
    /// Prepare the message with an output stream to receive network data
    /// </summary>
    _ASYNCRTIMP void _prepare_to_receive_data();

    /// <summary>
    /// Determine the remaining input stream length
    /// </summary>
    /// <returns>
    /// std::numeric_limits<size_t>::max() if the stream's remaining length cannot be determined
    /// length      if the stream's remaining length (which may be 0) can be determined
    /// </returns>
    /// <remarks>
    /// This routine should only be called after a msg (request/response) has been
    /// completely constructed.
    /// </remarks>
    _ASYNCRTIMP size_t _get_stream_length();

    /// <summary>
    /// Determine the content length
    /// </summary>
    /// <returns>
    /// std::numeric_limits<size_t>::max() if there is content with unknown length (transfer_encoding:chunked)
    /// 0           if there is no content
    /// length      if there is content with known length
    /// </returns>
    /// <remarks>
    /// This routine should only be called after a msg (request/response) has been
    /// completely constructed.
    /// </remarks>
    _ASYNCRTIMP size_t _get_content_length();

    /// <summary>
    /// Determine the content length, and, if necessary, manage compression in the Transfer-Encoding header
    /// </summary>
    /// <returns>
    /// std::numeric_limits<size_t>::max() if there is content with unknown length (transfer_encoding:chunked)
    /// 0           if there is no content
    /// length      if there is content with known length
    /// </returns>
    /// <remarks>
    /// This routine is like _get_content_length, except that it adds a compression algorithm to
    /// the Trasfer-Length header if compression is configured.  It throws if a Transfer-Encoding
    /// header exists and does not match the one it generated.
    /// </remarks>
    _ASYNCRTIMP size_t _get_content_length_and_set_compression();

    void _set_http_version(const http::http_version& http_version) { m_http_version = http_version; }

protected:
    std::unique_ptr<http::compression::compress_provider> m_compressor;
    std::unique_ptr<http::compression::decompress_provider> m_decompressor;
    std::vector<std::shared_ptr<http::compression::decompress_factory>> m_decompressors;

    /// <summary>
    /// Stream to read the message body.
    /// By default this is an invalid stream. The user could set the instream on
    /// a request by calling set_request_stream(...). This would also be set when
    /// set_body() is called - a stream from the body is constructed and set.
    /// Even in the presence of msg body this stream could be invalid. An example
    /// would be when the user sets an ostream for the response. With that API the
    /// user does not provide the ability to read the msg body.
    /// Thus m_instream is valid when there is a msg body and it can actually be read
    /// </summary>
    concurrency::streams::istream m_inStream;

    /// <summary>
    /// stream to write the msg body
    /// By default this is an invalid stream. The user could set this on the response
    /// (for http_client). In all the other cases we would construct one to transfer
    /// the data from the network into the message body.
    /// </summary>
    concurrency::streams::ostream m_outStream;

    http::http_version m_http_version;
    http_headers m_headers;
    bool m_default_outstream;

    /// <summary> The TCE is used to signal the availability of the message body. </summary>
    pplx::task_completion_event<utility::size64_t> m_data_available;

    size_t _get_content_length(bool honor_compression);
};

/// <summary>
/// Base structure for associating internal server information
/// with an HTTP request/response.
/// </summary>
class _http_server_context
{
public:
    _http_server_context() {}
    virtual ~_http_server_context() {}

private:
};

/// <summary>
/// Internal representation of an HTTP response.
/// </summary>
class _http_response final : public http::details::http_msg_base
{
public:
    _http_response() : m_status_code((std::numeric_limits<uint16_t>::max)()) {}

    _http_response(http::status_code code) : m_status_code(code) {}

    http::status_code status_code() const { return m_status_code; }

    void set_status_code(http::status_code code) { m_status_code = code; }

    const http::reason_phrase& reason_phrase() const { return m_reason_phrase; }

    void set_reason_phrase(const http::reason_phrase& reason) { m_reason_phrase = reason; }

    _ASYNCRTIMP utility::string_t to_string() const;

    _http_server_context* _get_server_context() const { return m_server_context.get(); }

    void _set_server_context(std::unique_ptr<details::_http_server_context> server_context)
    {
        m_server_context = std::move(server_context);
    }

private:
    std::unique_ptr<_http_server_context> m_server_context;

    http::status_code m_status_code;
    http::reason_phrase m_reason_phrase;
};

} // namespace details

/// <summary>
/// Represents an HTTP response.
/// </summary>
class http_response
{
public:
    /// <summary>
    /// Constructs a response with an empty status code, no headers, and no body.
    /// </summary>
    /// <returns>A new HTTP response.</returns>
    http_response() : _m_impl(std::make_shared<details::_http_response>()) {}

    /// <summary>
    /// Constructs a response with given status code, no headers, and no body.
    /// </summary>
    /// <param name="code">HTTP status code to use in response.</param>
    /// <returns>A new HTTP response.</returns>
    http_response(http::status_code code) : _m_impl(std::make_shared<details::_http_response>(code)) {}

    /// <summary>
    /// Gets the status code of the response message.
    /// </summary>
    /// <returns>status code.</returns>
    http::status_code status_code() const { return _m_impl->status_code(); }

    /// <summary>
    /// Sets the status code of the response message.
    /// </summary>
    /// <param name="code">Status code to set.</param>
    /// <remarks>
    /// This will overwrite any previously set status code.
    /// </remarks>
    void set_status_code(http::status_code code) const { _m_impl->set_status_code(code); }

    /// <summary>
    /// Gets the reason phrase of the response message.
    /// If no reason phrase is set it will default to the standard one corresponding to the status code.
    /// </summary>
    /// <returns>Reason phrase.</returns>
    const http::reason_phrase& reason_phrase() const { return _m_impl->reason_phrase(); }

    /// <summary>
    /// Sets the reason phrase of the response message.
    /// If no reason phrase is set it will default to the standard one corresponding to the status code.
    /// </summary>
    /// <param name="reason">The reason phrase to set.</param>
    void set_reason_phrase(const http::reason_phrase& reason) const { _m_impl->set_reason_phrase(reason); }

    /// <summary>
    /// Gets the headers of the response message.
    /// </summary>
    /// <returns>HTTP headers for this response.</returns>
    /// <remarks>
    /// Use the <seealso cref="http_headers::add Method"/> to fill in desired headers.
    /// </remarks>
    http_headers& headers() { return _m_impl->headers(); }

    /// <summary>
    /// Gets a const reference to the headers of the response message.
    /// </summary>
    /// <returns>HTTP headers for this response.</returns>
    const http_headers& headers() const { return _m_impl->headers(); }

    /// <summary>
    /// Generates a string representation of the message, including the body when possible.
    /// Mainly this should be used for debugging purposes as it has to copy the
    /// message body and doesn't have excellent performance.
    /// </summary>
    /// <returns>A string representation of this HTTP request.</returns>
    /// <remarks>Note this function is synchronous and doesn't wait for the
    /// entire message body to arrive. If the message body has arrived by the time this
    /// function is called and it is has a textual Content-Type it will be included.
    /// Otherwise just the headers will be present.</remarks>
    utility::string_t to_string() const { return _m_impl->to_string(); }

    /// <summary>
    /// Extracts the body of the response message as a string value, checking that the content type is a MIME text type.
    /// A body can only be extracted once because in some cases an optimization is made where the data is 'moved' out.
    /// </summary>
    /// <param name="ignore_content_type">If true, ignores the Content-Type header and assumes text.</param>
    /// <returns>String containing body of the message.</returns>
    pplx::task<utility::string_t> extract_string(bool ignore_content_type = false) const
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl, ignore_content_type](utility::size64_t) {
            return impl->extract_string(ignore_content_type);
        });
    }

    /// <summary>
    /// Extracts the body of the response message as a UTF-8 string value, checking that the content type is a MIME text
    /// type. A body can only be extracted once because in some cases an optimization is made where the data is 'moved'
    /// out.
    /// </summary>
    /// <param name="ignore_content_type">If true, ignores the Content-Type header and assumes text.</param>
    /// <returns>String containing body of the message.</returns>
    pplx::task<utf8string> extract_utf8string(bool ignore_content_type = false) const
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl, ignore_content_type](utility::size64_t) {
            return impl->extract_utf8string(ignore_content_type);
        });
    }

    /// <summary>
    /// Extracts the body of the response message as a UTF-16 string value, checking that the content type is a MIME
    /// text type. A body can only be extracted once because in some cases an optimization is made where the data is
    /// 'moved' out.
    /// </summary>
    /// <param name="ignore_content_type">If true, ignores the Content-Type header and assumes text.</param>
    /// <returns>String containing body of the message.</returns>
    pplx::task<utf16string> extract_utf16string(bool ignore_content_type = false) const
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl, ignore_content_type](utility::size64_t) {
            return impl->extract_utf16string(ignore_content_type);
        });
    }

    /// <summary>
    /// Extracts the body of the response message into a json value, checking that the content type is application/json.
    /// A body can only be extracted once because in some cases an optimization is made where the data is 'moved' out.
    /// </summary>
    /// <param name="ignore_content_type">If true, ignores the Content-Type header and assumes json.</param>
    /// <returns>JSON value from the body of this message.</returns>
    pplx::task<json::value> extract_json(bool ignore_content_type = false) const
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl, ignore_content_type](utility::size64_t) {
            return impl->_extract_json(ignore_content_type);
        });
    }

    /// <summary>
    /// Extracts the body of the response message into a vector of bytes.
    /// </summary>
    /// <returns>The body of the message as a vector of bytes.</returns>
    pplx::task<std::vector<unsigned char>> extract_vector() const
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl](utility::size64_t) {
            return impl->_extract_vector();
        });
    }

    /// <summary>
    /// Sets the body of the message to a textual string and set the "Content-Type" header. Assumes
    /// the character encoding of the string is UTF-8.
    /// </summary>
    /// <param name="body_text">String containing body text.</param>
    /// <param name="content_type">MIME type to set the "Content-Type" header to. Default to "text/plain;
    /// charset=utf-8".</param> <remarks> This will overwrite any previously set body data and "Content-Type" header.
    /// </remarks>
    void set_body(utf8string&& body_text, const utf8string& content_type = utf8string("text/plain; charset=utf-8"))
    {
        const auto length = body_text.size();
        _m_impl->set_body(
            concurrency::streams::bytestream::open_istream<std::string>(std::move(body_text)), length, content_type);
    }

    /// <summary>
    /// Sets the body of the message to a textual string and set the "Content-Type" header. Assumes
    /// the character encoding of the string is UTF-8.
    /// </summary>
    /// <param name="body_text">String containing body text.</param>
    /// <param name="content_type">MIME type to set the "Content-Type" header to. Default to "text/plain;
    /// charset=utf-8".</param> <remarks> This will overwrite any previously set body data and "Content-Type" header.
    /// </remarks>
    void set_body(const utf8string& body_text, const utf8string& content_type = utf8string("text/plain; charset=utf-8"))
    {
        _m_impl->set_body(
            concurrency::streams::bytestream::open_istream<std::string>(body_text), body_text.size(), content_type);
    }

    /// <summary>
    /// Sets the body of the message to a textual string and set the "Content-Type" header. Assumes
    /// the character encoding of the string is UTF-16 will perform conversion to UTF-8.
    /// </summary>
    /// <param name="body_text">String containing body text.</param>
    /// <param name="content_type">MIME type to set the "Content-Type" header to. Default to "text/plain".</param>
    /// <remarks>
    /// This will overwrite any previously set body data and "Content-Type" header.
    /// </remarks>
    void set_body(const utf16string& body_text,
                  utf16string content_type = utility::conversions::to_utf16string("text/plain"))
    {
        if (content_type.find(::utility::conversions::to_utf16string("charset=")) != content_type.npos)
        {
            throw std::invalid_argument("content_type can't contain a 'charset'.");
        }

        auto utf8body = utility::conversions::utf16_to_utf8(body_text);
        auto length = utf8body.size();
        _m_impl->set_body(concurrency::streams::bytestream::open_istream<std::string>(std::move(utf8body)),
                          length,
                          std::move(content_type.append(::utility::conversions::to_utf16string("; charset=utf-8"))));
    }

    /// <summary>
    /// Sets the body of the message to contain json value. If the 'Content-Type'
    /// header hasn't already been set it will be set to 'application/json'.
    /// </summary>
    /// <param name="body_text">json value.</param>
    /// <remarks>
    /// This will overwrite any previously set body data.
    /// </remarks>
    void set_body(const json::value& body_data)
    {
        auto body_text = utility::conversions::to_utf8string(body_data.serialize());
        auto length = body_text.size();
        set_body(concurrency::streams::bytestream::open_istream(std::move(body_text)),
                 length,
                 _XPLATSTR("application/json"));
    }

    /// <summary>
    /// Sets the body of the message to the contents of a byte vector. If the 'Content-Type'
    /// header hasn't already been set it will be set to 'application/octet-stream'.
    /// </summary>
    /// <param name="body_data">Vector containing body data.</param>
    /// <remarks>
    /// This will overwrite any previously set body data.
    /// </remarks>
    void set_body(std::vector<unsigned char>&& body_data)
    {
        auto length = body_data.size();
        set_body(concurrency::streams::bytestream::open_istream(std::move(body_data)), length);
    }

    /// <summary>
    /// Sets the body of the message to the contents of a byte vector. If the 'Content-Type'
    /// header hasn't already been set it will be set to 'application/octet-stream'.
    /// </summary>
    /// <param name="body_data">Vector containing body data.</param>
    /// <remarks>
    /// This will overwrite any previously set body data.
    /// </remarks>
    void set_body(const std::vector<unsigned char>& body_data)
    {
        set_body(concurrency::streams::bytestream::open_istream(body_data), body_data.size());
    }

    /// <summary>
    /// Defines a stream that will be relied on to provide the body of the HTTP message when it is
    /// sent.
    /// </summary>
    /// <param name="stream">A readable, open asynchronous stream.</param>
    /// <param name="content_type">A string holding the MIME type of the message body.</param>
    /// <remarks>
    /// This cannot be used in conjunction with any external means of setting the body of the request.
    /// The stream will not be read until the message is sent.
    /// </remarks>
    void set_body(const concurrency::streams::istream& stream,
                  const utility::string_t& content_type = _XPLATSTR("application/octet-stream"))
    {
        _m_impl->set_body(stream, content_type);
    }

    /// <summary>
    /// Defines a stream that will be relied on to provide the body of the HTTP message when it is
    /// sent.
    /// </summary>
    /// <param name="stream">A readable, open asynchronous stream.</param>
    /// <param name="content_length">The size of the data to be sent in the body.</param>
    /// <param name="content_type">A string holding the MIME type of the message body.</param>
    /// <remarks>
    /// This cannot be used in conjunction with any external means of setting the body of the request.
    /// The stream will not be read until the message is sent.
    /// </remarks>
    void set_body(const concurrency::streams::istream& stream,
                  utility::size64_t content_length,
                  const utility::string_t& content_type = _XPLATSTR("application/octet-stream"))
    {
        _m_impl->set_body(stream, content_length, content_type);
    }

    /// <summary>
    /// Produces a stream which the caller may use to retrieve data from an incoming request.
    /// </summary>
    /// <returns>A readable, open asynchronous stream.</returns>
    /// <remarks>
    /// This cannot be used in conjunction with any other means of getting the body of the request.
    /// It is not necessary to wait until the message has been sent before starting to write to the
    /// stream, but it is advisable to do so, since it will allow the network I/O to start earlier
    /// and the work of sending data can be overlapped with the production of more data.
    /// </remarks>
    concurrency::streams::istream body() const { return _m_impl->instream(); }

    /// <summary>
    /// Signals the user (client) when all the data for this response message has been received.
    /// </summary>
    /// <returns>A <c>task</c> which is completed when all of the response body has been received.</returns>
    pplx::task<http::http_response> content_ready() const
    {
        http_response resp = *this;
        return pplx::create_task(_m_impl->_get_data_available()).then([resp](utility::size64_t) mutable {
            return resp;
        });
    }

    std::shared_ptr<http::details::_http_response> _get_impl() const { return _m_impl; }

    http::details::_http_server_context* _get_server_context() const { return _m_impl->_get_server_context(); }
    void _set_server_context(std::unique_ptr<http::details::_http_server_context> server_context)
    {
        _m_impl->_set_server_context(std::move(server_context));
    }

private:
    std::shared_ptr<http::details::_http_response> _m_impl;
};

namespace details
{
/// <summary>
/// Internal representation of an HTTP request message.
/// </summary>
class _http_request final : public http::details::http_msg_base, public std::enable_shared_from_this<_http_request>
{
public:
    _ASYNCRTIMP _http_request(http::method mtd);

    _ASYNCRTIMP _http_request(std::unique_ptr<http::details::_http_server_context> server_context);

    virtual ~_http_request() {}

    http::method& method() { return m_method; }

    uri& request_uri() { return m_uri; }

    _ASYNCRTIMP uri absolute_uri() const;

    _ASYNCRTIMP uri relative_uri() const;

    _ASYNCRTIMP void set_request_uri(const uri&);

    const utility::string_t& remote_address() const { return m_remote_address; }

    const pplx::cancellation_token& cancellation_token() const { return m_cancellationToken; }

    void set_cancellation_token(const pplx::cancellation_token& token) { m_cancellationToken = token; }

    _ASYNCRTIMP utility::string_t to_string() const;

    _ASYNCRTIMP pplx::task<void> reply(const http_response& response);

    pplx::task<http_response> get_response() { return pplx::task<http_response>(m_response); }

    _ASYNCRTIMP pplx::task<void> _reply_if_not_already(http::status_code status);

    void set_response_stream(const concurrency::streams::ostream& stream) { m_response_stream = stream; }

    void set_progress_handler(const progress_handler& handler)
    {
        m_progress_handler = std::make_shared<progress_handler>(handler);
    }

    const concurrency::streams::ostream& _response_stream() const { return m_response_stream; }

    const std::shared_ptr<progress_handler>& _progress_handler() const { return m_progress_handler; }

    http::details::_http_server_context* _get_server_context() const { return m_server_context.get(); }

    void _set_server_context(std::unique_ptr<http::details::_http_server_context> server_context)
    {
        m_server_context = std::move(server_context);
    }

    void _set_listener_path(const utility::string_t& path) { m_listener_path = path; }

    void _set_base_uri(const http::uri& base_uri) { m_base_uri = base_uri; }

    void _set_remote_address(const utility::string_t& remote_address) { m_remote_address = remote_address; }

private:
    // Actual initiates sending the response, without checking if a response has already been sent.
    pplx::task<void> _reply_impl(http_response response);

    http::method m_method;

    // Tracks whether or not a response has already been started for this message.
    // 0 = No reply sent
    // 1 = Usual reply sent
    // 2 = Reply aborted by another thread; e.g. server shutdown
    pplx::details::atomic_long m_initiated_response;

    std::unique_ptr<http::details::_http_server_context> m_server_context;

    pplx::cancellation_token m_cancellationToken;

    http::uri m_base_uri;
    http::uri m_uri;
    utility::string_t m_listener_path;

    concurrency::streams::ostream m_response_stream;

    std::shared_ptr<progress_handler> m_progress_handler;

    pplx::task_completion_event<http_response> m_response;

    utility::string_t m_remote_address;
};

} // namespace details

/// <summary>
/// Represents an HTTP request.
/// </summary>
class http_request
{
public:
    /// <summary>
    /// Constructs a new HTTP request with the 'GET' method.
    /// </summary>
    http_request() : _m_impl(std::make_shared<http::details::_http_request>(methods::GET)) {}

    /// <summary>
    /// Constructs a new HTTP request with the given request method.
    /// </summary>
    /// <param name="mtd">Request method.</param>
    http_request(http::method mtd) : _m_impl(std::make_shared<http::details::_http_request>(std::move(mtd))) {}

    /// <summary>
    /// Destructor frees any held resources.
    /// </summary>
    ~http_request() {}

    /// <summary>
    /// Get the method (GET/PUT/POST/DELETE) of the request message.
    /// </summary>
    /// <returns>Request method of this HTTP request.</returns>
    const http::method& method() const { return _m_impl->method(); }

    /// <summary>
    /// Set the method (GET/PUT/POST/DELETE) of the request message.
    /// </summary>
    /// <param name="method">Request method of this HTTP request.</param>
    void set_method(const http::method& method) const { _m_impl->method() = method; }

    /// <summary>
    /// Get the underling URI of the request message.
    /// </summary>
    /// <returns>The uri of this message.</returns>
    const uri& request_uri() const { return _m_impl->request_uri(); }

    /// <summary>
    /// Set the underling URI of the request message.
    /// </summary>
    /// <param name="uri">The uri for this message.</param>
    void set_request_uri(const uri& uri) { return _m_impl->set_request_uri(uri); }

    /// <summary>
    /// Gets a reference the URI path, query, and fragment part of this request message.
    /// This will be appended to the base URI specified at construction of the http_client.
    /// </summary>
    /// <returns>A string.</returns>
    /// <remarks>When the request is the one passed to a listener's handler, the
    /// relative URI is the request URI less the listener's path. In all other circumstances,
    /// request_uri() and relative_uri() will return the same value.
    /// </remarks>
    uri relative_uri() const { return _m_impl->relative_uri(); }

    /// <summary>
    /// Get an absolute URI with scheme, host, port, path, query, and fragment part of
    /// the request message.
    /// </summary>
    /// <remarks>Absolute URI is only valid after this http_request object has been passed
    /// to http_client::request().
    /// </remarks>
    uri absolute_uri() const { return _m_impl->absolute_uri(); }

    /// <summary>
    /// Gets a reference to the headers of the response message.
    /// </summary>
    /// <returns>HTTP headers for this response.</returns>
    /// <remarks>
    /// Use the http_headers::add to fill in desired headers.
    /// </remarks>
    http_headers& headers() { return _m_impl->headers(); }

    /// <summary>
    /// Gets a const reference to the headers of the response message.
    /// </summary>
    /// <returns>HTTP headers for this response.</returns>
    /// <remarks>
    /// Use the http_headers::add to fill in desired headers.
    /// </remarks>
    const http_headers& headers() const { return _m_impl->headers(); }

    /// <summary>
    /// Returns the HTTP protocol version of this request message.
    /// </summary>
    /// <returns>The HTTP protocol version.</returns>
    http::http_version http_version() const { return _m_impl->http_version(); }

    /// <summary>
    /// Returns a string representation of the remote IP address.
    /// </summary>
    /// <returns>The remote IP address.</returns>
    const utility::string_t& remote_address() const { return _m_impl->remote_address(); }

    CASABLANCA_DEPRECATED("Use `remote_address()` instead.")
    const utility::string_t& get_remote_address() const { return _m_impl->remote_address(); }

    /// <summary>
    /// Extract the body of the request message as a string value, checking that the content type is a MIME text type.
    /// A body can only be extracted once because in some cases an optimization is made where the data is 'moved' out.
    /// </summary>
    /// <param name="ignore_content_type">If true, ignores the Content-Type header and assumes UTF-8.</param>
    /// <returns>String containing body of the message.</returns>
    pplx::task<utility::string_t> extract_string(bool ignore_content_type = false)
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl, ignore_content_type](utility::size64_t) {
            return impl->extract_string(ignore_content_type);
        });
    }

    /// <summary>
    /// Extract the body of the request message as a UTF-8 string value, checking that the content type is a MIME text
    /// type. A body can only be extracted once because in some cases an optimization is made where the data is 'moved'
    /// out.
    /// </summary>
    /// <param name="ignore_content_type">If true, ignores the Content-Type header and assumes UTF-8.</param>
    /// <returns>String containing body of the message.</returns>
    pplx::task<utf8string> extract_utf8string(bool ignore_content_type = false)
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl, ignore_content_type](utility::size64_t) {
            return impl->extract_utf8string(ignore_content_type);
        });
    }

    /// <summary>
    /// Extract the body of the request message as a UTF-16 string value, checking that the content type is a MIME text
    /// type. A body can only be extracted once because in some cases an optimization is made where the data is 'moved'
    /// out.
    /// </summary>
    /// <param name="ignore_content_type">If true, ignores the Content-Type header and assumes UTF-16.</param>
    /// <returns>String containing body of the message.</returns>
    pplx::task<utf16string> extract_utf16string(bool ignore_content_type = false)
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl, ignore_content_type](utility::size64_t) {
            return impl->extract_utf16string(ignore_content_type);
        });
    }

    /// <summary>
    /// Extracts the body of the request message into a json value, checking that the content type is application/json.
    /// A body can only be extracted once because in some cases an optimization is made where the data is 'moved' out.
    /// </summary>
    /// <param name="ignore_content_type">If true, ignores the Content-Type header and assumes UTF-8.</param>
    /// <returns>JSON value from the body of this message.</returns>
    pplx::task<json::value> extract_json(bool ignore_content_type = false) const
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl, ignore_content_type](utility::size64_t) {
            return impl->_extract_json(ignore_content_type);
        });
    }

    /// <summary>
    /// Extract the body of the response message into a vector of bytes. Extracting a vector can be done on
    /// </summary>
    /// <returns>The body of the message as a vector of bytes.</returns>
    pplx::task<std::vector<unsigned char>> extract_vector() const
    {
        auto impl = _m_impl;
        return pplx::create_task(_m_impl->_get_data_available()).then([impl](utility::size64_t) {
            return impl->_extract_vector();
        });
    }

    /// <summary>
    /// Sets the body of the message to a textual string and set the "Content-Type" header. Assumes
    /// the character encoding of the string is UTF-8.
    /// </summary>
    /// <param name="body_text">String containing body text.</param>
    /// <param name="content_type">MIME type to set the "Content-Type" header to. Default to "text/plain;
    /// charset=utf-8".</param> <remarks> This will overwrite any previously set body data and "Content-Type" header.
    /// </remarks>
    void set_body(utf8string&& body_text, const utf8string& content_type = utf8string("text/plain; charset=utf-8"))
    {
        const auto length = body_text.size();
        _m_impl->set_body(
            concurrency::streams::bytestream::open_istream<std::string>(std::move(body_text)), length, content_type);
    }

    /// <summary>
    /// Sets the body of the message to a textual string and set the "Content-Type" header. Assumes
    /// the character encoding of the string is UTF-8.
    /// </summary>
    /// <param name="body_text">String containing body text.</param>
    /// <param name="content_type">MIME type to set the "Content-Type" header to. Default to "text/plain;
    /// charset=utf-8".</param> <remarks> This will overwrite any previously set body data and "Content-Type" header.
    /// </remarks>
    void set_body(const utf8string& body_text, const utf8string& content_type = utf8string("text/plain; charset=utf-8"))
    {
        _m_impl->set_body(
            concurrency::streams::bytestream::open_istream<std::string>(body_text), body_text.size(), content_type);
    }

    /// <summary>
    /// Sets the body of the message to a textual string and set the "Content-Type" header. Assumes
    /// the character encoding of the string is UTF-16 will perform conversion to UTF-8.
    /// </summary>
    /// </summary>
    /// <param name="body_text">String containing body text.</param>
    /// <param name="content_type">MIME type to set the "Content-Type" header to. Default to "text/plain".</param>
    /// <remarks>
    /// This will overwrite any previously set body data and "Content-Type" header.
    /// </remarks>
    void set_body(const utf16string& body_text,
                  utf16string content_type = utility::conversions::to_utf16string("text/plain"))
    {
        if (content_type.find(::utility::conversions::to_utf16string("charset=")) != content_type.npos)
        {
            throw std::invalid_argument("content_type can't contain a 'charset'.");
        }

        auto utf8body = utility::conversions::utf16_to_utf8(body_text);
        auto length = utf8body.size();
        _m_impl->set_body(concurrency::streams::bytestream::open_istream(std::move(utf8body)),
                          length,
                          std::move(content_type.append(::utility::conversions::to_utf16string("; charset=utf-8"))));
    }

    /// <summary>
    /// Sets the body of the message to contain json value. If the 'Content-Type'
    /// header hasn't already been set it will be set to 'application/json'.
    /// </summary>
    /// <param name="body_data">json value.</param>
    /// <remarks>
    /// This will overwrite any previously set body data.
    /// </remarks>
    void set_body(const json::value& body_data)
    {
        auto body_text = utility::conversions::to_utf8string(body_data.serialize());
        auto length = body_text.size();
        _m_impl->set_body(concurrency::streams::bytestream::open_istream(std::move(body_text)),
                          length,
                          _XPLATSTR("application/json"));
    }

    /// <summary>
    /// Sets the body of the message to the contents of a byte vector. If the 'Content-Type'
    /// header hasn't already been set it will be set to 'application/octet-stream'.
    /// </summary>
    /// <param name="body_data">Vector containing body data.</param>
    /// <remarks>
    /// This will overwrite any previously set body data.
    /// </remarks>
    void set_body(std::vector<unsigned char>&& body_data)
    {
        auto length = body_data.size();
        _m_impl->set_body(concurrency::streams::bytestream::open_istream(std::move(body_data)),
                          length,
                          _XPLATSTR("application/octet-stream"));
    }

    /// <summary>
    /// Sets the body of the message to the contents of a byte vector. If the 'Content-Type'
    /// header hasn't already been set it will be set to 'application/octet-stream'.
    /// </summary>
    /// <param name="body_data">Vector containing body data.</param>
    /// <remarks>
    /// This will overwrite any previously set body data.
    /// </remarks>
    void set_body(const std::vector<unsigned char>& body_data)
    {
        set_body(concurrency::streams::bytestream::open_istream(body_data), body_data.size());
    }

    /// <summary>
    /// Defines a stream that will be relied on to provide the body of the HTTP message when it is
    /// sent.
    /// </summary>
    /// <param name="stream">A readable, open asynchronous stream.</param>
    /// <param name="content_type">A string holding the MIME type of the message body.</param>
    /// <remarks>
    /// This cannot be used in conjunction with any other means of setting the body of the request.
    /// The stream will not be read until the message is sent.
    /// </remarks>
    void set_body(const concurrency::streams::istream& stream,
                  const utility::string_t& content_type = _XPLATSTR("application/octet-stream"))
    {
        _m_impl->set_body(stream, content_type);
    }

    /// <summary>
    /// Defines a stream that will be relied on to provide the body of the HTTP message when it is
    /// sent.
    /// </summary>
    /// <param name="stream">A readable, open asynchronous stream.</param>
    /// <param name="content_length">The size of the data to be sent in the body.</param>
    /// <param name="content_type">A string holding the MIME type of the message body.</param>
    /// <remarks>
    /// This cannot be used in conjunction with any other means of setting the body of the request.
    /// The stream will not be read until the message is sent.
    /// </remarks>
    void set_body(const concurrency::streams::istream& stream,
                  utility::size64_t content_length,
                  const utility::string_t& content_type = _XPLATSTR("application/octet-stream"))
    {
        _m_impl->set_body(stream, content_length, content_type);
    }

    /// <summary>
    /// Produces a stream which the caller may use to retrieve data from an incoming request.
    /// </summary>
    /// <returns>A readable, open asynchronous stream.</returns>
    /// <remarks>
    /// This cannot be used in conjunction with any other means of getting the body of the request.
    /// It is not necessary to wait until the message has been sent before starting to write to the
    /// stream, but it is advisable to do so, since it will allow the network I/O to start earlier
    /// and the work of sending data can be overlapped with the production of more data.
    /// </remarks>
    concurrency::streams::istream body() const { return _m_impl->instream(); }

    /// <summary>
    /// Defines a stream that will be relied on to hold the body of the HTTP response message that
    /// results from the request.
    /// </summary>
    /// <param name="stream">A writable, open asynchronous stream.</param>
    /// <remarks>
    /// If this function is called, the body of the response should not be accessed in any other
    /// way.
    /// </remarks>
    void set_response_stream(const concurrency::streams::ostream& stream)
    {
        return _m_impl->set_response_stream(stream);
    }

    /// <summary>
    /// Sets a compressor that will be used to compress the body of the HTTP message as it is sent.
    /// </summary>
    /// <param name="compressor">A pointer to an instantiated compressor of the desired type.</param>
    /// <remarks>
    /// This cannot be used in conjunction with any external means of compression.  The Transfer-Encoding
    /// header will be managed internally, and must not be set by the client.
    /// </remarks>
    void set_compressor(std::unique_ptr<http::compression::compress_provider> compressor)
    {
        return _m_impl->set_compressor(std::move(compressor));
    }

    /// <summary>
    /// Sets a compressor that will be used to compress the body of the HTTP message as it is sent.
    /// </summary>
    /// <param name="algorithm">The built-in compression algorithm to use.</param>
    /// <returns>
    /// True if a built-in compressor was instantiated, otherwise false.
    /// </returns>
    /// <remarks>
    /// This cannot be used in conjunction with any external means of compression.  The Transfer-Encoding
    /// header will be managed internally, and must not be set by the client.
    /// </remarks>
    bool set_compressor(utility::string_t algorithm)
    {
        _m_impl->set_compressor(http::compression::builtin::make_compressor(algorithm));
        return (bool)_m_impl->compressor();
    }

    /// <summary>
    /// Gets the compressor to be used to compress the message body, if any.
    /// </summary>
    /// <returns>
    /// The compressor itself.
    /// </returns>
    std::unique_ptr<http::compression::compress_provider>& compressor() { return _m_impl->compressor(); }

    /// <summary>
    /// Sets the default collection of built-in factory classes for decompressors that may be used to
    /// decompress the body of the HTTP message as it is received, effectively enabling decompression.
    /// </summary>
    /// <param name="factories">The collection of factory classes for allowable decompressors. The
    /// supplied vector itself need not remain valid after the call returns.</param>
    /// <remarks>
    /// This default collection is implied if request_compressed_response() is set in the associated
    /// <c>client::http_client_config</c> and neither overload of this method has been called.
    ///
    /// This cannot be used in conjunction with any external means of decompression.  The TE and Accept-Encoding
    /// headers must not be set by the client, as they will be managed internally as appropriate.
    /// </remarks>
    _ASYNCRTIMP void set_decompress_factories();

    /// <summary>
    /// Sets a collection of factory classes for decompressors that may be used to decompress the
    /// body of the HTTP message as it is received, effectively enabling decompression.
    /// </summary>
    /// <remarks>
    /// If set, this collection takes the place of the built-in compression providers.  It may contain
    /// custom factory classes and/or factory classes for built-in providers, and may be used to adjust
    /// the weights of the built-in providers, which default to 500 (i.e. "q=0.500").
    ///
    /// This cannot be used in conjunction with any external means of decompression.  The TE and Accept-Encoding
    /// headers must not be set by the client, as they will be managed internally as appropriate.
    /// </remarks>
    void set_decompress_factories(const std::vector<std::shared_ptr<http::compression::decompress_factory>>& factories)
    {
        return _m_impl->set_decompress_factories(factories);
    }

    /// <summary>
    /// Gets the collection of factory classes for decompressors to be used to decompress the message body, if any.
    /// </summary>
    /// <returns>
    /// The collection of factory classes itself.
    /// </returns>
    /// <remarks>
    /// This cannot be used in conjunction with any external means of decompression.  The TE
    /// header must not be set by the client, as it will be managed internally.
    /// </remarks>
    const std::vector<std::shared_ptr<http::compression::decompress_factory>>& decompress_factories() const
    {
        return _m_impl->decompress_factories();
    }

    /// <summary>
    /// Defines a callback function that will be invoked for every chunk of data uploaded or downloaded
    /// as part of the request.
    /// </summary>
    /// <param name="handler">A function representing the progress handler. It's parameters are:
    ///    up:       a <c>message_direction::direction</c> value  indicating the direction of the message
    ///              that is being reported.
    ///    progress: the number of bytes that have been processed so far.
    /// </param>
    /// <remarks>
    ///   This function will be called at least once for upload and at least once for
    ///   the download body, unless there is some exception generated. An HTTP message with an error
    ///   code is not an exception. This means, that even if there is no body, the progress handler
    ///   will be called.
    ///
    ///   Setting the chunk size on the http_client does not guarantee that the client will be using
    ///   exactly that increment for uploading and downloading data.
    ///
    ///   The handler will be called only once for each combination of argument values, in order. Depending
    ///   on how a service responds, some download values may come before all upload values have been
    ///   reported.
    ///
    ///   The progress handler will be called on the thread processing the request. This means that
    ///   the implementation of the handler must take care not to block the thread or do anything
    ///   that takes significant amounts of time. In particular, do not do any kind of I/O from within
    ///   the handler, do not update user interfaces, and to not acquire any locks. If such activities
    ///   are necessary, it is the handler's responsibility to execute that work on a separate thread.
    /// </remarks>
    void set_progress_handler(const progress_handler& handler) { return _m_impl->set_progress_handler(handler); }

    /// <summary>
    /// Asynchronously responses to this HTTP request.
    /// </summary>
    /// <param name="response">Response to send.</param>
    /// <returns>An asynchronous operation that is completed once response is sent.</returns>
    pplx::task<void> reply(const http_response& response) const { return _m_impl->reply(response); }

    /// <summary>
    /// Asynchronously responses to this HTTP request.
    /// </summary>
    /// <param name="status">Response status code.</param>
    /// <returns>An asynchronous operation that is completed once response is sent.</returns>
    pplx::task<void> reply(http::status_code status) const { return reply(http_response(status)); }

    /// <summary>
    /// Responds to this HTTP request.
    /// </summary>
    /// <param name="status">Response status code.</param>
    /// <param name="body_data">Json value to use in the response body.</param>
    /// <returns>An asynchronous operation that is completed once response is sent.</returns>
    pplx::task<void> reply(http::status_code status, const json::value& body_data) const
    {
        http_response response(status);
        response.set_body(body_data);
        return reply(response);
    }

    /// Responds to this HTTP request with a string.
    /// Assumes the character encoding of the string is UTF-8.
    /// </summary>
    /// <param name="status">Response status code.</param>
    /// <param name="body_data">UTF-8 string containing the text to use in the response body.</param>
    /// <param name="content_type">Content type of the body.</param>
    /// <returns>An asynchronous operation that is completed once response is sent.</returns>
    /// <remarks>
    //  Callers of this function do NOT need to block waiting for the response to be
    /// sent to before the body data is destroyed or goes out of scope.
    /// </remarks>
    pplx::task<void> reply(http::status_code status,
                           utf8string&& body_data,
                           const utf8string& content_type = "text/plain; charset=utf-8") const
    {
        http_response response(status);
        response.set_body(std::move(body_data), content_type);
        return reply(response);
    }

    /// <summary>
    /// Responds to this HTTP request with a string.
    /// Assumes the character encoding of the string is UTF-8.
    /// </summary>
    /// <param name="status">Response status code.</param>
    /// <param name="body_data">UTF-8 string containing the text to use in the response body.</param>
    /// <param name="content_type">Content type of the body.</param>
    /// <returns>An asynchronous operation that is completed once response is sent.</returns>
    /// <remarks>
    //  Callers of this function do NOT need to block waiting for the response to be
    /// sent to before the body data is destroyed or goes out of scope.
    /// </remarks>
    pplx::task<void> reply(http::status_code status,
                           const utf8string& body_data,
                           const utf8string& content_type = "text/plain; charset=utf-8") const
    {
        http_response response(status);
        response.set_body(body_data, content_type);
        return reply(response);
    }

    /// <summary>
    /// Responds to this HTTP request with a string. Assumes the character encoding
    /// of the string is UTF-16 will perform conversion to UTF-8.
    /// </summary>
    /// <param name="status">Response status code.</param>
    /// <param name="body_data">UTF-16 string containing the text to use in the response body.</param>
    /// <param name="content_type">Content type of the body.</param>
    /// <returns>An asynchronous operation that is completed once response is sent.</returns>
    /// <remarks>
    //  Callers of this function do NOT need to block waiting for the response to be
    /// sent to before the body data is destroyed or goes out of scope.
    /// </remarks>
    pplx::task<void> reply(http::status_code status,
                           const utf16string& body_data,
                           const utf16string& content_type = utility::conversions::to_utf16string("text/plain")) const
    {
        http_response response(status);
        response.set_body(body_data, content_type);
        return reply(response);
    }

    /// <summary>
    /// Responds to this HTTP request.
    /// </summary>
    /// <param name="status">Response status code.</param>
    /// <param name="content_type">A string holding the MIME type of the message body.</param>
    /// <param name="body">An asynchronous stream representing the body data.</param>
    /// <returns>A task that is completed once a response from the request is received.</returns>
    pplx::task<void> reply(status_code status,
                           const concurrency::streams::istream& body,
                           const utility::string_t& content_type = _XPLATSTR("application/octet-stream")) const
    {
        http_response response(status);
        response.set_body(body, content_type);
        return reply(response);
    }

    /// <summary>
    /// Responds to this HTTP request.
    /// </summary>
    /// <param name="status">Response status code.</param>
    /// <param name="content_length">The size of the data to be sent in the body..</param>
    /// <param name="content_type">A string holding the MIME type of the message body.</param>
    /// <param name="body">An asynchronous stream representing the body data.</param>
    /// <returns>A task that is completed once a response from the request is received.</returns>
    pplx::task<void> reply(status_code status,
                           const concurrency::streams::istream& body,
                           utility::size64_t content_length,
                           const utility::string_t& content_type = _XPLATSTR("application/octet-stream")) const
    {
        http_response response(status);
        response.set_body(body, content_length, content_type);
        return reply(response);
    }

    /// <summary>
    /// Signals the user (listener) when all the data for this request message has been received.
    /// </summary>
    /// <returns>A <c>task</c> which is completed when all of the response body has been received</returns>
    pplx::task<http_request> content_ready() const
    {
        http_request req = *this;
        return pplx::create_task(_m_impl->_get_data_available()).then([req](utility::size64_t) mutable { return req; });
    }

    /// <summary>
    /// Gets a task representing the response that will eventually be sent.
    /// </summary>
    /// <returns>A task that is completed once response is sent.</returns>
    pplx::task<http_response> get_response() const { return _m_impl->get_response(); }

    /// <summary>
    /// Generates a string representation of the message, including the body when possible.
    /// Mainly this should be used for debugging purposes as it has to copy the
    /// message body and doesn't have excellent performance.
    /// </summary>
    /// <returns>A string representation of this HTTP request.</returns>
    /// <remarks>Note this function is synchronous and doesn't wait for the
    /// entire message body to arrive. If the message body has arrived by the time this
    /// function is called and it is has a textual Content-Type it will be included.
    /// Otherwise just the headers will be present.</remarks>
    utility::string_t to_string() const { return _m_impl->to_string(); }

    /// <summary>
    /// Sends a response if one has not already been sent.
    /// </summary>
    pplx::task<void> _reply_if_not_already(status_code status) { return _m_impl->_reply_if_not_already(status); }

    /// <summary>
    /// Gets the server context associated with this HTTP message.
    /// </summary>
    http::details::_http_server_context* _get_server_context() const { return _m_impl->_get_server_context(); }

    /// <summary>
    /// These are used for the initial creation of the HTTP request.
    /// </summary>
    static http_request _create_request(std::unique_ptr<http::details::_http_server_context> server_context)
    {
        return http_request(std::move(server_context));
    }
    void _set_server_context(std::unique_ptr<http::details::_http_server_context> server_context)
    {
        _m_impl->_set_server_context(std::move(server_context));
    }

    void _set_listener_path(const utility::string_t& path) { _m_impl->_set_listener_path(path); }

    const std::shared_ptr<http::details::_http_request>& _get_impl() const { return _m_impl; }

    void _set_cancellation_token(const pplx::cancellation_token& token) { _m_impl->set_cancellation_token(token); }

    const pplx::cancellation_token& _cancellation_token() const { return _m_impl->cancellation_token(); }

    void _set_base_uri(const http::uri& base_uri) { _m_impl->_set_base_uri(base_uri); }

private:
    friend class http::details::_http_request;
    friend class http::client::http_client;

    http_request(std::unique_ptr<http::details::_http_server_context> server_context)
        : _m_impl(std::make_shared<details::_http_request>(std::move(server_context)))
    {
    }

    std::shared_ptr<http::details::_http_request> _m_impl;
};

namespace client
{
class http_pipeline;
}

/// <summary>
/// HTTP client handler class, used to represent an HTTP pipeline stage.
/// </summary>
/// <remarks>
/// When a request goes out, it passes through a series of stages, customizable by
/// the application and/or libraries. The default stage will interact with lower-level
/// communication layers to actually send the message on the network. When creating a client
/// instance, an application may add pipeline stages in front of the already existing
/// stages. Each stage has a reference to the next stage available in the <seealso cref="http_pipeline_stage::next_stage
/// Method"/> value.
/// </remarks>
class http_pipeline_stage : public std::enable_shared_from_this<http_pipeline_stage>
{
public:
    http_pipeline_stage() = default;

    http_pipeline_stage& operator=(const http_pipeline_stage&) = delete;
    http_pipeline_stage(const http_pipeline_stage&) = delete;

    virtual ~http_pipeline_stage() = default;

    /// <summary>
    /// Runs this stage against the given request and passes onto the next stage.
    /// </summary>
    /// <param name="request">The HTTP request.</param>
    /// <returns>A task of the HTTP response.</returns>
    virtual pplx::task<http_response> propagate(http_request request) = 0;

protected:
    /// <summary>
    /// Gets the next stage in the pipeline.
    /// </summary>
    /// <returns>A shared pointer to a pipeline stage.</returns>
    const std::shared_ptr<http_pipeline_stage>& next_stage() const { return m_next_stage; }

    /// <summary>
    /// Gets a shared pointer to this pipeline stage.
    /// </summary>
    /// <returns>A shared pointer to a pipeline stage.</returns>
    CASABLANCA_DEPRECATED("This api is redundant. Use 'shared_from_this()' directly instead.")
    std::shared_ptr<http_pipeline_stage> current_stage() { return this->shared_from_this(); }

private:
    friend class ::web::http::client::http_pipeline;

    void set_next_stage(const std::shared_ptr<http_pipeline_stage>& next) { m_next_stage = next; }

    std::shared_ptr<http_pipeline_stage> m_next_stage;
};

} // namespace http
} // namespace web
