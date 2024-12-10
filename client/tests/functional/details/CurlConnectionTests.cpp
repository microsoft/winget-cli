// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "../../mock/MockWebServer.h"
#include "../../mock/ProxyServer.h"
#include "../../util/SFSExceptionMatcher.h"
#include "../../util/TestHelper.h"
#include "ReportingHandler.h"
#include "SFSUrlBuilder.h"
#include "TestOverride.h"
#include "connection/CurlConnection.h"
#include "connection/CurlConnectionManager.h"
#include "connection/HttpHeader.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <sstream>

#define TEST(...) TEST_CASE("[Functional][CurlConnectionTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::test;
using namespace std::chrono;
using json = nlohmann::json;

const std::string c_instanceId = "default";
const std::string c_namespace = "default";
const std::string c_productName = "testProduct";
const std::string c_version = "0.0.1";
const std::string c_nextVersion = "0.0.2";

namespace
{
class CurlConnectionTimeout : public CurlConnection
{
  public:
    CurlConnectionTimeout(const ConnectionConfig& config, const ReportingHandler& handler)
        : CurlConnection(config, handler)
    {
    }

    std::string Get(const std::string& url) override
    {
        // Timeout within 1ms
        curl_easy_setopt(m_handle, CURLOPT_TIMEOUT_MS, 1L);
        return CurlConnection::Get(url);
    }

    std::string Post(const std::string& url, const std::string& data) override
    {
        // Timeout within 1ms
        curl_easy_setopt(m_handle, CURLOPT_TIMEOUT_MS, 1L);
        return CurlConnection::Post(url, data);
    }
};

class CurlConnectionTimeoutManager : public CurlConnectionManager
{
  public:
    CurlConnectionTimeoutManager(const ReportingHandler& handler) : CurlConnectionManager(handler)
    {
    }

    std::unique_ptr<Connection> MakeConnection(const ConnectionConfig& config) override
    {
        return std::make_unique<CurlConnectionTimeout>(config, m_handler);
    }
};

std::string TimestampToHttpDateString(std::chrono::time_point<std::chrono::system_clock> time)
{
    auto timer = system_clock::to_time_t(time);

    std::stringstream timeStream;
    struct tm gmTime;
#ifdef _WIN32
    gmtime_s(&gmTime, &timer); // gmtime_s is the safe version of gmtime, not available on Linux
#else
    gmTime = (*std::gmtime(&timer));
#endif
    timeStream << std::put_time(&gmTime, "%a, %d %b %Y %X GMT"); // day, dd Mmm yyyy HH:MM:SS

    return timeStream.str();
}
} // namespace

TEST("Testing CurlConnection()")
{
    test::MockWebServer server;
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);
    CurlConnectionManager connectionManager(handler);
    auto connection = connectionManager.MakeConnection({});
    SFSUrlBuilder urlBuilder(SFSCustomUrl(server.GetBaseUrl()), c_instanceId, c_namespace, handler);
    server.RegisterExpectedRequestHeader(HttpHeader::UserAgent, GetUserAgentValue());

    SECTION("Testing CurlConnection::Get()")
    {
        const std::string url = urlBuilder.GetSpecificVersionUrl(c_productName, c_version);

        json expectedResponse;
        expectedResponse["ContentId"] = {{"Namespace", "default"}, {"Name", c_productName}, {"Version", c_version}};

        SECTION("Direct connection")
        {
            // Before registering the product, the URL returns 404 Not Found
            REQUIRE_THROWS_CODE(connection->Get(url), HttpNotFound);

            // Register the product
            server.RegisterProduct(c_productName, c_version);

            // After registering the product, the URL returns 200 OK
            std::string out;
            REQUIRE_NOTHROW(out = connection->Get(url));

            REQUIRE(json::parse(out) == expectedResponse);
        }

        SECTION("With proxy")
        {
            test::ProxyServer proxy;

            ConnectionConfig config;
            config.proxy = proxy.GetBaseUrl();
            connection = connectionManager.MakeConnection(config);

            // Before registering the product, the URL returns 404 Not Found
            REQUIRE_THROWS_CODE(connection->Get(url), HttpNotFound);

            // Register the product
            server.RegisterProduct(c_productName, c_version);

            // After registering the product, the URL returns 200 OK
            std::string out;
            REQUIRE_NOTHROW(out = connection->Get(url));

            REQUIRE(json::parse(out) == expectedResponse);

            REQUIRE(proxy.Stop() == Result::Success);
        }
    }

    SECTION("Testing CurlConnection::Post()")
    {
        server.RegisterExpectedRequestHeader(HttpHeader::ContentType, "application/json");

        SECTION("With GetLatestVersionBatch mock")
        {
            const std::string url = urlBuilder.GetLatestVersionBatchUrl();

            // Missing proper body returns HttpBadRequest
            REQUIRE_THROWS_CODE(connection->Post(url), HttpBadRequest);

            // Before registering the product, the URL returns 404 Not Found
            const json body = {{{"TargetingAttributes", {}}, {"Product", c_productName}}};
            REQUIRE_THROWS_CODE(connection->Post(url, body.dump()), HttpNotFound);

            // Register the product
            server.RegisterProduct(c_productName, c_version);

            // After registering the product, the URL returns 200 OK
            std::string out;
            REQUIRE_NOTHROW(out = connection->Post(url, body.dump()));

            json expectedResponse = json::array();
            expectedResponse.push_back(
                {{"ContentId", {{"Namespace", "default"}, {"Name", c_productName}, {"Version", c_version}}}});
            REQUIRE(json::parse(out) == expectedResponse);

            // Returns the next version now
            server.RegisterProduct(c_productName, c_nextVersion);
            REQUIRE_NOTHROW(out = connection->Post(url, body.dump()));

            expectedResponse = json::array();
            expectedResponse.push_back(
                {{"ContentId", {{"Namespace", "default"}, {"Name", c_productName}, {"Version", c_nextVersion}}}});
            REQUIRE(json::parse(out) == expectedResponse);

            SECTION("Testing with proxy")
            {
                test::ProxyServer proxy;

                ConnectionConfig config;
                config.proxy = proxy.GetBaseUrl();
                connection = connectionManager.MakeConnection(config);

                REQUIRE_NOTHROW(out = connection->Post(url, body.dump()));

                REQUIRE(json::parse(out) == expectedResponse);

                REQUIRE(proxy.Stop() == Result::Success);
            }
        }

        SECTION("With GetDownloadInfo mock")
        {
            const std::string url = urlBuilder.GetDownloadInfoUrl(c_productName, c_version);

            // Before registering the product, the URL returns 404 Not Found
            REQUIRE_THROWS_CODE(connection->Post(url), HttpNotFound);

            // Register the product
            server.RegisterProduct(c_productName, c_version);

            // After registering the product, the URL returns 200 OK
            std::string out;
            REQUIRE_NOTHROW(out = connection->Post(url));

            json expectedResponse = json::array();
            expectedResponse.push_back({{"Url", "http://localhost/1.json"},
                                        {"FileId", c_productName + ".json"},
                                        {"SizeInBytes", 100},
                                        {"Hashes", {{"Sha1", "123"}, {"Sha256", "456"}}}});
            expectedResponse[0]["DeliveryOptimization"] = {{"CatalogId", "789"}};
            expectedResponse[0]["DeliveryOptimization"]["Properties"] = {
                {"IntegrityCheckInfo", {{"PiecesHashFileUrl", "http://localhost/1.json"}, {"HashOfHashes", "abc"}}}};

            expectedResponse.push_back({{"Url", "http://localhost/2.bin"},
                                        {"FileId", c_productName + ".bin"},
                                        {"SizeInBytes", 200},
                                        {"Hashes", {{"Sha1", "421"}, {"Sha256", "132"}}}});
            expectedResponse[1]["DeliveryOptimization"] = {{"CatalogId", "14"}};
            expectedResponse[1]["DeliveryOptimization"]["Properties"] = {
                {"IntegrityCheckInfo", {{"PiecesHashFileUrl", "http://localhost/2.bin"}, {"HashOfHashes", "abcd"}}}};

            REQUIRE(json::parse(out) == expectedResponse);

            SECTION("Testing with proxy")
            {
                test::ProxyServer proxy;

                ConnectionConfig config;
                config.proxy = proxy.GetBaseUrl();
                connection = connectionManager.MakeConnection(config);

                REQUIRE_NOTHROW(out = connection->Post(url));

                REQUIRE(json::parse(out) == expectedResponse);

                REQUIRE(proxy.Stop() == Result::Success);
            }
        }
    }

    SECTION("Testing with a malformed url")
    {
        std::string url = server.GetBaseUrl();
        REQUIRE_THROWS_CODE(connection->Get(url), HttpNotFound);
        REQUIRE_THROWS_CODE(connection->Get(url + "/names"), HttpNotFound);
        REQUIRE_THROWS_CODE(connection->Post(url + "/names", "{}"), HttpNotFound);

        url = server.GetBaseUrl() + "/api/v1/contents/" + c_instanceId + "/namespaces/" + c_namespace + "/names/" +
              c_productName + "/versYions/" + c_version + "/files?action=GenerateDownloadInfo";

        std::string out;
        REQUIRE_THROWS_CODE(connection->Get(url), HttpNotFound);
        REQUIRE_THROWS_CODE(connection->Post(url, std::string()), HttpNotFound);
        REQUIRE_THROWS_CODE(out = connection->Post(url, {}), HttpNotFound);
        CHECK(out.empty());
    }

    REQUIRE(server.Stop() == Result::Success);
}

TEST("Testing CurlConnection when the server is not reachable")
{
    // Using a custom override class just to time out faster on an invalid URL
    ReportingHandler handler;
    CurlConnectionTimeoutManager connectionManager(handler);
    handler.SetLoggingCallback(LogCallbackToTest);
    auto connection = connectionManager.MakeConnection({});

    // Using a non-routable IP address to ensure the server is not reachable
    // https://www.rfc-editor.org/rfc/rfc5737#section-3: The blocks 192.0.2.0/24 (...) are provided for use in
    // documentation.
    std::string url = "192.0.2.0";
    std::string out;
    REQUIRE_THROWS_CODE(connection->Get(url), HttpTimeout);
    REQUIRE_THROWS_CODE(connection->Post(url + "/names", "{}"), HttpTimeout);

    url = "192.0.2.0/files?action=GenerateDownloadInfo";

    REQUIRE_THROWS_CODE_MSG_MATCHES(connection->Get(url),
                                    HttpTimeout,
                                    Catch::Matchers::ContainsSubstring("timed out after"));
    REQUIRE_THROWS_CODE_MSG_MATCHES(connection->Post(url, {}),
                                    HttpTimeout,
                                    Catch::Matchers::ContainsSubstring("timed out after"));
}

TEST("Testing CurlConnection works from a second ConnectionManager")
{
    ReportingHandler handler;

    // Create a first connection manager and see it leave scope - curl initializes and uninitializes
    {
        CurlConnectionManager connectionManager(handler);
        REQUIRE(connectionManager.MakeConnection({}) != nullptr);
    }

    test::MockWebServer server;
    CurlConnectionManager connectionManager(handler);
    handler.SetLoggingCallback(LogCallbackToTest);
    auto connection = connectionManager.MakeConnection({});
    SFSUrlBuilder urlBuilder(SFSCustomUrl(server.GetBaseUrl()), c_instanceId, c_namespace, handler);

    const std::string url = urlBuilder.GetSpecificVersionUrl(c_productName, c_version);

    // Register the product
    server.RegisterProduct(c_productName, c_version);

    // After registering the product, the URL returns 200 OK
    REQUIRE_NOTHROW(connection->Get(url));

    SECTION("A second connection also works")
    {
        auto connection2 = connectionManager.MakeConnection({});
        REQUIRE_NOTHROW(connection2->Get(url));
    }
}

TEST("Testing a url that's too big throws 414")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    test::MockWebServer server;
    CurlConnectionManager connectionManager(handler);
    auto connection = connectionManager.MakeConnection({});
    SFSUrlBuilder urlBuilder(SFSCustomUrl(server.GetBaseUrl()), c_instanceId, c_namespace, handler);

    // Will use a fake large product name to produce a large url
    const std::string largeProductName(90000, 'a');

    // Register the products
    server.RegisterProduct(largeProductName, c_version);

    // Url produces: 414 URI Too Long
    std::string out;
    REQUIRE_THROWS_CODE_MSG(connection->Get(urlBuilder.GetSpecificVersionUrl(largeProductName, c_version)),
                            HttpUnexpected,
                            "Unexpected HTTP code 414");
}

TEST("Testing a response over the limit fails the operation")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    test::MockWebServer server;
    CurlConnectionManager connectionManager(handler);
    auto connection = connectionManager.MakeConnection({});
    SFSUrlBuilder urlBuilder(SFSCustomUrl(server.GetBaseUrl()), c_instanceId, c_namespace, handler);

    // Will use a fake large product name to produce a response over the limit of 100k characters
    const std::string largeProductName(90000, 'a');
    const std::string overLimitProductName(1000000, 'a');

    // Register the products
    server.RegisterProduct(largeProductName, c_version);
    server.RegisterProduct(overLimitProductName, c_version);

    // Using GetLatestVersionBatch api since the product name is in the body and not in the url, to avoid a 414 error
    // like on the test above
    const std::string url = urlBuilder.GetLatestVersionBatchUrl();

    // Large one works
    json body = {{{"TargetingAttributes", {}}, {"Product", largeProductName}}};
    REQUIRE_NOTHROW(connection->Post(url, body.dump()));

    // Going over the limit fails with a message like "client returned ERROR on write of 16384 bytes"
    body[0]["Product"] = overLimitProductName;
    REQUIRE_THROWS_CODE_MSG_MATCHES(connection->Post(url, body.dump()),
                                    ConnectionUnexpectedError,
                                    Catch::Matchers::ContainsSubstring("client returned ERROR on write of"));
}

TEST("Testing MS-CV is sent to server")
{
    test::MockWebServer server;
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);
    CurlConnectionManager connectionManager(handler);

    const std::string cv = "aaaaaaaaaaaaaaaa.1";
    ConnectionConfig config;
    config.baseCV = cv;
    auto connection = connectionManager.MakeConnection(config);
    SFSUrlBuilder urlBuilder(SFSCustomUrl(server.GetBaseUrl()), c_instanceId, c_namespace, handler);

    const std::string url = urlBuilder.GetSpecificVersionUrl(c_productName, c_version);

    server.RegisterProduct(c_productName, c_version);

    INFO("First value ends with .0");
    server.RegisterExpectedRequestHeader(HttpHeader::MSCV, cv + ".0");
    connection->Get(url);

    INFO("Value gets incremented on subsequent requests");
    server.RegisterExpectedRequestHeader(HttpHeader::MSCV, cv + ".1");
    connection->Get(url);

    server.RegisterExpectedRequestHeader(HttpHeader::MSCV, cv + ".2");
    connection->Get(url);
}

TEST("Testing retry behavior")
{
    if (!AreTestOverridesAllowed())
    {
        INFO("Skipping. Test overrides not enabled");
        return;
    }

    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    MockWebServer server;
    CurlConnectionManager connectionManager(handler);
    SFSUrlBuilder urlBuilder(SFSCustomUrl(server.GetBaseUrl()), c_instanceId, c_namespace, handler);

    server.RegisterProduct(c_productName, c_version);
    const std::string url = urlBuilder.GetSpecificVersionUrl(c_productName, c_version);

    SECTION("Test exponential backoff")
    {
        auto connection = connectionManager.MakeConnection({});

        INFO("Sets the retry delay to 50ms to speed up the test");
        ScopedTestOverride override(TestOverride::BaseRetryDelayMs, 50);

        const int retriableError = 503; // ServerBusy

        auto RunTimedGet = [&](bool success = true) -> long long {
            std::string out;
            auto begin = steady_clock::now();
            if (success)
            {
                REQUIRE_NOTHROW(out = connection->Get(url));
                REQUIRE_FALSE(out.empty());
            }
            else
            {
                REQUIRE_THROWS_CODE(out = connection->Get(url), HttpServiceNotAvailable);
                REQUIRE(out.empty());
            }
            auto end = steady_clock::now();
            return duration_cast<milliseconds>(end - begin).count();
        };

        long long allowedTimeDeviation = 200LL;
        std::queue<HttpCode> forcedHttpErrors({retriableError});
        SECTION("Should take at least 50ms with a single retriable error")
        {
            server.SetForcedHttpErrors(forcedHttpErrors);
            const auto time = RunTimedGet();
            REQUIRE(time >= 50LL);
            REQUIRE(time < 50LL + allowedTimeDeviation);
        }

        forcedHttpErrors.push(retriableError);
        SECTION("Should take at least 150ms (50ms + 2*50ms) with two retriable errors")
        {
            server.SetForcedHttpErrors(forcedHttpErrors);
            const auto time = RunTimedGet();
            REQUIRE(time >= 150LL);
            REQUIRE(time < 150LL + allowedTimeDeviation);
        }

        forcedHttpErrors.push(retriableError);
        SECTION("Should take at least 300ms (50ms + 2*50ms + 3*50ms) with three retriable errors")
        {
            server.SetForcedHttpErrors(forcedHttpErrors);
            const auto time = RunTimedGet();
            REQUIRE(time >= 300LL);
            REQUIRE(time < 300LL + allowedTimeDeviation);
        }

        forcedHttpErrors.push(retriableError);
        SECTION("Should take at least 300ms (50ms + 2*50ms + 3*50ms) with four retriable errors, but fail")
        {
            server.SetForcedHttpErrors(forcedHttpErrors);
            const auto time = RunTimedGet(false /*success*/);
            REQUIRE(time >= 300LL);
            REQUIRE(time < 300LL + allowedTimeDeviation);
        }
    }

    SECTION("Test retriable errors with Retry-After headers")
    {
        auto connection = connectionManager.MakeConnection({});

        INFO("Sets the retry delay to 200ms to speed up the test");
        ScopedTestOverride override(TestOverride::BaseRetryDelayMs, 200);

        const int retriableError = 503;  // ServerBusy
        const int retriableError2 = 502; // BadGateway

        auto RunTimedGet = [&]() -> long long {
            std::string out;
            auto begin = steady_clock::now();
            REQUIRE_NOTHROW(out = connection->Get(url));
            REQUIRE_FALSE(out.empty());
            auto end = steady_clock::now();
            return duration_cast<milliseconds>(end - begin).count();
        };

        auto testForRetryAfterValue = [&](const std::string& retryAfterValue) -> void {
            INFO("retryAfterValue = " << retryAfterValue);

            std::unordered_map<HttpCode, HeaderMap> headersByCode;
            headersByCode[retriableError] = {{"Retry-After", retryAfterValue}};
            server.SetResponseHeaders(headersByCode);

            long long allowedTimeDeviation = 200LL;
            std::queue<HttpCode> forcedHttpErrors({retriableError});
            SECTION("Should take at least 2000ms with a single retriable error, with 2s in Retry-After")
            {
                server.SetForcedHttpErrors(forcedHttpErrors);
                const auto time = RunTimedGet();
                REQUIRE(time >= 2000LL);
                REQUIRE(time < 2000LL + allowedTimeDeviation);
            }

            SECTION(
                "Should take at least 2000ms + 200ms*2 (exponential backoff) with two retriable errors, with 2s in Retry-After of the first error")
            {
                forcedHttpErrors.push(retriableError2);
                server.SetForcedHttpErrors(forcedHttpErrors);
                const auto time = RunTimedGet();
                REQUIRE(time >= 2400LL);
                REQUIRE(time < 2400LL + allowedTimeDeviation);
            }
        };

        SECTION("Using seconds")
        {
            testForRetryAfterValue("2"); // 2s delay
        }
        SECTION("Using date")
        {
            const auto time = std::chrono::system_clock::now() + std::chrono::seconds(2);
            testForRetryAfterValue(TimestampToHttpDateString(time));
        }
    }

    SECTION("Test maxRetries")
    {
        INFO("Sets the retry delay to 1ms to speed up the test");
        ScopedTestOverride override(TestOverride::BaseRetryDelayMs, 1);

        const int retriableError = 503; // ServerBusy

        SECTION("Should pass with 3 errors")
        {
            auto connection = connectionManager.MakeConnection({});
            server.SetForcedHttpErrors(std::queue<HttpCode>({retriableError, retriableError, retriableError}));
            REQUIRE_NOTHROW(connection->Get(url));
        }

        SECTION("Should fail with 4 errors")
        {
            auto connection = connectionManager.MakeConnection({});
            server.SetForcedHttpErrors(
                std::queue<HttpCode>({retriableError, retriableError, retriableError, retriableError}));
            REQUIRE_THROWS_CODE(connection->Get(url), HttpServiceNotAvailable);
        }

        SECTION("Reducing retries to 2")
        {
            ConnectionConfig config;
            config.maxRetries = 2;
            auto connection = connectionManager.MakeConnection(config);

            SECTION("Should pass with 2 errors")
            {
                server.SetForcedHttpErrors(std::queue<HttpCode>({retriableError, retriableError}));
                REQUIRE_NOTHROW(connection->Get(url));
            }

            SECTION("Should fail with 3 errors")
            {
                server.SetForcedHttpErrors(std::queue<HttpCode>({retriableError, retriableError, retriableError}));
                REQUIRE_THROWS_CODE(connection->Get(url), HttpServiceNotAvailable);
            }
        }

        SECTION("Reducing retries to 0")
        {
            ConnectionConfig config;
            config.maxRetries = 0;
            auto connection = connectionManager.MakeConnection(config);

            SECTION("Should pass with no errors")
            {
                REQUIRE_NOTHROW(connection->Get(url));
            }

            SECTION("Should fail with 1 error")
            {
                server.SetForcedHttpErrors(std::queue<HttpCode>({retriableError}));
                REQUIRE_THROWS_CODE(connection->Get(url), HttpServiceNotAvailable);
            }
        }
    }
}
