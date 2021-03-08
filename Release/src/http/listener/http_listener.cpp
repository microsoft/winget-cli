/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: HTTP listener (server-side) APIs
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if !defined(_WIN32) || (_WIN32_WINNT >= _WIN32_WINNT_VISTA && !defined(__cplusplus_winrt)) ||                         \
    defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)

using namespace web::http::experimental;

namespace web
{
namespace http
{
namespace experimental
{
namespace listener
{
// Helper function to check URI components.
static void check_listener_uri(const http::uri& address)
{
    // Some things like proper URI schema are verified by the URI class.
    // We only need to check certain things specific to HTTP.

    // HTTP Server API includes SSL support
    if (address.scheme() != U("http") && address.scheme() != U("https"))
    {
        throw std::invalid_argument("URI scheme must be 'http' or 'https'");
    }

    if (address.host().empty())
    {
        throw std::invalid_argument("URI must contain a hostname.");
    }

    if (!address.query().empty())
    {
        throw std::invalid_argument("URI can't contain a query.");
    }

    if (!address.fragment().empty())
    {
        throw std::invalid_argument("URI can't contain a fragment.");
    }
}

details::http_listener_impl::http_listener_impl(http::uri address) : m_uri(std::move(address)), m_closed(true)
{
    check_listener_uri(m_uri);
}

details::http_listener_impl::http_listener_impl(http::uri address, http_listener_config config)
    : m_uri(std::move(address)), m_config(std::move(config)), m_closed(true)
{
    check_listener_uri(m_uri);
}

pplx::task<void> details::http_listener_impl::open()
{
    // Do nothing if the open operation was already attempted
    // Not thread safe
    if (!m_closed) return pplx::task_from_result();

    if (m_uri.is_empty()) throw std::invalid_argument("No URI defined for listener.");
    m_closed = false;

    return web::http::experimental::details::http_server_api::register_listener(this).then(
        [this](pplx::task<void> openOp) {
            try
            {
                // If failed to open need to mark as closed.
                openOp.wait();
            }
            catch (...)
            {
                m_closed = true;
                throw;
            }
            return openOp;
        });
}

pplx::task<void> details::http_listener_impl::close()
{
    // Do nothing if the close operation was already attempted
    // Not thread safe.
    // Note: Return the previous close task
    if (m_closed) return m_close_task;

    m_closed = true;
    m_close_task = web::http::experimental::details::http_server_api::unregister_listener(this);
    return m_close_task;
}

void details::http_listener_impl::handle_request(http_request msg)
{
    // Specific method handler takes priority over general.
    const method& mtd = msg.method();
    if (m_supported_methods.count(mtd))
    {
        m_supported_methods[mtd](msg);
    }
    else if (mtd == methods::OPTIONS)
    {
        handle_options(msg);
    }
    else if (mtd == methods::TRCE)
    {
        handle_trace(msg);
    }
    else if (m_all_requests != nullptr)
    {
        m_all_requests(msg);
    }
    else
    {
        // Method is not supported.
        // Send back a list of supported methods to the client.
        http_response response(status_codes::MethodNotAllowed);
        response.headers().add(U("Allow"), get_supported_methods());
        msg.reply(response);
    }
}

utility::string_t details::http_listener_impl::get_supported_methods() const
{
    utility::string_t allowed;
    bool first = true;
    for (auto iter = m_supported_methods.begin(); iter != m_supported_methods.end(); ++iter)
    {
        if (!first)
        {
            allowed += U(", ");
        }
        else
        {
            first = false;
        }
        allowed += (iter->first);
    }
    return allowed;
}

void details::http_listener_impl::handle_trace(http_request message)
{
    utility::string_t data = message.to_string();
    message.reply(status_codes::OK, data, U("message/http"));
}

void details::http_listener_impl::handle_options(http_request message)
{
    http_response response(status_codes::OK);
    response.headers().add(U("Allow"), get_supported_methods());
    message.reply(response);
}

} // namespace listener
} // namespace experimental
} // namespace http
} // namespace web

#endif
