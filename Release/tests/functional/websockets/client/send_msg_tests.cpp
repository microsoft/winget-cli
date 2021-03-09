/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * send_msg_tests.cpp
 *
 * Tests cases for covering sending messages from websocket client.
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

#if defined(__cplusplus_winrt)
using namespace Windows::Storage;
#endif

namespace tests
{
namespace functional
{
namespace websocket
{
namespace client
{
SUITE(send_msg_tests)
{
    utility::string_t get_full_name(const utility::string_t& name)
    {
#if defined(__cplusplus_winrt)
        // On WinRT, we must compensate for the fact that we will be accessing files in the
        // Documents folder
        auto file =
            pplx::create_task(KnownFolders::DocumentsLibrary->CreateFileAsync(ref new Platform::String(name.c_str()),
                                                                              CreationCollisionOption::ReplaceExisting))
                .get();
        return file->Path->Data();
#else
        return name;
#endif
    }

    template<typename _CharType>
    pplx::task<streams::streambuf<_CharType>> OPEN_R(const utility::string_t& name)
    {
#if !defined(__cplusplus_winrt)
        return streams::file_buffer<_CharType>::open(name, std::ios_base::in);
#else
        auto file =
            pplx::create_task(KnownFolders::DocumentsLibrary->GetFileAsync(ref new Platform::String(name.c_str())))
                .get();

        return streams::file_buffer<_CharType>::open(file, std::ios_base::in);
#endif
    }

    // Used to prepare data for stream tests
    void fill_file(const utility::string_t& name, const std::vector<uint8_t>& body, size_t repetitions = 1)
    {
        std::fstream stream(get_full_name(name), std::ios_base::out | std::ios_base::trunc);

        for (size_t i = 0; i < repetitions; i++)
            stream.write((char*)&body[0], body.size());
        stream.close();
    }

    void fill_buffer(streams::streambuf<uint8_t> rbuf, const std::vector<uint8_t>& body, size_t repetitions = 1)
    {
        size_t len = body.size();
        for (size_t i = 0; i < repetitions; i++)
            rbuf.putn_nocopy((const uint8_t*)&body[0], len).wait();
    }

    template<class SocketClientClass>
    pplx::task<void> send_text_msg_helper(SocketClientClass & client,
                                          web::uri uri,
                                          test_websocket_server & server,
                                          const std::string& body,
                                          bool connect_client = true)
    {
        server.next_message([body](test_websocket_msg msg) // Handler to verify the message sent by the client.
                            {
                                websocket_asserts::assert_message_equals(
                                    msg, body, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
                            });

        if (connect_client) client.connect(uri).wait();

        websocket_outgoing_message msg;
        msg.set_utf8_message(body);
        return client.send(msg);
    }

    template<class SocketClientClass>
    pplx::task<void> send_ping_msg_helper(SocketClientClass & client, web::uri uri, test_websocket_server & server,
                                          const std::string& body = "")
    {
        server.next_message(
            [body](test_websocket_msg msg) // Handler to verify the message sent by the client.
            { websocket_asserts::assert_message_equals(msg, body, test_websocket_message_type::WEB_SOCKET_PING_TYPE); });

        client.connect(uri).wait();

        websocket_outgoing_message msg;
        msg.set_ping_message(body);
        return client.send(msg);
    }

    template<class SocketClientClass>
    pplx::task<void> send_pong_msg_helper(SocketClientClass & client, web::uri uri, test_websocket_server & server,
                                          const std::string& body = "")
    {
        server.next_message(
            [body](test_websocket_msg msg) // Handler to verify the message sent by the client.
            { websocket_asserts::assert_message_equals(msg, body, test_websocket_message_type::WEB_SOCKET_PONG_TYPE); });

        client.connect(uri).wait();

        websocket_outgoing_message msg;
        msg.set_pong_message(body);
        return client.send(msg);
    }

    pplx::task<void> send_msg_from_stream(websocket_client & client,
                                          test_websocket_server & server,
                                          web::uri uri,
                                          const std::vector<uint8_t>& body,
                                          streams::streambuf<uint8_t> buf,
                                          test_websocket_message_type type,
                                          bool fill_data,
                                          bool connect_client = true)
    {
        server.next_message(
            [body, type](test_websocket_msg msg) { websocket_asserts::assert_message_equals(msg, body, type); });

        if (connect_client) client.connect(uri).wait();
        if (fill_data) fill_buffer(buf, body);

        websocket_outgoing_message msg;
        if (type == test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE)
            msg.set_utf8_message(streams::istream(buf), body.size());
        else if (type == test_websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE)
            msg.set_binary_message(streams::istream(buf), body.size());

        return client.send(msg);
    }

    // Send message from input stream -> data is already populated in the stream buffer
    pplx::task<void> send_msg_from_istream_helper(websocket_client & client,
                                                  test_websocket_server & server,
                                                  web::uri uri,
                                                  const std::vector<uint8_t>& body,
                                                  streams::streambuf<uint8_t> rbuf,
                                                  test_websocket_message_type type,
                                                  bool connect_client = true)
    {
        return send_msg_from_stream(client, server, uri, body, rbuf, type, false, connect_client);
    }

    pplx::task<void> send_msg_from_stream_helper(websocket_client & client,
                                                 test_websocket_server & server,
                                                 web::uri uri,
                                                 const std::vector<uint8_t>& body,
                                                 streams::streambuf<uint8_t> rbuf,
                                                 test_websocket_message_type type,
                                                 bool connect_client = true)
    {
        return send_msg_from_stream(client, server, uri, body, rbuf, type, true, connect_client);
    }

    // Send text message (no fragmentation)
    TEST_FIXTURE(uri_address, send_text_msg)
    {
        test_websocket_server server;
        websocket_client client;
        send_text_msg_helper(client, m_uri, server, "hello").wait();
        client.close().wait();
    }

    // Send text message with websocket_callback_client
    TEST_FIXTURE(uri_address, send_text_msg_callback_client)
    {
        test_websocket_server server;
        websocket_callback_client client;
        send_text_msg_helper(client, m_uri, server, "hello").wait();
        client.close().wait();
    }

    // Send text message (no fragmentation)
    // Test the stream interface to send data
    TEST_FIXTURE(uri_address, send_text_msg_stream)
    {
        test_websocket_server server;
        streams::producer_consumer_buffer<uint8_t> rbuf;
        std::vector<uint8_t> body(26);
        memcpy(&body[0], "abcdefghijklmnopqrstuvwxyz", 26);

        websocket_client client;
        send_msg_from_stream_helper(
            client, server, m_uri, body, rbuf, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE)
            .wait();

        rbuf.close(std::ios::out).wait();
        client.close().wait();
    }

    // Send Binary message (no fragmentation)
    TEST_FIXTURE(uri_address, send_binary_msg)
    {
        test_websocket_server server;
        streams::producer_consumer_buffer<uint8_t> rbuf;
        std::vector<uint8_t> body(6);
        memcpy(&body[0], "a\0b\0c\0", 6);

        websocket_client client;

        send_msg_from_stream_helper(
            client, server, m_uri, body, rbuf, test_websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE)
            .wait();
        rbuf.close(std::ios::out);
        client.close().wait();
    }

    // Send empty text message
    // WinRT client does not handle empty messages. Verify websocket_exception is thrown.
    TEST_FIXTURE(uri_address, send_empty_text_msg)
    {
        test_websocket_server server;
        websocket_client client;

        client.connect(m_uri).wait();

        websocket_outgoing_message msg;
        msg.set_utf8_message("");
        VERIFY_THROWS(client.send(msg).wait(), websocket_exception);

        client.close().wait();
    }

    // Send multiple text messages
    TEST_FIXTURE(uri_address, send_multiple_text_msges)
    {
        test_websocket_server server;
        websocket_client client;

        send_text_msg_helper(client, m_uri, server, "hello1").wait();
        send_text_msg_helper(client, m_uri, server, "hello2", false).wait();

        client.close().wait();
    }

    // Send multiple text messages
    TEST_FIXTURE(uri_address, send_multiple_text_msges_async)
    {
        test_websocket_server server;
        websocket_client client;

        auto t1 = send_text_msg_helper(client, m_uri, server, "hello1");
        auto t2 = send_text_msg_helper(client, m_uri, server, "hello2", false);

        t2.wait();
        t1.wait();
        client.close().wait();
    }

    // Send multiple text messages from a stream
    TEST_FIXTURE(uri_address, send_multiple_text_msges_stream)
    {
        test_websocket_server server;
        streams::producer_consumer_buffer<uint8_t> rbuf;
        std::vector<uint8_t> body1(26);
        memcpy(&body1[0], "abcdefghijklmnopqrstuvwxyz", 26);
        std::vector<uint8_t> body2(26);
        memcpy(&body2[0], "zyxwvutsrqponmlkjihgfedcba", 26);

        websocket_client client;

        auto t1 = send_msg_from_stream_helper(
            client, server, m_uri, body1, rbuf, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        auto t2 = send_msg_from_stream_helper(
            client, server, m_uri, body2, rbuf, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE, false);

        t1.wait();
        t2.wait();
        client.close().wait();
    }

    // Send multiple text messages from a file stream
    // send uses stream::acquire API, acquire will fail for file streams.
    TEST_FIXTURE(uri_address, send_text_msges_fstream)
    {
        test_websocket_server server;
        utility::string_t fname = U("send_multiple_text_msges_fstream.txt");
        std::vector<uint8_t> body1(26);
        memcpy(&body1[0], "abcdefghijklmnopqrstuvwxyz", 26);
        fill_file(fname, body1, 2);
        auto file_buf = OPEN_R<uint8_t>(fname).get();
        websocket_client client;

        auto t1 = send_msg_from_istream_helper(
            client, server, m_uri, body1, file_buf, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        auto t2 = send_msg_from_istream_helper(
            client, server, m_uri, body1, file_buf, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE, false);

        t1.wait();
        t2.wait();
        client.close().wait();
    }

    // Send multiple text messages from a container stream, where container stream has more data than what we want to
    // send in a single message
    TEST_FIXTURE(uri_address, send_text_msges_cstream)
    {
        test_websocket_server server;
        std::vector<uint8_t> body(26);
        memcpy(&body[0], "abcdefghijklmnopqrstuvwxyz", 26);
        auto cbuf = streams::container_stream<std::vector<uint8_t>>::open_istream(body).streambuf();

        websocket_client client;

        auto t1 = send_msg_from_istream_helper(client,
                                               server,
                                               m_uri,
                                               std::vector<uint8_t>(body.begin(), body.begin() + body.size() / 2),
                                               cbuf,
                                               test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        auto t2 = send_msg_from_istream_helper(client,
                                               server,
                                               m_uri,
                                               std::vector<uint8_t>(body.begin() + body.size() / 2, body.end()),
                                               cbuf,
                                               test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE,
                                               false);

        t1.wait();
        t2.wait();
        client.close().wait();
    }

    // Send multiple text messages from a producer consumer stream, where stream initially has less data than what we
    // want to send in a single message Write data to the buffer after initiating the send, send should succeed.
    TEST_FIXTURE(uri_address, send_text_msges_pcstream_lessdata)
    {
        test_websocket_server server;
        streams::producer_consumer_buffer<uint8_t> rbuf;
        std::vector<uint8_t> body(26);
        memcpy(&body[0], "abcdefghijklmnopqrstuvwxyz", 26);
        fill_buffer(rbuf, body);

        server.next_message([](test_websocket_msg msg) {
            websocket_asserts::assert_message_equals(
                msg, "abcdefghijklmnopqrstuvwxyzabcd", test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        });

        websocket_client client;
        client.connect(m_uri).wait();
        websocket_outgoing_message msg;
        msg.set_utf8_message(rbuf.create_istream(), 30);
        auto t1 = client.send(msg);

        fill_buffer(rbuf, body);
        t1.wait();
        client.close().wait();
    }

    // Send multiple text messages from a container stream, where stream has less data than what we want to send in a
    // single message Since container stream does not support in | out simultaneously, websocket send_msg will fail to
    // read the required number of bytes and throws an exception.
    TEST_FIXTURE(uri_address, send_text_msges_cstream_lessdata)
    {
        test_websocket_server server;
        std::vector<uint8_t> body(26);
        memcpy(&body[0], "abcdefghijklmnopqrstuvwxyz", 26);
        auto cbuf = streams::container_stream<std::vector<uint8_t>>::open_istream(body).streambuf();

        server.next_message([](test_websocket_msg msg) {
            websocket_asserts::assert_message_equals(
                msg, "abcdefghijklmnopqrstuvwxyzabcd", test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
        });

        websocket_client client;
        client.connect(m_uri).wait();
        websocket_outgoing_message msg;
        msg.set_utf8_message(cbuf.create_istream(), 30);

        VERIFY_THROWS(client.send(msg).wait(), websocket_exception);
        client.close().wait();
    }

    // Send multiple binary messages from the same stream
    TEST_FIXTURE(uri_address, send_multiple_binary_msg_same_stream)
    {
        test_websocket_server server;
        streams::producer_consumer_buffer<uint8_t> rbuf;
        std::vector<uint8_t> body1(6);
        memcpy(&body1[0], "a\0b\0c\0", 6);
        std::vector<uint8_t> body2(6);
        memcpy(&body2[0], "a\0b\0c\0", 6);

        websocket_client client;

        auto t1 = send_msg_from_stream_helper(
            client, server, m_uri, body1, rbuf, test_websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE);
        auto t2 = send_msg_from_stream_helper(
            client, server, m_uri, body2, rbuf, test_websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE, false);

        t1.wait();
        t2.wait();
        rbuf.close(std::ios_base::out);
        client.close().wait();
    }

    // Send text message followed by binary message
    TEST_FIXTURE(uri_address, send_text_and_binary)
    {
        test_websocket_server server;
        streams::producer_consumer_buffer<uint8_t> rbuf;
        std::vector<uint8_t> body2(6);
        memcpy(&body2[0], "a\0b\0c\0", 6);

        websocket_client client;

        send_text_msg_helper(client, m_uri, server, "hello1").wait();
        send_msg_from_stream_helper(
            client, server, m_uri, body2, rbuf, test_websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE, false)
            .wait();

        rbuf.close(std::ios::out).wait();
        client.close().wait();
    }

    // Send a multi byte UTF-8 text message
    TEST_FIXTURE(uri_address, send_multi_byte_utf8_msg)
    {
        test_websocket_server server;
        std::string body = "\xC3\xA0\xC3\xB8";
        websocket_client client;

        send_text_msg_helper(client, m_uri, server, body).wait();
        client.close().wait();
    }

    // Send a streamed text message without specifying length
    TEST_FIXTURE(uri_address, send_stream_utf8_msg_no_length)
    {
        test_websocket_server server;

        std::string body = "\xC3\xA0\xC3\xB8";
        std::vector<uint8_t> msgbuf(body.begin(), body.end());

        auto is = streams::container_stream<std::vector<uint8_t>>::open_istream(std::move(msgbuf));

        websocket_client client;
        {
            server.next_message([body](test_websocket_msg msg) // Handler to verify the message sent by the client.
                                {
                                    websocket_asserts::assert_message_equals(
                                        msg, body, test_websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE);
                                });

            client.connect(m_uri).wait();

            websocket_outgoing_message msg;
            msg.set_utf8_message(is);
            client.send(msg).wait();
        }

        client.close().wait();
    }

    // Send a streamed binary message without specifying length
    TEST_FIXTURE(uri_address, send_stream_binary_msg_no_length)
    {
        test_websocket_server server;

        std::string body = "\x00\x01\x02\x00";
        std::vector<uint8_t> msgbuf(body.begin(), body.end());

        auto is = streams::container_stream<std::vector<uint8_t>>::open_istream(std::move(msgbuf));

        websocket_client client;
        {
            server.next_message([body](test_websocket_msg msg) // Handler to verify the message sent by the client.
                                {
                                    websocket_asserts::assert_message_equals(
                                        msg, body, test_websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE);
                                });

            client.connect(m_uri).wait();

            websocket_outgoing_message msg;
            msg.set_binary_message(is);
            client.send(msg).wait();
        }

        client.close().wait();
    }

#if !defined(__cplusplus_winrt)
    // Send a ping message to the server
    TEST_FIXTURE(uri_address, send_ping_msg)
    {
        test_websocket_server server;
        websocket_client client;
        send_ping_msg_helper(client, m_uri, server).wait();
        client.close().wait();
    }

    // Send a ping message to the server with a body
    TEST_FIXTURE(uri_address, send_ping_msg_body)
    {
        test_websocket_server server;
        websocket_client client;
        send_ping_msg_helper(client, m_uri, server, "abcdefghijklmnopqrstuvwxyz").wait();
        client.close().wait();
    }

    // Send an unsolicited pong message to the server
    TEST_FIXTURE(uri_address, send_pong_msg)
    {
        test_websocket_server server;
        websocket_client client;
        send_pong_msg_helper(client, m_uri, server).wait();
        client.close().wait();
    }

    // Send an unsolicited pong message to the server with a body
    TEST_FIXTURE(uri_address, send_pong_msg_body)
    {
        test_websocket_server server;
        websocket_client client;
        send_pong_msg_helper(client, m_uri, server, "abcdefghijklmnopqrstuvwxyz").wait();
        client.close().wait();
    }

    // Send an unsolicited pong message to the server with websocket_callback_client
    TEST_FIXTURE(uri_address, send_pong_msg_callback_client)
    {
        test_websocket_server server;
        websocket_callback_client client;
        send_pong_msg_helper(client, m_uri, server).wait();
        client.close().wait();
    }
#endif

} // SUITE(send_msg_tests)

} // namespace client
} // namespace websocket
} // namespace functional
} // namespace tests

#endif
