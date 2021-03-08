/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * close_tests.cpp
 *
 * Tests cases for closing websocket_client objects.
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
SUITE(close_tests)
{
    // Test close websocket connection: client sends an empty close and server responds with close frame
    TEST_FIXTURE(uri_address, close_client_websocket)
    {
        test_websocket_server server;

        websocket_client client;

        client.connect(m_uri).wait();

        client.close().wait();
    }

    // Test close websocket connection: client sends a close with reason and server responds with close frame
    TEST_FIXTURE(uri_address, close_with_reason)
    {
        test_websocket_server server;

        websocket_client client;

        client.connect(m_uri).wait();

        client.close(websocket_close_status::going_away, U("Client disconnecting")).wait();
    }

    // Server sends a close frame (server initiated close)
    TEST_FIXTURE(uri_address, close_from_server)
    {
        std::string body("hello");
        test_websocket_server server;

        websocket_client client;

        client.connect(m_uri).wait();

        // Send close frame from server
        test_websocket_msg msg;
        msg.set_msg_type(test_websocket_message_type::WEB_SOCKET_CLOSE_TYPE);
        server.send_msg(msg);

        client.close().wait();
    }

    // Test close websocket connection with callback client: client sends an empty close and server responds with close
    // frame
    TEST_FIXTURE(uri_address, close_callback_client_websocket, "Ignore", "319")
    {
        test_websocket_server server;
        const utility::string_t close_reason = U("Too large");

        // verify it is ok not to set close handler
        websocket_callback_client client;

        client.connect(m_uri).wait();

        client.close().wait();

        websocket_callback_client client1;

        client1.set_close_handler([&close_reason](websocket_close_status status,
                                                  const utility::string_t& reason,
                                                  const std::error_code& code) {
            VERIFY_ARE_EQUAL(status, websocket_close_status::too_large);
            VERIFY_ARE_EQUAL(reason, close_reason);
            VERIFY_ARE_EQUAL(code.value(), 0);
        });

        client1.connect(m_uri).wait();

        client1.close(websocket_close_status::too_large, close_reason).wait();
    }

    // Test close websocket connection: client sends a close with reason and server responds with close frame
    TEST_FIXTURE(uri_address, close_callback_client_with_reason, "Ignore", "319")
    {
        const utility::string_t close_reason = U("Client disconnecting");
        test_websocket_server server;

        websocket_callback_client client;

        client.set_close_handler([close_reason](websocket_close_status status,
                                                const utility::string_t& reason,
                                                const std::error_code& code) {
            VERIFY_ARE_EQUAL(status, websocket_close_status::normal);
            VERIFY_ARE_EQUAL(reason, close_reason);
            VERIFY_ARE_EQUAL(code.value(), 0);
        });

        client.connect(m_uri).wait();

        client.close(websocket_close_status::normal, close_reason).wait();
    }

    // Server sends a close frame (server initiated close)
    TEST_FIXTURE(uri_address, close_callback_client_from_server, "Ignore", "319")
    {
        std::string body("hello");
        test_websocket_server server;

        websocket_callback_client client;

        int hitCount = 0;
        pplx::task_completion_event<void> closeEvent;
        client.set_close_handler([&hitCount, closeEvent](websocket_close_status status,
                                                         const utility::string_t& reason,
                                                         const std::error_code& code) {
            VERIFY_ARE_EQUAL(status, websocket_close_status::going_away);
            VERIFY_ARE_EQUAL(reason, U(""));
            VERIFY_ARE_EQUAL(code.value(), 0);

            hitCount++;
            closeEvent.set();
        });

        client.connect(m_uri).wait();

        // Send close frame from server
        test_websocket_msg msg;
        msg.set_msg_type(test_websocket_message_type::WEB_SOCKET_CLOSE_TYPE);
        server.send_msg(msg);

        // make sure it only called once.
        pplx::create_task(closeEvent).wait();
        VERIFY_ARE_EQUAL(hitCount, 1);
    }

} // SUITE(close_tests)

} // namespace client
} // namespace websocket
} // namespace functional
} // namespace tests

#endif
