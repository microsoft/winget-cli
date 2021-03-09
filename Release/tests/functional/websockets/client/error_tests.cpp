/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests cases error connection cases with websocket_client.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if defined(__cplusplus_winrt) || !defined(_M_ARM)

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
SUITE(error_tests)
{
    // Send before connecting
    TEST_FIXTURE(uri_address, send_before_connect)
    {
        websocket_client client;

        websocket_outgoing_message msg;
        msg.set_utf8_message("xyz");

        VERIFY_THROWS(client.send(msg).wait(), websocket_exception);
    }

    // Server does not exist
    TEST_FIXTURE(uri_address, server_doesnt_exist)
    {
        websocket_client client;
        VERIFY_THROWS(client.connect(m_uri).get(), websocket_exception);
    }

// Send after close
// CodePlex 319 fails on VS2013.
#if !defined(_MSC_VER) || _MSC_VER >= 1900
    TEST_FIXTURE(uri_address, send_after_close)
    {
        std::string body("hello");
        test_websocket_server server;

        server.next_message([&](test_websocket_msg msg) {
            websocket_asserts::assert_message_equals(
                msg, body, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        });
        websocket_client client;

        client.connect(m_uri).wait();
        client.close().wait();

        websocket_outgoing_message msg;
        msg.set_utf8_message(body);
        VERIFY_THROWS(client.send(msg).wait(), websocket_exception);
    }
#endif

    // Send after close for callback client
    TEST_FIXTURE(uri_address, send_after_close_callback_client, "Ignore", "319")
    {
        std::string body("hello");
        test_websocket_server server;

        server.next_message([&](test_websocket_msg msg) {
            websocket_asserts::assert_message_equals(
                msg, body, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        });
        websocket_callback_client client;

        client.connect(m_uri).wait();
        client.close().wait();

        websocket_outgoing_message msg;
        msg.set_utf8_message(body);
        VERIFY_THROWS(client.send(msg).wait(), websocket_exception);
    }

    // Receive after close
    TEST_FIXTURE(uri_address, receive_after_close)
    {
        test_websocket_server server;
        websocket_client client;
        client.connect(m_uri).wait();
        auto t = client.receive();
        client.close().wait();
        VERIFY_THROWS(t.wait(), websocket_exception);
    }

    // Start receive task after client has closed
    TEST_FIXTURE(uri_address, try_receive_after_close)
    {
        test_websocket_server server;
        websocket_client client;
        client.connect(m_uri).wait();
        client.close().wait();
        auto t = client.receive();
        VERIFY_THROWS(t.wait(), websocket_exception);
    }

    // Start the receive task after server has sent a close frame
    TEST_FIXTURE(uri_address, try_receive_after_server_initiated_close)
    {
        test_websocket_server server;
        websocket_client client;
        client.connect(m_uri).wait();

        // Send close frame from server
        test_websocket_msg msg;
        msg.set_msg_type(test_websocket_message_type::WEB_SOCKET_CLOSE_TYPE);
        server.send_msg(msg);

        // 100 ms should be plenty for local loopback
        std::chrono::milliseconds dura(100);
        std::this_thread::sleep_for(dura);

        auto t = client.receive();
        VERIFY_THROWS(t.wait(), websocket_exception);

        client.close().wait();
    }

    // Destroy the client without closing it explicitly
    TEST_FIXTURE(uri_address, destroy_without_close)
    {
        test_websocket_server server;
        websocket_client client;
        client.connect(m_uri).wait();
    }

    // Destroy the callback client without closing it explicitly
    TEST_FIXTURE(uri_address, destroy_without_close_callback_client)
    {
        // test won't finish if we can't release client properly
        test_websocket_server server;
        websocket_callback_client client;
        client.connect(m_uri).wait();
    }

    // connect fails while user is waiting on receive
    TEST_FIXTURE(uri_address, connect_fail_with_receive)
    {
        websocket_client client;
        auto t = client.receive();

        VERIFY_THROWS(client.connect(U("ws://localhost:9981/ws")).get(), websocket_exception);
        VERIFY_THROWS(t.get(), websocket_exception);
    }

} // SUITE(error_tests)

} // namespace client
} // namespace websocket
} // namespace functional
} // namespace tests

#endif
