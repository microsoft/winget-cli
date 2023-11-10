/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Websocket library: Client-side APIs.
 *
 * This file contains the implementation for the Windows Runtime based on
 *Windows::Networking::Sockets::MessageWebSocket.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#include <concrt.h>

#if !defined(CPPREST_EXCLUDE_WEBSOCKETS)

#include "ws_client_impl.h"

using namespace ::Windows::Foundation;
using namespace ::Windows::Storage;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::Networking;
using namespace ::Windows::Networking::Sockets;
using namespace Concurrency::streams::details;

namespace web
{
namespace websockets
{
namespace client
{
namespace details
{
// Helper function to build an error string from a Platform::Exception and a location.
static std::string build_error_msg(Platform::Exception ^ exc, const std::string& location)
{
    std::string msg(location);
    msg.append(": ");
    msg.append(std::to_string(exc->HResult));
    msg.append(": ");
    msg.append(utility::conversions::utf16_to_utf8(exc->Message->Data()));
    return msg;
}

// This class is required by the implementation in order to function:
// The TypedEventHandler requires the message received and close handler to be a member of WinRT class.
ref class ReceiveContext sealed
{
public:
    ReceiveContext() {}

    friend class winrt_callback_client;
    friend class winrt_task_client;
    void OnReceive(MessageWebSocket ^ sender, MessageWebSocketMessageReceivedEventArgs ^ args);
    void OnClosed(IWebSocket ^ sender, WebSocketClosedEventArgs ^ args);

private:
    // Public members cannot have native types
    ReceiveContext(
        std::function<void(const websocket_incoming_message&)> receive_handler,
        std::function<void(websocket_close_status, const utility::string_t&, const std::error_code&)> close_handler)
        : m_receive_handler(std::move(receive_handler)), m_close_handler(std::move(close_handler))
    {
    }

    // Handler to be executed when a message has been received by the client
    std::function<void(const websocket_incoming_message&)> m_receive_handler;

    // Handler to be executed when a close message has been received by the client
    std::function<void(websocket_close_status, const utility::string_t&, const std::error_code&)> m_close_handler;
};

class winrt_callback_client : public websocket_client_callback_impl,
                              public std::enable_shared_from_this<winrt_callback_client>
{
public:
    winrt_callback_client(websocket_client_config config)
        : websocket_client_callback_impl(std::move(config)), m_connected(false)
    {
        m_msg_websocket = ref new MessageWebSocket();

        // Sets the HTTP request headers to the HTTP request message used in the WebSocket protocol handshake
        const utility::string_t protocolHeader(_XPLATSTR("Sec-WebSocket-Protocol"));
        const auto& headers = m_config.headers();
        for (const auto& header : headers)
        {
            // Unfortunately the MessageWebSocket API throws a COMException if you try to set the
            // 'Sec-WebSocket-Protocol' header here. It requires you to go through their API instead.
            if (!utility::details::str_iequal(header.first, protocolHeader))
            {
                m_msg_websocket->SetRequestHeader(Platform::StringReference(header.first.c_str()),
                                                  Platform::StringReference(header.second.c_str()));
            }
        }

        // Add any specified subprotocols.
        if (headers.has(protocolHeader))
        {
            const std::vector<utility::string_t> protocols = m_config.subprotocols();
            for (const auto& value : protocols)
            {
                m_msg_websocket->Control->SupportedProtocols->Append(Platform::StringReference(value.c_str()));
            }
        }

        if (m_config.credentials().is_set())
        {
            auto password = m_config.credentials()._internal_decrypt();
            m_msg_websocket->Control->ServerCredential = ref new Windows::Security::Credentials::PasswordCredential(
                "WebSocketClientCredentialResource",
                Platform::StringReference(m_config.credentials().username().c_str()),
                Platform::StringReference(password->c_str()));
        }

        m_context = ref new ReceiveContext(
            [=](const websocket_incoming_message& msg) {
                if (m_external_message_handler)
                {
                    m_external_message_handler(msg);
                }
            },
            [=](websocket_close_status status, const utility::string_t& reason, const std::error_code& error_code) {
                if (m_external_close_handler)
                {
                    m_external_close_handler(status, reason, error_code);
                }

                // Locally copy the task completion event since there is a PPL bug
                // that the set method accesses internal state in the event and the websocket
                // client could be destroyed.
                auto local_close_tce = m_close_tce;
                local_close_tce.set();
            });
    }

    ~winrt_callback_client()
    {
        // Only call close if successfully connected.
        if (m_connected)
        {
            // Users should have already called close and wait on the returned task
            // before destroying the client. In case they didn't we call close and wait for
            // it to complete. It is safe to call MessageWebSocket::Close multiple times and
            // concurrently, it has safe guards in place to only execute once.
            close().wait();
        }
    }

    pplx::task<void> connect()
    {
        _ASSERTE(!m_connected);
        const auto& proxy = m_config.proxy();
        if (!proxy.is_default())
        {
            return pplx::task_from_exception<void>(websocket_exception("Only a default proxy server is supported."));
        }

        const auto& proxy_cred = proxy.credentials();
        if (proxy_cred.is_set())
        {
            auto password = proxy_cred._internal_decrypt();
            m_msg_websocket->Control->ProxyCredential = ref new Windows::Security::Credentials::PasswordCredential(
                "WebSocketClientProxyCredentialResource",
                Platform::StringReference(proxy_cred.username().c_str()),
                Platform::StringReference(password->c_str()));
        }

        const auto uri = ref new Windows::Foundation::Uri(Platform::StringReference(m_uri.to_string().c_str()));

        m_msg_websocket->MessageReceived +=
            ref new TypedEventHandler<MessageWebSocket ^, MessageWebSocketMessageReceivedEventArgs ^>(
                m_context, &ReceiveContext::OnReceive);
        m_msg_websocket->Closed +=
            ref new TypedEventHandler<IWebSocket ^, WebSocketClosedEventArgs ^>(m_context, &ReceiveContext::OnClosed);

        std::weak_ptr<winrt_callback_client> thisWeakPtr = shared_from_this();
        return pplx::create_task(m_msg_websocket->ConnectAsync(uri))
            .then([thisWeakPtr](pplx::task<void> result) -> pplx::task<void> {
                // result.get() should happen before anything else, to make sure there is no unobserved exception
                // in the task chain.
                try
                {
                    result.get();
                }
                catch (Platform::Exception ^ e)
                {
                    throw websocket_exception(e->HResult, build_error_msg(e, "ConnectAsync"));
                }

                if (auto pThis = thisWeakPtr.lock())
                {
                    try
                    {
                        pThis->m_messageWriter = ref new DataWriter(pThis->m_msg_websocket->OutputStream);
                    }
                    catch (Platform::Exception ^ e)
                    {
                        throw websocket_exception(e->HResult, build_error_msg(e, "ConnectAsync"));
                    }
                    pThis->m_connected = true;
                }
                else
                {
                    return pplx::task_from_exception<void>(websocket_exception("Websocket client is being destroyed"));
                }

                return pplx::task_from_result();
            });
    }

    pplx::task<void> send(websocket_outgoing_message& msg)
    {
        if (m_messageWriter == nullptr)
        {
            return pplx::task_from_exception<void>(websocket_exception("Client not connected."));
        }

        switch (msg.m_msg_type)
        {
            case websocket_message_type::binary_message:
                m_msg_websocket->Control->MessageType = SocketMessageType::Binary;
                break;
            case websocket_message_type::text_message:
                m_msg_websocket->Control->MessageType = SocketMessageType::Utf8;
                break;
            default: return pplx::task_from_exception<void>(websocket_exception("Message Type not supported."));
        }

        const auto length = msg.m_length;
        if (length == 0)
        {
            return pplx::task_from_exception<void>(websocket_exception("Cannot send empty message."));
        }
        if (length >= UINT_MAX && length != SIZE_MAX)
        {
            return pplx::task_from_exception<void>(
                websocket_exception("Message size too large. Ensure message length is less than UINT_MAX."));
        }

        auto msg_pending = m_out_queue.push(msg);

        // No sends in progress
        if (msg_pending == outgoing_msg_queue::state::was_empty)
        {
            // Start sending the message
            send_msg(msg);
        }

        return pplx::create_task(msg.body_sent());
    }

    void send_msg(websocket_outgoing_message& msg)
    {
        auto this_client = this->shared_from_this();
        auto& is_buf = msg.m_body;
        auto length = msg.m_length;

        if (length == SIZE_MAX)
        {
            // This indicates we should determine the length automatically.
            if (is_buf.has_size())
            {
                // The user's stream knows how large it is -- there's no need to buffer.
                auto buf_sz = is_buf.size();
                if (buf_sz >= SIZE_MAX)
                {
                    msg.signal_body_sent(
                        std::make_exception_ptr(websocket_exception("Cannot send messages larger than SIZE_MAX.")));
                    return;
                }
                length = static_cast<size_t>(buf_sz);
                // We have determined the length and can proceed normally.
            }
            else
            {
                // The stream needs to be buffered.
                auto is_buf_istream = is_buf.create_istream();
                msg.m_body = concurrency::streams::container_buffer<std::vector<uint8_t>>();
                is_buf_istream.read_to_end(msg.m_body).then([this_client, msg](pplx::task<size_t> t) mutable {
                    try
                    {
                        msg.m_length = t.get();
                        this_client->send_msg(msg);
                    }
                    catch (...)
                    {
                        msg.signal_body_sent(std::current_exception());
                    }
                });
                // We have postponed the call to send_msg() until after the data is buffered.
                return;
            }
        }

        // First try to acquire the data (Get a pointer to the next already allocated contiguous block of data)
        // If acquire succeeds, send the data over the socket connection, there is no copy of data from stream to
        // temporary buffer. If acquire fails, copy the data to a temporary buffer managed by sp_allocated and send it
        // over the socket connection.
        std::shared_ptr<uint8_t> sp_allocated;
        size_t acquired_size = 0;
        uint8_t* ptr;
        auto read_task = pplx::task_from_result();
        bool acquired = is_buf.acquire(ptr, acquired_size);

        if (!acquired ||
            acquired_size < length) // Stream does not support acquire or failed to acquire specified number of bytes
        {
            // If acquire did not return the required number of bytes, do not rely on its return value.
            if (acquired_size < length)
            {
                acquired = false;
                is_buf.release(ptr, 0);
            }

            // Allocate buffer to hold the data to be read from the stream.
            sp_allocated.reset(new uint8_t[length], [=](uint8_t* p) { delete[] p; });

            read_task = is_buf.getn(sp_allocated.get(), length).then([length](size_t bytes_read) {
                if (bytes_read != length)
                {
                    throw websocket_exception("Failed to read required length of data from the stream.");
                }
            });
        }
        else
        {
            // Acquire succeeded, assign the acquired pointer to sp_allocated. Use an empty custom destructor
            // so that the data is not released when sp_allocated goes out of scope. The streambuf will manage its
            // memory.
            sp_allocated.reset(ptr, [](uint8_t*) {});
        }

        read_task
            .then([this_client, acquired, sp_allocated, length]() {
                this_client->m_messageWriter->WriteBytes(
                    Platform::ArrayReference<unsigned char>(sp_allocated.get(), static_cast<unsigned int>(length)));

                // Send the data as one complete message, in WinRT we do not have an option to send fragments.
                return pplx::task<unsigned int>(this_client->m_messageWriter->StoreAsync());
            })
            .then([this_client, msg, is_buf, acquired, sp_allocated, length](
                      pplx::task<unsigned int> previousTask) mutable {
                std::exception_ptr eptr;
                unsigned int bytes_written = 0;
                try
                {
                    // Catch exceptions from previous tasks, if any and convert it to websocket exception.
                    bytes_written = previousTask.get();
                    if (bytes_written != length)
                    {
                        eptr = std::make_exception_ptr(websocket_exception("Failed to send all the bytes."));
                    }
                }
                catch (Platform::Exception ^ e)
                {
                    // Convert to websocket_exception.
                    eptr = std::make_exception_ptr(websocket_exception(e->HResult, build_error_msg(e, "send_msg")));
                }
                catch (const websocket_exception& e)
                {
                    // Catch to avoid slicing and losing the type if falling through to catch (...).
                    eptr = std::make_exception_ptr(e);
                }
                catch (...)
                {
                    eptr = std::make_exception_ptr(std::current_exception());
                }

                if (acquired)
                {
                    is_buf.release(sp_allocated.get(), bytes_written);
                }

                // Set the send_task_completion_event after calling release.
                if (eptr)
                {
                    msg.signal_body_sent(eptr);
                }
                else
                {
                    msg.signal_body_sent();
                }

                websocket_outgoing_message next_msg;
                bool msg_pending = this_client->m_out_queue.pop_and_peek(next_msg);
                if (msg_pending)
                {
                    this_client->send_msg(next_msg);
                }
            });
    }

    void set_message_handler(const std::function<void(const websocket_incoming_message&)>& handler)
    {
        m_external_message_handler = handler;
    }

    pplx::task<void> close()
    {
        // Send a close frame to the server
        return close(websocket_close_status::normal, _XPLATSTR("Normal"));
    }

    pplx::task<void> close(websocket_close_status status, const utility::string_t& strreason = {})
    {
        // Send a close frame to the server
        m_msg_websocket->Close(static_cast<unsigned short>(status), Platform::StringReference(strreason.c_str()));
        // Wait for the close response frame from the server.
        return pplx::create_task(m_close_tce);
    }

    void set_close_handler(
        const std::function<void(websocket_close_status, const utility::string_t&, const std::error_code&)>& handler)
    {
        m_external_close_handler = handler;
    }

private:
    // WinRT MessageWebSocket object
    Windows::Networking::Sockets::MessageWebSocket ^ m_msg_websocket;
    Windows::Storage::Streams::DataWriter ^ m_messageWriter;
    // Context object that implements the WinRT handlers: receive handler and close handler
    ReceiveContext ^ m_context;

    pplx::task_completion_event<void> m_close_tce;

    // Tracks whether or not the websocket client successfully connected to the server.
    std::atomic<bool> m_connected;

    // External callback for handling received and close event
    std::function<void(websocket_incoming_message)> m_external_message_handler;
    std::function<void(websocket_close_status, const utility::string_t&, const std::error_code&)>
        m_external_close_handler;

    // Queue to track pending sends
    outgoing_msg_queue m_out_queue;
};

void ReceiveContext::OnReceive(MessageWebSocket ^ sender, MessageWebSocketMessageReceivedEventArgs ^ args)
{
    websocket_incoming_message incoming_msg;

    switch (args->MessageType)
    {
        case SocketMessageType::Binary: incoming_msg.m_msg_type = websocket_message_type::binary_message; break;
        case SocketMessageType::Utf8: incoming_msg.m_msg_type = websocket_message_type::text_message; break;
    }

    try
    {
        DataReader ^ reader = args->GetDataReader();
        const auto len = reader->UnconsumedBufferLength;
        if (len > 0)
        {
            std::string payload;
            payload.resize(len);
            reader->ReadBytes(Platform::ArrayReference<uint8_t>(reinterpret_cast<uint8*>(&payload[0]), len));
            incoming_msg.m_body = concurrency::streams::container_buffer<std::string>(std::move(payload));
        }
        m_receive_handler(incoming_msg);
    }
    catch (Platform::Exception ^ e)
    {
        m_close_handler(websocket_close_status::abnormal_close,
                        _XPLATSTR("Abnormal close"),
                        utility::details::create_error_code(e->HResult));
    }
}

void ReceiveContext::OnClosed(IWebSocket ^ sender, WebSocketClosedEventArgs ^ args)
{
    m_close_handler(
        static_cast<websocket_close_status>(args->Code), args->Reason->Data(), utility::details::create_error_code(0));
}

websocket_client_task_impl::websocket_client_task_impl(websocket_client_config config)
    : m_callback_client(std::make_shared<details::winrt_callback_client>(std::move(config))), m_client_closed(false)
{
    set_handler();
}
} // namespace details

websocket_callback_client::websocket_callback_client()
    : m_client(std::make_shared<details::winrt_callback_client>(websocket_client_config()))
{
}

websocket_callback_client::websocket_callback_client(websocket_client_config config)
    : m_client(std::make_shared<details::winrt_callback_client>(std::move(config)))
{
}

} // namespace client
} // namespace websockets
} // namespace web
#endif
