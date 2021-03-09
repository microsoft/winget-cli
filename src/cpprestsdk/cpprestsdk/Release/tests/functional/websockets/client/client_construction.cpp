/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * client_construction.cpp
 *
 * Tests cases for covering creating websocket_clients.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if defined(__cplusplus_winrt) || !defined(_M_ARM)

using namespace concurrency::streams;

using namespace web;
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
SUITE(client_construction)
{
    // Helper function verifies that when constructing a websocket_client with invalid
    // URI std::invalid_argument is thrown.
    static void verify_client_invalid_argument(const uri& address)
    {
        try
        {
            websocket_client client;
            client.connect(address).wait();
            VERIFY_IS_TRUE(false);
        }
        catch (std::invalid_argument&)
        {
            // expected
        }
    }

    TEST_FIXTURE(uri_address, client_construction_error_cases)
    {
        uri address(U("notws://localhost:34567/"));

        // Invalid scheme.
        verify_client_invalid_argument(address);

        // empty host.
        address = uri(U("ws://:34567/"));
        verify_client_invalid_argument(address);
    }

    // Verify that we can read the config from the websocket_client
    TEST_FIXTURE(uri_address, get_client_config)
    {
        websocket_client_config config;

        web::credentials cred(U("username"), U("password"));
        config.set_credentials(cred);
        websocket_client client(config);

        const websocket_client_config& config2 = client.config();
        VERIFY_ARE_EQUAL(config2.credentials().username(), cred.username());
    }

    // Verify that we can read the config from the websocket_callback_client
    TEST_FIXTURE(uri_address, get_client_config_callback_client)
    {
        websocket_client_config config;

        web::credentials cred(U("username"), U("password"));
        config.set_credentials(cred);
        websocket_callback_client client(config);

        const websocket_client_config& config2 = client.config();
        VERIFY_ARE_EQUAL(config2.credentials().username(), cred.username());
    }

    // Verify that we can get the baseuri from websocket_client connect.
    TEST_FIXTURE(uri_address, uri_test)
    {
        websocket_client client1;
        VERIFY_ARE_EQUAL(client1.uri(), U("/"));

        test_websocket_server server;
        client1.connect(m_uri).wait();
        VERIFY_ARE_EQUAL(client1.uri(), m_uri);
        client1.close().wait();

        websocket_client_config config;
        websocket_client client2(config);
        VERIFY_ARE_EQUAL(client2.uri(), U("/"));

        client2.connect(m_uri).wait();
        VERIFY_ARE_EQUAL(client2.uri(), m_uri);
        client2.close().wait();
    }

    TEST_FIXTURE(uri_address, move_operations)
    {
        std::string body("hello");
        std::vector<unsigned char> body_vec(body.begin(), body.end());

        test_websocket_server server;
        websocket_client client;

        client.connect(m_uri).wait();

        // Move constructor
        websocket_client client2 = std::move(client);

        server.next_message([&](test_websocket_msg msg) // Handler to verify the message sent by the client.
                            {
                                websocket_asserts::assert_message_equals(
                                    msg, body, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
                            });

        websocket_outgoing_message msg;
        msg.set_utf8_message(body);
        client2.send(std::move(msg)).wait();

        auto t = client2.receive().then([&](websocket_incoming_message ret_msg) {
            VERIFY_ARE_EQUAL(ret_msg.length(), body.length());
            auto ret_str = ret_msg.extract_string().get();

            VERIFY_ARE_EQUAL(body.compare(ret_str), 0);
            VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);
        });

        test_websocket_msg rmsg;
        rmsg.set_data(body_vec);
        rmsg.set_msg_type(test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        server.send_msg(rmsg);
        t.wait();

        // Move assignment
        client = std::move(client2);
        server.next_message([&](test_websocket_msg msg) // Handler to verify the message sent by the client.
                            {
                                websocket_asserts::assert_message_equals(
                                    msg, body, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
                            });

        websocket_outgoing_message msg1;
        msg1.set_utf8_message(body);
        client.send(std::move(msg1)).wait();

        test_websocket_msg rmsg1;
        rmsg1.set_data(body_vec);
        rmsg1.set_msg_type(test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        server.send_msg(rmsg1);
        auto t1 = client.receive().then([&](websocket_incoming_message ret_msg) {
            VERIFY_ARE_EQUAL(ret_msg.length(), body.length());
            auto ret_str = ret_msg.extract_string().get();

            VERIFY_ARE_EQUAL(body.compare(ret_str), 0);
            VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);
        });
        t1.wait();
        client.close().wait();
    }

    void header_test_impl(const uri& address,
                          const utility::string_t& headerName,
                          const utility::string_t& headerValue,
                          const utility::string_t& expectedHeaderValue = U(""))
    {
        test_websocket_server server;
        websocket_client_config config;
        utility::string_t expectedValue = headerValue;
        if (!expectedHeaderValue.empty())
        {
            expectedValue = expectedHeaderValue;
        }
        config.headers().add(headerName, headerValue);
        websocket_client client(config);

        server.set_http_handler([&](test_http_request request) {
            test_http_response resp;
            if (request->get_header_val(utility::conversions::to_utf8string(headerName))
                    .compare(utility::conversions::to_utf8string(expectedValue)) == 0)
                resp.set_status_code(200); // Handshake request will be completed only if header match succeeds.
            else
                resp.set_status_code(400); // Else fail the handshake, websocket client connect will fail in this case.
            return resp;
        });
        client.connect(address).wait();
        client.close().wait();
    }

    TEST_FIXTURE(uri_address, connect_with_headers)
    {
        header_test_impl(m_uri, U("HeaderTest"), U("ConnectSuccessfully"));
    }

    TEST_FIXTURE(uri_address, manually_set_protocol_header)
    {
        utility::string_t headerName(U("Sec-WebSocket-Protocol"));
        header_test_impl(m_uri, headerName, U("myprotocol"));
        header_test_impl(m_uri, headerName, U("myprotocol2,"), U("myprotocol2"));
        header_test_impl(m_uri, headerName, U("myprotocol2,protocol3"), U("myprotocol2, protocol3"));
        header_test_impl(
            m_uri, headerName, U("myprotocol2, protocol3, protocol6,,"), U("myprotocol2, protocol3, protocol6"));
    }

    TEST_FIXTURE(uri_address, set_subprotocol)
    {
        test_websocket_server server;
        websocket_client_config config;

        utility::string_t expected1(U("pro1"));
        config.add_subprotocol(expected1);
        VERIFY_ARE_EQUAL(1, config.subprotocols().size());
        VERIFY_ARE_EQUAL(expected1, config.subprotocols()[0]);

        utility::string_t expected2(U("second"));
        config.add_subprotocol(expected2);
        VERIFY_ARE_EQUAL(2, config.subprotocols().size());
        VERIFY_ARE_EQUAL(expected1, config.subprotocols()[0]);
        VERIFY_ARE_EQUAL(expected2, config.subprotocols()[1]);

        websocket_client client(config);
        server.set_http_handler([&](test_http_request request) {
            test_http_response resp;
            if (request->get_header_val(utility::conversions::to_utf8string(U("Sec-WebSocket-Protocol")))
                    .compare(utility::conversions::to_utf8string(expected1 + U(", ") + expected2)) == 0)
                resp.set_status_code(200); // Handshake request will be completed only if header match succeeds.
            else
                resp.set_status_code(400); // Else fail the handshake, websocket client connect will fail in this case.
            return resp;
        });

        client.connect(m_uri).wait();
        client.close().wait();
    }

} // SUITE(client_construction)

} // namespace client
} // namespace websocket
} // namespace functional
} // namespace tests

#endif
