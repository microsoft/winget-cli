/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * receive_msg_tests.cpp
 *
 * Test cases covering receiving messages from websocket server.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if defined(__cplusplus_winrt) || !defined(_M_ARM)

using namespace concurrency;
using namespace concurrency::streams;

using namespace web::websockets;
using namespace web::websockets::client;

using namespace tests::functional::websocket::utilities;

namespace tests
{
namespace functional
{
namespace websocket
{
namespace client
{
SUITE(receive_msg_tests)
{
    pplx::task<void> receive_text_msg_helper(websocket_client & client,
                                             test_websocket_server & server,
                                             web::uri uri,
                                             const std::string& body_str,
                                             bool connect_client = true)
    {
        std::vector<unsigned char> body(body_str.begin(), body_str.end());

        if (connect_client) client.connect(uri).wait();

        auto t = client.receive().then([body_str](websocket_incoming_message ret_msg) {
            VERIFY_ARE_EQUAL(ret_msg.length(), body_str.length());
            auto ret_str = ret_msg.extract_string().get();

            VERIFY_ARE_EQUAL(body_str.compare(ret_str), 0);
            VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);
        });

        test_websocket_msg msg;
        msg.set_data(std::move(body));
        msg.set_msg_type(test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        server.send_msg(msg);

        return t;
    }

    pplx::task<void> receive_msg_stream_helper(websocket_client & client,
                                               test_websocket_server & server,
                                               web::uri uri,
                                               const std::vector<unsigned char>& body,
                                               test_websocket_message_type type,
                                               bool connect_client = true)
    {
        if (connect_client) client.connect(uri).wait();

        auto t = client.receive().then([body, type](websocket_incoming_message ret_msg) {
            auto is = ret_msg.body();
            streams::container_buffer<std::vector<uint8_t>> ret_data;
            is.read_to_end(ret_data).wait();

            VERIFY_ARE_EQUAL(ret_msg.length(), body.size());
            VERIFY_ARE_EQUAL(body, ret_data.collection());
            if (type == test_websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE)
                VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::binary_message);
            else if (type == test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE)
                VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);
        });

        test_websocket_msg msg;
        msg.set_data(std::move(body));
        msg.set_msg_type(type);
        server.send_msg(msg);

        return t;
    }

    // Receive text message (no fragmentation)
    TEST_FIXTURE(uri_address, receive_text_msg)
    {
        test_websocket_server server;
        websocket_client client;

        receive_text_msg_helper(client, server, m_uri, "hello").wait();
        client.close().wait();
    }

    // Receive text message (no fragmentation)
    // Test the stream interface to read data
    TEST_FIXTURE(uri_address, receive_text_msg_stream)
    {
        std::string body_str("hello");
        std::vector<unsigned char> body(body_str.begin(), body_str.end());
        test_websocket_server server;
        websocket_client client;

        receive_msg_stream_helper(
            client, server, m_uri, body, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE)
            .wait();
        client.close().wait();
    }

    // Receive binary message (no fragmentation)
    TEST_FIXTURE(uri_address, receive_binary_msg)
    {
        std::vector<uint8_t> body;
        body.resize(6);
        memcpy(&body[0], "a\0b\0c\0", 6);

        test_websocket_server server;

        websocket_client client;

        receive_msg_stream_helper(
            client, server, m_uri, body, test_websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE)
            .wait();
        client.close().wait();
    }

    // Server sends text message fragmented in 2 fragments
    TEST_FIXTURE(uri_address, receive_text_msg_fragments, "Ignore", "898451")
    {
        std::string body_str("hello");
        std::vector<unsigned char> body(body_str.begin(), body_str.end());
        test_websocket_server server;

        websocket_client client;

        client.connect(m_uri).wait();

        auto t = client.receive().then([&](websocket_incoming_message ret_msg) {
            auto ret_str = ret_msg.extract_string().get();

            VERIFY_ARE_EQUAL(body_str.compare(ret_str), 0);
            VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);
        });

        test_websocket_msg msg1;
        msg1.set_data(std::move(body));
        msg1.set_msg_type(test_websocket_message_type::WEB_SOCKET_UTF8_FRAGMENT_TYPE);
        server.send_msg(msg1);

        test_websocket_msg msg2;
        msg2.set_data(std::move(body));
        msg2.set_msg_type(test_websocket_message_type::WEB_SOCKET_UTF8_FRAGMENT_TYPE);
        server.send_msg(msg2);

        t.wait();
        client.close().wait();
    }

    // Server sends message of length 0
    TEST_FIXTURE(uri_address, receive_zero_length_msg)
    {
        test_websocket_server server;
        websocket_client client;

        receive_text_msg_helper(client, server, m_uri, "").wait();

        client.close().wait();
    }

    // Receive UTF-8 string with special characters
    TEST_FIXTURE(uri_address, receive_multi_byte_utf8_msg)
    {
        std::string body_str = "\xC3\xA0\xC3\xB8";
        test_websocket_server server;
        websocket_client client;

        receive_text_msg_helper(client, server, m_uri, body_str).wait();

        client.close().wait();
    }

    // Receive multiple messages
    TEST_FIXTURE(uri_address, receive_multiple_msges)
    {
        test_websocket_server server;
        websocket_client client;

        auto t1 = receive_text_msg_helper(client, server, m_uri, "hello1");
        auto t2 = receive_text_msg_helper(client, server, m_uri, "hello2", false);

        t1.wait();
        t2.wait();

        client.close().wait();
    }

    // Start the receive task after the server has sent a message
    TEST_FIXTURE(uri_address, receive_after_server_send)
    {
        std::string body_str("hello");
        std::vector<unsigned char> body(body_str.begin(), body_str.end());

        test_websocket_server server;

        websocket_client client;

        client.connect(m_uri).wait();

        test_websocket_msg msg;
        msg.set_data(std::move(body));
        msg.set_msg_type(test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        server.send_msg(msg);

        // We dont have a way of knowing if the message has been received by our client.
        // Hence Sleep for 100 msecs and then initiate the receive
        std::chrono::milliseconds dura(100);
        std::this_thread::sleep_for(dura);

        client.receive()
            .then([&](websocket_incoming_message ret_msg) {
                auto ret_str = ret_msg.extract_string().get();
                VERIFY_ARE_EQUAL(body_str.compare(ret_str), 0);
                VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);
            })
            .wait();

        client.close().wait();
    }

    // Start task to receive text message before connecting.
    TEST_FIXTURE(uri_address, receive_before_connect)
    {
        test_websocket_server server;
        websocket_client client;

        std::string body_str("hello");
        std::vector<unsigned char> body(body_str.begin(), body_str.end());

        auto t = client.receive().then([body_str](websocket_incoming_message ret_msg) {
            VERIFY_ARE_EQUAL(ret_msg.length(), body_str.length());
            auto ret_str = ret_msg.extract_string().get();

            VERIFY_ARE_EQUAL(body_str.compare(ret_str), 0);
            VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);
        });

        // Connect after the client is waiting on a receive task.
        client.connect(m_uri).wait();

        // Now send the message from the server
        test_websocket_msg msg;
        msg.set_data(std::move(body));
        msg.set_msg_type(test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        server.send_msg(msg);

        t.wait();
        client.close().wait();
    }

    // Receive message using callback APIs
    TEST_FIXTURE(uri_address, receive_text_msg_callback_client)
    {
        test_websocket_server server;
        websocket_callback_client client;

        client.connect(m_uri).wait();
        std::string body_str("hello");
        std::vector<unsigned char> body(body_str.begin(), body_str.end());

        pplx::task_completion_event<void> receiveEvent;
        // make sure client works fine without setting receive handler
        test_websocket_msg msg;
        msg.set_data(std::move(body));
        msg.set_msg_type(test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        server.send_msg(msg);

        // set receive handler
        client.set_message_handler([body_str, &receiveEvent](websocket_incoming_message ret_msg) {
            VERIFY_ARE_EQUAL(ret_msg.length(), body_str.length());
            auto ret_str = ret_msg.extract_string().get();

            VERIFY_ARE_EQUAL(body_str.compare(ret_str), 0);
            VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);

            receiveEvent.set();
        });

        server.send_msg(msg);

        pplx::create_task(receiveEvent).wait();
        client.close().wait();
    }
} // SUITE(receive_msg_tests)

} // namespace client
} // namespace websocket
} // namespace functional
} // namespace tests

#endif
