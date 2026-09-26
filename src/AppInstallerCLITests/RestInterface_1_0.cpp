// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <Rest/RestSource.h>
#include <Rest/Schema/1_0/Interface.h>
#include <Rest/Schema/1_4/Interface.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <AppInstallerErrors.h>
#include <AppInstallerRuntime.h>
#include <winget/ManifestValidation.h>
#include <AppInstallerSHA256.h>

using namespace TestCommon;
using namespace AppInstaller::Http;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_0;

namespace
{
    const std::string TestRestUriString = "http://restsource.com/api";

    utility::string_t GetSearchResponse_PackageIds(
        std::initializer_list<utility::string_t> identifiers, const utility::string_t& continuationToken = {})
    {
        web::json::value response;
        response[L"Data"] = web::json::value::array();
        size_t index = 0;
        for (const auto& identifier : identifiers)
        {
            web::json::value package;
            package[L"PackageIdentifier"] = web::json::value::string(identifier);
            package[L"PackageName"] = web::json::value::string(L"Microsoft Teams");
            package[L"Publisher"] = web::json::value::string(L"Microsoft");
            package[L"Versions"][0][L"PackageVersion"] = web::json::value::string(L"1.0.0");
            response[L"Data"][index++] = std::move(package);
        }

        if (!continuationToken.empty())
        {
            response[L"ContinuationToken"] = web::json::value::string(continuationToken);
        }

        return response.serialize();
    }

    struct CachedMetadataInterface : Interface
    {
        using Interface::Interface;
        std::vector<IRestClient::VersionInfo> Versions;

    protected:
        IRestClient::SearchResult GetSearchResult(const web::json::value& response) const override
        {
            auto result = Interface::GetSearchResult(response);
            result.Matches.at(0).Versions = Versions;
            return result;
        }
    };

    utility::string_t GetGoodManifest_RequiredFields()
    {
        return _XPLATSTR(
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

    utility::string_t GetManifestsResponse_MultipleVersions()
    {
        return _XPLATSTR(
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
                },
                {
                    "PackageVersion": "6.0.0",
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

    struct SearchAndManifestResponses
    {
        web::json::value SearchResponse = web::json::value::parse(GetSearchResponse_PackageIds({ L"Foo.Bar" }));
        web::json::value ManifestResponse = web::json::value::parse(GetGoodManifest_RequiredFields());
        web::http::status_code ManifestStatus = web::http::status_codes::OK;
        size_t SearchRequests = 0;
        size_t ManifestRequests = 0;
        web::http::http_request LastManifestRequest;

        SearchAndManifestResponses()
        {
            ManifestResponse[L"Data"][L"Versions"][0][L"PackageVersion"] = web::json::value::string(L"1.0.0");
        }

        void SetManifestNotFound()
        {
            ManifestStatus = web::http::status_codes::NotFound;
            ManifestResponse = web::json::value::parse(LR"({"code":"DataNotFound","message":"Not found"})");
        }

        std::shared_ptr<TestRestRequestHandler> GetHandler()
        {
            return std::make_shared<TestRestRequestHandler>(
                [this](web::http::http_request request) -> pplx::task<web::http::http_response>
                {
                    web::http::http_response response{ web::http::status_codes::BadRequest };
                    response.headers().set_content_type(web::http::details::mime_types::application_json);
                    response.headers().set_cache_control(L"no-store");
                    if (request.method() == web::http::methods::POST)
                    {
                        ++SearchRequests;
                        response.set_status_code(web::http::status_codes::OK);
                        response.set_body(SearchResponse);
                    }
                    else if (request.method() == web::http::methods::GET)
                    {
                        ++ManifestRequests;
                        LastManifestRequest = request;
                        response.set_status_code(ManifestStatus);
                        response.set_body(ManifestResponse);
                    }
                    return pplx::task_from_result(response);
                });
        }
    };

    struct GoodManifest_AllFields
    {
        utility::string_t GetSampleManifest_AllFields()
        {
            return _XPLATSTR(
                R"delimiter(
        {
          "Data": {
            "PackageIdentifier": "Foo.Bar",
            "Versions": [
              {
                "PackageVersion": "3.0.0abc",
                "DefaultLocale": {
                  "PackageLocale": "en-US",
                  "Publisher": "Foo",
                  "PublisherUrl": "http://publisher.net",
                  "PublisherSupportUrl": "http://publisherSupport.net",
                  "PrivacyUrl": "http://packagePrivacyUrl.net",
                  "Author": "FooBar",
                  "PackageName": "Bar",
                  "PackageUrl": "http://packageUrl.net",
                  "License": "Foo Bar License",
                  "LicenseUrl": "http://licenseUrl.net",
                  "Copyright": "Foo Bar Copyright",
                  "CopyrightUrl": "http://copyrightUrl.net",
                  "ShortDescription": "Foo bar is a foo bar.",
                  "Description": "Foo bar is a placeholder.",
                  "Tags": [
                    "FooBar",
                    "Foo",
                    "Bar"
                  ],
                  "Moniker": "FooBarMoniker"
                },
                "Channel": "",
                "Locales": [
                  {
                    "PackageLocale": "fr-Fr",
                    "Publisher": "Foo French",
                    "PublisherUrl": "http://publisher-fr.net",
                    "PublisherSupportUrl": "http://publisherSupport-fr.net",
                    "PrivacyUrl": "http://packagePrivacyUrl-fr.net",
                    "Author": "FooBar French",
                    "PackageName": "Bar",
                    "PackageUrl": "http://packageUrl-fr.net",
                    "License": "Foo Bar License",
                    "LicenseUrl": "http://licenseUrl-fr.net",
                    "Copyright": "Foo Bar Copyright",
                    "CopyrightUrl": "http://copyrightUrl-fr.net",
                    "ShortDescription": "Foo bar is a foo bar French.",
                    "Description": "Foo bar is a placeholder French.",
                    "Tags": [
                      "FooBarFr",
                      "FooFr",
                      "BarFr"
                    ]
                  }
                ],
                "Installers": [
                  {
                    "InstallerSha256": "011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6",
                    "InstallerUrl": "http://foobar.exe",
                    "Architecture": "x86",
                    "InstallerLocale": "en-US",
                    "Platform": [
                      "Windows.Desktop"
                    ],
                    "MinimumOSVersion": "1078",
                    "InstallerType": "msix",
                    "Scope": "user",
                    "SignatureSha256": "011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6",
                    "InstallModes": [
                      "interactive"
                    ],
                    "InstallerSwitches": {
                      "Silent": "/s",
                      "SilentWithProgress": "/s",
                      "Interactive": "/i",
                      "InstallLocation": "C:\\Users\\User1",
                      "Log": "/l",
                      "Upgrade": "/u",
                      "Custom": "/custom"
                    },
                    "InstallerSuccessCodes": [
                      0
                    ],
                    "UpgradeBehavior": "install",
                    "Commands": [
                      "command1"
                    ],
                    "Protocols": [
                       "protocol1"
                    ],
                    "FileExtensions": [
                      ".file-extension"
                    ],
                    "Dependencies": {
                      "WindowsFeatures": [
                        "feature1"
                      ],
                      "WindowsLibraries": [
                        "library1"
                      ],
                      "PackageDependencies": [
                        {
                          "PackageIdentifier": "Foo.Baz",
                          "MinimumVersion": "2.0.0"
                        }
                      ],
                      "ExternalDependencies": [
                        "FooBarBaz"
                      ]
                    },
                    "PackageFamilyName": "FooBar.PackageFamilyName",
                    "ProductCode": "",
                    "Capabilities": [
                      "Bluetooth"
                    ],
                    "RestrictedCapabilities": [
                      "restrictedCapability"
                    ]
                  }
                ]
              }
            ]
          },
          "ContinuationToken": "abcd"
        })delimiter");
        }

        void VerifyLocalizations_AllFields(const Manifest& manifest)
        {
            REQUIRE(manifest.DefaultLocalization.Locale == "en-US");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Publisher>() == "Foo");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::PublisherUrl>() == "http://publisher.net");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::PublisherSupportUrl>() == "http://publisherSupport.net");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::PrivacyUrl>() == "http://packagePrivacyUrl.net");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Author>() == "FooBar");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == "Bar");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageUrl>() == "http://packageUrl.net");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::License>() == "Foo Bar License");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::LicenseUrl>() == "http://licenseUrl.net");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Copyright>() == "Foo Bar Copyright");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::CopyrightUrl>() == "http://copyrightUrl.net");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::ShortDescription>() == "Foo bar is a foo bar.");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Description>() == "Foo bar is a placeholder.");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Tags>().size() == 3);
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Tags>().at(0) == "FooBar");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Tags>().at(1) == "Foo");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Tags>().at(2) == "Bar");

            REQUIRE(manifest.Localizations.size() == 1);
            ManifestLocalization frenchLocalization = manifest.Localizations.at(0);
            REQUIRE(frenchLocalization.Locale == "fr-Fr");
            REQUIRE(frenchLocalization.Get<Localization::Publisher>() == "Foo French");
            REQUIRE(frenchLocalization.Get<Localization::PublisherUrl>() == "http://publisher-fr.net");
            REQUIRE(frenchLocalization.Get<Localization::PublisherSupportUrl>() == "http://publisherSupport-fr.net");
            REQUIRE(frenchLocalization.Get<Localization::PrivacyUrl>() == "http://packagePrivacyUrl-fr.net");
            REQUIRE(frenchLocalization.Get<Localization::Author>() == "FooBar French");
            REQUIRE(frenchLocalization.Get<Localization::PackageName>() == "Bar");
            REQUIRE(frenchLocalization.Get<Localization::PackageUrl>() == "http://packageUrl-fr.net");
            REQUIRE(frenchLocalization.Get<Localization::License>() == "Foo Bar License");
            REQUIRE(frenchLocalization.Get<Localization::LicenseUrl>() == "http://licenseUrl-fr.net");
            REQUIRE(frenchLocalization.Get<Localization::Copyright>() == "Foo Bar Copyright");
            REQUIRE(frenchLocalization.Get<Localization::CopyrightUrl>() == "http://copyrightUrl-fr.net");
            REQUIRE(frenchLocalization.Get<Localization::ShortDescription>() == "Foo bar is a foo bar French.");
            REQUIRE(frenchLocalization.Get<Localization::Description>() == "Foo bar is a placeholder French.");
            REQUIRE(frenchLocalization.Get<Localization::Tags>().size() == 3);
            REQUIRE(frenchLocalization.Get<Localization::Tags>().at(0) == "FooBarFr");
            REQUIRE(frenchLocalization.Get<Localization::Tags>().at(1) == "FooFr");
            REQUIRE(frenchLocalization.Get<Localization::Tags>().at(2) == "BarFr");
        }

        void VerifyInstallers_AllFields(const Manifest& manifest)
        {
            REQUIRE(manifest.Installers.size() == 1);

            ManifestInstaller actualInstaller = manifest.Installers.at(0);
            REQUIRE(actualInstaller.Sha256 == AppInstaller::Utility::SHA256::ConvertToBytes("011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6"));
            REQUIRE(actualInstaller.Url == "http://foobar.exe");
            REQUIRE(actualInstaller.Arch == Architecture::X86);
            REQUIRE(actualInstaller.Locale == "en-US");
            REQUIRE(actualInstaller.Platform.size() == 1);
            REQUIRE(actualInstaller.Platform[0] == PlatformEnum::Desktop);
            REQUIRE(actualInstaller.MinOSVersion == "1078");
            REQUIRE(actualInstaller.BaseInstallerType == InstallerTypeEnum::Msix);
            REQUIRE(actualInstaller.Scope == ScopeEnum::User);
            REQUIRE(actualInstaller.SignatureSha256 == AppInstaller::Utility::SHA256::ConvertToBytes("011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6"));
            REQUIRE(actualInstaller.InstallModes.size() == 1);
            REQUIRE(actualInstaller.InstallModes.at(0) == InstallModeEnum::Interactive);
            REQUIRE(actualInstaller.Switches.size() == 7);
            REQUIRE(actualInstaller.Switches.at(InstallerSwitchType::Silent) == "/s");
            REQUIRE(actualInstaller.Switches.at(InstallerSwitchType::SilentWithProgress) == "/s");
            REQUIRE(actualInstaller.Switches.at(InstallerSwitchType::Interactive) == "/i");
            REQUIRE(actualInstaller.Switches.at(InstallerSwitchType::InstallLocation) == "C:\\Users\\User1");
            REQUIRE(actualInstaller.Switches.at(InstallerSwitchType::Log) == "/l");
            REQUIRE(actualInstaller.Switches.at(InstallerSwitchType::Update) == "/u");
            REQUIRE(actualInstaller.Switches.at(InstallerSwitchType::Custom) == "/custom");
            REQUIRE(actualInstaller.InstallerSuccessCodes.size() == 1);
            REQUIRE(actualInstaller.InstallerSuccessCodes.at(0) == 0);
            REQUIRE(actualInstaller.UpdateBehavior == UpdateBehaviorEnum::Install);
            REQUIRE(actualInstaller.Commands.at(0) == "command1");
            REQUIRE(actualInstaller.Protocols.at(0) == "protocol1");
            REQUIRE(actualInstaller.FileExtensions.at(0) == ".file-extension");
            REQUIRE(actualInstaller.Dependencies.HasExactDependency(DependencyType::WindowsFeature, "feature1"));
            REQUIRE(actualInstaller.Dependencies.HasExactDependency(DependencyType::WindowsLibrary, "library1"));
            REQUIRE(actualInstaller.Dependencies.HasExactDependency(DependencyType::Package, "Foo.Baz", "2.0.0"));
            REQUIRE(actualInstaller.Dependencies.HasExactDependency(DependencyType::External, "FooBarBaz"));
            REQUIRE(actualInstaller.PackageFamilyName == "FooBar.PackageFamilyName");
            REQUIRE(actualInstaller.ProductCode == "");
            REQUIRE(actualInstaller.Capabilities.at(0) == "Bluetooth");
            REQUIRE(actualInstaller.RestrictedCapabilities.at(0) == "restrictedCapability");
        }
    };
}

TEST_CASE("Search_GoodResponse", "[RestSource][Interface_1_0]")
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
                {   "PackageVersion": "2.0.0" }]
            }]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult searchResponse = v1.Search({});
    REQUIRE(searchResponse.Matches.size() == 1);
    Schema::IRestClient::Package package = searchResponse.Matches.at(0);
    REQUIRE(package.PackageInformation.PackageIdentifier.compare("git.package") == 0);
    REQUIRE(package.PackageInformation.Publisher.compare("git") == 0);
    REQUIRE(package.PackageInformation.PackageName.compare("package") == 0);
    REQUIRE(package.Versions.size() == 2);
    REQUIRE(package.Versions.at(0).VersionAndChannel.GetVersion().ToString().compare("1.0.0") == 0);
    REQUIRE(package.Versions.at(1).VersionAndChannel.GetVersion().ToString().compare("2.0.0") == 0);
}

TEST_CASE("Search_GoodResponse_AllFields", "[RestSource][Interface_1_0]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : [
               {
              "PackageIdentifier": "git.package",
              "PackageName": "package",
              "Publisher": "git",
              "Versions": [
                {
                    "PackageVersion": "1.0.0",
                    "PackageFamilyNames" : [
                        "pfn1",
                        "pfn2",
                        "pfn2"
                    ],
                    "ProductCodes" : [
                        "pc1",
                        "pc2"
                    ]
                }]
            }]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult searchResponse = v1.Search({});
    REQUIRE(searchResponse.Matches.size() == 1);
    Schema::IRestClient::Package package = searchResponse.Matches.at(0);
    REQUIRE(package.PackageInformation.PackageIdentifier.compare("git.package") == 0);
    REQUIRE(package.PackageInformation.Publisher.compare("git") == 0);
    REQUIRE(package.PackageInformation.PackageName.compare("package") == 0);
    REQUIRE(package.Versions.size() == 1);
    REQUIRE(package.Versions.at(0).VersionAndChannel.GetVersion().ToString().compare("1.0.0") == 0);
    REQUIRE(package.Versions.at(0).PackageFamilyNames.size() == 2);
    REQUIRE(package.Versions.at(0).PackageFamilyNames.at(0) == "pfn1");
    REQUIRE(package.Versions.at(0).PackageFamilyNames.at(1) == "pfn2");
    REQUIRE(package.Versions.at(0).ProductCodes.at(0) == "pc1");
    REQUIRE(package.Versions.at(0).ProductCodes.at(1) == "pc2");
}

TEST_CASE("Search_GoodResponse_404AsEmpty", "[RestSource][Interface_1_0]")
{
    utility::string_t notFoundResponse = _XPLATSTR(
        R"delimiter({"code":"DataNotFound","data":[],"details":[],"innererror":{"code":"DataNotFound","data":[],"details":[],"message":"Product is not present","source":"StoreEdgeFD"},"message":"Product is not present","source":"StoreEdgeFD"})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound, std::move(notFoundResponse)) };
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult searchResponse = v1.Search({});
    REQUIRE(searchResponse.Matches.size() == 0);
}

TEST_CASE("Search_ExplicitIdFilters", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK,
        GetSearchResponse_PackageIds({ L"XP8BT8DW290MPQ", L"Microsoft.Teams", L"Microsoft.Teams.Preview" })) };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::CaseInsensitive, "Microsoft Teams");
    std::vector<std::string> expected{ "Microsoft.Teams" };

    SECTION("Exact")
    {
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Microsoft.Teams");
    }
    SECTION("Exact case mismatch")
    {
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "microsoft.teams");
        expected.clear();
    }
    SECTION("Case insensitive")
    {
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, "microsoft.teams");
    }
    SECTION("Starts with")
    {
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::StartsWith, "microsoft.teams");
        expected.emplace_back("Microsoft.Teams.Preview");
    }
    SECTION("Substring")
    {
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::Substring, "teams");
        expected.emplace_back("Microsoft.Teams.Preview");
    }
    SECTION("All ID filters must match")
    {
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Microsoft.Teams");
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Microsoft.Teams.Preview");
        expected.clear();
    }
    SECTION("Query and inclusions cannot override a failed filter")
    {
        request.Query.emplace(MatchType::Substring, "Teams");
        request.Inclusions.emplace_back(PackageMatchField::Name, MatchType::Exact, "Microsoft Teams");
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Other.Package");
        expected.clear();
    }

    auto result = v1.Search(request);
    REQUIRE(result.Matches.size() == expected.size());
    REQUIRE_FALSE(result.Truncated);
    for (size_t i = 0; i < expected.size(); ++i)
    {
        REQUIRE(result.Matches[i].PackageInformation.PackageIdentifier == expected[i]);
    }
}

TEST_CASE("Search_ExplicitIdFilters_UnicodePrefix", "[RestSource][Interface_1_0]")
{
    std::wstring id = GENERATE(L"Vendor.\u1E9EApp", L"Vendor.\u00DFApp", L"Vendor.SSApp");
    std::string prefix = GENERATE(u8"vendor.\u00DF", u8"vendor.\u1E9E", "vendor.ss");
    CAPTURE(id, prefix);
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK,
        GetSearchResponse_PackageIds({ L"Vendor.Other", id })) };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::StartsWith, prefix);
    request.MaximumResults = 1;

    auto result = v1.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == ConvertToUTF8(id));
    REQUIRE_FALSE(result.Truncated);
}

TEST_CASE("Search_IdInclusions", "[RestSource][Interface_1_0]")
{
    SearchAndManifestResponses responses;
    responses.SearchResponse = web::json::value::parse(GetSearchResponse_PackageIds({ L"Foo.Bar", L"Foo.Baz", L"Other.Package" }));
    responses.SetManifestNotFound();
    HttpClientHelper helper{ responses.GetHandler() };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, "Foo.Bar");
    const std::vector<std::string> allIds{ "Foo.Bar", "Foo.Baz", "Other.Package" };
    std::vector<std::string> expected{ "Foo.Bar" };
    size_t expectedManifestRequests = 0;

    SECTION("Matching inclusion") {}
    SECTION("Any inclusion may match")
    {
        request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, "Other.Package");
        expected.emplace_back("Other.Package");
    }
    SECTION("No matching inclusion")
    {
        request.Inclusions[0].Value = "Missing.Package";
        expected.clear();
    }
    SECTION("Filters narrow matching inclusions")
    {
        request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, "Other.Package");
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::StartsWith, "Foo.");
    }
    SECTION("Matching filters cannot override failed inclusions")
    {
        request.Inclusions[0].Value = "Missing.Package";
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Foo.Bar");
        expected.clear();
    }
    SECTION("Unknown inclusion preserves candidates")
    {
        request.Inclusions.emplace_back(PackageMatchField::Name, MatchType::Exact, "Localized name");
        expected = allIds;
        expectedManifestRequests = 2;
    }
    SECTION("Unknown filter does not disable inclusion matching")
    {
        request.Filters.emplace_back(PackageMatchField::Name, MatchType::Exact, "Localized name");
        expectedManifestRequests = 1;
    }
    SECTION("A query may select independently of inclusions")
    {
        request.Query.emplace(MatchType::Substring, "Source-defined query");
        expected = allIds;
    }
    SECTION("Unsupported match types remain unknown")
    {
        request.Inclusions[0].Type = GENERATE(MatchType::Fuzzy, MatchType::FuzzySubstring, MatchType::Wildcard);
        expected = allIds;
    }

    auto result = v1.Search(request);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == expectedManifestRequests);
    REQUIRE(result.Matches.size() == expected.size());
    REQUIRE_FALSE(result.Truncated);
    for (size_t i = 0; i < expected.size(); ++i)
    {
        REQUIRE(result.Matches[i].PackageInformation.PackageIdentifier == expected[i]);
    }
}

TEST_CASE("Search_CachedManifestMetadata", "[RestSource][Interface_1_0]")
{
    auto field = GENERATE(PackageMatchField::Name, PackageMatchField::Moniker, PackageMatchField::Tag,
        PackageMatchField::Command, PackageMatchField::PackageFamilyName, PackageMatchField::ProductCode,
        PackageMatchField::UpgradeCode);
    bool useInclusions = GENERATE(false, true);
    CAPTURE(ToString(field), useInclusions);
    SearchAndManifestResponses responses;
    responses.SetManifestNotFound();
    HttpClientHelper helper{ responses.GetHandler() };
    CachedMetadataInterface v1{ TestRestUriString, helper };
    auto createManifest = [](std::string_view version, const NormalizedString& value)
    {
        Manifest manifest;
        manifest.Id = "Foo.Bar";
        manifest.Version = version;
        manifest.Moniker = value;
        manifest.DefaultLocalization.Add<Localization::PackageName>(value);
        manifest.DefaultLocalization.Add<Localization::Tags>({ value });
        auto& installer = manifest.Installers.emplace_back();
        installer.Commands.emplace_back(value);
        installer.PackageFamilyName = value;
        installer.ProductCode = value;
        installer.AppsAndFeaturesEntries.emplace_back().UpgradeCode = value;
        return manifest;
    };
    v1.Versions.emplace_back(VersionAndChannel{ Version{ "1.0.0" }, Channel{} }, createManifest("1.0.0", "Other"));
    v1.Versions.emplace_back(VersionAndChannel{ Version{ "2.0.0" }, Channel{} }, createManifest("2.0.0", "Other"));
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Foo.Bar");
    auto& criteria = useInclusions ? request.Inclusions : request.Filters;
    criteria.emplace_back(field, MatchType::Exact, "Match");
    size_t expectedCount = 0;
    size_t expectedManifestRequests = 0;

    SECTION("All cached versions mismatch") {}
    SECTION("An earlier cached version matches")
    {
        v1.Versions[0].Manifest = createManifest("1.0.0", "Match");
        expectedCount = 1;
    }
    SECTION("A later cached version matches")
    {
        v1.Versions[1].Manifest = createManifest("2.0.0", "Match");
        expectedCount = 1;
    }
    SECTION("An uncached version preserves unknown")
    {
        v1.Versions[0].Manifest.reset();
        expectedCount = 1;
        expectedManifestRequests = 1;
    }
    SECTION("A match alongside an uncached version is retained")
    {
        v1.Versions[0].Manifest.reset();
        v1.Versions[1].Manifest = createManifest("2.0.0", "Match");
        expectedCount = 1;
    }
    SECTION("Missing manifests remain unknown")
    {
        v1.Versions[0].Manifest.reset();
        v1.Versions[1].Manifest.reset();
        expectedCount = 1;
        expectedManifestRequests = 1;
    }
    SECTION("Unsupported comparisons remain unknown")
    {
        criteria.back().Type = GENERATE(MatchType::Fuzzy, MatchType::FuzzySubstring, MatchType::Wildcard);
        expectedCount = 1;
    }
    SECTION("A query can select independently but cannot bypass filters")
    {
        request.Query.emplace(MatchType::Substring, "Source-defined query");
        expectedCount = useInclusions ? size_t{ 1 } : size_t{ 0 };
    }
    SECTION("An ID mismatch still rejects matching metadata")
    {
        request.Filters[0].Value = "Other.Package";
        v1.Versions[1].Manifest = createManifest("2.0.0", "Match");
    }

    auto result = v1.Search(request);
    REQUIRE(result.Matches.size() == expectedCount);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == expectedManifestRequests);
    REQUIRE_FALSE(result.Truncated);
}

TEST_CASE("Search_ReturnedMetadata", "[RestSource][Interface_1_0]")
{
    auto field = GENERATE(PackageMatchField::Name, PackageMatchField::PackageFamilyName,
        PackageMatchField::ProductCode, PackageMatchField::UpgradeCode);
    auto type = GENERATE(MatchType::Exact, MatchType::CaseInsensitive);
    CAPTURE(ToString(field), ToString(type));
    SearchAndManifestResponses responses;
    responses.SetManifestNotFound();
    HttpClientHelper helper{ responses.GetHandler() };
    CachedMetadataInterface v1{ TestRestUriString, helper };
    Manifest manifest;
    manifest.Id = "Foo.Bar";
    manifest.Version = "1.0.0";
    manifest.DefaultLocalization.Add<Localization::PackageName>("Other");
    v1.Versions.emplace_back(VersionAndChannel{ manifest.Version, manifest.Channel }, manifest);
    auto setReturnedValue = [&](std::string_view value)
    {
        switch (field)
        {
        case PackageMatchField::Name:
            responses.SearchResponse[L"Data"][0][L"PackageName"] = web::json::value::string(ConvertToUTF16(value));
            break;
        case PackageMatchField::PackageFamilyName:
            v1.Versions[0].PackageFamilyNames = { std::string{ value } };
            break;
        case PackageMatchField::ProductCode:
            v1.Versions[0].ProductCodes = { std::string{ value } };
            break;
        case PackageMatchField::UpgradeCode:
            v1.Versions[0].UpgradeCodes = { std::string{ value } };
            break;
        }
    };
    setReturnedValue("Match");
    SearchRequest request;
    request.Filters.emplace_back(field, type, "Match");
    size_t expectedCount = 1;
    size_t expectedManifestRequests = 0;

    SECTION("Returned metadata confirms a match despite cached metadata") {}
    SECTION("Case differences follow field-specific rules")
    {
        request.Filters[0].Value = "match";
        if (field == PackageMatchField::Name && type == MatchType::Exact)
        {
            expectedCount = 0;
        }
    }
    SECTION("Nonmatching response metadata is inconclusive without a manifest")
    {
        setReturnedValue("Other");
        v1.Versions[0].Manifest.reset();
        expectedManifestRequests = 1;
    }

    auto result = v1.Search(request);
    REQUIRE(result.Matches.size() == expectedCount);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == expectedManifestRequests);
    REQUIRE_FALSE(result.Truncated);
}

TEST_CASE("Search_ManifestResolution_Fields", "[RestSource][Interface_1_4]")
{
    auto field = GENERATE(PackageMatchField::Name, PackageMatchField::Moniker, PackageMatchField::Tag,
        PackageMatchField::Command, PackageMatchField::PackageFamilyName, PackageMatchField::ProductCode,
        PackageMatchField::UpgradeCode);
    bool useInclusions = GENERATE(false, true);
    CAPTURE(ToString(field), useInclusions);
    SearchAndManifestResponses responses;
    auto& version = responses.ManifestResponse[L"Data"][L"Versions"][0];
    std::string value = "Wanted";
    switch (field)
    {
    case PackageMatchField::Name:
        version[L"Locales"][0][L"PackageLocale"] = web::json::value::string(L"fr-FR");
        version[L"Locales"][0][L"PackageName"] = web::json::value::string(L"Wanted");
        break;
    case PackageMatchField::Moniker:
        version[L"DefaultLocale"][L"Moniker"] = web::json::value::string(L"Wanted");
        break;
    case PackageMatchField::Tag:
        version[L"DefaultLocale"][L"Tags"][0] = web::json::value::string(L"Wanted");
        break;
    case PackageMatchField::Command:
        version[L"Installers"][0][L"Commands"][0] = web::json::value::string(L"Wanted");
        break;
    case PackageMatchField::PackageFamilyName:
        value = "Test.Package_8wekyb3d8bbwe";
        version[L"Installers"][0][L"InstallerType"] = web::json::value::string(L"msix");
        version[L"Installers"][0][L"PackageFamilyName"] = web::json::value::string(ConvertToUTF16(value));
        break;
    case PackageMatchField::ProductCode:
        value = "{A0000000-0000-0000-0000-000000000001}";
        version[L"Installers"][0][L"ProductCode"] = web::json::value::string(ConvertToUTF16(value));
        break;
    case PackageMatchField::UpgradeCode:
        value = "{A0000000-0000-0000-0000-000000000002}";
        version[L"Installers"][0][L"AppsAndFeaturesEntries"][0][L"UpgradeCode"] = web::json::value::string(ConvertToUTF16(value));
        break;
    }
    size_t expectedCount = 1;
    bool hasManifest = true;
    SECTION("Retrieved metadata matches") {}
    SECTION("Retrieved metadata rejects the candidate")
    {
        value = "Missing";
        expectedCount = 0;
    }
    SECTION("Missing manifests preserve unknown")
    {
        responses.SetManifestNotFound();
        hasManifest = false;
    }

    HttpClientHelper helper{ responses.GetHandler() };
    V1_4::Interface rest{ TestRestUriString, helper, {} };
    SearchRequest request;
    auto& criteria = useInclusions ? request.Inclusions : request.Filters;
    criteria.emplace_back(field, MatchType::Exact, value);
    auto result = rest.Search(request);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == 1);
    REQUIRE(responses.LastManifestRequest.absolute_uri().path() == L"/api/packageManifests/Foo.Bar");
    REQUIRE(result.Matches.size() == expectedCount);
    REQUIRE_FALSE(result.Truncated);
    if (expectedCount)
    {
        REQUIRE(result.Matches[0].PackageInformation.PackageName ==
            ConvertToUTF8(responses.SearchResponse[L"Data"][0][L"PackageName"].as_string()));
        REQUIRE(result.Matches[0].Versions[0].Manifest.has_value() == hasManifest);
        REQUIRE(result.Matches[0].Versions[0].VersionAndChannel.GetVersion().ToString() == "1.0.0");
    }
}

TEST_CASE("Search_ManifestResolution_PositionalQuery", "[RestSource][Interface_1_0]")
{
    std::string query = GENERATE("browser", ".");
    bool matches = GENERATE(false, true);
    bool unknownVersion = GENERATE(false, true);
    CAPTURE(query, matches, unknownVersion);
    SearchAndManifestResponses responses;
    responses.SearchResponse[L"Data"][0][L"PackageName"] = web::json::value::string(L"Unrelated application");
    if (unknownVersion)
    {
        responses.SearchResponse[L"Data"][0][L"Versions"][0][L"PackageVersion"] = web::json::value::string(L"Unknown");
    }
    if (matches)
    {
        responses.ManifestResponse[L"Data"][L"Versions"][0][L"DefaultLocale"][L"PackageName"] =
            web::json::value::string(ConvertToUTF16(query));
    }
    HttpClientHelper helper{ responses.GetHandler() };
    Interface rest{ TestRestUriString, helper };
    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::PackageFamilyName, MatchType::Exact, query);
    request.Inclusions.emplace_back(PackageMatchField::ProductCode, MatchType::Exact, query);
    request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, query);
    request.Inclusions.emplace_back(PackageMatchField::Name, MatchType::CaseInsensitive, query);
    request.Inclusions.emplace_back(PackageMatchField::Moniker, MatchType::CaseInsensitive, query);

    auto result = rest.Search(request);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == 1);
    REQUIRE(result.Matches.size() == (matches ? size_t{ 1 } : size_t{ 0 }));
    if (matches)
    {
        REQUIRE(result.Matches[0].Versions[0].Manifest.has_value());
        REQUIRE(result.Matches[0].Versions[0].VersionAndChannel.GetVersion().ToString() == "1.0.0");
    }
}

TEST_CASE("Search_ManifestResolution_SkipsUnnecessaryLookups", "[RestSource][Interface_1_0]")
{
    SearchAndManifestResponses responses;
    responses.ManifestStatus = web::http::status_codes::ServiceUnavailable;
    SearchRequest request;
    size_t expectedCount = 1;
    SECTION("No selectors") {}
    SECTION("Generic query")
    {
        request.Query.emplace(MatchType::Substring, "browser");
    }
    SECTION("Generic query can select independently")
    {
        request.Query.emplace(MatchType::Substring, "browser");
        request.Inclusions.emplace_back(PackageMatchField::Moniker, MatchType::Exact, "browser");
    }
    SECTION("Known inclusion matches after unknown inclusion")
    {
        request.Inclusions.emplace_back(PackageMatchField::Moniker, MatchType::Exact, "browser");
        request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, "Foo.Bar");
    }
    SECTION("Known filter fails after unknown filter")
    {
        request.Filters.emplace_back(PackageMatchField::Moniker, MatchType::Exact, "browser");
        request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Other.Package");
        expectedCount = 0;
    }
    SECTION("Known inclusions fail despite unknown filter")
    {
        request.Filters.emplace_back(PackageMatchField::Moniker, MatchType::Exact, "browser");
        request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, "Other.Package");
        expectedCount = 0;
    }
    SECTION("Unsupported comparison")
    {
        auto type = GENERATE(MatchType::Fuzzy, MatchType::FuzzySubstring, MatchType::Wildcard);
        request.Filters.emplace_back(PackageMatchField::Name, type, "browser");
    }
    SECTION("Unverifiable field")
    {
        auto field = GENERATE(PackageMatchField::Market, PackageMatchField::NormalizedNameAndPublisher);
        request.Filters.emplace_back(field, MatchType::Exact, "value");
    }
    SECTION("Installed-package correlation")
    {
        request.Purpose = GENERATE(SearchPurpose::CorrelationToInstalled, SearchPurpose::CorrelationToAvailable);
        request.Inclusions.emplace_back(PackageMatchField::ProductCode, MatchType::Exact, "Missing.Code");
    }
    SECTION("Returned name proves the filter")
    {
        request.Filters.emplace_back(PackageMatchField::Name, MatchType::Exact,
            ConvertToUTF8(responses.SearchResponse[L"Data"][0][L"PackageName"].as_string()));
    }
    HttpClientHelper helper{ responses.GetHandler() };
    Interface rest{ TestRestUriString, helper };
    auto result = rest.Search(request);
    REQUIRE(result.Matches.size() == expectedCount);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == 0);
}

TEST_CASE("Search_ManifestResolution_Errors", "[RestSource][Interface_1_1]")
{
    SearchAndManifestResponses responses;
    HRESULT expectedError = APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA;
    SECTION("Mismatching package identifier")
    {
        responses.ManifestResponse[L"Data"][L"PackageIdentifier"] = web::json::value::string(L"Other.Package");
    }
    SECTION("Malformed manifest data")
    {
        responses.ManifestResponse[L"Data"][L"Versions"] = web::json::value::array();
    }
    SECTION("Invalid installer")
    {
        responses.ManifestResponse[L"Data"][L"Versions"][0][L"Installers"][0].as_object().erase(L"InstallerUrl");
    }
    SECTION("Access denied")
    {
        responses.ManifestStatus = web::http::status_codes::Unauthorized;
        expectedError = HTTP_E_STATUS_DENIED;
    }
    SECTION("Service unavailable")
    {
        responses.ManifestStatus = web::http::status_codes::ServiceUnavailable;
        expectedError = APPINSTALLER_CLI_ERROR_SERVICE_UNAVAILABLE;
    }
    SECTION("Unsupported request reported by the server")
    {
        responses.ManifestResponse = web::json::value::parse(LR"({"Data":null,"RequiredQueryParameters":["Version"]})");
        expectedError = APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST;
    }
    HttpClientHelper helper{ responses.GetHandler() };
    V1_1::Interface rest{ TestRestUriString, helper, {} };
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::Exact, "Bar");
    REQUIRE_THROWS_HR(rest.Search(request), expectedError);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == 1);
}

TEST_CASE("Search_ManifestResolution_Versions", "[RestSource][Interface_1_0]")
{
    SearchAndManifestResponses responses;
    auto firstManifest = responses.ManifestResponse[L"Data"][L"Versions"][0];
    firstManifest[L"DefaultLocale"][L"Moniker"] = web::json::value::string(L"other");
    auto secondManifest = firstManifest;
    secondManifest[L"PackageVersion"] = web::json::value::string(L"2.0.0");
    secondManifest[L"DefaultLocale"][L"Moniker"] = web::json::value::string(L"target");
    responses.ManifestResponse[L"Data"][L"Versions"] = web::json::value::array({ firstManifest, secondManifest });
    responses.SearchResponse[L"Data"][0][L"Versions"][1][L"PackageVersion"] = web::json::value::string(L"2.0.0");
    size_t expectedCount = 1;
    bool secondManifestCached = true;
    std::string expectedChannel;

    SECTION("A different version can match") {}
    SECTION("Identifier casing is not a different package")
    {
        responses.ManifestResponse[L"Data"][L"PackageIdentifier"] = web::json::value::string(L"foo.bar");
    }
    SECTION("Canonically equivalent identifiers are the same package")
    {
        responses.SearchResponse[L"Data"][0][L"PackageIdentifier"] = web::json::value::string(L"Foo.Cafe\u0301");
        responses.ManifestResponse[L"Data"][L"PackageIdentifier"] = web::json::value::string(L"Foo.Caf\u00E9");
    }
    SECTION("An omitted version keeps the result unknown")
    {
        responses.ManifestResponse[L"Data"][L"Versions"] = web::json::value::array({ firstManifest });
        secondManifestCached = false;
    }
    SECTION("A manifest from another channel cannot fill the missing version")
    {
        responses.SearchResponse[L"Data"][0][L"Versions"][1][L"Channel"] = web::json::value::string(L"beta");
        expectedChannel = "beta";
        secondManifestCached = false;
    }
    SECTION("All versions reject the request")
    {
        responses.ManifestResponse[L"Data"][L"Versions"][1][L"DefaultLocale"][L"Moniker"] = web::json::value::string(L"other");
        expectedCount = 0;
    }
    SECTION("A version outside the search result cannot satisfy the request")
    {
        responses.ManifestResponse[L"Data"][L"Versions"][1][L"DefaultLocale"][L"Moniker"] = web::json::value::string(L"other");
        secondManifest[L"PackageVersion"] = web::json::value::string(L"3.0.0");
        responses.ManifestResponse[L"Data"][L"Versions"][2] = secondManifest;
        expectedCount = 0;
    }

    HttpClientHelper helper{ responses.GetHandler() };
    Interface rest{ TestRestUriString, helper };
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Moniker, MatchType::Exact, "target");
    auto result = rest.Search(request);
    REQUIRE(responses.ManifestRequests == 1);
    REQUIRE(result.Matches.size() == expectedCount);
    if (expectedCount)
    {
        const auto& versions = result.Matches[0].Versions;
        REQUIRE(versions.size() == 2);
        REQUIRE(versions[0].Manifest.has_value());
        REQUIRE(versions[1].Manifest.has_value() == secondManifestCached);
        REQUIRE(versions[1].VersionAndChannel.GetVersion().ToString() == "2.0.0");
        REQUIRE(versions[1].VersionAndChannel.GetChannel().ToString() == expectedChannel);
    }
}

TEST_CASE("Search_ManifestResolution_ReusesPackageCache", "[RestSource]")
{
    bool unknownVersion = GENERATE(false, true);
    CAPTURE(unknownVersion);
    SearchAndManifestResponses responses;
    if (unknownVersion)
    {
        responses.SearchResponse[L"Data"][0][L"Versions"][0][L"PackageVersion"] = web::json::value::string(L"Unknown");
    }
    responses.SearchResponse[L"Data"][0][L"Versions"][0][L"PackageFamilyNames"][0] = web::json::value::string(L"Search.Reference_123");
    responses.SearchResponse[L"Data"][0][L"Versions"][0][L"ProductCodes"][0] = web::json::value::string(L"Search.Product");
    responses.SearchResponse[L"Data"][0][L"Versions"][0][L"UpgradeCodes"][0] = web::json::value::string(L"Search.Upgrade");
    responses.SearchResponse[L"Data"][0][L"Versions"][0][L"AppsAndFeaturesEntryVersions"][0] = web::json::value::string(L"0.5.0");
    responses.SearchResponse[L"Data"][0][L"Versions"][0][L"AppsAndFeaturesEntryVersions"][1] = web::json::value::string(L"0.6.0");
    responses.ManifestResponse[L"Data"][L"Versions"][0][L"DefaultLocale"][L"Moniker"] = web::json::value::string(L"bar");
    HttpClientHelper helper{ responses.GetHandler() };
    IRestClient::Information information{ "TestSource", { "1.4.0" } };
    SourceDetails details;
    details.Identifier = "TestSource";
    auto source = std::make_shared<RestSource>(details, SourceInformation{},
        RestClient::Create(TestRestUriString, {}, {}, helper, information));
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::Exact, "Bar");
    request.Inclusions.emplace_back(PackageMatchField::Moniker, MatchType::Exact, "bar");

    auto result = source->Search(request);
    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package->GetAvailable().at(0);
    auto keys = package->GetVersionKeys();
    REQUIRE(keys.size() == 1);
    REQUIRE(keys[0].Version == "1.0.0");
    REQUIRE(package->GetVersion(keys[0])->GetManifest().Moniker == "bar");
    REQUIRE(package->GetLatestVersion()->GetManifest().Version == "1.0.0");
    auto references = package->GetMultiProperty(PackageMultiProperty::PackageFamilyName);
    REQUIRE(std::any_of(references.begin(), references.end(), [](const auto& value) { return value.get() == "Search.Reference_123"; }));
    REQUIRE(package->GetLatestVersion()->GetMultiProperty(PackageVersionMultiProperty::ProductCode).at(0).get() == "Search.Product");
    REQUIRE(package->GetLatestVersion()->GetMultiProperty(PackageVersionMultiProperty::UpgradeCode).at(0).get() == "Search.Upgrade");
    REQUIRE(package->GetLatestVersion()->GetProperty(PackageVersionProperty::ArpMinVersion).get() == (unknownVersion ? "" : "0.5.0"));
    REQUIRE(package->GetLatestVersion()->GetProperty(PackageVersionProperty::ArpMaxVersion).get() == (unknownVersion ? "" : "0.6.0"));
    auto pairs = package->GetMatrixProperty(PackageMatrixProperty::NormalizedNameAndPublisher);
    for (const auto& pair : pairs)
    {
        REQUIRE(pair.size() == 2);
    }
    auto containsPair = [&](const auto& name, const auto& publisher)
    {
        return std::any_of(pairs.begin(), pairs.end(), [&](const auto& pair)
            {
                return pair[0] == ConvertToUTF8(name.as_string()) &&
                    pair[1] == ConvertToUTF8(publisher.as_string());
            });
    };
    const auto& searchPackage = responses.SearchResponse.at(L"Data")[0];
    const auto& locale = responses.ManifestResponse.at(L"Data").at(L"Versions")[0].at(L"DefaultLocale");
    REQUIRE(containsPair(searchPackage.at(L"PackageName"), searchPackage.at(L"Publisher")));
    REQUIRE(containsPair(locale.at(L"PackageName"), locale.at(L"Publisher")));
    REQUIRE_FALSE(containsPair(searchPackage.at(L"PackageName"), locale.at(L"Publisher")));
    REQUIRE_FALSE(containsPair(locale.at(L"PackageName"), searchPackage.at(L"Publisher")));
    REQUIRE_THROWS_HR(package->GetMatrixProperty(static_cast<PackageMatrixProperty>(-1)), E_UNEXPECTED);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == 1);
}

TEST_CASE("Search_ManifestResolution_SourceCapabilities", "[RestSource][Interface_1_1]")
{
    SearchAndManifestResponses responses;
    IRestClient::Information information;
    information.RequiredPackageMatchFields = { "Market" };
    information.RequiredQueryParameters = { "Market" };
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::Exact, "Bar");
    std::string expectedMarket = AppInstaller::Runtime::GetOSRegion();
    size_t expectedManifestRequests = 1;
    SECTION("Use the required market") {}
    SECTION("Preserve an explicit market")
    {
        expectedMarket = "FR";
        request.Filters.emplace_back(PackageMatchField::Market, MatchType::Exact, expectedMarket);
    }
    SECTION("Conflicting markets cannot be represented")
    {
        request.Filters.emplace_back(PackageMatchField::Market, MatchType::Exact, "FR");
        request.Filters.emplace_back(PackageMatchField::Market, MatchType::Exact, "DE");
        expectedManifestRequests = 0;
    }
    SECTION("A market prefix cannot be represented")
    {
        request.Filters.emplace_back(PackageMatchField::Market, MatchType::StartsWith, "F");
        expectedManifestRequests = 0;
    }
    SECTION("Required version prevents an all-manifests lookup")
    {
        information.RequiredQueryParameters.emplace_back("Version");
        expectedManifestRequests = 0;
    }
    SECTION("An unsupported market prevents lookup")
    {
        information.UnsupportedQueryParameters.emplace_back("Market");
        expectedManifestRequests = 0;
    }
    SECTION("Unsupported version and channel parameters are not sent")
    {
        information.UnsupportedQueryParameters = { "Version", "Channel" };
    }
    HttpClientHelper helper{ responses.GetHandler() };
    V1_1::Interface rest{ TestRestUriString, helper, information, { { L"Windows-Package-Manager", L"TestHeader" } } };
    auto result = rest.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == expectedManifestRequests);
    REQUIRE(result.Matches[0].Versions[0].Manifest.has_value() == (expectedManifestRequests != 0));
    if (expectedManifestRequests)
    {
        auto query = web::uri::split_query(responses.LastManifestRequest.absolute_uri().query());
        REQUIRE(query.at(L"Market") == ConvertToUTF16(expectedMarket));
        REQUIRE(query.count(L"Version") == 0);
        REQUIRE(query.count(L"Channel") == 0);
        REQUIRE(responses.LastManifestRequest.headers()[L"Windows-Package-Manager"] == L"TestHeader");
        REQUIRE(responses.LastManifestRequest.headers()[L"Version"] == L"1.1.0");
    }
}

TEST_CASE("Search_ManifestResolution_Continuation", "[RestSource][Interface_1_0]")
{
    size_t searches = 0;
    size_t lookups = 0;
    std::vector<utility::string_t> tokens;
    bool manifestReceivedContinuation = false;
    auto handler = std::make_shared<TestRestRequestHandler>(
        [&](web::http::http_request request) -> pplx::task<web::http::http_response>
        {
            web::http::http_response response{ web::http::status_codes::OK };
            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.headers().set_cache_control(L"no-store");
            if (request.method() == web::http::methods::POST)
            {
                tokens.emplace_back(request.headers()[L"ContinuationToken"]);
                ++searches;
                response.set_body(web::json::value::parse(searches == 1 ?
                    GetSearchResponse_PackageIds({ L"Other.Package" }, L"next") : GetSearchResponse_PackageIds({ L"Foo.Bar" })));
            }
            else if (request.method() == web::http::methods::GET)
            {
                ++lookups;
                manifestReceivedContinuation |= request.headers().has(L"ContinuationToken");
                auto manifest = web::json::value::parse(GetGoodManifest_RequiredFields());
                manifest[L"Data"][L"Versions"][0][L"PackageVersion"] = web::json::value::string(L"1.0.0");
                if (request.absolute_uri().path() == L"/api/packageManifests/Other.Package")
                {
                    manifest[L"Data"][L"PackageIdentifier"] = web::json::value::string(L"Other.Package");
                }
                else
                {
                    manifest[L"Data"][L"Versions"][0][L"DefaultLocale"][L"Moniker"] = web::json::value::string(L"bar");
                }
                response.set_body(manifest);
            }
            return pplx::task_from_result(response);
        });
    HttpClientHelper helper{ handler };
    Interface rest{ TestRestUriString, helper };
    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::Moniker, MatchType::Exact, "bar");
    request.MaximumResults = 1;
    auto result = rest.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar");
    REQUIRE_FALSE(result.Truncated);
    REQUIRE(searches == 2);
    REQUIRE(lookups == 2);
    REQUIRE(tokens == std::vector<utility::string_t>{ L"", L"next" });
    REQUIRE_FALSE(manifestReceivedContinuation);
}

TEST_CASE("Search_ManifestResolution_RetrievalLimit", "[RestSource][Interface_1_0]")
{
    bool paginated = GENERATE(false, true);
    bool manifestAvailable = GENERATE(false, true);
    bool manifestMatches = GENERATE(false, true);
    bool useInclusions = GENERATE(false, true);
    size_t maximumResults = GENERATE(size_t{ 0 }, size_t{ 1 });
    CAPTURE(paginated, manifestAvailable, manifestMatches, useInclusions, maximumResults);
    const std::vector<std::string> identifiers{ "Foo.One", "Foo.Two", "Foo.Three", "Foo.Four", "Foo.Five" };
    size_t searches = 0;
    std::vector<utility::string_t> manifestPaths;
    auto handler = std::make_shared<TestRestRequestHandler>(
        [&](web::http::http_request request) -> pplx::task<web::http::http_response>
        {
            web::http::http_response response{ web::http::status_codes::OK };
            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.headers().set_cache_control(L"no-store");
            if (request.method() == web::http::methods::POST)
            {
                ++searches;
                auto page = paginated ?
                    (request.headers().has(L"ContinuationToken") ?
                        GetSearchResponse_PackageIds({ L"Foo.Three", L"Foo.Four", L"Foo.Five" }) :
                        GetSearchResponse_PackageIds({ L"Foo.One", L"Foo.Two" }, L"next")) :
                    GetSearchResponse_PackageIds({ L"Foo.One", L"Foo.Two", L"Foo.Three", L"Foo.Four", L"Foo.Five" });
                response.set_body(web::json::value::parse(page));
            }
            else if (request.method() == web::http::methods::GET)
            {
                auto path = request.absolute_uri().path();
                manifestPaths.emplace_back(path);
                if (manifestAvailable)
                {
                    auto manifest = web::json::value::parse(GetGoodManifest_RequiredFields());
                    manifest[L"Data"][L"PackageIdentifier"] = web::json::value::string(web::uri::split_path(path).back());
                    auto& version = manifest[L"Data"][L"Versions"][0];
                    version[L"PackageVersion"] = web::json::value::string(L"1.0.0");
                    version[L"DefaultLocale"][L"PackageName"] = web::json::value::string(manifestMatches ? L"Wanted" : L"Other");
                    response.set_body(manifest);
                }
                else
                {
                    response.set_status_code(web::http::status_codes::NotFound);
                    response.set_body(web::json::value::parse(LR"({"code":"DataNotFound","message":"Not found"})"));
                }
            }
            return pplx::task_from_result(response);
        });
    HttpClientHelper helper{ handler };
    Interface rest{ TestRestUriString, helper };
    SearchRequest request;
    auto& criteria = useInclusions ? request.Inclusions : request.Filters;
    criteria.emplace_back(PackageMatchField::Name, MatchType::Exact, "Wanted");
    request.MaximumResults = maximumResults;

    for (size_t attempt = 0; attempt < 2; ++attempt)
    {
        CAPTURE(attempt);
        searches = 0;
        manifestPaths.clear();
        auto result = rest.Search(request);
        bool firstPageSatisfiesLimit = paginated && maximumResults && (!manifestAvailable || manifestMatches);
        size_t expectedLookups = firstPageSatisfiesLimit ? 2 : 3;
        REQUIRE(manifestPaths.size() == expectedLookups);
        for (size_t i = 0; i < expectedLookups; ++i)
        {
            REQUIRE(manifestPaths[i] == L"/api/packageManifests/" + ConvertToUTF16(identifiers[i]));
        }

        size_t firstRetained = manifestAvailable && !manifestMatches ? 3 : 0;
        size_t expectedCount = identifiers.size() - firstRetained;
        if (maximumResults)
        {
            expectedCount = std::min(expectedCount, maximumResults);
        }
        REQUIRE(result.Matches.size() == expectedCount);
        REQUIRE(result.Truncated == (maximumResults != 0));
        REQUIRE(searches == (paginated && !firstPageSatisfiesLimit ? size_t{ 2 } : size_t{ 1 }));
        for (size_t i = 0; i < expectedCount; ++i)
        {
            REQUIRE(result.Matches[i].PackageInformation.PackageIdentifier == identifiers[firstRetained + i]);
            REQUIRE(result.Matches[i].Versions[0].Manifest.has_value() == (manifestAvailable && firstRetained + i < expectedLookups));
        }
    }
}

TEST_CASE("Search_ManifestResolution_RetrievalLimit_AvailableMetadata", "[RestSource][Interface_1_0]")
{
    SearchAndManifestResponses responses;
    responses.SearchResponse = web::json::value::parse(GetSearchResponse_PackageIds(
        { L"Other.First", L"Foo.Known", L"Foo.One", L"Foo.Two", L"Foo.Three", L"Other.Last", L"Foo.Last", L"Foo.Unknown" }));
    for (size_t index : { size_t{ 1 }, size_t{ 5 }, size_t{ 6 } })
    {
        responses.SearchResponse[L"Data"][index][L"PackageName"] = web::json::value::string(L"Wanted");
    }
    responses.SetManifestNotFound();
    HttpClientHelper helper{ responses.GetHandler() };
    Interface rest{ TestRestUriString, helper };
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::Exact, "Wanted");
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::StartsWith, "Foo.");

    auto result = rest.Search(request);
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == 3);
    REQUIRE_FALSE(result.Truncated);
    const std::vector<std::string> expected{ "Foo.Known", "Foo.One", "Foo.Two", "Foo.Three", "Foo.Last", "Foo.Unknown" };
    REQUIRE(result.Matches.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        REQUIRE(result.Matches[i].PackageInformation.PackageIdentifier == expected[i]);
    }
}

TEST_CASE("Search_ExplicitIdFilters_UnsupportedMatchType", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK,
        GetSearchResponse_PackageIds({ L"Foo.Bar" })) };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    auto type = GENERATE(MatchType::Fuzzy, MatchType::FuzzySubstring, MatchType::Wildcard);
    request.Filters.emplace_back(PackageMatchField::Id, type, "Other");

    auto result = v1.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar");
}

TEST_CASE("Search_ExplicitIdFilters_UnavailableMetadata", "[RestSource][Interface_1_0]")
{
    SearchAndManifestResponses responses;
    responses.SetManifestNotFound();
    HttpClientHelper helper{ responses.GetHandler() };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Foo.Bar");
    auto field = GENERATE(PackageMatchField::Name, PackageMatchField::Moniker, PackageMatchField::Tag,
        PackageMatchField::Command, PackageMatchField::PackageFamilyName, PackageMatchField::ProductCode,
        PackageMatchField::UpgradeCode, PackageMatchField::NormalizedNameAndPublisher, PackageMatchField::Market);
    request.Filters.emplace_back(field, MatchType::Exact, "Not in the response");

    auto result = v1.Search(request);
    size_t expectedManifestRequests = (field == PackageMatchField::NormalizedNameAndPublisher || field == PackageMatchField::Market) ? 0 : 1;
    REQUIRE(responses.SearchRequests == 1);
    REQUIRE(responses.ManifestRequests == expectedManifestRequests);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar");
}

TEST_CASE("Search_ExplicitIdFilters_NoFilters", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK,
        GetSearchResponse_PackageIds({ L"Foo.Bar" })) };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    size_t expectedCount = 1;

    SECTION("Everything") {}
    SECTION("Query")
    {
        request.Query.emplace(MatchType::Exact, "Not in the response");
    }
    SECTION("Inclusions")
    {
        request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, "Not in the response");
        expectedCount = 0;
    }
    SECTION("Correlation")
    {
        request.Purpose = SearchPurpose::CorrelationToAvailable;
        request.Inclusions.emplace_back(PackageMatchField::ProductCode, MatchType::Exact, "Not in the response");
    }

    auto result = v1.Search(request);
    REQUIRE(result.Matches.size() == expectedCount);
    if (expectedCount)
    {
        REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar");
    }
}

TEST_CASE("Search_ExplicitIdFilters_Continuation", "[RestSource][Interface_1_0]")
{
    bool allFiltered = GENERATE(false, true);
    bool useInclusions = GENERATE(false, true);
    CAPTURE(allFiltered, useInclusions);
    std::vector<utility::string_t> pages
    {
        GetSearchResponse_PackageIds({ L"Other.One", L"Other.Two" }, L"next"),
        GetSearchResponse_PackageIds({ allFiltered ? L"Other.Three" : L"Match.One" }, L"last"),
        GetSearchResponse_PackageIds({ allFiltered ? L"Other.Four" : L"Match.Two",
            allFiltered ? L"Other.Five" : L"Match.Three" }),
    };
    std::vector<utility::string_t> continuationTokens;
    size_t requestCount = 0;
    auto handler = std::make_shared<TestRestRequestHandler>(
        [&](web::http::http_request request) -> pplx::task<web::http::http_response>
        {
            web::http::http_response response{ web::http::status_codes::BadRequest };
            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.headers().set_cache_control(L"no-store");
            if (request.method() == web::http::methods::POST && requestCount < pages.size())
            {
                continuationTokens.emplace_back(request.headers()[L"ContinuationToken"]);
                response.set_status_code(web::http::status_codes::OK);
                response.set_body(web::json::value::parse(pages[requestCount]));
            }
            ++requestCount;
            return pplx::task_from_result(response);
        });
    HttpClientHelper helper{ handler };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    auto& criteria = useInclusions ? request.Inclusions : request.Filters;
    criteria.emplace_back(PackageMatchField::Id, MatchType::StartsWith, "Match.");
    request.MaximumResults = GENERATE(0, 1, 2, 3, 9);

    auto result = v1.Search(request);
    size_t expectedCount = allFiltered ? 0 : (request.MaximumResults ? std::min(size_t{ 3 }, request.MaximumResults) : 3);
    REQUIRE(result.Matches.size() == expectedCount);
    REQUIRE(result.Truncated == (!allFiltered && expectedCount < 3));
    REQUIRE(requestCount == (!allFiltered && request.MaximumResults == 1 ? size_t{ 2 } : size_t{ 3 }));
    REQUIRE(continuationTokens[0].empty());
    REQUIRE(continuationTokens[1] == L"next");
    if (requestCount == 3)
    {
        REQUIRE(continuationTokens[2] == L"last");
    }
    const std::vector<std::string> expectedIds{ "Match.One", "Match.Two", "Match.Three" };
    for (size_t i = 0; i < expectedCount; ++i)
    {
        REQUIRE(result.Matches[i].PackageInformation.PackageIdentifier == expectedIds[i]);
    }
}

TEST_CASE("Search_ContinuationToken", "[RestSource][Interface_1_0]")
{
    size_t requestCount = 0;
    std::vector<utility::string_t> sentTokens;
    auto handler = std::make_shared<TestRestRequestHandler>(
        [&](web::http::http_request request) -> pplx::task<web::http::http_response>
        {
            web::http::http_response response{ web::http::status_codes::BadRequest };
            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.headers().set_cache_control(L"no-store");
            if (request.method() == web::http::methods::POST)
            {
                sentTokens.emplace_back(request.headers()[L"ContinuationToken"]);
                ++requestCount;
                response.set_status_code(web::http::status_codes::OK);
                response.set_body(web::json::value::parse(
                    GetSearchResponse_PackageIds({ L"git.package", L"foo.package" }, std::to_wstring(requestCount))));
            }
            return pplx::task_from_result(response);
        });
    HttpClientHelper helper{ handler };
    Interface v1{ TestRestUriString, std::move(helper) };
    for (size_t maximumResults : { size_t{ 9 }, size_t{ 1 }, size_t{ 9 } })
    {
        CAPTURE(maximumResults);
        requestCount = 0;
        sentTokens.clear();
        SearchRequest request;
        request.MaximumResults = maximumResults;
        auto result = v1.Search(request);
        REQUIRE(result.Matches.size() == maximumResults);
        REQUIRE(result.Truncated);
        REQUIRE(requestCount == (maximumResults + 1) / 2);
        REQUIRE(sentTokens[0].empty());
        for (size_t i = 1; i < sentTokens.size(); ++i)
        {
            REQUIRE(sentTokens[i] == std::to_wstring(i));
        }
    }
}

TEST_CASE("Search_ContinuationToken_Cycle", "[RestSource][Interface_1_0]")
{
    bool longerCycle = GENERATE(false, true);
    CAPTURE(longerCycle);
    std::vector<utility::string_t> returnedTokens{ L"next", L"next" };
    if (longerCycle)
    {
        returnedTokens.insert(returnedTokens.begin() + 1, L"NEXT");
    }
    bool keepFirstResult = false;
    bool reachResultLimit = false;
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::StartsWith, "Match.");
    request.MaximumResults = 1;

    SECTION("All pages are rejected") {}
    SECTION("Partial results do not hide the invalid response")
    {
        keepFirstResult = true;
        request.MaximumResults = 2;
    }
    SECTION("Unlimited results still detect cycles")
    {
        request.MaximumResults = 0;
    }
    SECTION("A satisfied result limit does not follow the repeated token")
    {
        reachResultLimit = true;
    }

    size_t requestCount = 0;
    std::vector<utility::string_t> sentTokens;
    auto handler = std::make_shared<TestRestRequestHandler>(
        [&](web::http::http_request httpRequest) -> pplx::task<web::http::http_response>
        {
            web::http::http_response response{ web::http::status_codes::BadRequest };
            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.headers().set_cache_control(L"no-store");
            if (httpRequest.method() == web::http::methods::POST)
            {
                sentTokens.emplace_back(httpRequest.headers()[L"ContinuationToken"]);
                size_t page = requestCount++;
                response.set_status_code(web::http::status_codes::OK);
                if (page < returnedTokens.size())
                {
                    bool matches = (keepFirstResult && page == 0) || (reachResultLimit && page + 1 == returnedTokens.size());
                    response.set_body(web::json::value::parse(
                        GetSearchResponse_PackageIds({ matches ? L"Match.One" : L"Other.App" }, returnedTokens[page])));
                }
                else
                {
                    // End the fake chain if the client fails to detect the cycle.
                    response.set_body(web::json::value::parse(GetSearchResponse_PackageIds({})));
                }
            }
            return pplx::task_from_result(response);
        });
    HttpClientHelper helper{ handler };
    Interface v1{ TestRestUriString, helper };
    if (reachResultLimit)
    {
        auto result = v1.Search(request);
        REQUIRE(result.Matches.size() == 1);
        REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Match.One");
        REQUIRE(result.Truncated);
    }
    else
    {
        REQUIRE_THROWS_HR(v1.Search(request), APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
    }
    REQUIRE(requestCount == returnedTokens.size());
    REQUIRE(sentTokens[0].empty());
    for (size_t i = 1; i < sentTokens.size(); ++i)
    {
        REQUIRE(sentTokens[i] == returnedTokens[i - 1]);
    }
}

TEST_CASE("Search_BadResponse_NoVersions", "[RestSource][Interface_1_0]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : [
               {
              "PackageIdentifier": "git.package",
              "PackageName": "package",
              "Publisher": "git",
              "Versions": null }]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1{ TestRestUriString, std::move(helper) };
    REQUIRE_THROWS_HR(v1.Search({}), APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
}

TEST_CASE("Search_BadResponse_NotFoundCode", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound) };
    Interface v1{ TestRestUriString, std::move(helper) };
    REQUIRE_THROWS_HR(v1.Search({}), APPINSTALLER_CLI_ERROR_RESTAPI_ENDPOINT_NOT_FOUND);
}

TEST_CASE("Search_Optimized_ManifestResponse", "[RestSource][Interface_1_0]")
{
    utility::string_t sample = GetGoodManifest_RequiredFields();
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Id, MatchType::Exact, "Foo.Bar" };
    request.Filters.emplace_back(std::move(filter));
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult result = v1.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Versions.size() == 1);
    REQUIRE(result.Matches[0].Versions[0].VersionAndChannel.GetVersion().ToString() == "5.0.0");
    REQUIRE(result.Matches[0].Versions[0].VersionAndChannel.GetChannel().ToString() == "");
    REQUIRE(result.Matches[0].Versions[0].Manifest);
    
    // Verify manifest is populated
    Manifest manifest = result.Matches[0].Versions[0].Manifest.value();
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
    REQUIRE(manifest.Installers[0].BaseInstallerType == InstallerTypeEnum::Exe);
    REQUIRE(manifest.Installers[0].Url == "https://installer.example.com/foobar.exe");
}

TEST_CASE("Search_Optimized_ManifestResponse_MultipleVersions", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, GetManifestsResponse_MultipleVersions()) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Id, MatchType::Exact, "Foo.Bar" };
    request.Filters.emplace_back(std::move(filter));
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult result = v1.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Versions.size() == 2);
    REQUIRE(result.Matches[0].Versions[0].VersionAndChannel.GetVersion().ToString() == "5.0.0");
    REQUIRE(result.Matches[0].Versions[0].Manifest);
    REQUIRE(result.Matches[0].Versions[1].VersionAndChannel.GetVersion().ToString() == "6.0.0");
    REQUIRE(result.Matches[0].Versions[1].Manifest);
}

TEST_CASE("Search_Optimized_NoResponse_NotFoundCode", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Id, MatchType::Exact, "Foo" };
    request.Filters.emplace_back(std::move(filter));
    Interface v1{ TestRestUriString, std::move(helper) };
    REQUIRE_THROWS_HR(v1.Search(request), APPINSTALLER_CLI_ERROR_RESTAPI_ENDPOINT_NOT_FOUND);
}

TEST_CASE("Search_Optimized_ExplicitIdFilter", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, GetGoodManifest_RequiredFields()) };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    auto type = GENERATE(MatchType::Exact, MatchType::CaseInsensitive);
    std::string id = GENERATE("Foo.Bar", "foo.bar", "Foo", "Other.Package");
    request.Filters.emplace_back(PackageMatchField::Id, type, id);

    auto result = v1.Search(request);
    bool expected = id == "Foo.Bar" || (type == MatchType::CaseInsensitive && id == "foo.bar");
    REQUIRE(result.Matches.size() == (expected ? size_t{ 1 } : size_t{ 0 }));
    REQUIRE_FALSE(result.Truncated);
    if (expected)
    {
        REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar");
        REQUIRE(result.Matches[0].Versions[0].Manifest.has_value());
    }
}

TEST_CASE("Search_SubstringIdFallback_ManifestResponse", "[RestSource][Interface_1_0]")
{
    bool filteredSearch = GENERATE(false, true);
    bool manifestMatches = GENERATE(false, true);
    auto searchResponse = filteredSearch ? GetSearchResponse_PackageIds({ L"Unrelated.Package" }) : GetSearchResponse_PackageIds({});
    size_t searchCount = 0;
    size_t manifestCount = 0;

    auto handler = std::make_shared<TestRestRequestHandler>(
        [&](web::http::http_request request) -> pplx::task<web::http::http_response>
        {
            web::http::http_response response;
            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.headers().set_cache_control(L"no-store");

            if (request.method() == web::http::methods::POST)
            {
                ++searchCount;
                response.set_status_code(web::http::status_codes::OK);
                response.set_body(web::json::value::parse(searchResponse));
            }
            else if (request.method() == web::http::methods::GET)
            {
                ++manifestCount;
                response.set_status_code(web::http::status_codes::OK);
                response.set_body(web::json::value::parse(GetGoodManifest_RequiredFields()));
            }
            else
            {
                response.set_status_code(web::http::status_codes::BadRequest);
            }

            return pplx::task_from_result(response);
        });

    HttpClientHelper helper{ std::move(handler) };
    AppInstaller::Repository::SearchRequest request;
    std::string_view id = manifestMatches ? "Foo.Bar" : "Other.Id";
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Substring, id);
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult result = v1.Search(request);

    REQUIRE(searchCount == 1);
    REQUIRE(manifestCount == 1);
    REQUIRE(result.Matches.size() == (manifestMatches ? size_t{ 1 } : size_t{ 0 }));
    if (manifestMatches)
    {
        REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar");
        REQUIRE(result.Matches[0].Versions.size() == 1);
        REQUIRE(result.Matches[0].Versions[0].VersionAndChannel.GetVersion().ToString() == "5.0.0");
        REQUIRE(result.Matches[0].Versions[0].Manifest.has_value());
    }
}

TEST_CASE("Search_SubstringId_NoFallbackWhenSearchMatches", "[RestSource][Interface_1_0]")
{
    utility::string_t searchResponse = _XPLATSTR(
        R"delimiter({
            "Data" : [
                {
                    "PackageIdentifier": "Foo.Bar.Baz",
                    "PackageName": "Foo Package",
                    "Publisher": "Foo",
                    "Versions": [
                        { "PackageVersion": "1.0.0" }
                    ]
                }
            ]
        })delimiter");

    auto handler = std::make_shared<TestRestRequestHandler>(
        [searchResponse](web::http::http_request request) -> pplx::task<web::http::http_response>
        {
            web::http::http_response response;
            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.headers().set_cache_control(L"no-store");

            if (request.method() == web::http::methods::POST)
            {
                response.set_status_code(web::http::status_codes::OK);
                response.set_body(web::json::value::parse(searchResponse));
            }
            else
            {
                response.set_status_code(web::http::status_codes::NotFound);
            }

            return pplx::task_from_result(response);
        });

    HttpClientHelper helper{ std::move(handler) };
    AppInstaller::Repository::SearchRequest request;
    request.Filters.emplace_back(PackageMatchFilter{ PackageMatchField::Id, MatchType::Substring, "Foo.Bar" });
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult result = v1.Search(request);

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar.Baz");
}

TEST_CASE("GetManifests_GoodResponse", "[RestSource][Interface_1_0]")
{
    GoodManifest_AllFields sampleManifest;
    utility::string_t sample = sampleManifest.GetSampleManifest_AllFields();
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1{ TestRestUriString, std::move(helper) };
    std::vector<Manifest> manifests = v1.GetManifests("Foo.Bar");
    REQUIRE(manifests.size() == 1);

    // Verify manifest is populated
    Manifest manifest = manifests[0];
    REQUIRE(manifest.Id == "Foo.Bar");
    REQUIRE(manifest.Version == "3.0.0abc");
    REQUIRE(manifest.Moniker == "FooBarMoniker");
    REQUIRE(manifest.Channel == "");
    REQUIRE(manifest.ManifestVersion == AppInstaller::Manifest::ManifestVer{ "1.0.0" });
    sampleManifest.VerifyLocalizations_AllFields(manifest);
    sampleManifest.VerifyInstallers_AllFields(manifest);
}

TEST_CASE("GetManifests_GoodResponse_404AsEmpty", "[RestSource][Interface_1_0]")
{
    utility::string_t notFoundResponse = _XPLATSTR(
        R"delimiter({"code":"DataNotFound","data":[],"details":[],"innererror":{"code":"DataNotFound","data":[],"details":[],"message":"Product is not present","source":"StoreEdgeFD"},"message":"Product is not present","source":"StoreEdgeFD"})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound, std::move(notFoundResponse)) };
    Interface v1{ TestRestUriString, std::move(helper) };
    std::vector<Manifest> manifests = v1.GetManifests("Foo.Bar");
    REQUIRE(manifests.size() == 0);
}

TEST_CASE("GetManifests_GoodResponse_MultipleVersions", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, GetManifestsResponse_MultipleVersions()) };
    Interface v1{ TestRestUriString, std::move(helper) };

    // GetManifests
    std::vector<Manifest> manifests = v1.GetManifests("Foo.Bar");
    REQUIRE(manifests.size() == 2);
    REQUIRE(manifests[0].Version == "5.0.0");
    REQUIRE(manifests[1].Version == "6.0.0");
}

TEST_CASE("GetManifests_BadResponse_SuccessCode", "[RestSource][Interface_1_0]")
{
    utility::string_t badManifest = _XPLATSTR(
        R"delimiter({
        "Data": {
            "PackageIdentifier": "Foo.Bar",
            "Versions": [
                {
                    "PackageVersion": "5.0.0"
                }
            ]
        }
    })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(badManifest)) };
    Interface v1{ TestRestUriString, std::move(helper) };
    REQUIRE_THROWS_HR(v1.GetManifests("Foo.Bar"), APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
}

TEST_CASE("GetManifests_NotFoundCode", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound) };
    Interface v1{ TestRestUriString, std::move(helper) };
    REQUIRE_THROWS_HR(v1.GetManifests("Foo.Bar"), APPINSTALLER_CLI_ERROR_RESTAPI_ENDPOINT_NOT_FOUND);
}

TEST_CASE("GetManifests_GoodResponse_UnknownInstaller", "[RestSource][Interface_1_0]")
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
    Interface v1{ TestRestUriString, std::move(helper) };
    std::vector<Manifest> manifests = v1.GetManifests("Foo.Bar");
    REQUIRE(manifests.size() == 1);

    // Verify manifest is populated and manifest validation passed
    Manifest& manifest = manifests[0];
    REQUIRE(manifest.Installers.size() == 1);
    REQUIRE(manifest.Installers.at(0).BaseInstallerType == InstallerTypeEnum::Unknown);
    REQUIRE(manifest.Installers.at(0).ProductId.empty());
}

TEST_CASE("GetManifestByVersion_GoodResponse_MultipleVersions_VersionFound", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, GetManifestsResponse_MultipleVersions()) };
    Interface v1{ TestRestUriString, std::move(helper) };

    // GetManifests
    std::optional<Manifest> manifest = v1.GetManifestByVersion("Foo.Bar", "5.0.0", "");
    REQUIRE(manifest.has_value());
    REQUIRE(manifest->Version == "5.0.0");
}

TEST_CASE("GetManifestByVersion_GoodResponse_MultipleVersions_VersionNotFound", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, GetManifestsResponse_MultipleVersions()) };
    Interface v1{ TestRestUriString, std::move(helper) };

    // GetManifests
    std::optional<Manifest> manifest = v1.GetManifestByVersion("Foo.Bar", "7.0.0", "");
    REQUIRE_FALSE(manifest.has_value());
}
