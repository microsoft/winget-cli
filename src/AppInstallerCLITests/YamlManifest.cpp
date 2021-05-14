// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerSHA256.h>
#include <winget/ManifestYamlParser.h>
#include <winget/Yaml.h>

using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Manifest::YamlParser;
using namespace AppInstaller::Utility;
using namespace AppInstaller::YAML;

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
    auto manifestFile = TestDataFile("Manifest-Good.yaml");
    Manifest manifest = YamlParser::CreateFromPath(manifestFile);

    REQUIRE(manifest.Id == "microsoft.msixsdk");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == "MSIX SDK");
    REQUIRE(manifest.Moniker == "msixsdk");
    REQUIRE(manifest.Version == "1.7.32");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Publisher>() == "Microsoft");
    REQUIRE(manifest.Channel == "release");
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

    // Stream hash
    std::ifstream stream(manifestFile, std::ios_base::in | std::ios_base::binary);
    REQUIRE(!stream.fail());
    auto manifestHash = SHA256::ComputeHash(stream);
    REQUIRE(manifestHash.size() == manifest.StreamSha256.size());
    REQUIRE(std::equal(manifestHash.begin(), manifestHash.end(), manifest.StreamSha256.begin()));
}

TEST_CASE("ReadGoodManifestWithSpaces", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-Spaces.yaml"));

    REQUIRE(manifest.Id == "microsoft.msixsdk");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == "MSIX SDK");
    REQUIRE(manifest.Moniker == "msixsdk");
    REQUIRE(manifest.Version == "1.7.32");
    REQUIRE(manifest.Channel == "release");
    REQUIRE(manifest.DefaultInstallerInfo.MinOSVersion == "0.0.0.0");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Tags>() == MultiValue{ "msix", "appx" });
    REQUIRE(manifest.DefaultInstallerInfo.Commands == MultiValue{ "makemsix", "makeappx" });
    REQUIRE(manifest.DefaultInstallerInfo.Protocols == MultiValue{ "protocol1", "protocol2" });
    REQUIRE(manifest.DefaultInstallerInfo.FileExtensions == MultiValue{ "appx", "appxbundle", "msix", "msixbundle" });
}

struct ManifestExceptionMatcher : public Catch::MatcherBase<ManifestException>
{
    ManifestExceptionMatcher(std::string expectedMessage, bool expectedWarningOnly = false) :
        m_expectedMessage(expectedMessage), m_expectedWarningOnly(expectedWarningOnly) {}

    // Performs the test for this matcher
    bool match(ManifestException const& e) const override
    {
        return e.GetManifestErrorMessage().find(m_expectedMessage) != std::string::npos &&
            e.IsWarningOnly() == m_expectedWarningOnly;
    }

    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << std::boolalpha << "Expected exception message: " << m_expectedMessage << " Expected IsWarningOnly: " << m_expectedWarningOnly;
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
        { "Manifest-Bad-Channel-NotSupported.yaml", "Field is not supported. Field: Channel" },
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
        { "Manifest-Bad-InvalidLocale.yaml", "The locale value is not a well formed bcp47 language tag." },
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

void CopyTestDataFilesToFolder(const std::vector<std::string>& testDataFiles, const std::filesystem::path& dest)
{
    for (const auto& fileName : testDataFiles)
    {
        std::filesystem::copy(TestDataFile(fileName), dest);
    }
}

void VerifyV1ManifestContent(const Manifest& manifest, bool isSingleton)
{
    REQUIRE(manifest.Id == "microsoft.msixsdk");
    REQUIRE(manifest.Version == "1.7.32");
    REQUIRE(manifest.DefaultLocalization.Locale == "en-US");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Publisher>() == "Microsoft");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PublisherUrl>() == "https://www.microsoft.com");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PublisherSupportUrl>() == "https://www.microsoft.com/support");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PrivacyUrl>() == "https://www.microsoft.com/privacy");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Author>() == "Microsoft");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == "MSIX SDK");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageUrl>() == "https://www.microsoft.com/msixsdk/home");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::License>() == "MIT License");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::LicenseUrl>() == "https://www.microsoft.com/msixsdk/license");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Copyright>() == "Copyright Microsoft Corporation");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::CopyrightUrl>() == "https://www.microsoft.com/msixsdk/copyright");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::ShortDescription>() == "This is MSIX SDK");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Description>() == "The MSIX SDK project is an effort to enable developers");
    REQUIRE(manifest.Moniker == "msixsdk");
    REQUIRE(manifest.DefaultLocalization.Get<Localization::Tags>() == MultiValue{ "appxsdk", "msixsdk" });
    REQUIRE(manifest.DefaultInstallerInfo.Locale == "en-US");
    REQUIRE(manifest.DefaultInstallerInfo.Platform == std::vector<PlatformEnum>{ PlatformEnum::Desktop, PlatformEnum::Universal });
    REQUIRE(manifest.DefaultInstallerInfo.MinOSVersion == "10.0.0.0");
    REQUIRE(manifest.DefaultInstallerInfo.InstallerType == InstallerTypeEnum::Zip);
    REQUIRE(manifest.DefaultInstallerInfo.Scope == ScopeEnum::Machine);
    REQUIRE(manifest.DefaultInstallerInfo.InstallModes == std::vector<InstallModeEnum>{ InstallModeEnum::Interactive, InstallModeEnum::Silent, InstallModeEnum::SilentWithProgress });

    auto defaultSwitches = manifest.DefaultInstallerInfo.Switches;
    REQUIRE(defaultSwitches.at(InstallerSwitchType::Custom) == "/custom");
    REQUIRE(defaultSwitches.at(InstallerSwitchType::SilentWithProgress) == "/silentwithprogress");
    REQUIRE(defaultSwitches.at(InstallerSwitchType::Silent) == "/silence");
    REQUIRE(defaultSwitches.at(InstallerSwitchType::Interactive) == "/interactive");
    REQUIRE(defaultSwitches.at(InstallerSwitchType::Log) == "/log=<LOGPATH>");
    REQUIRE(defaultSwitches.at(InstallerSwitchType::InstallLocation) == "/dir=<INSTALLPATH>");
    REQUIRE(defaultSwitches.at(InstallerSwitchType::Update) == "/upgrade");

    REQUIRE(manifest.DefaultInstallerInfo.InstallerSuccessCodes == std::vector<DWORD>{ 1, static_cast<DWORD>(0x80070005) });
    REQUIRE(manifest.DefaultInstallerInfo.UpdateBehavior == UpdateBehaviorEnum::UninstallPrevious);
    REQUIRE(manifest.DefaultInstallerInfo.Commands == MultiValue{ "makemsix", "makeappx" });
    REQUIRE(manifest.DefaultInstallerInfo.Protocols == MultiValue{ "protocol1", "protocol2" });
    REQUIRE(manifest.DefaultInstallerInfo.FileExtensions == MultiValue{ "appx", "msix", "appxbundle", "msixbundle" });

    auto dependencies = manifest.DefaultInstallerInfo.Dependencies;
    REQUIRE(dependencies.WindowsFeatures == MultiValue{ "IIS" });
    REQUIRE(dependencies.WindowsLibraries == MultiValue{ "VC Runtime" });
    REQUIRE(dependencies.PackageDependencies.size() == 1);
    REQUIRE(dependencies.PackageDependencies[0].Id == "Microsoft.MsixSdkDep");
    REQUIRE(dependencies.PackageDependencies[0].MinVersion == "1.0.0");
    REQUIRE(dependencies.ExternalDependencies == MultiValue{ "Outside dependencies" });

    REQUIRE(manifest.DefaultInstallerInfo.Capabilities == MultiValue{ "internetClient" });
    REQUIRE(manifest.DefaultInstallerInfo.RestrictedCapabilities == MultiValue{ "runFullTrust" });
    REQUIRE(manifest.DefaultInstallerInfo.PackageFamilyName == "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe");
    REQUIRE(manifest.DefaultInstallerInfo.ProductCode == "{Foo}");

    if (isSingleton)
    {
        REQUIRE(manifest.Installers.size() == 1);
    }
    else
    {
        REQUIRE(manifest.Installers.size() == 2);
    }

    ManifestInstaller installer1 = manifest.Installers.at(0);
    REQUIRE(installer1.Arch == Architecture::X86);
    REQUIRE(installer1.Locale == "en-GB");
    REQUIRE(installer1.Platform == std::vector<PlatformEnum>{ PlatformEnum::Desktop });
    REQUIRE(installer1.MinOSVersion == "10.0.1.0");
    REQUIRE(installer1.InstallerType == InstallerTypeEnum::Msix);
    REQUIRE(installer1.Url == "https://www.microsoft.com/msixsdk/msixsdkx86.msix");
    REQUIRE(installer1.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
    REQUIRE(installer1.SignatureSha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
    REQUIRE(installer1.Scope == ScopeEnum::User);
    REQUIRE(installer1.InstallModes == std::vector<InstallModeEnum>{ InstallModeEnum::Interactive });

    auto installer1Switches = installer1.Switches;
    REQUIRE(installer1Switches.at(InstallerSwitchType::Custom) == "/c");
    REQUIRE(installer1Switches.at(InstallerSwitchType::SilentWithProgress) == "/sp");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Silent) == "/s");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Interactive) == "/i");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Log) == "/l=<LOGPATH>");
    REQUIRE(installer1Switches.at(InstallerSwitchType::InstallLocation) == "/d=<INSTALLPATH>");
    REQUIRE(installer1Switches.at(InstallerSwitchType::Update) == "/u");

    REQUIRE(installer1.UpdateBehavior == UpdateBehaviorEnum::Install);
    REQUIRE(installer1.Commands == MultiValue{ "makemsixPreview", "makeappxPreview" });
    REQUIRE(installer1.Protocols == MultiValue{ "protocol1preview", "protocol2preview" });
    REQUIRE(installer1.FileExtensions == MultiValue{ "appxbundle", "msixbundle", "appx", "msix" });

    auto installer1Dependencies = installer1.Dependencies;
    REQUIRE(installer1Dependencies.WindowsFeatures == MultiValue{ "PreviewIIS" });
    REQUIRE(installer1Dependencies.WindowsLibraries == MultiValue{ "Preview VC Runtime" });
    REQUIRE(installer1Dependencies.PackageDependencies.size() == 1);
    REQUIRE(installer1Dependencies.PackageDependencies[0].Id == "Microsoft.MsixSdkDepPreview");
    REQUIRE(installer1Dependencies.ExternalDependencies == MultiValue{ "Preview Outside dependencies" });

    REQUIRE(installer1.Capabilities == MultiValue{ "internetClientPreview" });
    REQUIRE(installer1.RestrictedCapabilities == MultiValue{ "runFullTrustPreview" });
    REQUIRE(installer1.PackageFamilyName == "Microsoft.DesktopAppInstallerPreview_8wekyb3d8bbwe");

    if (!isSingleton)
    {
        ManifestInstaller installer2 = manifest.Installers.at(1);
        REQUIRE(installer2.Arch == Architecture::X64);
        REQUIRE(installer2.InstallerType == InstallerTypeEnum::Exe);
        REQUIRE(installer2.Url == "https://www.microsoft.com/msixsdk/msixsdkx64.exe");
        REQUIRE(installer2.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
        REQUIRE(installer2.ProductCode == "{Bar}");

        // Localization
        REQUIRE(manifest.Localizations.size() == 1);
        ManifestLocalization localization1 = manifest.Localizations.at(0);
        REQUIRE(localization1.Locale == "en-GB");
        REQUIRE(localization1.Get<Localization::Publisher>() == "Microsoft UK");
        REQUIRE(localization1.Get<Localization::PublisherUrl>() == "https://www.microsoft.com/UK");
        REQUIRE(localization1.Get<Localization::PublisherSupportUrl>() == "https://www.microsoft.com/support/UK");
        REQUIRE(localization1.Get<Localization::PrivacyUrl>() == "https://www.microsoft.com/privacy/UK");
        REQUIRE(localization1.Get<Localization::Author>() == "Microsoft UK");
        REQUIRE(localization1.Get<Localization::PackageName>() == "MSIX SDK UK");
        REQUIRE(localization1.Get<Localization::PackageUrl>() == "https://www.microsoft.com/msixsdk/home/UK");
        REQUIRE(localization1.Get<Localization::License>() == "MIT License UK");
        REQUIRE(localization1.Get<Localization::LicenseUrl>() == "https://www.microsoft.com/msixsdk/license/UK");
        REQUIRE(localization1.Get<Localization::Copyright>() == "Copyright Microsoft Corporation UK");
        REQUIRE(localization1.Get<Localization::CopyrightUrl>() == "https://www.microsoft.com/msixsdk/copyright/UK");
        REQUIRE(localization1.Get<Localization::ShortDescription>() == "This is MSIX SDK UK");
        REQUIRE(localization1.Get<Localization::Description>() == "The MSIX SDK project is an effort to enable developers UK");
        REQUIRE(localization1.Get<Localization::Tags>() == MultiValue{ "appxsdkUK", "msixsdkUK" });
    }
}

TEST_CASE("ValidateV1GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, true, true);
    VerifyV1ManifestContent(singletonManifest, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1-MultiFile-Version.yaml",
        "ManifestV1-MultiFile-Installer.yaml",
        "ManifestV1-MultiFile-DefaultLocale.yaml",
        "ManifestV1-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, true, true, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false);

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false);
}

YamlManifestInfo CreateYamlManifestInfo(std::string testDataFile)
{
    YamlManifestInfo result;
    result.Root = AppInstaller::YAML::Load(TestDataFile(testDataFile));
    result.FileName = testDataFile;
    return result;
}

TEST_CASE("MultifileManifestInputValidation", "[ManifestValidation]")
{
    auto previewManifest = CreateYamlManifestInfo("Manifest-Good.yaml");
    auto v1SingletonManifest = CreateYamlManifestInfo("ManifestV1-Singleton.yaml");
    auto v1VersionManifest = CreateYamlManifestInfo("ManifestV1-MultiFile-Version.yaml");
    auto v1InstallerManifest = CreateYamlManifestInfo("ManifestV1-MultiFile-Installer.yaml");
    auto v1DefaultLocaleManifest = CreateYamlManifestInfo("ManifestV1-MultiFile-DefaultLocale.yaml");
    auto v1LocaleManifest = CreateYamlManifestInfo("ManifestV1-MultiFile-Locale.yaml");

    {
        // Preview and multi file manifest together
        std::vector<YamlManifestInfo> input = { previewManifest, v1VersionManifest, v1InstallerManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("Preview manifest does not support multi file manifest format"));
    }

    {
        // Singleton and multi file manifest together
        std::vector<YamlManifestInfo> input = { v1SingletonManifest, v1VersionManifest, v1InstallerManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest should not contain file with the particular ManifestType. Field: ManifestType Value: singleton"));
    }

    {
        // More than 1 version manifest
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1VersionManifest, v1InstallerManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest should contain only one file with the particular ManifestType. Field: ManifestType Value: version"));
    }

    {
        // More than 1 installer manifest
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1InstallerManifest, v1InstallerManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest should contain only one file with the particular ManifestType. Field: ManifestType Value: installer"));
    }

    {
        // More than 1 default locale manifest
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1InstallerManifest, v1DefaultLocaleManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest should contain only one file with the particular ManifestType. Field: ManifestType Value: defaultLocale"));
    }

    {
        // Duplicate locales
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1InstallerManifest, v1DefaultLocaleManifest, v1LocaleManifest, v1LocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest contains duplicate PackageLocale. Field: PackageLocale Value: en-GB"));
    }

    {
        // default locale not match
        auto defaultLocaleManifestCopy = v1DefaultLocaleManifest;
        defaultLocaleManifestCopy.Root["PackageLocale"].SetScalar("fr-fr");
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1InstallerManifest, defaultLocaleManifestCopy, v1LocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("DefaultLocale value in version manifest does not match PackageLocale value in defaultLocale manifest"));
    }

    {
        // Package Id does not match
        auto installerManifestCopy = v1InstallerManifest;
        installerManifestCopy.Root["PackageIdentifier"].SetScalar("Another.Identifier");
        std::vector<YamlManifestInfo> input = { v1VersionManifest, installerManifestCopy, v1DefaultLocaleManifest, v1LocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest has inconsistent field values. Field: PackageIdentifier Value: Another.Identifier"));
    }

    {
        // Package Version does not match
        auto installerManifestCopy = v1InstallerManifest;
        installerManifestCopy.Root["PackageVersion"].SetScalar("Another.Version");
        std::vector<YamlManifestInfo> input = { v1VersionManifest, installerManifestCopy, v1DefaultLocaleManifest, v1LocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest has inconsistent field values. Field: PackageVersion Value: Another.Version"));
    }

    {
        // Incomplete multi file manifest, missing installer
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest is incomplete"));
    }

    {
        // Incomplete multi file manifest, missing default locale
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1InstallerManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest is incomplete"));
    }
}

TEST_CASE("ManifestApplyLocale", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-MultiLocale.yaml"));

    // No better alternative locale, default is used.
    manifest.ApplyLocale("zh-CN");
    REQUIRE(manifest.CurrentLocalization.Locale == "es-MX");
    REQUIRE(manifest.CurrentLocalization.Get<Localization::PackageName>() == "es-MX package name");
    REQUIRE(manifest.CurrentLocalization.Get<Localization::Publisher>() == "es-MX publisher");

    // en-US results in en-GB, which is better than default.
    manifest.ApplyLocale("en-US");
    REQUIRE(manifest.CurrentLocalization.Locale == "en-GB");
    REQUIRE(manifest.CurrentLocalization.Get<Localization::PackageName>() == "en-GB package name");
    REQUIRE(manifest.CurrentLocalization.Get<Localization::Publisher>() == "en-GB publisher");

    // fr-FR results in fr-FR, but only package name is localized.
    manifest.ApplyLocale("fr-FR");
    REQUIRE(manifest.CurrentLocalization.Locale == "fr-FR");
    REQUIRE(manifest.CurrentLocalization.Get<Localization::PackageName>() == "fr-FR package name");
    REQUIRE(manifest.CurrentLocalization.Get<Localization::Publisher>() == "es-MX publisher");
}