// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "../../util/SFSExceptionMatcher.h"
#include "../../util/TestHelper.h"
#include "SFSClientImpl.h"
#include "TestOverride.h"
#include "connection/Connection.h"
#include "connection/ConnectionManager.h"
#include "connection/CurlConnection.h"
#include "connection/CurlConnectionManager.h"
#include "connection/mock/MockConnectionManager.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#define TEST(...) TEST_CASE("[SFSClientImplTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::test;
using json = nlohmann::json;

namespace
{
class MockCurlConnection : public CurlConnection
{
  public:
    MockCurlConnection(const ReportingHandler& handler,
                       Result::Code& responseCode,
                       std::string& getResponse,
                       std::string& postResponse,
                       bool& expectEmptyPostBody)
        : CurlConnection({}, handler)
        , m_responseCode(responseCode)
        , m_getResponse(getResponse)
        , m_postResponse(postResponse)
        , m_expectEmptyPostBody(expectEmptyPostBody)
    {
    }

    std::string Get(const std::string&) override
    {
        if (m_responseCode == Result::Success)
        {
            INFO("MockCurlConnection::Get() called, response: " << m_getResponse);
            return m_getResponse;
        }
        throw SFSException(m_responseCode);
    }

    std::string Post(const std::string&, const std::string& data) override
    {
        if (m_responseCode == Result::Success)
        {
            INFO("MockCurlConnection::Post() called, response: " << m_postResponse);
            if (m_expectEmptyPostBody)
            {
                REQUIRE(data.empty());
            }
            else
            {
                REQUIRE(!data.empty());
            }
            return m_postResponse;
        }
        throw SFSException(m_responseCode);
    }

  private:
    Result::Code& m_responseCode;
    std::string& m_getResponse;
    std::string& m_postResponse;
    bool& m_expectEmptyPostBody;
};

void CheckProduct(const VersionEntity& entity, std::string_view ns, std::string_view name, std::string_view version)
{
    REQUIRE(entity.GetContentType() == ContentType::Generic);
    REQUIRE(entity.contentId.nameSpace == ns);
    REQUIRE(entity.contentId.name == name);
    REQUIRE(entity.contentId.version == version);
}

void CheckDownloadInfo(const FileEntities& files, const std::string& name)
{
    REQUIRE(files.size() == 2);
    REQUIRE(files[0]->fileId == (name + ".json"));
    REQUIRE(files[0]->url == ("http://localhost/1.json"));
    REQUIRE(files[1]->fileId == (name + ".bin"));
    REQUIRE(files[1]->url == ("http://localhost/2.bin"));
}
} // namespace

TEST("Testing class SFSClientImpl()")
{
    const std::string ns = "testNameSpace";
    SFSClientImpl<CurlConnectionManager> sfsClient({"testAccountId", "testInstanceId", ns, LogCallbackToTest});

    Result::Code responseCode = Result::Success;
    std::string getResponse;
    std::string postResponse;
    bool expectEmptyPostBody = true;
    std::unique_ptr<Connection> connection = std::make_unique<MockCurlConnection>(sfsClient.GetReportingHandler(),
                                                                                  responseCode,
                                                                                  getResponse,
                                                                                  postResponse,
                                                                                  expectEmptyPostBody);

    const std::string productName = "productName";
    const std::string expectedVersion = "0.0.0.2";

    SECTION("Testing SFSClientImpl::GetLatestVersion()")
    {
        expectEmptyPostBody = false;
        std::unique_ptr<VersionEntity> entity;

        SECTION("Expected response")
        {
            const json latestVersionResponse = {
                {"ContentId", {{"Namespace", ns}, {"Name", productName}, {"Version", expectedVersion}}}};
            postResponse = latestVersionResponse.dump();
            SECTION("No attributes")
            {
                REQUIRE_NOTHROW(entity = sfsClient.GetLatestVersion({productName, {}}, *connection));
                REQUIRE(entity);
                CheckProduct(*entity, ns, productName, expectedVersion);
            }

            SECTION("With attributes")
            {
                const TargetingAttributes attributes{{"attr1", "value"}};
                REQUIRE_NOTHROW(entity = sfsClient.GetLatestVersion({productName, attributes}, *connection));
                REQUIRE(entity);
                CheckProduct(*entity, ns, productName, expectedVersion);
            }

            SECTION("Failing")
            {
                responseCode = Result::HttpNotFound;
                REQUIRE_THROWS_CODE(entity = sfsClient.GetLatestVersion({"badName", {}}, *connection), HttpNotFound);
                REQUIRE(!entity);

                const TargetingAttributes attributes{{"attr1", "value"}};
                REQUIRE_THROWS_CODE(entity = sfsClient.GetLatestVersion({"badName", attributes}, *connection),
                                    HttpNotFound);
                REQUIRE(!entity);
            }
        }

        SECTION("Unexpected response")
        {
            SECTION("Wrong ns")
            {
                const json latestVersionResponse = {
                    {"ContentId", {{"Namespace", "wrong"}, {"Name", productName}, {"Version", expectedVersion}}}};
                postResponse = latestVersionResponse.dump();
            }

            SECTION("Wrong name")
            {
                const json latestVersionResponse = {
                    {"ContentId", {{"Namespace", ns}, {"Name", "wrong"}, {"Version", expectedVersion}}}};
                postResponse = latestVersionResponse.dump();
            }

            REQUIRE_THROWS_CODE_MSG(entity = sfsClient.GetLatestVersion({productName, {}}, *connection),
                                    ServiceInvalidResponse,
                                    "Response does not match the requested product");
            REQUIRE(!entity);
        }
    }

    SECTION("Testing SFSClientImpl::GetLatestVersionBatch()")
    {
        expectEmptyPostBody = false;
        json latestVersionResponse = json::array();
        latestVersionResponse.push_back(
            {{"ContentId", {{"Namespace", ns}, {"Name", productName}, {"Version", expectedVersion}}}});
        postResponse = latestVersionResponse.dump();
        VersionEntities entities;
        SECTION("No attributes")
        {
            REQUIRE_NOTHROW(entities = sfsClient.GetLatestVersionBatch({{productName, {}}}, *connection));
            REQUIRE(!entities.empty());
            CheckProduct(*entities[0], ns, productName, expectedVersion);
        }

        SECTION("With attributes")
        {
            const TargetingAttributes attributes{{"attr1", "value"}};
            REQUIRE_NOTHROW(entities = sfsClient.GetLatestVersionBatch({{productName, attributes}}, *connection));
            REQUIRE(!entities.empty());
            CheckProduct(*entities[0], ns, productName, expectedVersion);
        }

        SECTION("Failing")
        {
            responseCode = Result::HttpNotFound;
            REQUIRE_THROWS_CODE(entities = sfsClient.GetLatestVersionBatch({{"badName", {}}}, *connection),
                                HttpNotFound);

            const TargetingAttributes attributes{{"attr1", "value"}};
            REQUIRE_THROWS_CODE(entities = sfsClient.GetLatestVersionBatch({{"badName", attributes}}, *connection),
                                HttpNotFound);
        }
    }

    SECTION("Testing SFSClientImpl::GetSpecificVersion()")
    {
        json specificVersionResponse;
        specificVersionResponse["ContentId"] = {{"Namespace", ns}, {"Name", productName}, {"Version", expectedVersion}};
        specificVersionResponse["Files"] = json::array({productName + ".json", productName + ".bin"});
        getResponse = specificVersionResponse.dump();
        std::unique_ptr<VersionEntity> entity;
        SECTION("Getting version")
        {
            REQUIRE_NOTHROW(entity = sfsClient.GetSpecificVersion(productName, expectedVersion, *connection));
            REQUIRE(entity);
            CheckProduct(*entity, ns, productName, expectedVersion);
        }

        SECTION("Failing")
        {
            responseCode = Result::HttpNotFound;
            REQUIRE_THROWS_CODE(entity = sfsClient.GetSpecificVersion(productName, expectedVersion, *connection),
                                HttpNotFound);
        }
    }

    SECTION("Testing SFSClientImpl::GetDownloadInfo()")
    {
        expectEmptyPostBody = true;
        json downloadInfoResponse;
        downloadInfoResponse = json::array();
        downloadInfoResponse.push_back({{"Url", "http://localhost/1.json"},
                                        {"FileId", productName + ".json"},
                                        {"SizeInBytes", 100},
                                        {"Hashes", {{"Sha1", "123"}, {"Sha256", "456"}}}});
        downloadInfoResponse[0]["DeliveryOptimization"] = {{"CatalogId", "789"}};
        downloadInfoResponse[0]["DeliveryOptimization"]["Properties"] = {
            {"IntegrityCheckInfo", {{"PiecesHashFileUrl", "http://localhost/1.json"}, {"HashOfHashes", "abc"}}}};

        downloadInfoResponse.push_back({{"Url", "http://localhost/2.bin"},
                                        {"FileId", productName + ".bin"},
                                        {"SizeInBytes", 200},
                                        {"Hashes", {{"Sha1", "421"}, {"Sha256", "132"}}}});
        downloadInfoResponse[1]["DeliveryOptimization"] = downloadInfoResponse[0]["DeliveryOptimization"];
        postResponse = downloadInfoResponse.dump();

        FileEntities files;
        SECTION("Getting version")
        {
            REQUIRE_NOTHROW(files = sfsClient.GetDownloadInfo(productName, expectedVersion, *connection));
            REQUIRE(!files.empty());
            CheckDownloadInfo(files, productName);
        }

        SECTION("Failing")
        {
            responseCode = Result::HttpNotFound;
            REQUIRE_THROWS_CODE(files = sfsClient.GetDownloadInfo(productName, expectedVersion, *connection),
                                HttpNotFound);
            REQUIRE(files.empty());
        }
    }
}

TEST("Testing ClientConfig validation")
{
    SECTION("accountId must not be empty")
    {
        const std::string expectedErrorMsg = "ClientConfig::accountId must not be empty";
        for (size_t i = 0; i <= 10; ++i)
        {
            ClientConfig config;
            config.accountId = std::string(i, 'a');
            if (i >= 1)
            {
                REQUIRE_NOTHROW(SFSClientImpl<CurlConnectionManager>(std::move(config)));
            }
            else
            {
                REQUIRE_THROWS_CODE_MSG(SFSClientImpl<CurlConnectionManager>(std::move(config)),
                                        InvalidArg,
                                        expectedErrorMsg);
            }
        }
    }

    SECTION("instanceId must not be empty")
    {
        const std::string expectedErrorMsg = "ClientConfig::instanceId must not be empty";
        for (size_t i = 0; i <= 10; ++i)
        {
            ClientConfig config;
            config.accountId = "testAccountId";
            config.instanceId = std::string(i, 'a');
            if (i >= 1)
            {
                REQUIRE_NOTHROW(SFSClientImpl<CurlConnectionManager>(std::move(config)));
            }
            else
            {
                REQUIRE_THROWS_CODE_MSG(SFSClientImpl<CurlConnectionManager>(std::move(config)),
                                        InvalidArg,
                                        expectedErrorMsg);
            }
        }
    }

    SECTION("NameSpace must not be empty")
    {
        const std::string expectedErrorMsg = "ClientConfig::nameSpace must not be empty";
        for (size_t i = 0; i <= 10; ++i)
        {
            ClientConfig config;
            config.accountId = "testAccountId";
            config.nameSpace = std::string(i, 'a');
            if (i >= 1)
            {
                REQUIRE_NOTHROW(SFSClientImpl<CurlConnectionManager>(std::move(config)));
            }
            else
            {
                REQUIRE_THROWS_CODE_MSG(SFSClientImpl<CurlConnectionManager>(std::move(config)),
                                        InvalidArg,
                                        expectedErrorMsg);
            }
        }
    }
}

TEST("Testing SFSClientImpl::SetCustomBaseUrl()")
{
    ClientConfig config;
    config.accountId = "testAccountId";
    SFSClientImpl<MockConnectionManager> sfsClient(std::move(config));

    REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "https://testAccountId.api.cdp.microsoft.com/");

    sfsClient.SetCustomBaseUrl("customUrl");
    REQUIRE_THROWS_CODE_MSG(sfsClient.MakeUrlBuilder().GetUrl(),
                            ConnectionUrlSetupFailed,
                            "Curl URL error: Bad scheme");

    sfsClient.SetCustomBaseUrl("http://customUrl.com/");
    REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "http://customUrl.com/");
}

TEST("Testing test override SFS_TEST_OVERRIDE_BASE_URL")
{
    ClientConfig config;
    config.accountId = "testAccountId";
    SFSClientImpl<MockConnectionManager> sfsClient(std::move(config));

    REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "https://testAccountId.api.cdp.microsoft.com/");

    {
        INFO("Can override the base url with the test key");
        ScopedTestOverride override(TestOverride::BaseUrl, "http://override.com");
        if (AreTestOverridesAllowed())
        {
            REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "http://override.com/");
        }
        else
        {
            REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "https://testAccountId.api.cdp.microsoft.com/");
        }
    }

    if (AreTestOverridesAllowed())
    {
        INFO("Fails if the override is not a valid URL");
        ScopedTestOverride override(TestOverride::BaseUrl, "override");
        {
            REQUIRE_THROWS_CODE_MSG(sfsClient.MakeUrlBuilder().GetUrl(),
                                    ConnectionUrlSetupFailed,
                                    "Curl URL error: Bad scheme");
        }
    }

    INFO("Override is unset after ScopedEnv goes out of scope");
    REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "https://testAccountId.api.cdp.microsoft.com/");

    sfsClient.SetCustomBaseUrl("http://customUrl.com");
    REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "http://customUrl.com/");

    {
        INFO("Can also override a custom base base url with the test key");
        ScopedTestOverride override(TestOverride::BaseUrl, "http://override.com");
        if (AreTestOverridesAllowed())
        {
            REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "http://override.com/");
        }
        else
        {
            REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "http://customUrl.com/");
        }
    }

    REQUIRE(sfsClient.MakeUrlBuilder().GetUrl() == "http://customUrl.com/");
}

TEST("Testing passing a logging callback to constructor of SFSClientImpl")
{
    SFSClientImpl<MockConnectionManager> sfsClient(
        {"testAccountId", "testInstanceId", "testNameSpace", [](const LogData&) {}});
    SFSClientImpl<MockConnectionManager> sfsClient2({"testAccountId", "testInstanceId", "testNameSpace", nullptr});
}
