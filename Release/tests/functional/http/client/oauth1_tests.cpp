/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Test cases for oauth1.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "cpprest/details/http_helpers.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::http::details;
using namespace web::http::oauth1::experimental;
using namespace web::http::oauth1::details;
using namespace utility;
using namespace concurrency;

using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace client
{
SUITE(oauth1_tests)
{
    struct oauth1_test_config
    {
        oauth1_test_config()
            : m_server_uri(U("http://localhost:17778/"))
            , m_test_token(U("test_token"), U("test_token_secret"))
            , m_oauth1_config(U("test_key"),
                              U("test_secret"),
                              m_server_uri,
                              m_server_uri,
                              m_server_uri,
                              m_server_uri,
                              oauth1_methods::hmac_sha1)
            , m_oauth1_handler(std::shared_ptr<oauth1_config>(new oauth1_config(m_oauth1_config)))
        {
        }

        const utility::string_t m_server_uri;
        const oauth1_token m_test_token;

        oauth1_config m_oauth1_config;
        oauth1_handler m_oauth1_handler;
    };

    struct oauth1_token_setup : public oauth1_test_config
    {
        oauth1_token_setup() { m_oauth1_config.set_token(m_test_token); }
    };

    struct oauth1_server_setup : public oauth1_test_config
    {
        oauth1_server_setup() : m_server(m_server_uri) {}

        test_http_server::scoped_server m_server;
    };

#define TEST_ACCESSOR(value_, name_)                                                                                   \
    t.set_##name_(value_);                                                                                             \
    VERIFY_ARE_EQUAL(value_, t.name_());

    TEST(oauth1_token_accessors)
    {
        oauth1_token t(U(""), U(""));
        TEST_ACCESSOR(U("a%123"), access_token)
        TEST_ACCESSOR(U("b%20456"), secret)

        const auto key1 = U("abc");
        const auto value1 = U("123");
        const auto key2 = U("xyz");
        const auto value2 = U("456");
        t.set_additional_parameter(key1, value1);
        t.set_additional_parameter(U("xyz"), U("456"));
        const auto& parameters = t.additional_parameters();
        VERIFY_ARE_EQUAL(parameters.at(key1), value1);
        VERIFY_ARE_EQUAL(parameters.at(key2), value2);
        t.clear_additional_parameters();
        VERIFY_ARE_EQUAL(0, t.additional_parameters().size());
    }

    TEST(oauth1_config_accessors)
    {
        oauth1_config t(U(""), U(""), U(""), U(""), U(""), U(""), oauth1_methods::hmac_sha1);
        TEST_ACCESSOR(U("Test123"), consumer_key)
        TEST_ACCESSOR(U("bar456"), consumer_secret)
        TEST_ACCESSOR(U("file:///123?123=a&1="), temp_endpoint)
        TEST_ACCESSOR(U("x:yxw#0"), auth_endpoint)
        TEST_ACCESSOR(U("baz:"), token_endpoint)
        TEST_ACCESSOR(U("/xyzzy=2"), callback_uri)
        TEST_ACCESSOR(oauth1_methods::plaintext, method)
        TEST_ACCESSOR(U("wally.world x"), realm)

        const auto key1 = U("abc");
        const auto value1 = U("123");
        const auto key2 = U("xyz");
        const auto value2 = U("456");
        t.add_parameter(key1, value1);
        t.add_parameter(U("xyz"), U("456"));
        const auto parameters = t.parameters();
        VERIFY_ARE_EQUAL(parameters.at(key1), value1);
        VERIFY_ARE_EQUAL(parameters.at(key2), value2);
        t.clear_parameters();
        VERIFY_ARE_EQUAL(0, t.parameters().size());
        t.set_parameters(parameters);
        const auto parameters2 = t.parameters();
        VERIFY_ARE_EQUAL(parameters2.at(key1), value1);
        VERIFY_ARE_EQUAL(parameters2.at(key2), value2);
    }

#undef TEST_ACCESSOR

    // clang-format off
    TEST_FIXTURE(oauth1_token_setup, oauth1_signature_base_string)
    {
        // Basic base string generation.
        {
            http_request r;
            r.set_method(methods::POST);
            r.set_request_uri(U("http://example.com:80/request?a=b&c=d")); // Port set to avoid default.

            auto state = m_oauth1_config._generate_auth_state();
            state.set_timestamp(U("12345678"));
            state.set_nonce(U("ABCDEFGH"));

            utility::string_t base_string = m_oauth1_config._build_signature_base_string(r, state);
            utility::string_t correct_base_string(
                U("POST&http%3A%2F%2Fexample.com%2Frequest&a%3Db%26c%3Dd%26oauth_consumer_key%3Dtest_key%26oauth_nonce%")
                U("3DABCDEFGH%26oauth_signature_method%3DHMAC-SHA1%26oauth_timestamp%3D12345678%26oauth_token%3Dtest_")
                U("token%26oauth_version%3D1.0"));
            VERIFY_ARE_EQUAL(correct_base_string, base_string);
        }

        // Added "extra_param" and proper parameter normalization.
        {
            http_request r;
            r.set_method(methods::POST);
            r.set_request_uri(U("http://example.com:80/request?a=b&c=d"));

            auto state = m_oauth1_config._generate_auth_state(U("oauth_test"), U("xyzzy"));
            state.set_timestamp(U("12345678"));
            state.set_nonce(U("ABCDEFGH"));

            utility::string_t base_string = m_oauth1_config._build_signature_base_string(r, state);
            utility::string_t correct_base_string(
                U("POST&http%3A%2F%2Fexample.com%2Frequest&a%3Db%26c%3Dd%26oauth_consumer_key%3Dtest_key%26oauth_nonce%")
                U("3DABCDEFGH%26oauth_signature_method%3DHMAC-SHA1%26oauth_test%3Dxyzzy%26oauth_timestamp%3D12345678%")
                U("26oauth_token%3Dtest_token%26oauth_version%3D1.0"));
            VERIFY_ARE_EQUAL(correct_base_string, base_string);
        }

        // Use application/x-www-form-urlencoded with parameters in body
        {
            http_request r(methods::POST);
            r.set_request_uri(U("http://example.com:80/request?a=b&c=d")); // Port set to avoid default.
            r.set_body("MyVariableOne=ValueOne&MyVariableTwo=ValueTwo", "application/x-www-form-urlencoded");

            auto state = m_oauth1_config._generate_auth_state();
            state.set_timestamp(U("12345678"));
            state.set_nonce(U("ABCDEFGH"));

            utility::string_t base_string = m_oauth1_config._build_signature_base_string(r, state);
            utility::string_t correct_base_string(
                U("POST&http%3A%2F%2Fexample.com%2Frequest&a%3Db%26c%3Dd%26MyVariableOne%3DValueOne%26%26MyVariableTwo%")
                U("3DValueTwo%26oauth_consumer_key%3Dtest_key%26oauth_nonce%3DABCDEFGH%26oauth_signature_method%3DHMAC-")
                U("SHA1%26oauth_timestamp%3D12345678%26oauth_token%3Dtest_token%26oauth_version%3D1.0"));
        }
    }

    TEST_FIXTURE(oauth1_token_setup, oauth1_hmac_sha1_method)
    {
        http_request r;
        r.set_method(methods::POST);
        r.set_request_uri(U("http://example.com:80/request?a=b&c=d")); // Port set to avoid default.

        auto state = m_oauth1_config._generate_auth_state();
        state.set_timestamp(U("12345678"));
        state.set_nonce(U("ABCDEFGH"));

        utility::string_t signature = m_oauth1_config._build_hmac_sha1_signature(r, state);

        utility::string_t correct_signature(U("iUq3VlP39UNXoJHXlKjgSTmjEs8="));
        VERIFY_ARE_EQUAL(correct_signature, signature);
    }

    TEST_FIXTURE(oauth1_token_setup, oauth1_plaintext_method)
    {
        utility::string_t signature(m_oauth1_config._build_plaintext_signature());
        utility::string_t correct_signature(U("test_secret&test_token_secret"));
        VERIFY_ARE_EQUAL(correct_signature, signature);
    }

    TEST_FIXTURE(oauth1_server_setup, oauth1_hmac_sha1_request)
    {
        m_oauth1_config.set_token(m_test_token);
        m_oauth1_config.set_method(oauth1_methods::hmac_sha1);

        http_client_config client_config;
        client_config.set_oauth1(m_oauth1_config);
        http_client client(m_server_uri, client_config);

        m_server.server()->next_request().then([](test_request* request) {
            const utility::string_t header_authorization(request->m_headers[header_names::authorization]);
            const utility::string_t prefix(
                U("OAuth oauth_version=\"1.0\", oauth_consumer_key=\"test_key\", oauth_token=\"test_token\", ")
                U("oauth_signature_method=\"HMAC-SHA1\", oauth_timestamp=\""));
            VERIFY_ARE_EQUAL(0, header_authorization.find(prefix));
            request->reply(status_codes::OK);
        });

        VERIFY_IS_TRUE(m_oauth1_config.token().is_valid_access_token());
        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
    }

    TEST_FIXTURE(oauth1_server_setup, oauth1_plaintext_request)
    {
        m_oauth1_config.set_token(m_test_token);
        m_oauth1_config.set_method(oauth1_methods::plaintext);

        http_client_config client_config;
        client_config.set_oauth1(m_oauth1_config);
        http_client client(m_server_uri, client_config);

        m_server.server()->next_request().then([](test_request* request) {
            const utility::string_t header_authorization(request->m_headers[header_names::authorization]);
            const utility::string_t prefix(
                U("OAuth oauth_version=\"1.0\", oauth_consumer_key=\"test_key\", oauth_token=\"test_token\", ")
                U("oauth_signature_method=\"PLAINTEXT\", oauth_timestamp=\""));
            VERIFY_ARE_EQUAL(0, header_authorization.find(prefix));
            request->reply(status_codes::OK);
        });

        VERIFY_IS_TRUE(m_oauth1_config.token().is_valid_access_token());
        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
    }

    TEST_FIXTURE(oauth1_server_setup, oauth1_build_authorization_uri)
    {
        m_server.server()->next_request().then([](test_request* request) {
            const utility::string_t header_authorization(request->m_headers[header_names::authorization]);

            // Verify prefix, and without 'oauth_token'.
            const utility::string_t prefix(U("OAuth oauth_version=\"1.0\", oauth_consumer_key=\"test_key\", ")
                                           U("oauth_signature_method=\"HMAC-SHA1\", oauth_timestamp=\""));
            VERIFY_ARE_EQUAL(0, header_authorization.find(prefix));

            // Verify suffix with proper 'oauth_callback'.
            const utility::string_t suffix(U(", oauth_callback=\"http%3A%2F%2Flocalhost%3A17778%2F\""));
            VERIFY_IS_TRUE(std::equal(suffix.rbegin(), suffix.rend(), header_authorization.rbegin()));

            // Reply with temporary token and secret.
            std::map<utility::string_t, utility::string_t> headers;
            headers[header_names::content_type] = mime_types::application_x_www_form_urlencoded;
            request->reply(status_codes::OK,
                           U(""),
                           headers,
                           "oauth_token=testbar&oauth_token_secret=xyzzy&oauth_callback_confirmed=true");
        });

        VERIFY_IS_FALSE(m_oauth1_config.token().is_valid_access_token());
        utility::string_t auth_uri = m_oauth1_config.build_authorization_uri().get();
        VERIFY_ARE_EQUAL(auth_uri, U("http://localhost:17778/?oauth_token=testbar"));
        VERIFY_IS_FALSE(m_oauth1_config.token().is_valid_access_token());
    }

    // NOTE: This test also covers token_from_verifier().
    TEST_FIXTURE(oauth1_server_setup, oauth1_token_from_redirected_uri)
    {
        m_server.server()->next_request().then([](test_request* request) {
            const utility::string_t header_authorization(request->m_headers[header_names::authorization]);

            // Verify temporary token prefix.
            const utility::string_t prefix(
                U("OAuth oauth_version=\"1.0\", oauth_consumer_key=\"test_key\", oauth_token=\"xyzzy\", ")
                U("oauth_signature_method=\"HMAC-SHA1\", oauth_timestamp=\""));
            VERIFY_ARE_EQUAL(0, header_authorization.find(prefix));

            // Verify suffix with 'oauth_verifier'.
            const utility::string_t suffix(U(", oauth_verifier=\"simsalabim\""));
            VERIFY_IS_TRUE(std::equal(suffix.rbegin(), suffix.rend(), header_authorization.rbegin()));

            // Verify we have 'oauth_nonce' and 'oauth_signature'.
            VERIFY_ARE_NOT_EQUAL(utility::string_t::npos, header_authorization.find(U("oauth_nonce")));
            VERIFY_ARE_NOT_EQUAL(utility::string_t::npos, header_authorization.find(U("oauth_signature")));

            // Reply with access token and secret.
            std::map<utility::string_t, utility::string_t> headers;
            headers[header_names::content_type] = mime_types::application_x_www_form_urlencoded;
            request->reply(status_codes::OK, U(""), headers, "oauth_token=test&oauth_token_secret=bar");
        });

        m_oauth1_config.set_token(oauth1_token(U("xyzzy"), U(""))); // Simulate temporary token.

        const web::http::uri redirected_uri(U("http://localhost:17778/?oauth_token=xyzzy&oauth_verifier=simsalabim"));
        m_oauth1_config.token_from_redirected_uri(redirected_uri).wait();

        VERIFY_IS_TRUE(m_oauth1_config.token().is_valid_access_token());
        VERIFY_ARE_EQUAL(m_oauth1_config.token().access_token(), U("test"));
        VERIFY_ARE_EQUAL(m_oauth1_config.token().secret(), U("bar"));
    }

    // clang-format on

} // SUITE(oauth1_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
