// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <Rest/Schema/1_1/Interface.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <AppInstallerErrors.h>

using namespace TestCommon;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_1;

namespace
{
    const std::string TestRestUriString = "http://restsource.com/api";

    IRestClient::Information GetTestSourceInformation()
    {
        IRestClient::Information result;

        result.RequiredPackageMatchFields.emplace_back("Market");
        result.RequiredQueryParameters.emplace_back("Market");
        result.UnsupportedPackageMatchFields.emplace_back("Moniker");
        result.UnsupportedQueryParameters.emplace_back("Channel");

        return result;
    }
}

TEST_CASE("Search_BadResponse_UnsupportedPackageMatchFields", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : [],
            "UnsupportedPackageMatchFields" : [ "Moniker" ]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), std::move(helper) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Name, MatchType::Exact, "Foo" };
    request.Filters.emplace_back(std::move(filter));
    REQUIRE_THROWS_HR(v1_1.Search(request), APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST);
}

TEST_CASE("Search_BadResponse_RequiredPackageMatchFields", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : [],
            "RequiredPackageMatchFields" : [ "Moniker" ]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), std::move(helper) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Name, MatchType::Exact, "Foo" };
    request.Filters.emplace_back(std::move(filter));
    REQUIRE_THROWS_HR(v1_1.Search(request), APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST);
}

TEST_CASE("GetManifests_BadResponse_UnsupportedQueryParameters", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : null,
            "UnsupportedQueryParameters" : [ "Channel" ]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), std::move(helper) };
    REQUIRE_THROWS_HR(v1_1.GetManifests("Foo"), APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST);
}

TEST_CASE("GetManifests_BadResponse_RequiredQueryParameters", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : null,
            "RequiredQueryParameters" : [ "Version" ]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), std::move(helper) };
    REQUIRE_THROWS_HR(v1_1.GetManifests("Foo"), APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST);
}

TEST_CASE("Search_BadRequest_UnsupportedPackageMatchFields", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : [
               {
              "PackageIdentifier": "git.package",
              "PackageName": "package",
              "Publisher": "git",
              "Versions": [
                {   "PackageVersion": "1.0.0" },
                {   "PackageVersion": "2.0.0"}]
            }]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), std::move(helper) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Moniker, MatchType::Exact, "Foo" };
    request.Filters.emplace_back(std::move(filter));
    REQUIRE_THROWS_HR(v1_1.Search(request), APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST);
}

TEST_CASE("Search_GoodRequest_OnlyMarketRequired", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : [
               {
              "PackageIdentifier": "git.package",
              "PackageName": "package",
              "Publisher": "git",
              "Versions": [
                {   "PackageVersion": "1.0.0" },
                {   "PackageVersion": "2.0.0"}]
            }]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), std::move(helper) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Name, MatchType::Exact, "Foo" };
    request.Filters.emplace_back(std::move(filter));
    Schema::IRestClient::SearchResult searchResponse = v1_1.Search(request);
    REQUIRE(searchResponse.Matches.size() == 1);
    Schema::IRestClient::Package package = searchResponse.Matches.at(0);
    REQUIRE(package.PackageInformation.PackageIdentifier.compare("git.package") == 0);
    REQUIRE(package.PackageInformation.Publisher.compare("git") == 0);
    REQUIRE(package.PackageInformation.PackageName.compare("package") == 0);
    REQUIRE(package.Versions.size() == 2);
    REQUIRE(package.Versions.at(0).VersionAndChannel.GetVersion().ToString().compare("1.0.0") == 0);
    REQUIRE(package.Versions.at(1).VersionAndChannel.GetVersion().ToString().compare("2.0.0") == 0);
}

TEST_CASE("GetManifests_BadRequest_UnsupportedQueryParameters", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
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

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), std::move(helper) };
    REQUIRE_THROWS_HR(v1_1.GetManifestByVersion("Foo", "1.0", "beta"), APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST);
}

TEST_CASE("GetManifests_GoodRequest_OnlyMarketRequired", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
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

    IRestClient::Information info = GetTestSourceInformation();
    info.UnsupportedQueryParameters.clear();
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, info, std::move(helper) };
    auto manifestResult = v1_1.GetManifestByVersion("Foo", "5.0.0", "");
    REQUIRE(manifestResult.has_value());
    const Manifest& manifest = manifestResult.value();
    REQUIRE(manifest.Id == "Foo.Bar");
    REQUIRE(manifest.Version == "5.0.0");
    REQUIRE(manifest.DefaultLocalization.Locale == "en-us");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Publisher>() == "Foo");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == "Bar");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::License>() == "Foo bar license");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::ShortDescription>() == "Foo bar description");
    REQUIRE(manifest.Installers.size() == 1);
    REQUIRE(manifest.Installers[0].Arch == Architecture::X64);
    REQUIRE(manifest.Installers[0].Sha256 == AppInstaller::Utility::SHA256::ConvertToBytes("011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6"));
    REQUIRE(manifest.Installers[0].InstallerType == InstallerTypeEnum::Exe);
    REQUIRE(manifest.Installers[0].Url == "https://installer.example.com/foobar.exe");
}

TEST_CASE("GetManifests_GoodResponse_MSStoreType", "[RestSource][Interface_1_1]")
{
    utility::string_t msstoreInstallerResponse = _XPLATSTR(
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
                            "InstallerType": "msstore",
                            "MSStoreProductIdentifier": "9nblggh4nns1"
                        }
                    ]
                }
            ]
        }
    })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(msstoreInstallerResponse)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), std::move(helper) };
    std::vector<Manifest> manifests = v1_1.GetManifests("Foo.Bar");
    REQUIRE(manifests.size() == 1);

    // Verify manifest is populated and manifest validation passed
    Manifest manifest = manifests[0];
    REQUIRE(manifest.Installers.size() == 1);
    REQUIRE(manifest.Installers.at(0).InstallerType == InstallerTypeEnum::MSStore);
    REQUIRE(manifest.Installers.at(0).ProductId == "9nblggh4nns1");
}