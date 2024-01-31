// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "TestRestRequestHandler.h"
#include <Rest/Schema/1_7/Interface.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <AppInstallerErrors.h>
#include <winget/Authentication.h>
#include <winget/JsonUtil.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Authentication;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_7;

namespace
{
    const std::string TestRestUriString = "http://restsource.com/api";

    IRestClient::Information GetTestSourceInformation()
    {
        IRestClient::Information result;

        result.Authentication.Type = AuthenticationType::MicrosoftEntraId;
        MicrosoftEntraIdAuthenticationInfo microsoftEntraIdInfo;
        microsoftEntraIdInfo.Resource = "GUID";
        result.Authentication.MicrosoftEntraIdInfo = std::move(microsoftEntraIdInfo);

        return result;
    }

    AuthenticationArguments GetTestAuthenticationArguments()
    {
        AuthenticationArguments result;
        result.Mode = AuthenticationMode::Silent;
        return result;
    }

    utility::string_t SampleSearchResponse = _XPLATSTR(
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

    utility::string_t SampleGetManifestResponse = _XPLATSTR(
        R"delimiter({
        "Data": {
            "PackageIdentifier": "Foo.Bar",
            "Versions": [
                {
                    "PackageVersion": "5.0.0",
                    "DefaultLocale": {
                        "PackageLocale": "en-us",
                        "Publisher": "Foo",
                        "PackageName": "Bar",
                        "License": "Foo bar license",
                        "ShortDescription": "Foo bar description"
                    },
                    "Installers": [
                        {
                            "Architecture": "x64",
                            "InstallerSha256": "011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6",
                            "InstallerType": "exe",
                            "InstallerUrl": "https://installer.example.com/foobar.exe"
                        }
                    ]
                }
            ]
        }
    })delimiter");
}

TEST_CASE("GetManifests_GoodRequest_Authentication", "[RestSource][Interface_1_7]")
{
    std::string expectedToken = "TestToken";

    // Set good authentication result
    AuthenticationResult authResultOverride;
    authResultOverride.Status = S_OK;
    authResultOverride.Token = expectedToken;
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    // GetManifest should succeed with expected value.
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, SampleGetManifestResponse, { web::http::header_names::authorization, JSON::GetUtilityString(CreateBearerToken(expectedToken)) }, web::http::status_codes::Unauthorized) };
    Interface v1_7{ TestRestUriString, GetTestSourceInformation(), {}, GetTestAuthenticationArguments(), std::move(helper) };
    auto manifestResult = v1_7.GetManifestByVersion("Foo.Bar", "5.0.0", "");
    REQUIRE(manifestResult.has_value());
    const auto& manifest = manifestResult.value();
    REQUIRE(manifest.Id == "Foo.Bar");
    REQUIRE(manifest.Version == "5.0.0");
}

TEST_CASE("GetManifests_BadRequest_AuthenticationFailed", "[RestSource][Interface_1_7]")
{
    std::string expectedToken = "TestToken";

    // Set authentication failed result
    AuthenticationResult authResultOverride;
    authResultOverride.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED;
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    // GetManifest should fail with authentication failure
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, SampleGetManifestResponse, { web::http::header_names::authorization, JSON::GetUtilityString(CreateBearerToken(expectedToken)) }, web::http::status_codes::Unauthorized) };
    Interface v1_7{ TestRestUriString, GetTestSourceInformation(), {}, GetTestAuthenticationArguments(), std::move(helper) };
    REQUIRE_THROWS_HR(v1_7.GetManifestByVersion("Foo.Bar", "5.0.0", ""), APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED);
}

TEST_CASE("GetManifests_BadRequest_InvalidAuthenticationToken", "[RestSource][Interface_1_7]")
{
    std::string expectedToken = "TestToken";

    // Set authentication result with incorrect token
    AuthenticationResult authResultOverride;
    authResultOverride.Status = S_OK;
    authResultOverride.Token = "OtherToken";
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    // GetManifest should fail with access denied
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, SampleGetManifestResponse, { web::http::header_names::authorization, JSON::GetUtilityString(CreateBearerToken(expectedToken)) }, web::http::status_codes::Unauthorized) };
    Interface v1_7{ TestRestUriString, GetTestSourceInformation(), {}, GetTestAuthenticationArguments(), std::move(helper) };
    REQUIRE_THROWS_HR(v1_7.GetManifestByVersion("Foo.Bar", "5.0.0", ""), HTTP_E_STATUS_DENIED);
}

TEST_CASE("Search_GoodRequest_Authentication", "[RestSource][Interface_1_7]")
{
    std::string expectedToken = "TestToken";

    // Set good authentication result
    AuthenticationResult authResultOverride;
    authResultOverride.Status = S_OK;
    authResultOverride.Token = expectedToken;
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    // Search should succeed with expected value.
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, SampleSearchResponse, { web::http::header_names::authorization, JSON::GetUtilityString(CreateBearerToken(expectedToken)) }, web::http::status_codes::Unauthorized) };
    Interface v1_7{ TestRestUriString, GetTestSourceInformation(), {}, GetTestAuthenticationArguments(), std::move(helper) };
    SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Name, MatchType::Exact, "package" };
    request.Filters.emplace_back(std::move(filter));
    IRestClient::SearchResult searchResponse = v1_7.Search(request);
    REQUIRE(searchResponse.Matches.size() == 1);
    IRestClient::Package package = searchResponse.Matches.at(0);
    REQUIRE(package.PackageInformation.PackageIdentifier.compare("git.package") == 0);
}

TEST_CASE("Search_BadRequest_AuthenticationFailed", "[RestSource][Interface_1_7]")
{
    std::string expectedToken = "TestToken";

    // Set authentication failed result
    AuthenticationResult authResultOverride;
    authResultOverride.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED;
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    // Search should fail with authentication failure
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, SampleSearchResponse, { web::http::header_names::authorization, JSON::GetUtilityString(CreateBearerToken(expectedToken)) }, web::http::status_codes::Unauthorized) };
    Interface v1_7{ TestRestUriString, GetTestSourceInformation(), {}, GetTestAuthenticationArguments(), std::move(helper) };
    SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Name, MatchType::Exact, "package" };
    request.Filters.emplace_back(std::move(filter));
    REQUIRE_THROWS_HR(v1_7.Search(request), APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED);
}

TEST_CASE("Search_BadRequest_InvalidAuthenticationToken", "[RestSource][Interface_1_7]")
{
    std::string expectedToken = "TestToken";

    // Set authentication result with incorrect token
    AuthenticationResult authResultOverride;
    authResultOverride.Status = S_OK;
    authResultOverride.Token = "OtherToken";
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    // Search should fail with access denied
    HttpClientHelper helper{ GetHeaderVerificationHandler(web::http::status_codes::OK, SampleSearchResponse, { web::http::header_names::authorization, JSON::GetUtilityString(CreateBearerToken(expectedToken)) }, web::http::status_codes::Unauthorized) };
    Interface v1_7{ TestRestUriString, GetTestSourceInformation(), {}, GetTestAuthenticationArguments(), std::move(helper) };
    SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Name, MatchType::Exact, "package" };
    request.Filters.emplace_back(std::move(filter));
    REQUIRE_THROWS_HR(v1_7.Search(request), HTTP_E_STATUS_DENIED);
}