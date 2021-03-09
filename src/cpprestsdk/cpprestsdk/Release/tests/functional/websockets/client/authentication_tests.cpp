/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * authentication_tests.cpp
 *
 * Tests cases for covering authentication using websocket_client
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if defined(__cplusplus_winrt) || !defined(_M_ARM)

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
SUITE(authentication_tests)
{
// Authorization not implemented in non WinRT websocket_client yet - CodePlex 254
#if defined(__cplusplus_winrt)
    void auth_helper(test_websocket_server & server,
                     const utility::string_t& username = U(""),
                     const utility::string_t& password = U(""))
    {
        server.set_http_handler([username, password](test_http_request request) {
            test_http_response resp;
            if (request->username().empty()) // No credentials -> challenge the request
            {
                resp.set_status_code(401); // Unauthorized.
                resp.set_realm("My Realm");
            }
            else if (request->username().compare(utility::conversions::to_utf8string(username)) ||
                     request->password().compare(utility::conversions::to_utf8string(password)))
            {
                resp.set_status_code(403); // User name/password did not match: Forbidden - auth failure.
            }
            else
            {
                resp.set_status_code(200); // User name and passwords match. Successful auth.
            }
            return resp;
        });
    }

    // connect without credentials, when the server expects credentials
    TEST_FIXTURE(uri_address, auth_no_credentials, "Ignore", "245")
    {
        test_websocket_server server;
        websocket_client client;
        auth_helper(server);
        VERIFY_THROWS(client.connect(m_uri).wait(), websocket_exception);
    }

    // Connect with credentials
    TEST_FIXTURE(uri_address, auth_with_credentials, "Ignore", "245")
    {
        test_websocket_server server;
        websocket_client_config config;
        web::credentials cred(U("user"), U("password"));
        config.set_credentials(cred);
        websocket_client client(config);

        auth_helper(server, cred.username(), U("password"));
        client.connect(m_uri).wait();
        client.close().wait();
    }
#endif

    // helper function to check if failure is due to timeout.
    bool is_timeout(const std::string& msg)
    {
        if (msg.find("set_fail_handler") != std::string::npos)
        {
            if (msg.find("handshake timed out") != std::string::npos || msg.find("Timer Expired") != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    TEST(ssl_test)
    {
        websocket_client client;
        std::string body_str("hello");

        try
        {
            client.connect(U("wss://echo.websocket.org/")).wait();
            auto receive_task = client.receive().then([body_str](websocket_incoming_message ret_msg) {
                VERIFY_ARE_EQUAL(ret_msg.length(), body_str.length());
                auto ret_str = ret_msg.extract_string().get();

                VERIFY_ARE_EQUAL(body_str.compare(ret_str), 0);
                VERIFY_ARE_EQUAL(ret_msg.message_type(), websocket_message_type::text_message);
            });

            websocket_outgoing_message msg;
            msg.set_utf8_message(body_str);
            client.send(msg).wait();

            receive_task.wait();
            client.close().wait();
        }
        catch (const websocket_exception& e)
        {
            if (is_timeout(e.what()))
            {
                // Since this test depends on an outside server sometimes it sporadically can fail due to timeouts
                // especially on our build machines.
                return;
            }
            throw;
        }
    }

    void handshake_error_test_impl(const ::utility::string_t& host)
    {
        websocket_client client;
        try
        {
            client.connect(host).wait();
            VERIFY_IS_TRUE(false);
        }
        catch (const websocket_exception& e)
        {
            if (is_timeout(e.what()))
            {
                // Since this test depends on an outside server sometimes it sporadically can fail due to timeouts
                // especially on our build machines.
                return;
            }
            VERIFY_ARE_EQUAL("TLS handshake failed", e.error_code().message());
        }
    }

    TEST(self_signed_cert) { handshake_error_test_impl(U("wss://self-signed.badssl.com/")); }

    TEST(hostname_mismatch) { handshake_error_test_impl(U("wss://wrong.host.badssl.com/")); }

    TEST(cert_expired) { handshake_error_test_impl(U("wss://expired.badssl.com/")); }

} // SUITE(authentication_tests)

} // namespace client
} // namespace websocket
} // namespace functional
} // namespace tests

#endif
