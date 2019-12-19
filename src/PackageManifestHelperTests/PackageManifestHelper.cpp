// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "yaml-cpp\yaml.h"
#include "Manifest.h"

using namespace AppInstaller::Package::Manifest;

TEST_CASE("Read good package manifest and verify contents", "[PackageManifestHelper]")
{
    Manifest manifest = Manifest::CreatePackageManifest("GoodManifest.yml");

    REQUIRE(manifest.Id.compare("microsoft.msixsdk") == 0);
    REQUIRE(manifest.Name.compare("MSIX SDK") == 0);
    REQUIRE(manifest.ShortId.compare("msixsdk") == 0);
    REQUIRE(manifest.Version.compare("1.7.32") == 0);
    REQUIRE(manifest.CompanyName.compare("Microsoft") == 0);
    REQUIRE(manifest.Channel.compare("release") == 0);
    REQUIRE(manifest.Author.compare("Microsoft") == 0);
    REQUIRE(manifest.License.compare("MIT License") == 0);
    REQUIRE(manifest.LicenseUrl.compare("https://github.com/microsoft/msix-packaging/blob/master/LICENSE") == 0);
    REQUIRE(manifest.MinOSVersion.compare("0.0.0.0") == 0);
    REQUIRE(manifest.Description.compare("The MSIX SDK project is an effort to enable developers") == 0);
    REQUIRE(manifest.Homepage.compare("https://github.com/microsoft/msix-packaging") == 0);
    REQUIRE(manifest.Tags.compare("msix,appx") == 0);
    REQUIRE(manifest.Commands.compare("makemsix,makeappx") == 0);
    REQUIRE(manifest.Protocols.compare("protocol1,protocol2") == 0);
    REQUIRE(manifest.FileExtensions.compare("appx,appxbundle,msix,msixbundle") == 0);
    REQUIRE(manifest.InstallerType.compare("Zip") == 0);

    // default switches
    REQUIRE(manifest.Switches.has_value());
    InstallerSwitches switches = manifest.Switches.value();
    REQUIRE(switches.Verbose.compare("/verbose") == 0);
    REQUIRE(switches.Silent.compare("/silence") == 0);
    REQUIRE(switches.Default.compare("/default") == 0);

    // installers
    REQUIRE(manifest.Installers.size() == 2);
    ManifestInstaller installer1 = manifest.Installers.at(0);
    REQUIRE(installer1.Arch.compare("x86") == 0);
    REQUIRE(installer1.Url.compare("https://rubengustorage.blob.core.windows.net/publiccontainer/msixsdkx86.zip") == 0);
    REQUIRE(installer1.Sha256.compare("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82") == 0);
    REQUIRE(installer1.Language.compare("en-US") == 0);
    REQUIRE(installer1.InstallerType.compare("Zip") == 0);
    REQUIRE(installer1.Scope.compare("user") == 0);

    REQUIRE(installer1.Switches.has_value());
    InstallerSwitches installer1Switches = installer1.Switches.value();
    REQUIRE(installer1Switches.Verbose.compare("/v") == 0);
    REQUIRE(installer1Switches.Silent.compare("/s") == 0);
    REQUIRE(installer1Switches.Default.compare("/d") == 0);

    ManifestInstaller installer2 = manifest.Installers.at(1);
    REQUIRE(installer2.Arch.compare("x64") == 0);
    REQUIRE(installer2.Url.compare("https://rubengustorage.blob.core.windows.net/publiccontainer/msixsdkx64.zip") == 0);
    REQUIRE(installer2.Sha256.compare("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF0000") == 0);
    REQUIRE(installer2.Language.compare("en-US") == 0);
    REQUIRE(installer2.InstallerType.compare("Zip") == 0);
    REQUIRE(installer2.Scope.compare("user") == 0);

    REQUIRE_FALSE(installer2.Switches.has_value());

    // Localization
    REQUIRE(manifest.Localization.size() == 1);
    ManifestLocalization localization1 = manifest.Localization.at(0);
    REQUIRE(localization1.Language.compare("es-MX") == 0);
    REQUIRE(localization1.Description.compare("El proyecto MSIX SDK es habilita desarrolladores de diferentes") == 0);
    REQUIRE(localization1.Homepage.compare("https://github.com/microsoft/msix-packaging/es-MX") == 0);
    REQUIRE(localization1.LicenseUrl.compare("https://github.com/microsoft/msix-packaging/blob/master/LICENSE-es-MX") == 0);
}

TEST_CASE("Read bad package manifest and fail", "[PackageManifestHelper]")
{
    REQUIRE_THROWS_WITH(Manifest::CreatePackageManifest("BadManifest-MissingName.yml"), "invalid node; first invalid key: \"Name\"");
}
