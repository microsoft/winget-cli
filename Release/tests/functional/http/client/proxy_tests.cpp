/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * proxy_tests.cpp
 *
 * Tests cases for using proxies with http_clients.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

using namespace web::http;
using namespace web::http::client;

using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace client
{
// In order to run this test, replace this proxy uri with one that you have access to.
static const auto proxy_uri = U("http://netproxy.redmond.corp.microsoft.com");

SUITE(proxy_tests)
{
    TEST_FIXTURE(uri_address, web_proxy_uri)
    {
        uri u(proxy_uri);

        web_proxy uri_proxy(u);
        VERIFY_IS_TRUE(uri_proxy.is_specified());
        VERIFY_IS_FALSE(uri_proxy.is_disabled());
        VERIFY_IS_FALSE(uri_proxy.is_auto_discovery());
        VERIFY_IS_FALSE(uri_proxy.is_default());
        VERIFY_ARE_EQUAL(u, uri_proxy.address());
    }

    TEST_FIXTURE(uri_address, web_proxy_disabled)
    {
        web_proxy disabled_proxy(web_proxy::disabled);
        VERIFY_IS_FALSE(disabled_proxy.is_specified());
        VERIFY_IS_TRUE(disabled_proxy.is_disabled());
        VERIFY_IS_FALSE(disabled_proxy.is_auto_discovery());
        VERIFY_IS_FALSE(disabled_proxy.is_default());
    }

    TEST_FIXTURE(uri_address, web_proxy_discover)
    {
        web_proxy discover_proxy(web_proxy::use_auto_discovery);
        VERIFY_IS_FALSE(discover_proxy.is_specified());
        VERIFY_IS_FALSE(discover_proxy.is_disabled());
        VERIFY_IS_TRUE(discover_proxy.is_auto_discovery());
        VERIFY_IS_FALSE(discover_proxy.is_default());
    }

    TEST_FIXTURE(uri_address, web_proxy_default)
    {
        web_proxy default_proxy(web_proxy::use_default);
        VERIFY_IS_FALSE(default_proxy.is_specified());
        VERIFY_IS_FALSE(default_proxy.is_disabled());
        VERIFY_IS_FALSE(default_proxy.is_auto_discovery());
        VERIFY_IS_TRUE(default_proxy.is_default());
    }

    TEST_FIXTURE(uri_address, web_proxy_default_construct)
    {
        web_proxy default_proxy_2;
        VERIFY_IS_FALSE(default_proxy_2.is_specified());
        VERIFY_IS_FALSE(default_proxy_2.is_disabled());
        VERIFY_IS_FALSE(default_proxy_2.is_auto_discovery());
        VERIFY_IS_TRUE(default_proxy_2.is_default());
    }

    TEST_FIXTURE(uri_address, http_client_config_set_proxy)
    {
        http_client_config hconfig;
        VERIFY_IS_TRUE(hconfig.proxy().is_default());

        uri u = U("http://x");

        hconfig.set_proxy(web_proxy(u));
        VERIFY_ARE_EQUAL(u, hconfig.proxy().address());
    }

#ifndef __cplusplus_winrt
    // IXHR2 does not allow the proxy settings to be changed
    TEST_FIXTURE(uri_address, auto_discovery_proxy)
    {
        test_http_server::scoped_server scoped(m_uri);
        auto t = scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(
                p_request, methods::PUT, U("/"), U("text/plain"), U("this is a test"));
            p_request->reply(status_codes::OK);
        });
        http_client_config config;
        config.set_proxy(web_proxy::use_auto_discovery);

        http_client client(m_uri, config);
        http_asserts::assert_response_equals(client.request(methods::PUT, U("/"), U("this is a test")).get(),
                                             status_codes::OK);

        t.get();
    }

    TEST_FIXTURE(uri_address, disabled_proxy)
    {
        test_http_server::scoped_server scoped(m_uri);
        auto t = scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(
                p_request, methods::PUT, U("/"), U("text/plain"), U("sample data"));
            p_request->reply(status_codes::OK);
        });

        http_client_config config;
        config.set_proxy(web_proxy::disabled);

        http_client client(m_uri, config);
        http_asserts::assert_response_equals(client.request(methods::PUT, U("/"), U("sample data")).get(),
                                             status_codes::OK);

        t.get();
    }

#endif // __cplusplus_winrt

#ifdef __cplusplus_winrt
    TEST_FIXTURE(uri_address, no_proxy_options_on_winrt)
    {
        http_client_config config;
        config.set_proxy(web_proxy::use_auto_discovery);
        http_client client(m_uri, config);

        VERIFY_THROWS(client.request(methods::GET, U("/")).get(), http_exception);
    }
#endif

#ifndef __cplusplus_winrt
    // Can't specify a proxy with WinRT implementation.
    TEST_FIXTURE(uri_address,
                 http_proxy_with_credentials,
                 "Ignore:Linux",
                 "Github 53",
                 "Ignore:Apple",
                 "Github 53",
                 "Ignore:Android",
                 "Github 53",
                 "Ignore:IOS",
                 "Github 53",
                 "Ignore",
                 "Manual")
    {
        web_proxy proxy(proxy_uri);
        web::credentials cred(U("artur"), U("fred")); // relax, this is not my real password
        proxy.set_credentials(cred);

        http_client_config config;
        config.set_proxy(proxy);

        // Access to this server will succeed because the first request will not be challenged and hence
        // my bogus credentials will not be supplied.
        http_client client(U("http://www.microsoft.com"), config);

        try
        {
            http_response response = client.request(methods::GET).get();
            VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
            response.content_ready().wait();
        }
        catch (web::http::http_exception const& e)
        {
            if (e.error_code().value() == 12007)
            {
                // The above "netproxy.redmond.corp.microsoft.com" is an internal site not generally accessible.
                // This will cause a failure to resolve the URL.
                // This is ok.
                return;
            }
            throw;
        }
    }

    TEST_FIXTURE(uri_address, http_proxy, "Ignore", "Manual")
    {
        http_client_config config;
        config.set_proxy(web_proxy(proxy_uri));

        http_client client(U("http://httpbin.org"), config);

        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
        response.content_ready().wait();
    }

    TEST_FIXTURE(uri_address, https_proxy, "Ignore", "Manual")
    {
        http_client_config config;
        config.set_proxy(web_proxy(proxy_uri));

        http_client client(U("https://httpbin.org"), config);

        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
        response.content_ready().wait();
    }

#endif

} // SUITE(proxy_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
