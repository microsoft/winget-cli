// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <set>
#include <Rest/Schema/1_0/Interface.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <AppInstallerErrors.h>
#include <winget/ManifestValidation.h>
#include <Public/AppInstallerSHA256.h>

using namespace TestCommon;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_0;

namespace
{
    const std::string TestRestUriString = "http://restsource.com/api";

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

    struct GoodManifest_AllFields
    {
    public:
        utility::string_t GetSampleManifest_AllFields()
        {
            utility::string_t id = L"Foo.Bar";
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

        void VerifyLocalizations_AllFields(Manifest manifest)
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

        void VerifyInstallers_AllFields(Manifest manifest)
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
            REQUIRE(actualInstaller.InstallerType == InstallerTypeEnum::Msix);
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
            REQUIRE(actualInstaller.Dependencies.WindowsFeatures.at(0) == "feature1");
            REQUIRE(actualInstaller.Dependencies.WindowsLibraries.at(0) == "library1");
            REQUIRE(actualInstaller.Dependencies.PackageDependencies.at(0).Id == "Foo.Baz");
            REQUIRE(actualInstaller.Dependencies.PackageDependencies.at(0).MinVersion == "2.0.0");
            REQUIRE(actualInstaller.Dependencies.ExternalDependencies.at(0) == "FooBarBaz");
            REQUIRE(actualInstaller.PackageFamilyName == "FooBar.PackageFamilyName");
            REQUIRE(actualInstaller.ProductCode == "");
            REQUIRE(actualInstaller.Capabilities.at(0) == "Bluetooth");
            REQUIRE(actualInstaller.RestrictedCapabilities.at(0) == "restrictedCapability");
        }
    };
}

TEST_CASE("Search_GoodResponse", "[RestSource]")
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

TEST_CASE("Search_GoodResponse_AllFields", "[RestSource][Rest]")
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

TEST_CASE("Search_ContinuationToken", "[RestSource]")
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

TEST_CASE("Search_BadResponse_NoVersions", "[RestSource]")
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

TEST_CASE("Search_BadResponse_NotFoundCode", "[RestSource]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound) };
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult result = v1.Search({});
    REQUIRE(result.Matches.empty());
}

TEST_CASE("Search_Optimized_ManifestResponse", "[RestSource]")
{
    utility::string_t sample = GetGoodManifest_RequiredFields();
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Id, MatchType::Exact, "Foo" };
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
    REQUIRE(manifest.Installers[0].InstallerType == InstallerTypeEnum::Exe);
    REQUIRE(manifest.Installers[0].Url == "https://installer.example.com/foobar.exe");
}

TEST_CASE("Search_Optimized_NoResponse_NotFoundCode", "[RestSource]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound) };
    AppInstaller::Repository::SearchRequest request;
    PackageMatchFilter filter{ PackageMatchField::Id, MatchType::Exact, "Foo" };
    request.Filters.emplace_back(std::move(filter));
    Interface v1{ TestRestUriString, std::move(helper) };
    Schema::IRestClient::SearchResult result = v1.Search(request);
    REQUIRE(result.Matches.empty());
}

TEST_CASE("GetManifests_GoodResponse", "[RestSource]")
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
    sampleManifest.VerifyLocalizations_AllFields(manifest);
    sampleManifest.VerifyInstallers_AllFields(manifest);
}

TEST_CASE("GetManifests_BadResponse_SuccessCode", "[RestSource]")
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

TEST_CASE("GetManifests_NotFoundCode", "[RestSource]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound) };
    Interface v1{ TestRestUriString, std::move(helper) };
    std::vector<Manifest> manifests = v1.GetManifests("Foo.Bar");
    REQUIRE(manifests.empty());
}
