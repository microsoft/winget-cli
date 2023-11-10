/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: interface to implement HTTP server to service http_listeners.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
#error "Error: http server APIs are not supported in XP"
#endif //_WIN32_WINNT < _WIN32_WINNT_VISTA

#include "cpprest/http_listener.h"

namespace web
{
namespace http
{
namespace experimental
{
namespace details
{
/// <summary>
/// Interface http listeners interact with for receiving and responding to http requests.
/// </summary>
class http_server
{
public:
    /// <summary>
    /// Release any held resources.
    /// </summary>
    virtual ~http_server() {};

    /// <summary>
    /// Start listening for incoming requests.
    /// </summary>
    virtual pplx::task<void> start() = 0;

    /// <summary>
    /// Registers an http listener.
    /// </summary>
    virtual pplx::task<void> register_listener(
        _In_ web::http::experimental::listener::details::http_listener_impl* pListener) = 0;

    /// <summary>
    /// Unregisters an http listener.
    /// </summary>
    virtual pplx::task<void> unregister_listener(
        _In_ web::http::experimental::listener::details::http_listener_impl* pListener) = 0;

    /// <summary>
    /// Stop processing and listening for incoming requests.
    /// </summary>
    virtual pplx::task<void> stop() = 0;

    /// <summary>
    /// Asynchronously sends the specified http response.
    /// </summary>
    /// <param name="response">The http_response to send.</param>
    /// <returns>A operation which is completed once the response has been sent.</returns>
    virtual pplx::task<void> respond(http::http_response response) = 0;
};

} // namespace details
} // namespace experimental
} // namespace http
} // namespace web
