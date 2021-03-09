/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * test_websocket_server.h -- Defines a test server to handle incoming and outgoing messages.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <unittestpp.h>

#ifndef WEBSOCKET_UTILITY_API
#ifdef WEBSOCKETTESTUTILITY_EXPORTS
#define WEBSOCKET_UTILITY_API __declspec(dllexport)
#else
#define WEBSOCKET_UTILITY_API __declspec(dllimport)
#endif
#endif

#if !defined(_M_ARM) || defined(__cplusplus_winrt)

namespace tests
{
namespace functional
{
namespace websocket
{
namespace utilities
{
class _test_websocket_server;

// The different types of a websocket message.
enum test_websocket_message_type
{
    WEB_SOCKET_BINARY_MESSAGE_TYPE,
    WEB_SOCKET_BINARY_FRAGMENT_TYPE,
    WEB_SOCKET_UTF8_MESSAGE_TYPE,
    WEB_SOCKET_UTF8_FRAGMENT_TYPE,
    WEB_SOCKET_CLOSE_TYPE,
    WEB_SOCKET_PING_TYPE,
    WEB_SOCKET_PONG_TYPE
};

// Interface containing details about the HTTP handshake request received by the test server.
class test_http_request_interface
{
public:
    virtual ~test_http_request_interface() {}
    virtual const std::string& username() = 0;
    virtual const std::string& password() = 0;
    virtual const std::string& get_header_val(const std::string& header_name) = 0;
};
typedef std::unique_ptr<test_http_request_interface> test_http_request;

// Class that contains details about the HTTP handshake response to be sent by the test server
class test_http_response
{
public:
    void set_realm(std::string realm) { m_realm = std::move(realm); }
    void set_status_code(unsigned short code) { m_status_code = code; }
    const std::string& realm() const { return m_realm; }
    unsigned short status_code() const { return m_status_code; }

private:
    std::string m_realm;
    unsigned short m_status_code;
};

// Represents a websocket message at the test server.
// Contains a vector that can contain text/binary data
// and a type variable to denote the message type.
class test_websocket_msg
{
public:
    const std::vector<unsigned char>& data() const { return m_data; }
    void set_data(std::vector<unsigned char> data) { m_data = std::move(data); }

    test_websocket_message_type msg_type() const { return m_msg_type; }
    void set_msg_type(test_websocket_message_type type) { m_msg_type = type; }

private:
    std::vector<unsigned char> m_data;
    test_websocket_message_type m_msg_type;
};

class websocket_asserts
{
public:
    static void assert_message_equals(test_websocket_msg& msg,
                                      const std::string& expected_data,
                                      test_websocket_message_type expected_flag)
    {
        std::vector<unsigned char> temp_vec(expected_data.begin(), expected_data.end());
        assert_message_equals(msg, temp_vec, expected_flag);
    }

    static void assert_message_equals(test_websocket_msg& msg,
                                      const std::vector<unsigned char>& expected_data,
                                      test_websocket_message_type expected_flag)
    {
        VERIFY_ARE_EQUAL(msg.msg_type(), expected_flag);
        auto& data = msg.data();
        VERIFY_ARE_EQUAL(data.size(), expected_data.size());
        VERIFY_IS_TRUE(std::equal(expected_data.begin(), expected_data.end(), data.begin()));
    }

private:
    websocket_asserts() {}
    ~websocket_asserts() CPPREST_NOEXCEPT {}
};

// Test websocket server.
class test_websocket_server
{
public:
    WEBSOCKET_UTILITY_API test_websocket_server();

    // Tests can add a handler to handle (verify) the next message received by the server.
    // If the test plans to send n messages, n handlers must be registered.
    // The server will call the handler in order, for each incoming message.
    WEBSOCKET_UTILITY_API void next_message(std::function<void __cdecl(test_websocket_msg)> msg_handler);
    WEBSOCKET_UTILITY_API std::function<void(test_websocket_msg)> get_next_message_handler();

    // Handler for initial HTTP request.
    typedef std::function<test_http_response __cdecl(test_http_request)> http_handler;
    WEBSOCKET_UTILITY_API void set_http_handler(http_handler handler) { m_http_handler = handler; }
    WEBSOCKET_UTILITY_API http_handler get_http_handler() { return m_http_handler; }

    // Tests can use this API to send a message from the server to the client.
    WEBSOCKET_UTILITY_API void send_msg(const test_websocket_msg& msg);
    WEBSOCKET_UTILITY_API std::shared_ptr<_test_websocket_server> get_impl();

private:
#if !defined(_MSC_VER) || _MSC_VER >= 1800
    test_websocket_server(const test_websocket_server&) = delete;
    test_websocket_server& operator=(const test_websocket_server&) = delete;
    test_websocket_server(test_websocket_server&&) = delete;
    test_websocket_server& operator=(test_websocket_server&&) = delete;
#endif

    // Queue to maintain the request handlers.
    // Note: This queue is not thread-safe. Use m_handler_queue_lock to synchronize.
    std::mutex m_handler_queue_lock;
    std::queue<std::function<void(test_websocket_msg)>> m_handler_queue;
    // Handler to address the HTTP handshake request. To be used in scenarios where tests may wish to fail the HTTP
    // request and not proceed with the websocket connection.
    http_handler m_http_handler;
    std::shared_ptr<_test_websocket_server> m_p_impl;
};
} // namespace utilities
} // namespace websocket
} // namespace functional
} // namespace tests

#endif
