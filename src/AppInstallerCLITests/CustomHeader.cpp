// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include "TestSource.h"
#include "TestRestRequestHandler.h"
#include <Rest/Schema/1_1/Interface.h>
#include <winget/JsonUtil.h>
#include <Rest/RestClient.h>
#include <winget/Settings.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Http;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_0;

namespace
{
    utility::string_t CustomHeaderName = L"Windows-Package-Manager";

    constexpr std::string_view s_EmptySources = R"(
        Sources:
        )"sv;

    utility::string_t sampleSearchResponse = _XPLATSTR(
        R"delimiter({
            "Data" : [
               {
              "PackageIdentifier": "git.package",
              "PackageName": "package",
              "Publisher": "git",
              "Versions": [
                {   "PackageVersion": "1.0.0" }]
            }]
        })delimiter");
}

// In RestClient.cpp tests
extern RestClient CreateRestClient(
    const std::string& restApi,
    const std::optional<std::string>& customHeader,
    std::string_view caller,
    const Http::HttpClientHelper& helper,
    const Authentication::AuthenticationArguments& authArgs = {});

TEST_CASE("RestClient_CustomHeader", "[RestSource][CustomHeader]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.0.0",
                "2.0.0"]
        }})delimiter");

    std::optional<std::string> customHeader = "Testing custom header";
    auto header = std::make_pair<>(CustomHeaderName, JSON::GetUtilityString(customHeader.value()));
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, sample, header) };
    RestClient client = CreateRestClient(utility::conversions::to_utf8string("https://restsource.com/api"), customHeader, {}, helper);
    REQUIRE(client.GetSourceIdentifier() == "Source123");
}

TEST_CASE("RestSourceSearch_CustomHeader", "[RestSource][CustomHeader]")
{
    utility::string_t customHeader = L"Testing custom header";
    auto header = std::make_pair<>(CustomHeaderName, customHeader);
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, sampleSearchResponse, header) };
    std::unordered_map<utility::string_t, utility::string_t> headers;
    headers.emplace(CustomHeaderName, customHeader);

    V1_1::Interface v1_1{ "https://restsource.com/api", std::move(helper) , {}, headers};
    Schema::IRestClient::SearchResult searchResponse = v1_1.Search({});
    REQUIRE(searchResponse.Matches.size() == 1);
    Schema::IRestClient::Package package = searchResponse.Matches.at(0);
}

TEST_CASE("RestSourceSearch_WhitespaceCustomHeader", "[RestSource][CustomHeader]")
{
    utility::string_t customHeader = L"    ";
    auto header = std::make_pair<>(CustomHeaderName, customHeader);
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, sampleSearchResponse, header) };
    std::unordered_map<utility::string_t, utility::string_t> headers;
    headers.emplace(CustomHeaderName, customHeader);

    V1_1::Interface v1_1{ "https://restsource.com/api", std::move(helper), {}, headers };
    Schema::IRestClient::SearchResult searchResponse = v1_1.Search({});
    REQUIRE(searchResponse.Matches.size() == 1);
}

TEST_CASE("RestSourceSearch_NoCustomHeader", "[RestSource][CustomHeader]")
{
    utility::string_t customHeader = L"    ";
    auto header = std::make_pair<>(CustomHeaderName, customHeader);
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, sampleSearchResponse, header) };
    std::unordered_map<utility::string_t, utility::string_t> headers;
    headers.emplace(CustomHeaderName, customHeader);

    V1_1::Interface v1_1{ "https://restsource.com/api", std::move(helper), {}, {} };
    REQUIRE_THROWS_HR(v1_1.Search({}), APPINSTALLER_CLI_ERROR_RESTAPI_INTERNAL_ERROR);
}

TEST_CASE("RestSourceSearch_CustomHeaderExceedingSize", "[RestSource][CustomHeader]")
{
    std::string customHeader = "This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. This is a custom header that is longer than 1024 characters. ";
    auto header = std::make_pair<>(CustomHeaderName, JSON::GetUtilityString(customHeader));
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, sampleSearchResponse, header) };

    REQUIRE_THROWS_HR(CreateRestClient(utility::conversions::to_utf8string("https://restsource.com/api"), customHeader, {}, helper),
        APPINSTALLER_CLI_ERROR_CUSTOMHEADER_EXCEEDS_MAXLENGTH);
}

TEST_CASE("RestClient_CustomUserAgentHeader", "[RestSource][CustomHeader]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.0.0",
                "2.0.0"]
        }})delimiter");

    std::string testCaller = "TestCaller";
    auto header = std::make_pair<>(web::http::header_names::user_agent, JSON::GetUtilityString(Runtime::GetUserAgent(testCaller)));
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, sample, header) };
    RestClient client = CreateRestClient(utility::conversions::to_utf8string("https://restsource.com/api"), {}, testCaller, helper);
    REQUIRE(client.GetSourceIdentifier() == "Source123");
}

TEST_CASE("RestClient_DefaultUserAgentHeader", "[RestSource][CustomHeader]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.0.0",
                "2.0.0"]
        }})delimiter");

    auto header = std::make_pair<>(web::http::header_names::user_agent, JSON::GetUtilityString(Runtime::GetDefaultUserAgent()));
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, sample, header) };
    RestClient client = CreateRestClient(utility::conversions::to_utf8string("https://restsource.com/api"), {}, {}, helper);
    REQUIRE(client.GetSourceIdentifier() == "Source123");
}
