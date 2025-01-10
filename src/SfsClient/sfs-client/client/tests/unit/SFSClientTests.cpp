// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "sfsclient/ApplicabilityDetails.h"
#include "sfsclient/SFSClient.h"

#include <catch2/catch_test_macros.hpp>

#include <optional>

#define TEST(...) TEST_CASE("[SFSClientTests] " __VA_ARGS__)
#define TEST_SCENARIO(...) TEST_CASE("[SFSClientTests] Scenario: " __VA_ARGS__)

using namespace SFS;

namespace
{
std::unique_ptr<SFSClient> GetSFSClient(std::optional<std::string> instanceId = std::nullopt)
{
    std::unique_ptr<SFSClient> sfsClient;
    ClientConfig options;
    options.accountId = "testAccountId";
    if (instanceId)
    {
        options.instanceId = *instanceId;
    }
    REQUIRE(SFSClient::Make(options, sfsClient) == Result::Success);
    REQUIRE(sfsClient != nullptr);
    return sfsClient;
}

void TestLoggingCallback(const LogData&)
{
}

struct TestLoggingCallbackStruct
{
    static void TestLoggingCallback(const LogData&)
    {
    }
};
} // namespace

static void StaticTestLoggingCallback(const LogData&)
{
}

TEST("Testing SFSClient::Make()")
{
#ifdef __GNUG__
// For GCC, explicitly turning off "missing-field-initializers" warning as this block is testing the scenario
// in which a user calls explicitly onto the API with field initializers for ClientConfig
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

    const std::string accountId{"testAccountId"};
    const std::string instanceId{"testInstanceId"};
    const std::string nameSpace{"testNameSpace"};

    std::unique_ptr<SFSClient> sfsClient;

    SECTION("Make({accountId}, out)")
    {
        REQUIRE(SFSClient::Make({accountId}, sfsClient) == Result::Success);
        REQUIRE(sfsClient != nullptr);

        ClientConfig config{accountId};
        REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
        REQUIRE(sfsClient != nullptr);
    }

    SECTION("Make(accountId, instanceId, out)")
    {
        REQUIRE(SFSClient::Make({accountId, instanceId}, sfsClient) == Result::Success);
        REQUIRE(sfsClient != nullptr);

        ClientConfig config{accountId, instanceId};
        REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
        REQUIRE(sfsClient != nullptr);
    }

    SECTION("Make(accountId, instanceId, namespace, out)")
    {
        REQUIRE(SFSClient::Make({accountId, instanceId, nameSpace}, sfsClient) == Result::Success);
        REQUIRE(sfsClient != nullptr);

        SECTION("Call make when the pointer is reset")
        {
            sfsClient.reset();
            REQUIRE(SFSClient::Make({accountId, instanceId, nameSpace}, sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("Call make if the pointer is not reset, as Make() resets it")
        {
            REQUIRE(SFSClient::Make({accountId, instanceId, nameSpace}, sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("We can also use a separate ClientConfig object")
        {
            ClientConfig config{accountId, instanceId, nameSpace};
            REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("We can also move a separate ClientConfig object")
        {
            ClientConfig config{accountId, instanceId, nameSpace};
            REQUIRE(SFSClient::Make(std::move(config), sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }
    }

    SECTION("Make(accountId, std::nullopt, nameSpace, out)")
    {
        REQUIRE(SFSClient::Make({accountId, std::nullopt, nameSpace}, sfsClient) == Result::Success);
        REQUIRE(sfsClient != nullptr);

        ClientConfig config;
        config.accountId = accountId;
        config.nameSpace = nameSpace;
        REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
        REQUIRE(sfsClient != nullptr);
    }

    SECTION("Make(accountId, instanceId, namespace, logCallbackFn, out) works")
    {
        SECTION("Using a lambda with {} initialization")
        {
            REQUIRE(SFSClient::Make({accountId, instanceId, nameSpace, [](const LogData&) {}}, sfsClient) ==
                    Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("Using a lambda with a ClientConfig object")
        {
            ClientConfig config{accountId, instanceId, nameSpace, [](const LogData&) {}};
            REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("Using a nullptr with a ClientConfig object")
        {
            ClientConfig config{accountId, instanceId, nameSpace, nullptr};
            REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("Using a valid empty-namespace function within a ClientConfig object")
        {
            ClientConfig config{accountId, instanceId, nameSpace, TestLoggingCallback};
            REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("Using a valid static function within a ClientConfig object")
        {
            ClientConfig config{accountId, instanceId, nameSpace, StaticTestLoggingCallback};
            REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("Using a valid static member method within a ClientConfig object")
        {
            ClientConfig config{accountId, instanceId, nameSpace, &TestLoggingCallbackStruct::TestLoggingCallback};
            REQUIRE(SFSClient::Make(config, sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }

        SECTION("Can also move a lambda")
        {
            ClientConfig config{accountId, instanceId, nameSpace, [](const LogData&) {}};
            REQUIRE(SFSClient::Make(std::move(config), sfsClient) == Result::Success);
            REQUIRE(sfsClient != nullptr);
        }
    }

    SECTION("AccountId cannot be empty")
    {
        REQUIRE(SFSClient::Make({}, sfsClient) == Result::InvalidArg);
        REQUIRE(sfsClient == nullptr);

        ClientConfig config;
        REQUIRE(SFSClient::Make(config, sfsClient) == Result::InvalidArg);
        REQUIRE(sfsClient == nullptr);

        config = {};
        REQUIRE(SFSClient::Make(config, sfsClient) == Result::InvalidArg);
        REQUIRE(sfsClient == nullptr);

        config.accountId = std::string();
        REQUIRE(SFSClient::Make(config, sfsClient) == Result::InvalidArg);
        REQUIRE(sfsClient == nullptr);

        config.instanceId = instanceId;
        REQUIRE(SFSClient::Make(config, sfsClient) == Result::InvalidArg);
        REQUIRE(sfsClient == nullptr);
    }

#ifdef __GNUG__
// For "-Wmissing-field-initializers"
#pragma GCC diagnostic pop
#endif
}

namespace
{
void TestProductInRequestParams(const std::function<Result(const RequestParams&)>& apiCall,
                                const std::function<void()>& checkContents)
{
    RequestParams params;
    SECTION("Product must not be empty")
    {
        const std::string expectedErrorMsg = "product must not be empty";

        params.productRequests = {{"", {}}};
        auto result = apiCall(params);
        REQUIRE(result.GetCode() == Result::InvalidArg);
        REQUIRE(result.GetMsg() == expectedErrorMsg);
        checkContents();

        const TargetingAttributes attributes{{"attr1", "value"}};
        params.productRequests = {{"", attributes}};
        result = apiCall(params);
        REQUIRE(result.GetCode() == Result::InvalidArg);
        REQUIRE(result.GetMsg() == expectedErrorMsg);
        checkContents();
    }

    SECTION("Does not allow an empty request")
    {
        auto result = apiCall(params);
        REQUIRE(result.GetCode() == Result::InvalidArg);
        REQUIRE(result.GetMsg() == "productRequests cannot be empty");
        checkContents();
    }

    SECTION("Accepting multiple products is not implemented yet")
    {
        params.productRequests = {{"p1", {}}, {"p2", {}}};
        auto result = apiCall(params);
        REQUIRE(result.GetCode() == Result::NotImpl);
        REQUIRE(result.GetMsg() == "There cannot be more than 1 productRequest at the moment");
        checkContents();
    }

    SECTION("Fails if base cv is not correct")
    {
        params.productRequests = {{"p1", {}}};
        params.baseCV = "";
        auto result = apiCall(params);
        REQUIRE(result.GetCode() == Result::InvalidArg);
        REQUIRE(result.GetMsg() == "cv must not be empty");
        checkContents();

        params.baseCV = "cv";
        result = apiCall(params);
        REQUIRE(result.GetCode() == Result::InvalidArg);
        REQUIRE(result.GetMsg().find("baseCV is not a valid correlation vector:") == 0);
        checkContents();
    }

    SECTION("Fails if proxy is setup incorrectly")
    {
        params.productRequests = {{"p1", {}}};
        params.proxy = "bad://";
        auto result = apiCall(params);
        REQUIRE(result.GetCode() == Result::ConnectionUnexpectedError);
        REQUIRE(result.GetMsg() == "Unsupported proxy syntax in 'bad://': No host part in the URL");
        checkContents();

        params.proxy = "bad://bad.com";
        result = apiCall(params);
        REQUIRE(result.GetCode() == Result::ConnectionUnexpectedError);
        REQUIRE(result.GetMsg() == "Unsupported proxy scheme for 'bad://bad.com'");
        checkContents();

        params.proxy = ":";
        result = apiCall(params);
        REQUIRE(result.GetCode() == Result::ConnectionUnexpectedError);
        REQUIRE(result.GetMsg() ==
                "Unsupported proxy syntax in ':': Port number was not a decimal number between 0 and 65535");
        checkContents();

        params.proxy = "http://bad:bad";
        result = apiCall(params);
        REQUIRE(result.GetCode() == Result::ConnectionUnexpectedError);
        REQUIRE(
            result.GetMsg() ==
            "Unsupported proxy syntax in 'http://bad:bad': Port number was not a decimal number between 0 and 65535");
        checkContents();

        params.proxy = "bad:bad";
        result = apiCall(params);
        REQUIRE(result.GetCode() == Result::ConnectionUnexpectedError);
        REQUIRE(result.GetMsg() ==
                "Unsupported proxy syntax in 'bad:bad': Port number was not a decimal number between 0 and 65535");
        checkContents();
    }
}
} // namespace

TEST("Testing SFSClient::GetLatestDownloadInfo()")
{
    auto sfsClient = GetSFSClient();
    std::vector<Content> contents;

    TestProductInRequestParams(
        [&](const RequestParams& params) { return sfsClient->GetLatestDownloadInfo(params, contents); },
        [&contents] { REQUIRE(contents.empty()); });
}

TEST("Testing SFSClient::GetAppLatestDownloadInfo()")
{
    SECTION("With storeapps instance")
    {
        auto sfsClient = GetSFSClient("storeapps");
        std::vector<AppContent> contents;

        TestProductInRequestParams(
            [&](const RequestParams& params) { return sfsClient->GetLatestAppDownloadInfo(params, contents); },
            [&contents] { REQUIRE(contents.empty()); });
    }

    SECTION("Fails if not storeapps instanceId")
    {
        auto sfsClient = GetSFSClient("testInstanceId");
        std::vector<AppContent> contents;
        RequestParams params;
        params.productRequests = {{"a", {}}};
        auto result = sfsClient->GetLatestAppDownloadInfo(params, contents);
        REQUIRE(result.GetCode() == Result::Unexpected);
        REQUIRE(result.GetMsg() == "At this moment only the \"storeapps\" instanceId can send app requests");
        REQUIRE(contents.empty());
    }
}
