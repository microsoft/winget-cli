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

TEST_CASE("ReadGoodManifestAndVerifyContents", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good.yaml"));

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
    REQUIRE(manifest.PackageFamilyName == "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe");
    REQUIRE(manifest.ProductCode == "{Foo}");
    REQUIRE(manifest.UpdateBehavior == ManifestInstaller::UpdateBehaviorEnum::UninstallPrevious);

    // default switches
    auto switches = manifest.Switches;
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Custom) == "/custom");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::SilentWithProgress) == "/silentwithprogress");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Silent) == "/silence");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Interactive) == "/interactive");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Language) == "/en-us");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Log) == "/log=<LOGPATH>");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::InstallLocation) == "/dir=<INSTALLPATH>");
    REQUIRE(switches.at(ManifestInstaller::InstallerSwitchType::Update) == "/update");

    // installers
    REQUIRE(manifest.Installers.size() == 2);
    ManifestInstaller installer1 = manifest.Installers.at(0);
    REQUIRE(installer1.Arch == Architecture::X86);
    REQUIRE(installer1.Url == "https://rubengustorage.blob.core.windows.net/publiccontainer/msixsdkx86.zip");
    REQUIRE(installer1.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
    REQUIRE(installer1.Language == "en-US");
    REQUIRE(installer1.InstallerType == ManifestInstaller::InstallerTypeEnum::Zip);
    REQUIRE(installer1.Scope == ManifestInstaller::ScopeEnum::User);
    REQUIRE(installer1.PackageFamilyName == "");
    REQUIRE(installer1.ProductCode == "");
    REQUIRE(installer1.UpdateBehavior == ManifestInstaller::UpdateBehaviorEnum::Install);

    auto installer1Switches = installer1.Switches;
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Custom) == "/c");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::SilentWithProgress) == "/sp");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Silent) == "/s");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Interactive) == "/i");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Language) == "/en");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Log) == "/l=<LOGPATH>");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::InstallLocation) == "/d=<INSTALLPATH>");
    REQUIRE(installer1Switches.at(ManifestInstaller::InstallerSwitchType::Update) == "/u");

    ManifestInstaller installer2 = manifest.Installers.at(1);
    REQUIRE(installer2.Arch == Architecture::X64);
    REQUIRE(installer2.Url == "https://rubengustorage.blob.core.windows.net/publiccontainer/msixsdkx64.zip");
    REQUIRE(installer2.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF0000"));
    REQUIRE(installer2.Language == "en-US");
    REQUIRE(installer2.InstallerType == ManifestInstaller::InstallerTypeEnum::Zip);
    REQUIRE(installer2.Scope == ManifestInstaller::ScopeEnum::User);
    REQUIRE(installer2.PackageFamilyName == "");
    REQUIRE(installer2.ProductCode == "");
    REQUIRE(installer2.UpdateBehavior == ManifestInstaller::UpdateBehaviorEnum::UninstallPrevious);

    // Installer2 does not declare switches, it inherits switches from package default.
    auto installer2Switches = installer2.Switches;
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Custom) == "/custom");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::SilentWithProgress) == "/silentwithprogress");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Silent) == "/silence");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Interactive) == "/interactive");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Language) == "/en-us");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Log) == "/log=<LOGPATH>");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::InstallLocation) == "/dir=<INSTALLPATH>");
    REQUIRE(installer2Switches.at(ManifestInstaller::InstallerSwitchType::Update) == "/update");

    // Localization
    REQUIRE(manifest.Localization.size() == 1);
    ManifestLocalization localization1 = manifest.Localization.at(0);
    REQUIRE(localization1.Language == "es-MX");
    REQUIRE(localization1.Description == "El proyecto MSIX SDK es habilita desarrolladores de diferentes");
    REQUIRE(localization1.Homepage == "https://github.com/microsoft/msix-packaging/es-MX");
    REQUIRE(localization1.LicenseUrl == "https://github.com/microsoft/msix-packaging/blob/master/LICENSE-es-MX");
}

TEST_CASE("ReadGoodManifestWithSpaces", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-Spaces.yaml"));

    REQUIRE(manifest.Id == "microsoft.msixsdk");
    REQUIRE(manifest.Name == "MSIX SDK");
    REQUIRE(manifest.AppMoniker == "msixsdk");
    REQUIRE(manifest.Version == "1.7.32");
    REQUIRE(manifest.Channel == "release");
    REQUIRE(manifest.MinOSVersion == "0.0.0.0");
    REQUIRE(manifest.Tags == MultiValue{ "msix", "appx" });
    REQUIRE(manifest.Commands == MultiValue{ "makemsix", "makeappx" });
    REQUIRE(manifest.Protocols == MultiValue{ "protocol1", "protocol2" });
    REQUIRE(manifest.FileExtensions == MultiValue{ "appx", "appxbundle", "msix", "msixbundle" });
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
        { "Manifest-Bad-ArchMissing.yaml", "Required field missing. Field: Arch" },
        { "Manifest-Bad-Channel-NotSupported.yaml", "Field is not supported. Field: Channel" },
        { "Manifest-Bad-DifferentCase-camelCase.yaml", "All field names should be PascalCased. Field: installerType" },
        { "Manifest-Bad-DifferentCase-lower.yaml", "All field names should be PascalCased. Field: installertype" },
        { "Manifest-Bad-DifferentCase-UPPER.yaml", "All field names should be PascalCased. Field: INSTALLERTYPE" },
        { "Manifest-Bad-DuplicateKey.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-DuplicateKey-DifferentCase.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-DuplicateKey-DifferentCase-lower.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-IdInvalid.yaml", "Invalid field value. Field: Id" },
        { "Manifest-Bad-IdMissing.yaml", "Required field missing. Field: Id" },
        { "Manifest-Bad-InstallersMissing.yaml", "Required field missing. Field: Installers" },
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
        { "Manifest-Bad-LicenseMissing.yaml", "Required field missing. Field: License" },
        { "Manifest-Bad-NameMissing.yaml", "Required field missing. Field: Name" },
        { "Manifest-Bad-PublisherMissing.yaml", "Required field missing. Field: Publisher" },
        { "Manifest-Bad-Sha256Invalid.yaml", "Invalid field value. Field: Sha256" },
        { "Manifest-Bad-Sha256Missing.yaml", "Required field missing. Field: Sha256" },
        { "Manifest-Bad-SwitchInvalid.yaml", "Unknown field. Field: NotASwitch", true },
        { "Manifest-Bad-UnknownProperty.yaml", "Unknown field. Field: Fake", true },
        { "Manifest-Bad-UnsupportedVersion.yaml", "Unsupported ManifestVersion" },
        { "Manifest-Bad-UrlInvalid.yaml", "Invalid field value. Field: Url" },
        { "Manifest-Bad-UrlMissing.yaml", "Required field missing. Field: Url" },
        { "Manifest-Bad-VersionInvalid.yaml", "Invalid field value. Field: Version" },
        { "Manifest-Bad-VersionMissing.yaml", "Required field missing. Field: Version" },
        { "Manifest-Bad-InvalidManifestVersionValue.yaml", "Invalid field value. Field: ManifestVersion" },
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
        REQUIRE(manifest.Name == u8"MSIX SDK\xA9");
    }
}

TEST_CASE("ComplexSystemReference", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-SystemReferenceComplex.yaml"));

    REQUIRE(manifest.Installers.size() == 5);

    // Zip installer does not inherit
    REQUIRE(manifest.Installers[0].InstallerType == ManifestInstaller::InstallerTypeEnum::Zip);
    REQUIRE(manifest.Installers[0].PackageFamilyName == "");
    REQUIRE(manifest.Installers[0].ProductCode == "");

    // MSIX installer does inherit
    REQUIRE(manifest.Installers[1].InstallerType == ManifestInstaller::InstallerTypeEnum::Msix);
    REQUIRE(manifest.Installers[1].Arch == Architecture::X86);
    REQUIRE(manifest.Installers[1].PackageFamilyName == "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe");
    REQUIRE(manifest.Installers[1].ProductCode == "");

    // MSI installer does inherit
    REQUIRE(manifest.Installers[2].InstallerType == ManifestInstaller::InstallerTypeEnum::Msi);
    REQUIRE(manifest.Installers[2].Arch == Architecture::X86);
    REQUIRE(manifest.Installers[2].PackageFamilyName == "");
    REQUIRE(manifest.Installers[2].ProductCode == "{Foo}");

    // MSIX installer with override
    REQUIRE(manifest.Installers[3].InstallerType == ManifestInstaller::InstallerTypeEnum::Msix);
    REQUIRE(manifest.Installers[3].Arch == Architecture::X64);
    REQUIRE(manifest.Installers[3].PackageFamilyName == "Override_8wekyb3d8bbwe");
    REQUIRE(manifest.Installers[3].ProductCode == "");

    // MSI installer with override
    REQUIRE(manifest.Installers[4].InstallerType == ManifestInstaller::InstallerTypeEnum::Msi);
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
