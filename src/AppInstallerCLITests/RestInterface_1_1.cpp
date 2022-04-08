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

    struct GoodManifest_AllFields
    {
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
                  "Moniker": "FooBarMoniker",
                  "ReleaseNotes": "Default release notes",
                  "ReleaseNotesUrl": "https://DefaultReleaseNotes.net",
                  "Agreements": [{
                    "AgreementLabel": "DefaultLabel",
                    "Agreement": "DefaultText",
                    "AgreementUrl": "https://DefaultAgreementUrl.net"
                  }]
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
                    ],
                    "ReleaseNotes": "Release notes",
                    "ReleaseNotesUrl": "https://ReleaseNotes.net",
                    "Agreements": [{
                      "AgreementLabel": "Label",
                      "Agreement": "Text",
                      "AgreementUrl": "https://AgreementUrl.net"
                    }]
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
                    "InstallerType": "msi",
                    "Scope": "user",
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
                    "ProductCode": "5b6e0f8a-3bbf-4a17-aefd-024c2b3e075d",
                    "ReleaseDate": "2021-01-01",
                    "InstallerAbortsTerminal": true,
                    "InstallLocationRequired": true,
                    "RequireExplicitUpgrade": true,
                    "UnsupportedOSArchitectures": [ "arm" ],
                    "ElevationRequirement": "elevatesSelf",
                    "AppsAndFeaturesEntries": [{
                      "DisplayName": "DisplayName",
                      "DisplayVersion": "DisplayVersion",
                      "Publisher": "Publisher",
                      "ProductCode": "ProductCode",
                      "UpgradeCode": "UpgradeCode",
                      "InstallerType": "exe"
                    }],
                    "Markets" : {
                      "AllowedMarkets": [ "US" ]
                    },
                    "ExpectedReturnCodes": [{
                      "InstallerReturnCode": 3,
                      "ReturnResponse": "InstallInProgress"
                    }]
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
            REQUIRE(manifest.DefaultLocalization.Get<Localization::ReleaseNotes>() == "Default release notes");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::ReleaseNotesUrl>() == "https://DefaultReleaseNotes.net");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Agreements>().size() == 1);
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Agreements>().at(0).Label == "DefaultLabel");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Agreements>().at(0).AgreementText == "DefaultText");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Agreements>().at(0).AgreementUrl == "https://DefaultAgreementUrl.net");

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
            REQUIRE(frenchLocalization.Get<Localization::ReleaseNotes>() == "Release notes");
            REQUIRE(frenchLocalization.Get<Localization::ReleaseNotesUrl>() == "https://ReleaseNotes.net");
            REQUIRE(frenchLocalization.Get<Localization::Agreements>().size() == 1);
            REQUIRE(frenchLocalization.Get<Localization::Agreements>().at(0).Label == "Label");
            REQUIRE(frenchLocalization.Get<Localization::Agreements>().at(0).AgreementText == "Text");
            REQUIRE(frenchLocalization.Get<Localization::Agreements>().at(0).AgreementUrl == "https://AgreementUrl.net");
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
            REQUIRE(actualInstaller.InstallerType == InstallerTypeEnum::Msi);
            REQUIRE(actualInstaller.Scope == ScopeEnum::User);
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
            REQUIRE(actualInstaller.PackageFamilyName == "");
            REQUIRE(actualInstaller.ProductCode == "5b6e0f8a-3bbf-4a17-aefd-024c2b3e075d");
            REQUIRE(actualInstaller.ReleaseDate == "2021-01-01");
            REQUIRE(actualInstaller.InstallerAbortsTerminal);
            REQUIRE(actualInstaller.InstallLocationRequired);
            REQUIRE(actualInstaller.RequireExplicitUpgrade);
            REQUIRE(actualInstaller.ElevationRequirement == ElevationRequirementEnum::ElevatesSelf);
            REQUIRE(actualInstaller.UnsupportedOSArchitectures.size() == 1);
            REQUIRE(actualInstaller.UnsupportedOSArchitectures.at(0) == Architecture::Arm);
            REQUIRE(actualInstaller.AppsAndFeaturesEntries.size() == 1);
            REQUIRE(actualInstaller.AppsAndFeaturesEntries.at(0).DisplayName == "DisplayName");
            REQUIRE(actualInstaller.AppsAndFeaturesEntries.at(0).DisplayVersion == "DisplayVersion");
            REQUIRE(actualInstaller.AppsAndFeaturesEntries.at(0).Publisher == "Publisher");
            REQUIRE(actualInstaller.AppsAndFeaturesEntries.at(0).ProductCode == "ProductCode");
            REQUIRE(actualInstaller.AppsAndFeaturesEntries.at(0).UpgradeCode == "UpgradeCode");
            REQUIRE(actualInstaller.AppsAndFeaturesEntries.at(0).InstallerType == InstallerTypeEnum::Exe);
            REQUIRE(actualInstaller.Markets.AllowedMarkets.size() == 1);
            REQUIRE(actualInstaller.Markets.AllowedMarkets.at(0) == "US");
            REQUIRE(actualInstaller.ExpectedReturnCodes.at(3).ReturnResponseEnum == ExpectedReturnCodeEnum::InstallInProgress);
        }
    };
}

TEST_CASE("Search_BadResponse_UnsupportedPackageMatchFields", "[RestSource][Interface_1_1]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : [],
            "UnsupportedPackageMatchFields" : [ "Moniker" ]
        })delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), {}, std::move(helper) };
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
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), {}, std::move(helper) };
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
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), {}, std::move(helper) };
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
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), {}, std::move(helper) };
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
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), {}, std::move(helper) };
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
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), {}, std::move(helper) };
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
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), {}, std::move(helper) };
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
    Interface v1_1{ TestRestUriString, info, {}, std::move(helper) };
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
    Interface v1_1{ TestRestUriString, GetTestSourceInformation(), {}, std::move(helper) };
    std::vector<Manifest> manifests = v1_1.GetManifests("Foo.Bar");
    REQUIRE(manifests.size() == 1);

    // Verify manifest is populated and manifest validation passed
    Manifest manifest = manifests[0];
    REQUIRE(manifest.Installers.size() == 1);
    REQUIRE(manifest.Installers.at(0).InstallerType == InstallerTypeEnum::MSStore);
    REQUIRE(manifest.Installers.at(0).ProductId == "9nblggh4nns1");
}

TEST_CASE("GetManifests_GoodResponse_V1_1", "[RestSource][Interface_1_0]")
{
    GoodManifest_AllFields sampleManifest;
    utility::string_t sample = sampleManifest.GetSampleManifest_AllFields();
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_1{ TestRestUriString, {}, {}, std::move(helper) };
    std::vector<Manifest> manifests = v1_1.GetManifests("Foo.Bar");
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