// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "../mock/MockWebServer.h"
#include "../util/TestHelper.h"
#include "TestOverride.h"
#include "sfsclient/SFSClient.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>

#define TEST(...) TEST_CASE("[Functional][SFSClientTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::test;
using namespace std::chrono;

const std::string c_instanceId = "testInstanceId";
const std::string c_namespace = "testNamespace";
const std::string c_productName = "testProduct";
const std::string c_version = "0.0.1";
const std::string c_nextVersion = "0.0.2";

namespace
{
void CheckContentId(const ContentId& contentId, const std::string& name, const std::string& version)
{
    REQUIRE(contentId.GetNameSpace() == c_namespace);
    REQUIRE(contentId.GetName() == name);
    REQUIRE(contentId.GetVersion() == version);
}

void CheckFiles(const std::vector<File>& files)
{
    REQUIRE(files.size() == 2);
    REQUIRE(files[0].GetFileId() == (c_productName + ".json"));
    REQUIRE(files[0].GetUrl() == ("http://localhost/1.json"));
    REQUIRE(files[1].GetFileId() == (c_productName + ".bin"));
    REQUIRE(files[1].GetUrl() == ("http://localhost/2.bin"));
}

void CheckAppFiles(const std::vector<AppFile>& files, const std::string& name)
{
    REQUIRE(files.size() == 2);
    REQUIRE(files[0].GetFileId() == (name + ".json"));
    REQUIRE(files[0].GetUrl() == ("http://localhost/1.json"));
    REQUIRE_FALSE(files[0].GetFileMoniker().empty());
    REQUIRE_FALSE(files[0].GetApplicabilityDetails().GetArchitectures().empty());
    REQUIRE_FALSE(files[0].GetApplicabilityDetails().GetPlatformApplicabilityForPackage().empty());
    REQUIRE(files[1].GetFileId() == (name + ".bin"));
    REQUIRE(files[1].GetUrl() == ("http://localhost/2.bin"));
    REQUIRE_FALSE(files[1].GetFileMoniker().empty());
    REQUIRE_FALSE(files[1].GetApplicabilityDetails().GetArchitectures().empty());
    REQUIRE_FALSE(files[1].GetApplicabilityDetails().GetPlatformApplicabilityForPackage().empty());
}

void CheckMockContent(const Content& content, const std::string& version)
{
    CheckContentId(content.GetContentId(), c_productName, version);
    CheckFiles(content.GetFiles());
}

void CheckMockAppContent(const AppContent& content,
                         const std::string& version,
                         const std::vector<MockPrerequisite> mockPrereqs)
{
    CheckContentId(content.GetContentId(), c_productName, version);
    REQUIRE_FALSE(content.GetUpdateId().empty());
    CheckAppFiles(content.GetFiles(), c_productName);

    REQUIRE(content.GetPrerequisites().size() == mockPrereqs.size());
    for (size_t i = 0; i < mockPrereqs.size(); ++i)
    {
        const auto& prereq = content.GetPrerequisites()[i];
        const auto& mockPrereq = mockPrereqs[i];
        CheckContentId(prereq.GetContentId(), mockPrereq.name, mockPrereq.version);

        CheckAppFiles(prereq.GetFiles(), mockPrereq.name);
    }
}
} // namespace

TEST("Testing SFSClient::GetLatestDownloadInfo()")
{
    if (!AreTestOverridesAllowed())
    {
        return;
    }

    test::MockWebServer server;
    ScopedTestOverride override(TestOverride::BaseUrl, server.GetBaseUrl());

    std::unique_ptr<SFSClient> sfsClient;
    REQUIRE(SFSClient::Make({"testAccountId", c_instanceId, c_namespace, LogCallbackToTest}, sfsClient) ==
            Result::Success);
    REQUIRE(sfsClient != nullptr);

    server.RegisterProduct(c_productName, c_version);

    std::vector<Content> contents;

    SECTION("Single product request")
    {
        RequestParams params;
        params.baseCV = "aaaaaaaaaaaaaaaa.1";
        SECTION("No attributes")
        {
            params.productRequests = {{c_productName, {}}};
            REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::Success);
            REQUIRE(contents.size() == 1);
            CheckMockContent(contents[0], c_version);
        }

        SECTION("With attributes")
        {
            const TargetingAttributes attributes{{"attr1", "value"}};
            params.productRequests = {{c_productName, attributes}};
            REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::Success);
            REQUIRE(contents.size() == 1);
            CheckMockContent(contents[0], c_version);
        }

        SECTION("Wrong product name")
        {
            params.productRequests = {{"badName", {}}};
            REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::HttpNotFound);
            REQUIRE(contents.empty());

            const TargetingAttributes attributes{{"attr1", "value"}};
            params.productRequests = {{"badName", attributes}};
            REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::HttpNotFound);
            REQUIRE(contents.empty());
        }

        SECTION("Adding new version")
        {
            server.RegisterProduct(c_productName, c_nextVersion);

            params.productRequests = {{c_productName, {}}};
            REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::Success);
            REQUIRE(contents.size() == 1);
            CheckMockContent(contents[0], c_nextVersion);
        }
    }
}

TEST("Testing SFSClient::GetLatestAppDownloadInfo()")
{
    if (!AreTestOverridesAllowed())
    {
        return;
    }

    test::MockWebServer server;
    ScopedTestOverride override(TestOverride::BaseUrl, server.GetBaseUrl());

    const std::string prereq1 = "prereq1";
    const std::string prereq1Version = "1.0";
    const std::string prereq2 = "prereq2";
    const std::string prereq2Version = "2.0";

    std::unique_ptr<SFSClient> sfsClient;
    REQUIRE(SFSClient::Make({"testAccountId", "storeapps", c_namespace, LogCallbackToTest}, sfsClient) ==
            Result::Success);
    REQUIRE(sfsClient != nullptr);

    SECTION("App products")
    {
        auto CheckApp = [&](const std::vector<MockPrerequisite>& mockPrereqs) {
            server.RegisterAppProduct(c_productName, c_version, mockPrereqs);

            std::vector<AppContent> contents;

            RequestParams params;
            params.baseCV = "aaaaaaaaaaaaaaaa.1";
            SECTION("No attributes")
            {
                params.productRequests = {{c_productName, {}}};
                REQUIRE(sfsClient->GetLatestAppDownloadInfo(params, contents) == Result::Success);
                REQUIRE(contents.size() == 1);
                CheckMockAppContent(contents[0], c_version, mockPrereqs);
            }

            SECTION("With attributes")
            {
                const TargetingAttributes attributes{{"attr1", "value"}};
                params.productRequests = {{c_productName, attributes}};
                REQUIRE(sfsClient->GetLatestAppDownloadInfo(params, contents) == Result::Success);
                REQUIRE(contents.size() == 1);
                CheckMockAppContent(contents[0], c_version, mockPrereqs);
            }

            SECTION("Wrong product name")
            {
                params.productRequests = {{"badName", {}}};
                REQUIRE(sfsClient->GetLatestAppDownloadInfo(params, contents) == Result::HttpNotFound);
                REQUIRE(contents.empty());

                const TargetingAttributes attributes{{"attr1", "value"}};
                params.productRequests = {{"badName", attributes}};
                REQUIRE(sfsClient->GetLatestAppDownloadInfo(params, contents) == Result::HttpNotFound);
                REQUIRE(contents.empty());
            }

            SECTION("Adding new version")
            {
                server.RegisterAppProduct(c_productName, c_nextVersion, mockPrereqs);

                params.productRequests = {{c_productName, {}}};
                REQUIRE(sfsClient->GetLatestAppDownloadInfo(params, contents) == Result::Success);
                REQUIRE(contents.size() == 1);
                CheckMockAppContent(contents[0], c_nextVersion, mockPrereqs);
            }
        };

        SECTION("No prerequisites")
        {
            CheckApp({});
        }

        SECTION("1 prereq")
        {
            CheckApp({{prereq1, prereq1Version}});
        }

        SECTION("2 prereqs")
        {
            CheckApp({{prereq1, prereq1Version}, {prereq2, prereq2Version}});
        }
    }

    SECTION("Non-app products")
    {
        server.RegisterProduct(c_productName, c_version);

        std::vector<AppContent> contents;

        RequestParams params;
        params.productRequests = {{c_productName, {}}};
        auto result = sfsClient->GetLatestAppDownloadInfo(params, contents);
        REQUIRE(result.GetCode() == Result::ServiceUnexpectedContentType);
        REQUIRE(result.GetMsg() ==
                "Unexpected content type [Generic] returned by the service does not match the expected [App]");
        REQUIRE(contents.empty());
    }
}

TEST("Testing SFSClient retry behavior")
{
    if (!AreTestOverridesAllowed())
    {
        INFO("Skipping. Test overrides not enabled");
        return;
    }

    MockWebServer server;
    ScopedTestOverride urlOverride(TestOverride::BaseUrl, server.GetBaseUrl());

    server.RegisterProduct(c_productName, c_version);
    RequestParams params;
    params.productRequests = {{c_productName, {}}};
    std::vector<Content> contents;

    std::unique_ptr<SFSClient> sfsClient;
    ClientConfig clientConfig{"testAccountId", c_instanceId, c_namespace, LogCallbackToTest};

    SECTION("Test exponential backoff")
    {
        INFO("Sets the retry delay to 50ms to speed up the test");
        ScopedTestOverride override(TestOverride::BaseRetryDelayMs, 50);

        REQUIRE(SFSClient::Make(clientConfig, sfsClient));
        REQUIRE(sfsClient != nullptr);

        const int retriableError = 503; // ServerBusy

        auto RunTimedGet = [&](bool success = true) -> long long {
            auto begin = steady_clock::now();
            if (success)
            {
                REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents));
                REQUIRE(contents.size() == 1);
            }
            else
            {
                REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::HttpServiceNotAvailable);
                REQUIRE(contents.empty());
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
        INFO("Sets the retry delay to 200ms to speed up the test");
        ScopedTestOverride override(TestOverride::BaseRetryDelayMs, 200);

        REQUIRE(SFSClient::Make(clientConfig, sfsClient));
        REQUIRE(sfsClient != nullptr);

        const int retriableError = 503;  // ServerBusy
        const int retriableError2 = 502; // BadGateway

        auto RunTimedGet = [&]() -> long long {
            auto begin = steady_clock::now();
            REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents));
            REQUIRE(contents.size() == 1);
            auto end = steady_clock::now();
            return duration_cast<milliseconds>(end - begin).count();
        };

        std::unordered_map<HttpCode, HeaderMap> headersByCode;
        headersByCode[retriableError] = {{"Retry-After", "1"}}; // 1s delay
        server.SetResponseHeaders(headersByCode);

        long long allowedTimeDeviation = 200LL;
        std::queue<HttpCode> forcedHttpErrors({retriableError});
        SECTION("Should take at least 1000ms with a single retriable error with 1s in Retry-After")
        {
            server.SetForcedHttpErrors(forcedHttpErrors);
            const auto time = RunTimedGet();
            REQUIRE(time >= 1000LL);
            REQUIRE(time < 1000LL + allowedTimeDeviation);
        }

        SECTION(
            "Should take at least 1000ms + 200ms with a retriable error with 1s in Retry-After and one with 200ms*2 as default value")
        {
            forcedHttpErrors.push(retriableError2);
            server.SetForcedHttpErrors(forcedHttpErrors);
            const auto time = RunTimedGet();
            REQUIRE(time >= 1400LL);
            REQUIRE(time < 1400LL + allowedTimeDeviation);
        }
    }

    SECTION("Test maxRetries")
    {
        INFO("Sets the retry delay to 1ms to speed up the test");
        ScopedTestOverride override(TestOverride::BaseRetryDelayMs, 1);

        const int retriableError = 503; // ServerBusy

        SECTION("With default retries")
        {
            REQUIRE(SFSClient::Make(clientConfig, sfsClient));
            REQUIRE(sfsClient != nullptr);

            SECTION("Should pass with 3 errors")
            {
                server.SetForcedHttpErrors(std::queue<HttpCode>({retriableError, retriableError, retriableError}));
                REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents));
            }

            SECTION("Should fail with 4 errors")
            {
                server.SetForcedHttpErrors(
                    std::queue<HttpCode>({retriableError, retriableError, retriableError, retriableError}));
                REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::HttpServiceNotAvailable);
            }
        }

        SECTION("Forcing retries to 3")
        {
            params.retryOnError = true;
            REQUIRE(SFSClient::Make(clientConfig, sfsClient));
            REQUIRE(sfsClient != nullptr);

            SECTION("Should pass with 3 errors")
            {
                server.SetForcedHttpErrors(std::queue<HttpCode>({retriableError, retriableError, retriableError}));
                REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents));
            }

            SECTION("Should fail with 4 errors")
            {
                server.SetForcedHttpErrors(
                    std::queue<HttpCode>({retriableError, retriableError, retriableError, retriableError}));
                REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::HttpServiceNotAvailable);
            }
        }

        SECTION("Reducing retries to 0")
        {
            params.retryOnError = false;
            REQUIRE(SFSClient::Make(clientConfig, sfsClient));
            REQUIRE(sfsClient != nullptr);

            SECTION("Should pass with no errors")
            {
                REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents));
            }

            SECTION("Should fail with 1 error")
            {
                server.SetForcedHttpErrors(std::queue<HttpCode>({retriableError}));
                REQUIRE(sfsClient->GetLatestDownloadInfo(params, contents) == Result::HttpServiceNotAvailable);
            }
        }
    }
}
