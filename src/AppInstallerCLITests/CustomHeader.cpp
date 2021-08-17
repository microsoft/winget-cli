// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include "RestSourceTestHelper.h"
#include "TestRestRequestHandler.h"
#include <Rest/Schema/1_0/Interface.h>
#include <Rest/Schema/JsonHelper.h>
#include <Rest/Schema/IRestClient.h>
#include <Rest/RestClient.h>
#include <AppInstallerRepositorySource.h>
#include <AppInstallerErrors.h>
#include <winget/Settings.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Settings;
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

    std::shared_ptr<TestRestRequestHandler> GetCustomHeaderVerificationHandler(
        const web::http::status_code statusCode, const utility::string_t& sampleResponseString, const std::pair<utility::string_t, utility::string_t>& customHeader)
    {
        return std::make_shared<TestRestRequestHandler>([statusCode, sampleResponseString, customHeader](web::http::http_request request) ->
            pplx::task<web::http::http_response>
            {
                web::http::http_response response;
                auto& headers = request.headers();
                if (!headers.has(customHeader.first) ||
                    (utility::conversions::to_utf8string(customHeader.second).compare(utility::conversions::to_utf8string(headers[customHeader.first]))) != 0)
                {
                    response.set_status_code(web::http::status_codes::BadRequest);
                    return pplx::task_from_result(response);
                }

                if (!sampleResponseString.empty())
                {
                    response.set_body(web::json::value::parse(sampleResponseString));
                }

                response.headers().set_content_type(web::http::details::mime_types::application_json);
                response.set_status_code(statusCode);
                return pplx::task_from_result(response);
            });
    }
}

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
    auto header = std::make_pair<>(CustomHeaderName, JsonHelper::GetUtilityString(customHeader.value()));
    HttpClientHelper helper{ GetCustomHeaderVerificationHandler(web::http::status_codes::OK, sample, header) };
    RestClient client = RestClient::Create(utility::conversions::to_utf8string("https://restsource.com/api"), customHeader, std::move(helper));
    REQUIRE(client.GetSourceIdentifier() == "Source123");
}

TEST_CASE("RestSourceSearch_CustomHeader", "[RestSource][CustomHeader]")
{
    utility::string_t customHeader = L"Testing custom header";
    auto header = std::make_pair<>(CustomHeaderName, customHeader);
    HttpClientHelper helper{ GetCustomHeaderVerificationHandler(web::http::status_codes::OK, sampleSearchResponse, header) };
    std::unordered_map<utility::string_t, utility::string_t> headers;
    headers.emplace(CustomHeaderName, customHeader);

    Interface v1{ "https://restsource.com/api", headers, std::move(helper) };
    Schema::IRestClient::SearchResult searchResponse = v1.Search({});
    REQUIRE(searchResponse.Matches.size() == 1);
    Schema::IRestClient::Package package = searchResponse.Matches.at(0);
}

TEST_CASE("RestSourceSearch_WhitespaceCustomHeader", "[RestSource][CustomHeader]")
{
    utility::string_t customHeader = L"    ";
    auto header = std::make_pair<>(CustomHeaderName, customHeader);
    HttpClientHelper helper{ GetCustomHeaderVerificationHandler(web::http::status_codes::OK, sampleSearchResponse, header) };
    std::unordered_map<utility::string_t, utility::string_t> headers;
    headers.emplace(CustomHeaderName, customHeader);

    Interface v1{ "https://restsource.com/api", headers, std::move(helper) };
    Schema::IRestClient::SearchResult searchResponse = v1.Search({});
    REQUIRE(searchResponse.Matches.size() == 1);
}

TEST_CASE("RestSourceSearch_NoCustomHeader", "[RestSource][CustomHeader]")
{
    utility::string_t customHeader = L"    ";
    auto header = std::make_pair<>(CustomHeaderName, customHeader);
    HttpClientHelper helper{ GetCustomHeaderVerificationHandler(web::http::status_codes::OK, sampleSearchResponse, header) };
    std::unordered_map<utility::string_t, utility::string_t> headers;
    headers.emplace(CustomHeaderName, customHeader);

    Interface v1{ "https://restsource.com/api", {}, std::move(helper) };
    REQUIRE_THROWS_HR(v1.Search({}), APPINSTALLER_CLI_ERROR_RESTSOURCE_INTERNAL_ERROR);
}


TEST_CASE("AddSource_CustomHeader", "[RestSource][CustomHeader]")
{
    SetSetting(Streams::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string customHeader = "Testing custom header with open source";

    SourceDetails details;
    details.Name = "restsource";
    details.Type = "thisIsTheType";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";
    details.CustomHeader = customHeader;

    bool receivedCustomHeader = false;
    TestSourceFactory factory{ TestRestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { receivedCustomHeader = customHeader.compare(sd.CustomHeader.value()) == 0; };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    TestProgress progress;
    AddSource(details, progress);
    REQUIRE(receivedCustomHeader);
}

TEST_CASE("CreateSource_CustomHeader", "[RestSource][CustomHeader]")
{
    SetSetting(Streams::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string customHeader = "Testing custom header with open source";

    SourceDetails details;
    details.Name = "restsource";
    details.Type = "Microsoft.Rest";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";
    details.CustomHeader = customHeader;

    bool receivedCustomHeader = false;
    TestSourceFactory factory{ TestRestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { receivedCustomHeader = customHeader.compare(sd.CustomHeader.value()) == 0; };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    TestProgress progress;
    AddSource(details, progress);

    std::string customHeaderTwo = "Custom Header";
    auto source = OpenSource(details.Name, customHeaderTwo, progress).Source;
    REQUIRE(customHeaderTwo.compare(source.get()->GetDetails().CustomHeader.value_or("")) == 0);
}

TEST_CASE("CreateSource_CustomHeaderNotApplicable", "[RestSource][CustomHeader]")
{
    SetSetting(Streams::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string customHeader = "Testing custom header with open source";

    SourceDetails details;
    details.Name = "restsource";
    details.Type = "thisIsTheType";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";
    details.CustomHeader = customHeader;

    bool receivedCustomHeader = false;
    TestSourceFactory factory{ TestRestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { receivedCustomHeader = customHeader.compare(sd.CustomHeader.value()) == 0; };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    TestProgress progress;
    AddSource(details, progress);

    std::string customHeaderTwo = "Custom Header";
    auto source = OpenSource(details.Name, customHeaderTwo, progress).Source;
    REQUIRE(!source.get()->GetDetails().CustomHeader.has_value());
}
