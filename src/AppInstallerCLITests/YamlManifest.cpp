// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Manifest/Manifest.h"
#include "AppInstallerSHA256.h"

using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;

using MultiValue = std::vector<NormalizedString>;
bool operator==(const MultiValue& a, const MultiValue& b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }

    return true;
}

TEST_CASE("ReadGoodManifestAndVerifyContents", "[PackageManifestHelper]")
{
    Manifest manifest = Manifest::CreateFromPath(TestDataFile("GoodManifest.yml"));

    REQUIRE(manifest.Id == "microsoft.msixsdk");
    REQUIRE(manifest.Name == "MSIX SDK");
    REQUIRE(manifest.AppMoniker == "msixsdk");
    REQUIRE(manifest.Version == "1.7.32");
    REQUIRE(manifest.Publisher == "Microsoft");
    REQUIRE(manifest.Channel == "release");
    REQUIRE(manifest.Author == "Microsoft");
    REQUIRE(manifest.License == "MIT License");
    REQUIRE(manifest.LicenseUrl == "https://github.com/microsoft/msix-packaging/blob/master/LICENSE");
    REQUIRE(manifest.MinOSVersion == "0.0.0.0");
    REQUIRE(manifest.Description == "The MSIX SDK project is an effort to enable developers");
    REQUIRE(manifest.Homepage == "https://github.com/microsoft/msix-packaging");
    REQUIRE(manifest.Tags == MultiValue{ "msix", "appx" });
    REQUIRE(manifest.Commands == MultiValue{ "makemsix", "makeappx" });
    REQUIRE(manifest.Protocols == MultiValue{ "protocol1", "protocol2" });
    REQUIRE(manifest.FileExtensions == MultiValue{ "appx", "appxbundle", "msix", "msixbundle" });
    REQUIRE(manifest.InstallerType == ManifestInstaller::InstallerTypeEnum::Zip);

    // default switches
    auto switches = manifest.Switches;
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Custom) == "/custom");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::SilentWithProgress) == "/silentwithprogress");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Silent) == "/silence");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Interactive) == "/interactive");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Language) == "/en-us");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Log) == "/log=<LOGPATH>");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::InstallLocation) == "/dir=<INSTALLPATH>");

    // installers
    REQUIRE(manifest.Installers.size() == 2);
    ManifestInstaller installer1 = manifest.Installers.at(0);
    REQUIRE(installer1.Arch == Architecture::X86);
    REQUIRE(installer1.Url == "https://rubengustorage.blob.core.windows.net/publiccontainer/msixsdkx86.zip");
    REQUIRE(installer1.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
    REQUIRE(installer1.Language == "en-US");
    REQUIRE(installer1.InstallerType == ManifestInstaller::InstallerTypeEnum::Zip);
    REQUIRE(installer1.Scope == "user");

    auto installer1Switches = installer1.Switches;
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Custom) == "/c");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::SilentWithProgress) == "/sp");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Silent) == "/s");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Interactive) == "/i");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Language) == "/en");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Log) == "/l=<LOGPATH>");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::InstallLocation) == "/d=<INSTALLPATH>");

    ManifestInstaller installer2 = manifest.Installers.at(1);
    REQUIRE(installer2.Arch == Architecture::X64);
    REQUIRE(installer2.Url == "https://rubengustorage.blob.core.windows.net/publiccontainer/msixsdkx64.zip");
    REQUIRE(installer2.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF0000"));
    REQUIRE(installer2.Language == "en-US");
    REQUIRE(installer2.InstallerType == ManifestInstaller::InstallerTypeEnum::Zip);
    REQUIRE(installer2.Scope == "user");

    // Installer2 does not declare switches, it inherits switches from package default.
    auto installer2Switches = installer2.Switches;
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Custom) == "/custom");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::SilentWithProgress) == "/silentwithprogress");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Silent) == "/silence");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Interactive) == "/interactive");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Language) == "/en-us");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Log) == "/log=<LOGPATH>");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::InstallLocation) == "/dir=<INSTALLPATH>");

    // Localization
    REQUIRE(manifest.Localization.size() == 1);
    ManifestLocalization localization1 = manifest.Localization.at(0);
    REQUIRE(localization1.Language == "es-MX");
    REQUIRE(localization1.Description == "El proyecto MSIX SDK es habilita desarrolladores de diferentes");
    REQUIRE(localization1.Homepage == "https://github.com/microsoft/msix-packaging/es-MX");
    REQUIRE(localization1.LicenseUrl == "https://github.com/microsoft/msix-packaging/blob/master/LICENSE-es-MX");
}

TEST_CASE("ReadBadManifestAndVerifyThrow", "[PackageManifestHelper]")
{
    REQUIRE_THROWS_WITH(Manifest::CreateFromPath(TestDataFile("BadManifest-MissingName.yml")), Catch::Contains("invalid node; first invalid key: \"Name\""));
}
