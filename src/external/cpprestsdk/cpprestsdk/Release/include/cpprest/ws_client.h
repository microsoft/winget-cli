/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Websocket client side implementation
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

#ifndef CASA_WS_CLIENT_H
#define CASA_WS_CLIENT_H

#include "cpprest/details/basic_types.h"

#if !defined(CPPREST_EXCLUDE_WEBSOCKETS)

#include "cpprest/asyncrt_utils.h"
#include "cpprest/details/web_utilities.h"
#include "cpprest/http_headers.h"
#include "cpprest/uri.h"
#include "cpprest/ws_msg.h"
#include "pplx/pplxtasks.h"
#include <condition_variable>
#include <limits>
#include <memory>
#include <mutex>

#if !defined(_WIN32) || !defined(__cplusplus_winrt)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif
#include "boost/asio/ssl.hpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif

namespace web
{
// For backwards compatibility for when in the experimental namespace.
// At next major release this should be deleted.
namespace experimental = web;

// In the past namespace was accidentally called 'web_sockets'. To avoid breaking code
// alias it. At our next major release this should be deleted.
namespace web_sockets = websockets;

namespace websockets
{
/// WebSocket client side library.
namespace client
{
/// Websocket close status values.
enum class websocket_close_status
{
    normal = 1000,
    going_away = 1001,
    protocol_error = 1002,
    unsupported = 1003, // or data_mismatch
    abnormal_close = 1006,
    inconsistent_datatype = 1007,
    policy_violation = 1008,
    too_large = 1009,
    negotiate_error = 1010,
    server_terminate = 1011,
};

/// <summary>
/// Websocket client configuration class, used to set the possible configuration options
/// used to create an websocket_client instance.
/// </summary>
class websocket_client_config
{
public:
    /// <summary>
    /// Creates a websocket client configuration with default settings.
    /// </summary>
    websocket_client_config() : m_sni_enabled(true), m_validate_certificates(true) {}

    /// <summary>
    /// Get the web proxy object
    /// </summary>
    /// <returns>A reference to the web proxy object.</returns>
    const web_proxy& proxy() const { return m_proxy; }

    /// <summary>
    /// Set the web proxy object
    /// </summary>
    /// <param name="proxy">The web proxy object.</param>
    void set_proxy(const web_proxy& proxy) { m_proxy = proxy; }

    /// <summary>
    /// Get the client credentials
    /// </summary>
    /// <returns>A reference to the client credentials.</returns>
    const web::credentials& credentials() const { return m_credentials; }

    /// <summary>
    /// Set the client credentials
    /// </summary>
    /// <param name="cred">The client credentials.</param>
    void set_credentials(const web::credentials& cred) { m_credentials = cred; }

    /// <summary>
    /// Disables Server Name Indication (SNI). Default is on.
    /// </summary>
    void disable_sni() { m_sni_enabled = false; }

    /// <summary>
    /// Determines if Server Name Indication (SNI) is enabled.
    /// </summary>
    /// <returns>True if enabled, false otherwise.</returns>
    bool is_sni_enabled() const { return m_sni_enabled; }

    /// <summary>
    /// Sets the server host name to use for TLS Server Name Indication (SNI).
    /// </summary>
    /// <remarks>By default the host name is set to the websocket URI host.</remarks>
    /// <param name="name">The host name to use, as a string.</param>
    void set_server_name(const utf8string& name) { m_sni_hostname = name; }

    /// <summary>
    /// Gets the server host name to use for TLS Server Name Indication (SNI).
    /// </summary>
    /// <returns>Host name as a string.</returns>
    const utf8string& server_name() const { return m_sni_hostname; }

    /// <summary>
    /// Sets the User Agent to be used for the connection
    /// </summary>
    /// <param name="name">The User Agent to use, as a string.</param>
    _ASYNCRTIMP void set_user_agent(const utf8string& user_agent);

    /// <summary>
    /// Gets the headers of the HTTP request message used in the WebSocket protocol handshake.
    /// </summary>
    /// <returns>HTTP headers for the WebSocket protocol handshake.</returns>
    /// <remarks>
    /// Use the <seealso cref="http_headers::add Method"/> to fill in desired headers.
    /// </remarks>
    web::http::http_headers& headers() { return m_headers; }

    /// <summary>
    /// Gets a const reference to the headers of the WebSocket protocol handshake HTTP message.
    /// </summary>
    /// <returns>HTTP headers.</returns>
    const web::http::http_headers& headers() const { return m_headers; }

    /// <summary>
    /// Adds a subprotocol to the request headers.
    /// </summary>
    /// <param name="name">The name of the subprotocol.</param>
    /// <remarks>If additional subprotocols have already been specified, the new one will just be added.</remarks>
    _ASYNCRTIMP void add_subprotocol(const ::utility::string_t& name);

    /// <summary>
    /// Gets list of the specified subprotocols.
    /// </summary>
    /// <returns>Vector of all the subprotocols </returns>
    /// <remarks>If you want all the subprotocols in a comma separated string
    /// they can be directly looked up in the headers using 'Sec-WebSocket-Protocol'.</remarks>
    _ASYNCRTIMP std::vector<::utility::string_t> subprotocols() const;

    /// <summary>
    /// Gets the server certificate validation property.
    /// </summary>
    /// <returns>True if certificates are to be verified, false otherwise.</returns>
    bool validate_certificates() const { return m_validate_certificates; }

    /// <summary>
    /// Sets the server certificate validation property.
    /// </summary>
    /// <param name="validate_certs">False to turn ignore all server certificate validation errors, true
    /// otherwise.</param> <remarks>Note ignoring certificate errors can be dangerous and should be done with
    /// caution.</remarks>
    void set_validate_certificates(bool validate_certs) { m_validate_certificates = validate_certs; }

#if !defined(_WIN32) || !defined(__cplusplus_winrt)
    /// <summary>
    /// Sets a callback to enable custom setting of the ssl context, at construction time.
    /// </summary>
    /// <param name="callback">A user callback allowing for customization of the ssl context at construction
    /// time.</param>
    void set_ssl_context_callback(const std::function<void(boost::asio::ssl::context&)>& callback)
    {
        m_ssl_context_callback = callback;
    }

    /// <summary>
    /// Gets the user's callback to allow for customization of the ssl context.
    /// </summary>
    const std::function<void(boost::asio::ssl::context&)>& get_ssl_context_callback() const
    {
        return m_ssl_context_callback;
    }
#endif

private:
    web::web_proxy m_proxy;
    web::credentials m_credentials;
    web::http::http_headers m_headers;
    bool m_sni_enabled;
    utf8string m_sni_hostname;
    bool m_validate_certificates;
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
    std::function<void(boost::asio::ssl::context&)> m_ssl_context_callback;
#endif
};

/// <summary>
/// Represents a websocket error. This class holds an error message and an optional error code.
/// </summary>
class websocket_exception : public std::exception
{
public:
    /// <summary>
    /// Creates an <c>websocket_exception</c> with just a string message and no error code.
    /// </summary>
    /// <param name="whatArg">Error message string.</param>
    websocket_exception(const utility::string_t& whatArg) : m_msg(utility::conversions::to_utf8string(whatArg)) {}

#ifdef _WIN32
    /// <summary>
    /// Creates an <c>websocket_exception</c> with just a string message and no error code.
    /// </summary>
    /// <param name="whatArg">Error message string.</param>
    websocket_exception(std::string whatArg) : m_msg(std::move(whatArg)) {}
#endif

    /// <summary>
    /// Creates a <c>websocket_exception</c> from a error code using the current platform error category.
    /// The message of the error code will be used as the what() string message.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    websocket_exception(int errorCode) : m_errorCode(utility::details::create_error_code(errorCode))
    {
        m_msg = m_errorCode.message();
    }

    /// <summary>
    /// Creates a <c>websocket_exception</c> from a error code using the current platform error category.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    /// <param name="whatArg">Message to use in what() string.</param>
    websocket_exception(int errorCode, const utility::string_t& whatArg)
        : m_errorCode(utility::details::create_error_code(errorCode))
        , m_msg(utility::conversions::to_utf8string(whatArg))
    {
    }

#ifdef _WIN32
    /// <summary>
    /// Creates a <c>websocket_exception</c> from a error code and string message.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    /// <param name="whatArg">Message to use in what() string.</param>
    websocket_exception(int errorCode, std::string whatArg)
        : m_errorCode(utility::details::create_error_code(errorCode)), m_msg(std::move(whatArg))
    {
    }

    /// <summary>
    /// Creates a <c>websocket_exception</c> from a error code and string message to use as the what() argument.
    /// <param name="code">Error code.</param>
    /// <param name="whatArg">Message to use in what() string.</param>
    /// </summary>
    websocket_exception(std::error_code code, std::string whatArg)
        : m_errorCode(std::move(code)), m_msg(std::move(whatArg))
    {
    }
#endif

    /// <summary>
    /// Creates a <c>websocket_exception</c> from a error code and category. The message of the error code will be used
    /// as the <c>what</c> string message.
    /// </summary>
    /// <param name="errorCode">Error code value.</param>
    /// <param name="cat">Error category for the code.</param>
    websocket_exception(int errorCode, const std::error_category& cat) : m_errorCode(std::error_code(errorCode, cat))
    {
        m_msg = m_errorCode.message();
    }

    /// <summary>
    /// Creates a <c>websocket_exception</c> from a error code and string message to use as the what() argument.
    /// <param name="code">Error code.</param>
    /// <param name="whatArg">Message to use in what() string.</param>
    /// </summary>
    websocket_exception(std::error_code code, const utility::string_t& whatArg)
        : m_errorCode(std::move(code)), m_msg(utility::conversions::to_utf8string(whatArg))
    {
    }

    /// <summary>
    /// Gets a string identifying the cause of the exception.
    /// </summary>
    /// <returns>A null terminated character string.</returns>
    const char* what() const CPPREST_NOEXCEPT { return m_msg.c_str(); }

    /// <summary>
    /// Gets the underlying error code for the cause of the exception.
    /// </summary>
    /// <returns>The <c>error_code</c> object associated with the exception.</returns>
    const std::error_code& error_code() const CPPREST_NOEXCEPT { return m_errorCode; }

private:
    std::error_code m_errorCode;
    std::string m_msg;
};

namespace details
{
// Interface to be implemented by the websocket client callback implementations.
class websocket_client_callback_impl
{
public:
    websocket_client_callback_impl(websocket_client_config config) : m_config(std::move(config)) {}

    virtual ~websocket_client_callback_impl() CPPREST_NOEXCEPT {}

    virtual pplx::task<void> connect() = 0;

    virtual pplx::task<void> send(websocket_outgoing_message& msg) = 0;

    virtual void set_message_handler(const std::function<void(const websocket_incoming_message&)>& handler) = 0;

    virtual pplx::task<void> close() = 0;

    virtual pplx::task<void> close(websocket_close_status close_status, const utility::string_t& close_reason = {}) = 0;

    virtual void set_close_handler(
        const std::function<void(websocket_close_status, const utility::string_t&, const std::error_code&)>&
            handler) = 0;

    const web::uri& uri() const { return m_uri; }

    void set_uri(const web::uri& uri) { m_uri = uri; }

    const websocket_client_config& config() const { return m_config; }

    static void verify_uri(const web::uri& uri)
    {
        // Most of the URI schema validation is taken care by URI class.
        // We only need to check certain things specific to websockets.
        if (uri.scheme() != _XPLATSTR("ws") && uri.scheme() != _XPLATSTR("wss"))
        {
            throw std::invalid_argument("URI scheme must be 'ws' or 'wss'");
        }

        if (uri.host().empty())
        {
            throw std::invalid_argument("URI must contain a hostname.");
        }

        // Fragment identifiers are meaningless in the context of WebSocket URIs
        // and MUST NOT be used on these URIs.
        if (!uri.fragment().empty())
        {
            throw std::invalid_argument("WebSocket URI must not contain fragment identifiers");
        }
    }

protected:
    web::uri m_uri;
    websocket_client_config m_config;
};

// Interface to be implemented by the websocket client task implementations.
class websocket_client_task_impl
{
public:
    _ASYNCRTIMP websocket_client_task_impl(websocket_client_config config);

    _ASYNCRTIMP virtual ~websocket_client_task_impl() CPPREST_NOEXCEPT;

    _ASYNCRTIMP pplx::task<websocket_incoming_message> receive();

    _ASYNCRTIMP void close_pending_tasks_with_error(const websocket_exception& exc);

    const std::shared_ptr<websocket_client_callback_impl>& callback_client() const { return m_callback_client; };

private:
    void set_handler();

    // When a message arrives, if there are tasks waiting for a message, signal the topmost one.
    // Else enqueue the message in a queue.
    // m_receive_queue_lock : to guard access to the queue & m_client_closed
    std::mutex m_receive_queue_lock;
    // Queue to store incoming messages when there are no tasks waiting for a message
    std::queue<websocket_incoming_message> m_receive_msg_queue;
    // Queue to maintain the receive tasks when there are no messages(yet).
    std::queue<pplx::task_completion_event<websocket_incoming_message>> m_receive_task_queue;

    // Initially set to false, becomes true if a close frame is received from the server or
    // if the underlying connection is aborted or terminated.
    bool m_client_closed;

    std::shared_ptr<websocket_client_callback_impl> m_callback_client;
};
} // namespace details

/// <summary>
/// Websocket client class, used to maintain a connection to a remote host for an extended session.
/// </summary>
class websocket_client
{
public:
    /// <summary>
    ///  Creates a new websocket_client.
    /// </summary>
    websocket_client() : m_client(std::make_shared<details::websocket_client_task_impl>(websocket_client_config())) {}

    /// <summary>
    ///  Creates a new websocket_client.
    /// </summary>
    /// <param name="config">The client configuration object containing the possible configuration options to initialize
    /// the <c>websocket_client</c>. </param>
    websocket_client(websocket_client_config config)
        : m_client(std::make_shared<details::websocket_client_task_impl>(std::move(config)))
    {
    }

    /// <summary>
    /// Connects to the remote network destination. The connect method initiates the websocket handshake with the
    /// remote network destination, takes care of the protocol upgrade request.
    /// </summary>
    /// <param name="uri">The uri address to connect. </param>
    /// <returns>An asynchronous operation that is completed once the client has successfully connected to the websocket
    /// server.</returns>
    pplx::task<void> connect(const web::uri& uri)
    {
        m_client->callback_client()->verify_uri(uri);
        m_client->callback_client()->set_uri(uri);
        auto client = m_client;
        return m_client->callback_client()->connect().then([client](pplx::task<void> result) {
            try
            {
                result.get();
            }
            catch (const websocket_exception& ex)
            {
                client->close_pending_tasks_with_error(ex);
                throw;
            }
        });
    }

    /// <summary>
    /// Sends a websocket message to the server .
    /// </summary>
    /// <returns>An asynchronous operation that is completed once the message is sent.</returns>
    pplx::task<void> send(websocket_outgoing_message msg) { return m_client->callback_client()->send(msg); }

    /// <summary>
    /// Receive a websocket message.
    /// </summary>
    /// <returns>An asynchronous operation that is completed when a message has been received by the client
    /// endpoint.</returns>
    pplx::task<websocket_incoming_message> receive() { return m_client->receive(); }

    /// <summary>
    /// Closes a websocket client connection, sends a close frame to the server and waits for a close message from the
    /// server.
    /// </summary>
    /// <returns>An asynchronous operation that is completed the connection has been successfully closed.</returns>
    pplx::task<void> close() { return m_client->callback_client()->close(); }

    /// <summary>
    /// Closes a websocket client connection, sends a close frame to the server and waits for a close message from the
    /// server.
    /// </summary>
    /// <param name="close_status">Endpoint MAY use the following pre-defined status codes when sending a Close
    /// frame.</param> <param name="close_reason">While closing an established connection, an endpoint may indicate the
    /// reason for closure.</param> <returns>An asynchronous operation that is completed the connection has been
    /// successfully closed.</returns>
    pplx::task<void> close(websocket_close_status close_status, const utility::string_t& close_reason = {})
    {
        return m_client->callback_client()->close(close_status, close_reason);
    }

    /// <summary>
    /// Gets the websocket client URI.
    /// </summary>
    /// <returns>URI connected to.</returns>
    const web::uri& uri() const { return m_client->callback_client()->uri(); }

    /// <summary>
    /// Gets the websocket client config object.
    /// </summary>
    /// <returns>A reference to the client configuration object.</returns>
    const websocket_client_config& config() const { return m_client->callback_client()->config(); }

private:
    std::shared_ptr<details::websocket_client_task_impl> m_client;
};

/// <summary>
/// Websocket client class, used to maintain a connection to a remote host for an extended session, uses callback APIs
/// for handling receive and close event instead of async task. For some scenarios would be a alternative for the
/// websocket_client like if you want to special handling on close event.
/// </summary>
class websocket_callback_client
{
public:
    /// <summary>
    ///  Creates a new websocket_callback_client.
    /// </summary>
    _ASYNCRTIMP websocket_callback_client();

    /// <summary>
    ///  Creates a new websocket_callback_client.
    /// </summary>
    /// <param name="client_config">The client configuration object containing the possible configuration options to
    /// initialize the <c>websocket_client</c>. </param>
    _ASYNCRTIMP websocket_callback_client(websocket_client_config client_config);

    /// <summary>
    /// Connects to the remote network destination. The connect method initiates the websocket handshake with the
    /// remote network destination, takes care of the protocol upgrade request.
    /// </summary>
    /// <param name="uri">The uri address to connect. </param>
    /// <returns>An asynchronous operation that is completed once the client has successfully connected to the websocket
    /// server.</returns>
    pplx::task<void> connect(const web::uri& uri)
    {
        m_client->verify_uri(uri);
        m_client->set_uri(uri);
        return m_client->connect();
    }

    /// <summary>
    /// Sends a websocket message to the server .
    /// </summary>
    /// <returns>An asynchronous operation that is completed once the message is sent.</returns>
    pplx::task<void> send(websocket_outgoing_message msg) { return m_client->send(msg); }

    /// <summary>
    /// Set the received handler for notification of client websocket messages.
    /// </summary>
    /// <param name="handler">A function representing the incoming websocket messages handler. It's parameters are:
    ///    msg:  a <c>websocket_incoming_message</c> value indicating the message received
    /// </param>
    /// <remarks>If this handler is not set before connecting incoming messages will be missed.</remarks>
    void set_message_handler(const std::function<void(const websocket_incoming_message& msg)>& handler)
    {
        m_client->set_message_handler(handler);
    }

    /// <summary>
    /// Closes a websocket client connection, sends a close frame to the server and waits for a close message from the
    /// server.
    /// </summary>
    /// <returns>An asynchronous operation that is completed the connection has been successfully closed.</returns>
    pplx::task<void> close() { return m_client->close(); }

    /// <summary>
    /// Closes a websocket client connection, sends a close frame to the server and waits for a close message from the
    /// server.
    /// </summary>
    /// <param name="close_status">Endpoint MAY use the following pre-defined status codes when sending a Close
    /// frame.</param> <param name="close_reason">While closing an established connection, an endpoint may indicate the
    /// reason for closure.</param> <returns>An asynchronous operation that is completed the connection has been
    /// successfully closed.</returns>
    pplx::task<void> close(websocket_close_status close_status, const utility::string_t& close_reason = {})
    {
        return m_client->close(close_status, close_reason);
    }

    /// <summary>
    /// Set the closed handler for notification of client websocket closing event.
    /// </summary>
    /// <param name="handler">The handler for websocket closing event, It's parameters are:
    ///   close_status: The pre-defined status codes used by the endpoint when sending a Close frame.
    ///   reason: The reason string used by the endpoint when sending a Close frame.
    ///   error: The error code if the websocket is closed with abnormal error.
    /// </param>
    void set_close_handler(const std::function<void(websocket_close_status close_status,
                                                    const utility::string_t& reason,
                                                    const std::error_code& error)>& handler)
    {
        m_client->set_close_handler(handler);
    }

    /// <summary>
    /// Gets the websocket client URI.
    /// </summary>
    /// <returns>URI connected to.</returns>
    const web::uri& uri() const { return m_client->uri(); }

    /// <summary>
    /// Gets the websocket client config object.
    /// </summary>
    /// <returns>A reference to the client configuration object.</returns>
    const websocket_client_config& config() const { return m_client->config(); }

private:
    std::shared_ptr<details::websocket_client_callback_impl> m_client;
};

} // namespace client
} // namespace websockets
} // namespace web

#endif

#endif
