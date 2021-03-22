/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Portions common to both WinRT and Websocket++ implementations.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "cpprest/ws_client.h"

#if !defined(CPPREST_EXCLUDE_WEBSOCKETS)

namespace web
{
namespace websockets
{
namespace client
{
namespace details
{
websocket_client_task_impl::~websocket_client_task_impl() CPPREST_NOEXCEPT
{
    close_pending_tasks_with_error(websocket_exception("Websocket client is being destroyed"));
}

void websocket_client_task_impl::set_handler()
{
    m_callback_client->set_message_handler([=](const websocket_incoming_message& msg) {
        pplx::task_completion_event<websocket_incoming_message>
            tce; // This will be set if there are any tasks waiting to receive a message
        {
            std::lock_guard<std::mutex> lock(m_receive_queue_lock);
            if (m_receive_task_queue.empty()) // Push message to the queue as no one is waiting to receive
            {
                m_receive_msg_queue.push(msg);
                return;
            }
            else // There are tasks waiting to receive a message.
            {
                tce = m_receive_task_queue.front();
                m_receive_task_queue.pop();
            }
        }
        // Setting the tce outside the receive lock for better performance
        tce.set(msg);
    });

    m_callback_client->set_close_handler(
        [=](websocket_close_status, const utility::string_t& reason, const std::error_code& error_code) {
            close_pending_tasks_with_error(websocket_exception(error_code, reason));
        });
}

void websocket_client_task_impl::close_pending_tasks_with_error(const websocket_exception& exc)
{
    std::lock_guard<std::mutex> lock(m_receive_queue_lock);
    m_client_closed = true;
    while (!m_receive_task_queue.empty()) // There are tasks waiting to receive a message, signal them
    {
        auto tce = m_receive_task_queue.front();
        m_receive_task_queue.pop();
        tce.set_exception(std::make_exception_ptr(exc));
    }
}

pplx::task<websocket_incoming_message> websocket_client_task_impl::receive()
{
    std::lock_guard<std::mutex> lock(m_receive_queue_lock);
    if (m_client_closed == true)
    {
        return pplx::task_from_exception<websocket_incoming_message>(
            std::make_exception_ptr(websocket_exception("Websocket connection has closed.")));
    }

    if (m_receive_msg_queue
            .empty()) // Push task completion event to the tce queue, so that it gets signaled when we have a message.
    {
        pplx::task_completion_event<websocket_incoming_message> tce;
        m_receive_task_queue.push(tce);
        return pplx::create_task(tce);
    }
    else // Receive message queue is not empty, return a message from the queue.
    {
        auto msg = m_receive_msg_queue.front();
        m_receive_msg_queue.pop();
        return pplx::task_from_result<websocket_incoming_message>(msg);
    }
}

} // namespace details
} // namespace client
} // namespace websockets
} // namespace web

#endif
