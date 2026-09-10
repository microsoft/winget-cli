// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <Rest/Schema/1_0/Interface.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <AppInstallerErrors.h>
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
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK,
        GetSearchResponse_PackageIds({ L"Foo.Bar" })) };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Foo.Bar");
    auto field = GENERATE(PackageMatchField::Name, PackageMatchField::Moniker, PackageMatchField::Tag,
        PackageMatchField::Command, PackageMatchField::PackageFamilyName, PackageMatchField::ProductCode,
        PackageMatchField::UpgradeCode, PackageMatchField::NormalizedNameAndPublisher, PackageMatchField::Market);
    request.Filters.emplace_back(field, MatchType::Exact, "Not in the response");

    auto result = v1.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar");
}

TEST_CASE("Search_ExplicitIdFilters_NoFilters", "[RestSource][Interface_1_0]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK,
        GetSearchResponse_PackageIds({ L"Foo.Bar" })) };
    Interface v1{ TestRestUriString, helper };
    SearchRequest request;

    SECTION("Everything") {}
    SECTION("Query")
    {
        request.Query.emplace(MatchType::Exact, "Not in the response");
    }
    SECTION("Inclusions")
    {
        request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, "Not in the response");
    }
    SECTION("Correlation")
    {
        request.Purpose = SearchPurpose::CorrelationToAvailable;
        request.Inclusions.emplace_back(PackageMatchField::ProductCode, MatchType::Exact, "Not in the response");
    }

    auto result = v1.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].PackageInformation.PackageIdentifier == "Foo.Bar");
}

TEST_CASE("Search_ExplicitIdFilters_Continuation", "[RestSource][Interface_1_0]")
{
    bool allFiltered = GENERATE(false, true);
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
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::StartsWith, "Match.");
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
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : [
               {
              "PackageIdentifier": "git.package",
              "PackageName": "package",
              "Publisher": "git",
              "Versions": [
                {   "PackageVersion": "1.0.0" }]
            },
            {
              "PackageIdentifier": "foo.package",
              "PackageName": "package",
              "Publisher": "foo",
              "Versions": [
                {   "PackageVersion": "1.0.0" }]
            }],
           "ContinuationToken" : "abcd-ct="
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1{ TestRestUriString, std::move(helper) };
    SearchRequest request{};
    request.MaximumResults = 9;
    Schema::IRestClient::SearchResult results = v1.Search(request);
    REQUIRE(results.Matches.size() == request.MaximumResults);

    SearchRequest requestWithSize1{};
    requestWithSize1.MaximumResults = 1;
    Schema::IRestClient::SearchResult resultsWithSize1 = v1.Search(requestWithSize1);
    REQUIRE(resultsWithSize1.Matches.size() == requestWithSize1.MaximumResults);
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
