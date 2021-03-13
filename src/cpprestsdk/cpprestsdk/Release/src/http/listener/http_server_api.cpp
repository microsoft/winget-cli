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

#include "stdafx.h"

#if !defined(_WIN32) || (_WIN32_WINNT >= _WIN32_WINNT_VISTA && !defined(__cplusplus_winrt)) ||                         \
    defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
#include "http_server_impl.h"

using namespace web;
using namespace utility;
using namespace web::http::experimental::listener;

namespace web
{
namespace http
{
namespace experimental
{
namespace details
{
pplx::extensibility::critical_section_t http_server_api::s_lock;

std::unique_ptr<http_server> http_server_api::s_server_api((http_server*)nullptr);

pplx::details::atomic_long http_server_api::s_registrations(0L);

bool http_server_api::has_listener() { return s_registrations > 0L; }

void http_server_api::register_server_api(std::unique_ptr<http_server> server_api)
{
    pplx::extensibility::scoped_critical_section_t lock(s_lock);
    http_server_api::unsafe_register_server_api(std::move(server_api));
}

void http_server_api::unregister_server_api()
{
    pplx::extensibility::scoped_critical_section_t lock(s_lock);

    if (http_server_api::has_listener())
    {
        throw http_exception(_XPLATSTR("Server API was cleared while listeners were still attached"));
    }

    s_server_api.reset();
}

void http_server_api::unsafe_register_server_api(std::unique_ptr<http_server> server_api)
{
    // we assume that the lock has been taken here.
    if (http_server_api::has_listener())
    {
        throw http_exception(_XPLATSTR("Current server API instance has listeners attached."));
    }

    s_server_api.swap(server_api);
}

pplx::task<void> http_server_api::register_listener(
    _In_ web::http::experimental::listener::details::http_listener_impl* listener)
{
    return pplx::create_task([listener]() {
        pplx::extensibility::scoped_critical_section_t lock(s_lock);

        // the server API was not initialized, register a default
        if (s_server_api == nullptr)
        {
#if defined(_WIN32) && !defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
            auto server_api = make_http_httpsys_server();
#else
            auto server_api = make_http_asio_server();
#endif
            http_server_api::unsafe_register_server_api(std::move(server_api));

            _ASSERTE(s_server_api != nullptr);
        }

        std::exception_ptr except;
        try
        {
            // start the server if necessary
            if (pplx::details::atomic_increment(s_registrations) == 1L)
            {
                s_server_api->start().wait();
            }

            // register listener
            s_server_api->register_listener(listener).wait();
        }
        catch (...)
        {
            except = std::current_exception();
        }

        // Registration failed, need to decrement registration count.
        if (except != nullptr)
        {
            if (pplx::details::atomic_decrement(s_registrations) == 0L)
            {
                try
                {
                    server_api()->stop().wait();
                    http_server_api::unsafe_register_server_api(nullptr);
                }
                catch (...)
                {
                    // ignore this exception since we want to report the original one
                }
            }
            std::rethrow_exception(except);
        }
    });
}

pplx::task<void> http_server_api::unregister_listener(
    _In_ web::http::experimental::listener::details::http_listener_impl* pListener)
{
    return pplx::create_task([pListener]() {
        pplx::extensibility::scoped_critical_section_t lock(s_lock);

        // unregister listener
        std::exception_ptr except;
        try
        {
            server_api()->unregister_listener(pListener).wait();
        }
        catch (...)
        {
            except = std::current_exception();
        }

        // stop server if necessary
        try
        {
            if (pplx::details::atomic_decrement(s_registrations) == 0L)
            {
                server_api()->stop().wait();
                http_server_api::unsafe_register_server_api(nullptr);
            }
        }
        catch (...)
        {
            // save the original exception from unregister listener
            if (except == nullptr)
            {
                except = std::current_exception();
            }
        }

        // rethrow exception if one occurred
        if (except != nullptr)
        {
            std::rethrow_exception(except);
        }
    });
}

http_server* http_server_api::server_api() { return s_server_api.get(); }

} // namespace details
} // namespace experimental
} // namespace http
} // namespace web

#endif
