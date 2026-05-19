// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <Rest/Schema/1_28/Interface.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <AppInstallerErrors.h>

using namespace TestCommon;
using namespace AppInstaller::Http;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_28;

namespace
{
    const std::string TestRestUriString = "http://restsource.com/api";

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
                  "Moniker": "FooBarMoniker",
                  "ReleaseNotes": "Default release notes",
                  "ReleaseNotesUrl": "https://DefaultReleaseNotes.net",
                  "Agreements": [{
                    "AgreementLabel": "DefaultLabel",
                    "Agreement": "DefaultText",
                    "AgreementUrl": "https://DefaultAgreementUrl.net"
                  }],
                  "PurchaseUrl": "http://DefaultPurchaseUrl.net",
                  "InstallationNotes": "Default Installation Notes",
                  "Documentations": [{
                    "DocumentLabel": "Default Document Label",
                    "DocumentUrl": "http://DefaultDocumentUrl.net"
                  }],
                  "Icons": [{
                    "IconUrl": "https://DefaultTestIcon",
                    "IconFileType": "ico",
                    "IconResolution": "custom",
                    "IconTheme": "default",
                    "IconSha256": "69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8123"
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
                    }],
                    "PurchaseUrl": "http://purchaseUrl.net",
                    "InstallationNotes": "Installation Notes",
                    "Documentations": [{
                      "DocumentLabel": "Document Label",
                      "DocumentUrl": "http://documentUrl.net"
                    }],
                    "Icons": [{
                      "IconUrl": "https://testIcon",
                      "IconFileType": "png",
                      "IconResolution": "32x32",
                      "IconTheme": "light",
                      "IconSha256": "69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8321"
                    }]
                  }
                ],)delimiter") _XPLATSTR(R"delimiter(
                "Installers": [
                  {
                    "InstallerSha256": "011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6",
                    "InstallerUrl": "http://foobar.zip",
                    "Architecture": "x86",
                    "InstallerLocale": "en-US",
                    "Platform": [
                      "Windows.Desktop"
                    ],
                    "MinimumOSVersion": "1078",
                    "InstallerType": "zip",
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
                      "Custom": "/custom",
                      "Repair": "/repair"
                    },
                    "InstallerSuccessCodes": [
                      0
                    ],
                    "UpgradeBehavior": "deny",
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
                      "ReturnResponse": "custom",
                      "ReturnResponseUrl": "http://returnResponseUrl.net"
                    }],
                    "NestedInstallerType": "portable",
                    "DisplayInstallWarnings": true,
                    "UnsupportedArguments": [ "log" ],
                    "NestedInstallerFiles": [{
                      "RelativeFilePath": "test\\app.exe",
                      "PortableCommandAlias": "test.exe"
                    }],
                    "InstallationMetadata": {
                      "DefaultInstallLocation": "%TEMP%\\DefaultInstallLocation",
                      "Files": [{
                        "RelativeFilePath": "test\\app.exe",
                        "FileSha256": "011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6",
                        "FileType": "launch",
                        "InvocationParameter": "/parameter",
                        "DisplayName": "test"
                      }]
                    },
                    "DownloadCommandProhibited": true,
                    "RepairBehavior": "uninstaller",
                    "ArchiveBinariesDependOnPath": true,
                    "Authentication": {
                      "AuthenticationType": "microsoftEntraId",
                      "MicrosoftEntraIdAuthenticationInfo" : {
                        "Resource": "TestResource",
                        "Scope" : "TestScope"
                      }
                    },
                    "DesiredStateConfiguration": {
                      "PowerShell": [{
                        "RepositoryUrl": "https://www.powershellgallery.com/api/v2",
                        "ModuleName": "TestModule",
                        "Resources": [{
                          "Name": "TestResource"
                        }]
                      }],
                      "DSCv3": {
                        "Resources": [{
                          "Type": "TestPublisher.TestProduct/TestResource"
                        }]
                      }
                    }
                  }
                ]
              }
            ]
          },
          "ContinuationToken": "abcd"
        })delimiter");
        }

        void VerifyInstallers_AllFields(const AppInstaller::Manifest::Manifest& manifest)
        {
            REQUIRE(manifest.Installers.size() == 1);

            ManifestInstaller actualInstaller = manifest.Installers.at(0);
            REQUIRE(actualInstaller.Sha256 == AppInstaller::Utility::SHA256::ConvertToBytes("011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6"));
            REQUIRE(actualInstaller.Url == "http://foobar.zip");
            REQUIRE(actualInstaller.Arch == Architecture::X86);
            REQUIRE(actualInstaller.Locale == "en-US");
            REQUIRE(actualInstaller.Platform.size() == 1);
            REQUIRE(actualInstaller.Platform[0] == PlatformEnum::Desktop);
            REQUIRE(actualInstaller.MinOSVersion == "1078");
            REQUIRE(actualInstaller.BaseInstallerType == InstallerTypeEnum::Zip);
            REQUIRE(actualInstaller.Scope == ScopeEnum::User);

            // DesiredStateConfiguration
            REQUIRE(actualInstaller.DesiredStateConfiguration.size() == 2);

            // PowerShell entry
            REQUIRE(actualInstaller.DesiredStateConfiguration[0].Type == DesiredStateConfigurationContainerType::PowerShell);
            REQUIRE(actualInstaller.DesiredStateConfiguration[0].RepositoryURL == "https://www.powershellgallery.com/api/v2");
            REQUIRE(actualInstaller.DesiredStateConfiguration[0].ModuleName == "TestModule");
            REQUIRE(actualInstaller.DesiredStateConfiguration[0].Resources.size() == 1);
            REQUIRE(actualInstaller.DesiredStateConfiguration[0].Resources[0].Name == "TestResource");

            // DSCv3 entry
            REQUIRE(actualInstaller.DesiredStateConfiguration[1].Type == DesiredStateConfigurationContainerType::DSCv3);
            REQUIRE(actualInstaller.DesiredStateConfiguration[1].Resources.size() == 1);
            REQUIRE(actualInstaller.DesiredStateConfiguration[1].Resources[0].Name == "TestPublisher.TestProduct/TestResource");
        }
    };
}

TEST_CASE("GetManifests_GoodResponse_V1_28", "[RestSource][Interface_1_28]")
{
    GoodManifest_AllFields sampleManifest;
    utility::string_t sample = sampleManifest.GetSampleManifest_AllFields();
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, std::move(sample)) };
    Interface v1_28{ TestRestUriString, std::move(helper), {} };
    std::vector<Manifest> manifests = v1_28.GetManifests("Foo.Bar");
    REQUIRE(manifests.size() == 1);

    // Verify manifest is populated
    Manifest& manifest = manifests[0];
    REQUIRE(manifest.Id == "Foo.Bar");
    REQUIRE(manifest.Version == "3.0.0abc");
    REQUIRE(manifest.Moniker == "FooBarMoniker");
    REQUIRE(manifest.Channel == "");
    REQUIRE(manifest.ManifestVersion == AppInstaller::Manifest::ManifestVer{ "1.28.0" });
    sampleManifest.VerifyInstallers_AllFields(manifest);
}
