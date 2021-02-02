// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerSHA256.h>
#include <winget/ManifestYamlParser.h>

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

TEST_CASE("ReadPreviewGoodManifestAndVerifyContents", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good.yaml"));

    REQUIRE(manifest.Id == "microsoft.msixsdk");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == "MSIX SDK");
    REQUIRE(manifest.Moniker == "msixsdk");
    REQUIRE(manifest.Version == "1.7.32");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Publisher>() == "Microsoft");
    REQUIRE(manifest.DefaultInstallerInfo.Channel == "release");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Author>() == "Microsoft");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::License>() == "MIT License");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::LicenseUrl>() == "https://github.com/microsoft/msix-packaging/blob/master/LICENSE");
    REQUIRE(manifest.DefaultInstallerInfo.MinOSVersion == "0.0.0.0");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Description>() == "The MSIX SDK project is an effort to enable developers");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageUrl>() == "https://github.com/microsoft/msix-packaging");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Tags>() == MultiValue{ "msix", "appx" });
    REQUIRE(manifest.DefaultInstallerInfo.Commands == MultiValue{ "makemsix", "makeappx" });
    REQUIRE(manifest.DefaultInstallerInfo.Protocols == MultiValue{ "protocol1", "protocol2" });
    REQUIRE(manifest.DefaultInstallerInfo.FileExtensions == MultiValue{ "appx", "appxbundle", "msix", "msixbundle" });
    REQUIRE(manifest.DefaultInstallerInfo.InstallerType == InstallerTypeEnum::Zip);
    REQUIRE(manifest.DefaultInstallerInfo.PackageFamilyName == "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe");
    REQUIRE(manifest.DefaultInstallerInfo.ProductCode == "{Foo}");
    REQUIRE(manifest.DefaultInstallerInfo.UpdateBehavior == UpdateBehaviorEnum::UninstallPrevious);

    // default switches
    auto switches = manifest.DefaultInstallerInfo.Switches;
    REQUIRE(switches.at(InstallerSwitchType::Custom) == "/custom");
    REQUIRE(switches.at(InstallerSwitchType::SilentWithProgress) == "/silentwithprogress");
    REQUIRE(switches.at(InstallerSwitchType::Silent) == "/silence");
    REQUIRE(switches.at(InstallerSwitchType::Interactive) == "/interactive");
    REQUIRE(switches.at(InstallerSwitchType::Language) == "/en-us");
    REQUIRE(switches.at(InstallerSwitchType::Log) == "/log=<LOGPATH>");
    REQUIRE(switches.at(InstallerSwitchType::InstallLocation) == "/dir=<INSTALLPATH>");
    REQUIRE(switches.at(InstallerSwitchType::Update) == "/update");

    // installers
    REQUIRE(manifest.Installers.size() == 2);
    ManifestInstaller installer1 = manifest.Installers.at(0);
    REQUIRE(installer1.Arch == Architecture::X86);
    REQUIRE(installer1.Url == "https://rubengustorage.blob.core.windows.net/publiccontainer/msixsdkx86.zip");
    REQUIRE(installer1.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
    REQUIRE(installer1.Locale == "en-US");
    REQUIRE(installer1.InstallerType == InstallerTypeEnum::Zip);
    REQUIRE(installer1.Scope == ScopeEnum::User);
    REQUIRE(installer1.PackageFamilyName == "");
    REQUIRE(installer1.ProductCode == "");
    REQUIRE(installer1.UpdateBehavior == UpdateBehaviorEnum::Install);

    auto installer1Switches = installer1.Switches;
    REQUIRE(installer1Switches.at(InstallerSwitchType::Custom) == "/c");
    REQUIRE(installer1Switches.at(InstallerSwitchType::SilentWithProgress) == "/sp");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Silent) == "/s");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Interactive) == "/i");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Language) == "/en");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Log) == "/l=<LOGPATH>");
    REQUIRE(installer1Switches.at(InstallerSwitchType::InstallLocation) == "/d=<INSTALLPATH>");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Update) == "/u");

    ManifestInstaller installer2 = manifest.Installers.at(1);
    REQUIRE(installer2.Arch == Architecture::X64);
    REQUIRE(installer2.Url == "https://rubengustorage.blob.core.windows.net/publiccontainer/msixsdkx64.zip");
    REQUIRE(installer2.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF0000"));
    REQUIRE(installer2.Locale == "en-US");
    REQUIRE(installer2.InstallerType == InstallerTypeEnum::Zip);
    REQUIRE(installer2.Scope == ScopeEnum::User);
    REQUIRE(installer2.PackageFamilyName == "");
    REQUIRE(installer2.ProductCode == "");
    REQUIRE(installer2.UpdateBehavior == UpdateBehaviorEnum::UninstallPrevious);

    // Installer2 does not declare switches, it inherits switches from package default.
    auto installer2Switches = installer2.Switches;
    REQUIRE(installer2Switches.at(InstallerSwitchType::Custom) == "/custom");
    REQUIRE(installer2Switches.at(InstallerSwitchType::SilentWithProgress) == "/silentwithprogress");
    REQUIRE(installer2Switches.at(InstallerSwitchType::Silent) == "/silence");
    REQUIRE(installer2Switches.at(InstallerSwitchType::Interactive) == "/interactive");
    REQUIRE(installer2Switches.at(InstallerSwitchType::Language) == "/en-us");
    REQUIRE(installer2Switches.at(InstallerSwitchType::Log) == "/log=<LOGPATH>");
    REQUIRE(installer2Switches.at(InstallerSwitchType::InstallLocation) == "/dir=<INSTALLPATH>");
    REQUIRE(installer2Switches.at(InstallerSwitchType::Update) == "/update");

    // Localization
    REQUIRE(manifest.Localizations.size() == 1);
    ManifestLocalization localization1 = manifest.Localizations.at(0);
    REQUIRE(localization1.Locale == "es-MX");
    REQUIRE(localization1.Get<Localization::Description>() == "El proyecto MSIX SDK es habilita desarrolladores de diferentes");
    REQUIRE(localization1.Get<Localization::PackageUrl>() == "https://github.com/microsoft/msix-packaging/es-MX");
    REQUIRE(localization1.Get<Localization::LicenseUrl>() == "https://github.com/microsoft/msix-packaging/blob/master/LICENSE-es-MX");
}

TEST_CASE("ReadGoodManifestWithSpaces", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-Spaces.yaml"));

    REQUIRE(manifest.Id == "microsoft.msixsdk");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == "MSIX SDK");
    REQUIRE(manifest.Moniker == "msixsdk");
    REQUIRE(manifest.Version == "1.7.32");
    REQUIRE(manifest.DefaultInstallerInfo.Channel == "release");
    REQUIRE(manifest.DefaultInstallerInfo.MinOSVersion == "0.0.0.0");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Tags>() == MultiValue{ "msix", "appx" });
    REQUIRE(manifest.DefaultInstallerInfo.Commands == MultiValue{ "makemsix", "makeappx" });
    REQUIRE(manifest.DefaultInstallerInfo.Protocols == MultiValue{ "protocol1", "protocol2" });
    REQUIRE(manifest.DefaultInstallerInfo.FileExtensions == MultiValue{ "appx", "appxbundle", "msix", "msixbundle" });
}

struct ManifestExceptionMatcher : public Catch::MatcherBase<ManifestException>
{
    ManifestExceptionMatcher(std::string expectedMessage, bool expectedWarningOnly) :
        m_expectedMessage(expectedMessage), m_expectedWarningOnly(expectedWarningOnly) {}

    // Performs the test for this matcher
    bool match(ManifestException const& e) const override
    {
        return e.GetManifestErrorMessage().find(m_expectedMessage) != std::string::npos &&
            e.IsWarningOnly() == m_expectedWarningOnly;
    }

    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << std::boolalpha << "Expected exception message: " << m_expectedMessage << "Expected IsWarningOnly: " << m_expectedWarningOnly;
        return ss.str();
    }

private:
    std::string m_expectedMessage;
    bool m_expectedWarningOnly;
};

void TestManifest(const std::filesystem::path& manifestPath, const std::string& expectedMessage = {}, bool expectedWarningOnly = false)
{
    INFO(manifestPath.u8string());
    if (expectedMessage.empty())
    {
        CHECK_NOTHROW(YamlParser::CreateFromPath(TestDataFile(manifestPath), true, true));
    }
    else
    {
        CHECK_THROWS_MATCHES(YamlParser::CreateFromPath(TestDataFile(manifestPath), true, true), ManifestException, ManifestExceptionMatcher(expectedMessage, expectedWarningOnly));
    }
}

struct ManifestTestCase
{
    std::string TestFile;
    std::string ExpectedMessage = {};
    bool IsWarningOnly = false;
};

TEST_CASE("ReadGoodManifests", "[ManifestValidation]")
{
    ManifestTestCase TestCases[] =
    {
        { "Manifest-Good-InstallerTypeExeRoot-Silent.yaml" },
        { "Manifest-Good-InstallerTypeExeRoot-SilentRoot.yaml" },
        { "Manifest-Good-InstallerTypeExe-Silent.yaml" },
        { "Manifest-Good-InstallerTypeExe-SilentRoot.yaml" },
        { "Manifest-Good-Installeruniqueness-DefaultLang.yaml" },
        { "Manifest-Good-Installeruniqueness-DiffLangs.yaml" },
        { "Manifest-Good-InstallerUniqueness-DiffScope.yaml" },
        { "Manifest-Good-Minimum.yaml" },
        { "Manifest-Good-Minimum-InstallerType.yaml" },
        { "Manifest-Good-Switches.yaml" },
    };

    for (auto const& testCase : TestCases)
    {
        TestManifest(testCase.TestFile);
    }
}

TEST_CASE("ReadBadManifests", "[ManifestValidation]")
{
    ManifestTestCase TestCases[] =
    {
        { "Manifest-Bad-ArchInvalid.yaml", "Invalid field value. Field: Arch" },
        { "Manifest-Bad-ArchMissing.yaml", "Missing required property 'Arch'" },
        { "Manifest-Bad-DifferentCase-camelCase.yaml", "All field names should be PascalCased. Field: installerType" },
        { "Manifest-Bad-DifferentCase-lower.yaml", "All field names should be PascalCased. Field: installertype" },
        { "Manifest-Bad-DifferentCase-UPPER.yaml", "All field names should be PascalCased. Field: INSTALLERTYPE" },
        { "Manifest-Bad-DuplicateKey.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-DuplicateKey-DifferentCase.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-DuplicateKey-DifferentCase-lower.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-IdInvalid.yaml", "Failed to validate against schema associated with property name 'Id'" },
        { "Manifest-Bad-IdMissing.yaml", "Missing required property 'Id'" },
        { "Manifest-Bad-InstallersMissing.yaml", "Missing required property 'Installers'" },
        { "Manifest-Bad-InstallerTypeExe-NoSilent.yaml", "Silent and SilentWithProgress switches are not specified for InstallerType exe.", true },
        { "Manifest-Bad-InstallerTypeExe-NoSilentRoot.yaml", "Silent and SilentWithProgress switches are not specified for InstallerType exe.", true },
        { "Manifest-Bad-InstallerTypeExeRoot-NoSilent.yaml", "Silent and SilentWithProgress switches are not specified for InstallerType exe.", true },
        { "Manifest-Bad-InstallerTypeExeRoot-NoSilentRoot.yaml", "Silent and SilentWithProgress switches are not specified for InstallerType exe.", true },
        { "Manifest-Bad-InstallerTypeInvalid.yaml", "Invalid field value. Field: InstallerType" },
        { "Manifest-Bad-InstallerTypeMissing.yaml", "Invalid field value. Field: InstallerType" },
        { "Manifest-Bad-InstallerUniqueness.yaml", "Duplicate installer entry found." },
        { "Manifest-Bad-InstallerUniqueness-DefaultScope.yaml", "Duplicate installer entry found." },
        { "Manifest-Bad-InstallerUniqueness-DefaultValues.yaml", "Duplicate installer entry found." },
        { "Manifest-Bad-InstallerUniqueness-SameLang.yaml", "Duplicate installer entry found." },
        { "Manifest-Bad-LicenseMissing.yaml", "Missing required property 'License'" },
        { "Manifest-Bad-NameMissing.yaml", "Missing required property 'Name'" },
        { "Manifest-Bad-PublisherMissing.yaml", "Missing required property 'Publisher'" },
        { "Manifest-Bad-Sha256Invalid.yaml", "Failed to validate against schema associated with property name 'Sha256'" },
        { "Manifest-Bad-Sha256Missing.yaml", "Required field missing. Field: Sha256" },
        { "Manifest-Bad-SwitchInvalid.yaml", "Unknown field. Field: NotASwitch", true },
        { "Manifest-Bad-UnknownProperty.yaml", "Unknown field. Field: Fake", true },
        { "Manifest-Bad-UnsupportedVersion.yaml", "Unsupported ManifestVersion" },
        { "Manifest-Bad-UrlInvalid.yaml", "Invalid field value. Field: Url" },
        { "Manifest-Bad-UrlMissing.yaml", "Required field missing. Field: Url" },
        { "Manifest-Bad-VersionInvalid.yaml", "Failed to validate against schema associated with property name 'Version'" },
        { "Manifest-Bad-VersionMissing.yaml", "Missing required property 'Version'" },
        { "Manifest-Bad-InvalidManifestVersionValue.yaml", "Failed to validate against schema associated with property name 'ManifestVersion'" },
        { "InstallFlowTest_MSStore.yaml", "Field value is not supported. Field: InstallerType Value: MSStore" },
        { "Manifest-Bad-PackageFamilyNameOnMSI.yaml", "The specified installer type does not support PackageFamilyName. Field: InstallerType Value: Msi" },
        { "Manifest-Bad-ProductCodeOnMSIX.yaml", "The specified installer type does not support ProductCode. Field: InstallerType Value: Msix" },
        { "Manifest-Bad-InvalidUpdateBehavior.yaml", "Invalid field value. Field: UpdateBehavior" },
    };

    for (auto const& testCase : TestCases)
    {
        TestManifest(testCase.TestFile, testCase.ExpectedMessage, testCase.IsWarningOnly);
    }
}

TEST_CASE("ManifestEncoding", "[ManifestValidation]")
{
    ManifestTestCase TestCases[] =
    {
        { "Manifest-Encoding-ANSI.yaml" },
        { "Manifest-Encoding-UTF8.yaml" },
        { "Manifest-Encoding-UTF8-BOM.yaml" },
        { "Manifest-Encoding-UTF16BE.yaml" },
        { "Manifest-Encoding-UTF16BE-BOM.yaml" },
        { "Manifest-Encoding-UTF16LE.yaml" },
        { "Manifest-Encoding-UTF16LE-BOM.yaml" },
    };

    for (auto const& testCase : TestCases)
    {
        INFO(testCase.TestFile);
        Manifest manifest = YamlParser::CreateFromPath(TestDataFile(testCase.TestFile), true, true);
        REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == u8"MSIX SDK\xA9");
    }
}

TEST_CASE("ComplexSystemReference", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-SystemReferenceComplex.yaml"));

    REQUIRE(manifest.Installers.size() == 5);

    // Zip installer does not inherit
    REQUIRE(manifest.Installers[0].InstallerType == InstallerTypeEnum::Zip);
    REQUIRE(manifest.Installers[0].PackageFamilyName == "");
    REQUIRE(manifest.Installers[0].ProductCode == "");

    // MSIX installer does inherit
    REQUIRE(manifest.Installers[1].InstallerType == InstallerTypeEnum::Msix);
    REQUIRE(manifest.Installers[1].Arch == Architecture::X86);
    REQUIRE(manifest.Installers[1].PackageFamilyName == "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe");
    REQUIRE(manifest.Installers[1].ProductCode == "");

    // MSI installer does inherit
    REQUIRE(manifest.Installers[2].InstallerType == InstallerTypeEnum::Msi);
    REQUIRE(manifest.Installers[2].Arch == Architecture::X86);
    REQUIRE(manifest.Installers[2].PackageFamilyName == "");
    REQUIRE(manifest.Installers[2].ProductCode == "{Foo}");

    // MSIX installer with override
    REQUIRE(manifest.Installers[3].InstallerType == InstallerTypeEnum::Msix);
    REQUIRE(manifest.Installers[3].Arch == Architecture::X64);
    REQUIRE(manifest.Installers[3].PackageFamilyName == "Override_8wekyb3d8bbwe");
    REQUIRE(manifest.Installers[3].ProductCode == "");

    // MSI installer with override
    REQUIRE(manifest.Installers[4].InstallerType == InstallerTypeEnum::Msi);
    REQUIRE(manifest.Installers[4].Arch == Architecture::X64);
    REQUIRE(manifest.Installers[4].PackageFamilyName == "");
    REQUIRE(manifest.Installers[4].ProductCode == "Override");
}

TEST_CASE("ManifestVersionExtensions", "[ManifestValidation]")
{
    REQUIRE(!ManifestVer("1.0.0"sv).HasExtension("msstore"));
    REQUIRE(!ManifestVer("1.0.0-other"sv).HasExtension("msstore"));
    REQUIRE(!ManifestVer("1.0.0-other-other2"sv).HasExtension("msstore"));

    REQUIRE(ManifestVer("1.0.0-msstore"sv).HasExtension("msstore"));
    REQUIRE(ManifestVer("1.0.0-msstore.2"sv).HasExtension("msstore"));
    REQUIRE(ManifestVer("1.0.0-other-msstore.2"sv).HasExtension("msstore"));
    REQUIRE(ManifestVer("1.0.0-msstore.2-other"sv).HasExtension("msstore"));
}
