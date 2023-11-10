/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Websocket library: Client-side APIs.
 *
 * This file contains the websocket message implementation
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#include "cpprest/ws_msg.h"

#include "../../http/common/internal_http_helpers.h"
#include "cpprest/ws_client.h"
#include <sstream>

#if !defined(CPPREST_EXCLUDE_WEBSOCKETS)

using namespace concurrency;
using namespace concurrency::streams::details;

namespace web
{
namespace websockets
{
namespace client
{
static ::utility::string_t g_subProtocolHeader = _XPLATSTR("Sec-WebSocket-Protocol");

void websocket_client_config::set_user_agent(const utf8string& user_agent)
{
    headers().add(web::http::header_names::user_agent, utility::conversions::to_string_t(user_agent));
}

void websocket_client_config::add_subprotocol(const ::utility::string_t& name)
{
    m_headers.add(g_subProtocolHeader, name);
}

std::vector<::utility::string_t> websocket_client_config::subprotocols() const
{
    std::vector<::utility::string_t> values;
    auto subprotocolHeader = m_headers.find(g_subProtocolHeader);
    if (subprotocolHeader != m_headers.end())
    {
        utility::istringstream_t header(subprotocolHeader->second);
        utility::string_t token;
        while (std::getline(header, token, U(',')))
        {
            http::details::trim_whitespace(token);
            if (!token.empty())
            {
                values.push_back(token);
            }
        }
    }
    return values;
}

pplx::task<std::string> websocket_incoming_message::extract_string() const
{
    if (m_msg_type == websocket_message_type::binary_message)
    {
        return pplx::task_from_exception<std::string>(websocket_exception("Invalid message type"));
    }
    return pplx::task_from_result(std::move(m_body.collection()));
}

} // namespace client
} // namespace websockets
} // namespace web

#endif
