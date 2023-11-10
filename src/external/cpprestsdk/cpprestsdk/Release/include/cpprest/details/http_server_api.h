/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: exposes the entry points to the http server transport apis.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
#error "Error: http server APIs are not supported in XP"
#endif //_WIN32_WINNT < _WIN32_WINNT_VISTA

#include "cpprest/http_listener.h"
#include <memory>

namespace web
{
namespace http
{
namespace experimental
{
namespace details
{
class http_server;

/// <summary>
/// Singleton class used to register for http requests and send responses.
///
/// The lifetime is tied to http listener registration. When the first listener registers an instance is created
/// and when the last one unregisters the receiver stops and is destroyed. It can be started back up again if
/// listeners are again registered.
/// </summary>
class http_server_api
{
public:
    /// <summary>
    /// Returns whether or not any listeners are registered.
    /// </summary>
    static bool __cdecl has_listener();

    /// <summary>
    /// Registers a HTTP server API.
    /// </summary>
    static void __cdecl register_server_api(std::unique_ptr<http_server> server_api);

    /// <summary>
    /// Clears the http server API.
    /// </summary>
    static void __cdecl unregister_server_api();

    /// <summary>
    /// Registers a listener for HTTP requests and starts receiving.
    /// </summary>
    static pplx::task<void> __cdecl register_listener(
        _In_ web::http::experimental::listener::details::http_listener_impl* pListener);

    /// <summary>
    /// Unregisters the given listener and stops listening for HTTP requests.
    /// </summary>
    static pplx::task<void> __cdecl unregister_listener(
        _In_ web::http::experimental::listener::details::http_listener_impl* pListener);

    /// <summary>
    /// Gets static HTTP server API. Could be null if no registered listeners.
    /// </summary>
    static http_server* __cdecl server_api();

private:
    /// Used to lock access to the server api registration
    static pplx::extensibility::critical_section_t s_lock;

    /// Registers a server API set -- this assumes the lock has already been taken
    static void unsafe_register_server_api(std::unique_ptr<http_server> server_api);

    // Static instance of the HTTP server API.
    static std::unique_ptr<http_server> s_server_api;

    /// Number of registered listeners;
    static pplx::details::atomic_long s_registrations;

    // Static only class. No creation.
    http_server_api();
};

} // namespace details
} // namespace experimental
} // namespace http
} // namespace web
