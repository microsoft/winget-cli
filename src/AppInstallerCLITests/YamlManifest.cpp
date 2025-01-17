// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerSHA256.h>
#include <winget/ManifestYamlParser.h>
#include <winget/ManifestYamlWriter.h>
#include <winget/Yaml.h>

using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Manifest::YamlParser;
using namespace AppInstaller::Manifest::YamlWriter;
using namespace AppInstaller::Utility;
using namespace AppInstaller::YAML;

namespace
{
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

    void ValidateError(
        const ValidationError& error,
        ValidationError::Level level,
        AppInstaller::StringResource::StringId message,
        std::string field,
        std::string value)
    {
        REQUIRE(level == error.ErrorLevel);
        REQUIRE(message == error.Message);
        REQUIRE(field == error.Context);
        REQUIRE(value == error.Value);
    }

    void ValidateError(const ValidationError& error, ValidationError::Level level, AppInstaller::StringResource::StringId message)
    {
        ValidateError(error, level, message, std::string(), std::string());
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

    ManifestValidateOption GetTestManifestValidateOption(
        bool schemaValidationOnly = false,
        bool errorOnVerifiedPublisher = false)
    {
        ManifestValidateOption validateOption;
        validateOption.FullValidation = true;
        validateOption.ThrowOnWarning = true;
        validateOption.SchemaValidationOnly = schemaValidationOnly;
        validateOption.ErrorOnVerifiedPublisherFields = errorOnVerifiedPublisher;
        return validateOption;
    }

    void TestManifest(
        const std::filesystem::path& manifestPath,
        const std::string& expectedMessage = {},
        bool expectedWarningOnly = false,
        ManifestValidateOption validateOption = GetTestManifestValidateOption())
    {
        INFO(manifestPath.u8string());

        if (expectedMessage.empty())
        {
            CHECK_NOTHROW(YamlParser::CreateFromPath(TestDataFile(manifestPath), validateOption));
        }
        else
        {
            CHECK_THROWS_MATCHES(YamlParser::CreateFromPath(TestDataFile(manifestPath), validateOption), ManifestException, ManifestExceptionMatcher(expectedMessage, expectedWarningOnly));
        }
    }

    struct ManifestTestCase
    {
        std::string TestFile;
        std::string ExpectedMessage = {};
        bool IsWarningOnly = false;
        ManifestValidateOption ValidateOption = GetTestManifestValidateOption();
    };

    void CopyTestDataFilesToFolder(const std::vector<std::string>& testDataFiles, const std::filesystem::path& dest)
    {
        for (const auto& fileName : testDataFiles)
        {
            std::filesystem::copy(TestDataFile(fileName), dest);
        }
    }

    void VerifyV1ManifestContent(const Manifest& manifest, bool isSingleton, ManifestVer manifestVer = { s_ManifestVersionV1 }, bool isExported = false)
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

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            REQUIRE(manifest.DefaultLocalization.Get<Localization::ReleaseNotes>() == "Default release notes");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::ReleaseNotesUrl>() == "https://DefaultReleaseNotes.net");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Agreements>().size() == 1);
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Agreements>().at(0).Label == "DefaultLabel");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Agreements>().at(0).AgreementText == "DefaultText");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Agreements>().at(0).AgreementUrl == "https://DefaultAgreementUrl.net");
        }

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_2 })
        {
            REQUIRE(manifest.DefaultLocalization.Get<Localization::PurchaseUrl>() == "https://DefaultPurchaseUrl.com");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::InstallationNotes>() == "Default installation notes");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Documentations>().size() == 1);
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Documentations>().at(0).DocumentLabel == "Default document label");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Documentations>().at(0).DocumentUrl == "https://DefaultDocumentUrl.com");
        }

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_5 })
        {
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().size() == 1);
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Url == "https://testIcon-en-US");
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).FileType == IconFileTypeEnum::Ico);
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Resolution == IconResolutionEnum::Custom);
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Theme == IconThemeEnum::Default);
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8123"));
        }

        if (!isExported)
        {
            REQUIRE(manifest.DefaultInstallerInfo.Locale == "en-US");
            REQUIRE(manifest.DefaultInstallerInfo.Platform == std::vector<PlatformEnum>{ PlatformEnum::Desktop, PlatformEnum::Universal });
            REQUIRE(manifest.DefaultInstallerInfo.MinOSVersion == "10.0.0.0");
            REQUIRE(manifest.DefaultInstallerInfo.BaseInstallerType == InstallerTypeEnum::Exe);
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
            REQUIRE(dependencies.HasExactDependency(DependencyType::WindowsFeature, "IIS"));
            REQUIRE(dependencies.HasExactDependency(DependencyType::WindowsLibrary, "VC Runtime"));
            REQUIRE(dependencies.HasExactDependency(DependencyType::Package, "Microsoft.MsixSdkDep", "1.0.0"));
            REQUIRE(dependencies.HasExactDependency(DependencyType::External, "Outside dependencies"));
            REQUIRE(dependencies.Size() == 4);

            REQUIRE(manifest.DefaultInstallerInfo.Capabilities == MultiValue{ "internetClient" });
            REQUIRE(manifest.DefaultInstallerInfo.RestrictedCapabilities == MultiValue{ "runFullTrust" });
            REQUIRE(manifest.DefaultInstallerInfo.PackageFamilyName == "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe");
            REQUIRE(manifest.DefaultInstallerInfo.ProductCode == "{Foo}");

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_1 })
            {
                REQUIRE(manifest.DefaultInstallerInfo.ReleaseDate == "2021-01-01");
                REQUIRE(manifest.DefaultInstallerInfo.InstallerAbortsTerminal);
                REQUIRE(manifest.DefaultInstallerInfo.InstallLocationRequired);
                REQUIRE(manifest.DefaultInstallerInfo.RequireExplicitUpgrade);
                REQUIRE(manifest.DefaultInstallerInfo.ElevationRequirement == ElevationRequirementEnum::ElevatesSelf);
                REQUIRE(manifest.DefaultInstallerInfo.UnsupportedOSArchitectures.size() == 1);
                REQUIRE(manifest.DefaultInstallerInfo.UnsupportedOSArchitectures.at(0) == Architecture::Arm);
                REQUIRE(manifest.DefaultInstallerInfo.AppsAndFeaturesEntries.size() == 1);
                REQUIRE(manifest.DefaultInstallerInfo.AppsAndFeaturesEntries.at(0).DisplayName == "DisplayName");
                REQUIRE(manifest.DefaultInstallerInfo.AppsAndFeaturesEntries.at(0).DisplayVersion == "DisplayVersion");
                REQUIRE(manifest.DefaultInstallerInfo.AppsAndFeaturesEntries.at(0).Publisher == "Publisher");
                REQUIRE(manifest.DefaultInstallerInfo.AppsAndFeaturesEntries.at(0).ProductCode == "ProductCode");
                REQUIRE(manifest.DefaultInstallerInfo.AppsAndFeaturesEntries.at(0).UpgradeCode == "UpgradeCode");
                REQUIRE(manifest.DefaultInstallerInfo.AppsAndFeaturesEntries.at(0).InstallerType == InstallerTypeEnum::Exe);
                REQUIRE(manifest.DefaultInstallerInfo.Markets.AllowedMarkets.size() == 1);
                REQUIRE(manifest.DefaultInstallerInfo.Markets.AllowedMarkets.at(0) == "US");
                REQUIRE(manifest.DefaultInstallerInfo.ExpectedReturnCodes.size() == 1);
                REQUIRE(manifest.DefaultInstallerInfo.ExpectedReturnCodes.at(10).ReturnResponseEnum == ExpectedReturnCodeEnum::PackageInUse);
            }

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_2 })
            {
                REQUIRE(manifest.DefaultInstallerInfo.DisplayInstallWarnings);
                REQUIRE(manifest.DefaultInstallerInfo.UnsupportedArguments.size() == 1);
                REQUIRE(manifest.DefaultInstallerInfo.UnsupportedArguments.at(0) == UnsupportedArgumentEnum::Log);
            }

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_4 })
            {
                REQUIRE(manifest.DefaultInstallerInfo.NestedInstallerType == InstallerTypeEnum::Msi);
                REQUIRE(manifest.DefaultInstallerInfo.NestedInstallerFiles.size() == 1);
                REQUIRE(manifest.DefaultInstallerInfo.NestedInstallerFiles.at(0).RelativeFilePath == "RelativeFilePath");
                REQUIRE(manifest.DefaultInstallerInfo.NestedInstallerFiles.at(0).PortableCommandAlias == "PortableCommandAlias");
                REQUIRE(manifest.DefaultInstallerInfo.InstallationMetadata.DefaultInstallLocation == "%ProgramFiles%\\TestApp");
                REQUIRE(manifest.DefaultInstallerInfo.InstallationMetadata.Files.size() == 1);
                REQUIRE(manifest.DefaultInstallerInfo.InstallationMetadata.Files.at(0).RelativeFilePath == "main.exe");
                REQUIRE(manifest.DefaultInstallerInfo.InstallationMetadata.Files.at(0).FileType == InstalledFileTypeEnum::Launch);
                REQUIRE(manifest.DefaultInstallerInfo.InstallationMetadata.Files.at(0).FileSha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
                REQUIRE(manifest.DefaultInstallerInfo.InstallationMetadata.Files.at(0).InvocationParameter == "/arg");
            }

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_6 })
            {
                REQUIRE(manifest.DefaultInstallerInfo.DownloadCommandProhibited);
            }

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_7 })
            {
                REQUIRE(defaultSwitches.at(InstallerSwitchType::Repair) == "/repair");
                REQUIRE(manifest.DefaultInstallerInfo.RepairBehavior == RepairBehaviorEnum::Modify);
            }

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_9 })
            {
                REQUIRE(manifest.DefaultInstallerInfo.ArchiveBinariesDependOnPath);
            }
        }

        if (isSingleton || isExported)
        {
            REQUIRE(manifest.Installers.size() == 1);
        }
        else
        {
            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_7 })
            {
                REQUIRE(manifest.Installers.size() == 5);
            }
            else if (manifestVer >= ManifestVer{ s_ManifestVersionV1_4 })
            {
                REQUIRE(manifest.Installers.size() == 4);
            }
            else if (manifestVer == ManifestVer{ s_ManifestVersionV1_2 })
            {
                REQUIRE(manifest.Installers.size() == 3);
            }
            else
            {
                REQUIRE(manifest.Installers.size() == 2);
            }
        }

        ManifestInstaller installer1 = manifest.Installers.at(0);
        REQUIRE(installer1.Arch == Architecture::X86);
        REQUIRE(installer1.Locale == "en-GB");
        REQUIRE(installer1.Platform == std::vector<PlatformEnum>{ PlatformEnum::Desktop });
        REQUIRE(installer1.MinOSVersion == "10.0.1.0");
        REQUIRE(installer1.BaseInstallerType == InstallerTypeEnum::Msix);
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
        REQUIRE(installer1Dependencies.HasExactDependency(DependencyType::WindowsFeature, "PreviewIIS"));
        REQUIRE(installer1Dependencies.HasExactDependency(DependencyType::WindowsLibrary, "Preview VC Runtime"));
        REQUIRE(installer1Dependencies.HasExactDependency(DependencyType::Package, "Microsoft.MsixSdkDepPreview", "1.0.0"));
        REQUIRE(installer1Dependencies.HasExactDependency(DependencyType::External, "Preview Outside dependencies"));
        REQUIRE(installer1Dependencies.Size() == 4);

        REQUIRE(installer1.Capabilities == MultiValue{ "internetClientPreview" });
        REQUIRE(installer1.RestrictedCapabilities == MultiValue{ "runFullTrustPreview" });
        REQUIRE(installer1.PackageFamilyName == "Microsoft.DesktopAppInstallerPreview_8wekyb3d8bbwe");

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            REQUIRE(installer1.ReleaseDate == "2021-02-02");
            REQUIRE_FALSE(installer1.InstallerAbortsTerminal);
            REQUIRE_FALSE(installer1.InstallLocationRequired);
            REQUIRE_FALSE(installer1.RequireExplicitUpgrade);
            REQUIRE(installer1.ElevationRequirement == ElevationRequirementEnum::ElevationRequired);
            REQUIRE(installer1.UnsupportedOSArchitectures.size() == 1);
            REQUIRE(installer1.UnsupportedOSArchitectures.at(0) == Architecture::Arm64);
            REQUIRE(installer1.AppsAndFeaturesEntries.size() == 0);
            REQUIRE(installer1.Markets.AllowedMarkets.size() == 0);
            REQUIRE(installer1.Markets.ExcludedMarkets.size() == 1);
            REQUIRE(installer1.Markets.ExcludedMarkets.at(0) == "US");
            REQUIRE(installer1.ExpectedReturnCodes.at(2).ReturnResponseEnum == ExpectedReturnCodeEnum::ContactSupport);
        }

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_2 })
        {
            REQUIRE_FALSE(installer1.DisplayInstallWarnings);
            REQUIRE(installer1.ExpectedReturnCodes.at(3).ReturnResponseEnum == ExpectedReturnCodeEnum::Custom);
            REQUIRE(installer1.ExpectedReturnCodes.at(3).ReturnResponseUrl == "https://defaultReturnResponseUrl.com");
            REQUIRE(installer1.UnsupportedArguments.size() == 1);
            REQUIRE(installer1.UnsupportedArguments.at(0) == UnsupportedArgumentEnum::Location);
        }

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_4 })
        {
            // NestedInstaller metadata should not be populated unless the InstallerType is zip.
            REQUIRE(installer1.NestedInstallerType == InstallerTypeEnum::Unknown);
            REQUIRE(installer1.NestedInstallerFiles.size() == 0);

            REQUIRE(installer1.InstallationMetadata.DefaultInstallLocation == "%ProgramFiles%\\TestApp");
            REQUIRE(installer1.InstallationMetadata.Files.size() == 1);
            REQUIRE(installer1.InstallationMetadata.Files.at(0).RelativeFilePath == "main.exe");
            REQUIRE(installer1.InstallationMetadata.Files.at(0).FileType == InstalledFileTypeEnum::Launch);
            REQUIRE(installer1.InstallationMetadata.Files.at(0).FileSha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
            REQUIRE(installer1.InstallationMetadata.Files.at(0).InvocationParameter == "/arg");
            REQUIRE(installer1.InstallationMetadata.Files.at(0).DisplayName == "DisplayName");
        }

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_6 })
        {
            REQUIRE_FALSE(installer1.DownloadCommandProhibited);
        }

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_7 })
        {
            REQUIRE(installer1.Switches.at(InstallerSwitchType::Repair) == "/r");
            REQUIRE(installer1.RepairBehavior == RepairBehaviorEnum::Modify);
        }

        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_9 })
        {
            REQUIRE_FALSE(installer1.ArchiveBinariesDependOnPath);
        }

        if (!isSingleton)
        {
            if (!isExported)
            {
                ManifestInstaller installer2 = manifest.Installers.at(1);
                REQUIRE(installer2.BaseInstallerType == InstallerTypeEnum::Exe);
                REQUIRE(installer2.Arch == Architecture::X64);
                REQUIRE(installer2.Url == "https://www.microsoft.com/msixsdk/msixsdkx64.exe");
                REQUIRE(installer2.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
                REQUIRE(installer2.ProductCode == "{Bar}");

                if (manifestVer >= ManifestVer{ s_ManifestVersionV1_1 })
                {
                    REQUIRE(installer2.ReleaseDate == "2021-01-01");
                    REQUIRE(installer2.InstallerAbortsTerminal);
                    REQUIRE(installer2.InstallLocationRequired);
                    REQUIRE(installer2.RequireExplicitUpgrade);
                    REQUIRE(installer2.ElevationRequirement == ElevationRequirementEnum::ElevatesSelf);
                    REQUIRE(installer2.UnsupportedOSArchitectures.size() == 1);
                    REQUIRE(installer2.UnsupportedOSArchitectures.at(0) == Architecture::Arm);
                    REQUIRE(installer2.AppsAndFeaturesEntries.size() == 1);
                    REQUIRE(installer2.AppsAndFeaturesEntries.at(0).DisplayName == "DisplayName");
                    REQUIRE(installer2.AppsAndFeaturesEntries.at(0).DisplayVersion == "DisplayVersion");
                    REQUIRE(installer2.AppsAndFeaturesEntries.at(0).Publisher == "Publisher");
                    REQUIRE(installer2.AppsAndFeaturesEntries.at(0).ProductCode == "ProductCode");
                    REQUIRE(installer2.AppsAndFeaturesEntries.at(0).UpgradeCode == "UpgradeCode");
                    REQUIRE(installer2.AppsAndFeaturesEntries.at(0).InstallerType == InstallerTypeEnum::Exe);
                    REQUIRE(installer2.Markets.AllowedMarkets.size() == 1);
                    REQUIRE(installer2.Markets.AllowedMarkets.at(0) == "US");
                    REQUIRE(installer2.ExpectedReturnCodes.size() == 1);
                    REQUIRE(installer2.ExpectedReturnCodes.at(10).ReturnResponseEnum == ExpectedReturnCodeEnum::PackageInUse);
                }

                if (manifestVer >= ManifestVer{ s_ManifestVersionV1_2 })
                {
                    ManifestInstaller installer3 = manifest.Installers.at(2);
                    REQUIRE(installer3.BaseInstallerType == InstallerTypeEnum::Portable);
                    REQUIRE(installer3.Arch == Architecture::X86);
                    REQUIRE(installer3.Url == "https://www.microsoft.com/msixsdk/msixsdkx86.exe");
                    REQUIRE(installer3.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
                    REQUIRE(installer3.Commands == MultiValue{ "standalone" });
                    REQUIRE(installer3.ExpectedReturnCodes.size() == 1);
                    REQUIRE(installer3.ExpectedReturnCodes.at(11).ReturnResponseEnum == ExpectedReturnCodeEnum::Custom);
                    REQUIRE(installer3.ExpectedReturnCodes.at(11).ReturnResponseUrl == "https://defaultReturnResponseUrl.com");
                    REQUIRE_FALSE(installer3.DisplayInstallWarnings);
                    REQUIRE(installer3.UnsupportedArguments.size() == 1);
                    REQUIRE(installer3.UnsupportedArguments.at(0) == UnsupportedArgumentEnum::Log);
                }

                if (manifestVer >= ManifestVer{ s_ManifestVersionV1_4 })
                {
                    ManifestInstaller installer4 = manifest.Installers.at(3);
                    REQUIRE(installer4.BaseInstallerType == InstallerTypeEnum::Zip);
                    REQUIRE(installer4.Arch == Architecture::X64);
                    REQUIRE(installer4.Url == "https://www.microsoft.com/msixsdk/msixsdkx64.exe");
                    REQUIRE(installer4.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
                    REQUIRE(installer4.ProductCode == "{Foo}");
                    REQUIRE(installer4.NestedInstallerType == InstallerTypeEnum::Portable);
                    REQUIRE(installer4.NestedInstallerFiles.size() == 2);
                    REQUIRE(installer4.NestedInstallerFiles.at(0).RelativeFilePath == "relativeFilePath1");
                    REQUIRE(installer4.NestedInstallerFiles.at(0).PortableCommandAlias == "portableAlias1");
                    REQUIRE(installer4.NestedInstallerFiles.at(1).RelativeFilePath == "relativeFilePath2");
                    REQUIRE(installer4.NestedInstallerFiles.at(1).PortableCommandAlias == "portableAlias2");
                    REQUIRE(installer4.InstallationMetadata.DefaultInstallLocation == "%ProgramFiles%\\TestApp2");
                    REQUIRE(installer4.InstallationMetadata.Files.size() == 1);
                    REQUIRE(installer4.InstallationMetadata.Files.at(0).RelativeFilePath == "main2.exe");
                    REQUIRE(installer4.InstallationMetadata.Files.at(0).FileType == InstalledFileTypeEnum::Other);
                    REQUIRE(installer4.InstallationMetadata.Files.at(0).FileSha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
                    REQUIRE(installer4.InstallationMetadata.Files.at(0).InvocationParameter == "/arg2");
                    REQUIRE(installer4.InstallationMetadata.Files.at(0).DisplayName == "DisplayName2");
                }

                if (manifestVer >= ManifestVer{ s_ManifestVersionV1_6 })
                {
                    REQUIRE(installer2.DownloadCommandProhibited);
                    REQUIRE(installer2.UpdateBehavior == UpdateBehaviorEnum::Deny);
                }

                if (manifestVer >= ManifestVer{ s_ManifestVersionV1_7 })
                {
                    REQUIRE(installer2.RepairBehavior == RepairBehaviorEnum::Uninstaller);
                    REQUIRE(installer2.Switches.at(InstallerSwitchType::Repair) == "/r");

                    ManifestInstaller installer5 = manifest.Installers.at(4);
                    REQUIRE(installer5.BaseInstallerType == InstallerTypeEnum::Burn);
                    REQUIRE(installer5.Arch == Architecture::X64);
                    REQUIRE(installer5.Url == "https://www.microsoft.com/msixsdk/msixsdkx64.exe");
                    REQUIRE(installer5.Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82"));
                    REQUIRE(installer5.ProductCode == "{Bar}");
                    REQUIRE(installer5.Switches.at(InstallerSwitchType::Repair) == "/repair");
                    REQUIRE(installer5.RepairBehavior == RepairBehaviorEnum::Modify);
                }

                if (manifestVer >= ManifestVer{ s_ManifestVersionV1_9 })
                {
                    ManifestInstaller installer4 = manifest.Installers.at(3);
                    REQUIRE(installer4.ArchiveBinariesDependOnPath);
                }
            }

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

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_1 })
            {
                REQUIRE(localization1.Get<Localization::ReleaseNotes>() == "Release notes");
                REQUIRE(localization1.Get<Localization::ReleaseNotesUrl>() == "https://ReleaseNotes.net");
                REQUIRE(localization1.Get<Localization::Agreements>().size() == 1);
                REQUIRE(localization1.Get<Localization::Agreements>().at(0).Label == "Label");
                REQUIRE(localization1.Get<Localization::Agreements>().at(0).AgreementText == "Text");
                REQUIRE(localization1.Get<Localization::Agreements>().at(0).AgreementUrl == "https://AgreementUrl.net");
            }

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_2 })
            {
                REQUIRE(localization1.Get<Localization::PurchaseUrl>() == "https://DefaultPurchaseUrl.com");
                REQUIRE(localization1.Get<Localization::InstallationNotes>() == "Default installation notes");
                REQUIRE(localization1.Get<Localization::Documentations>().size() == 1);
                REQUIRE(localization1.Get<Localization::Documentations>().at(0).DocumentLabel == "Default document label");
                REQUIRE(localization1.Get<Localization::Documentations>().at(0).DocumentUrl == "https://DefaultDocumentUrl.com");
            }

            if (manifestVer >= ManifestVer{ s_ManifestVersionV1_5 })
            {
                REQUIRE(localization1.Get<Localization::Icons>().size() == 1);
                REQUIRE(localization1.Get<Localization::Icons>().at(0).Url == "https://localeTestIcon-en-GB");
                REQUIRE(localization1.Get<Localization::Icons>().at(0).FileType == IconFileTypeEnum::Png);
                REQUIRE(localization1.Get<Localization::Icons>().at(0).Resolution == IconResolutionEnum::Square32);
                REQUIRE(localization1.Get<Localization::Icons>().at(0).Theme == IconThemeEnum::Light);
                REQUIRE(localization1.Get<Localization::Icons>().at(0).Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8321"));
            }
        }
    }

    struct ManifestShadowTestInfo
    {
        bool shadowDefaultLocale;
        bool shadowEnGbLocale;
    };

    void VerifyV1ManifestContentCreatedWithShadow(const Manifest& manifest, ManifestShadowTestInfo shadowInfo, ManifestVer manifestVer = { s_ManifestVersionV1_5 })
    {
        REQUIRE(manifest.Id == "microsoft.msixsdk");
        REQUIRE(manifest.Version == "1.7.32");
        REQUIRE(manifest.Installers.size() == 1);

        // Default localization
        REQUIRE(manifest.DefaultLocalization.Locale == "en-US");
        REQUIRE(manifest.DefaultLocalization.Get<Localization::Publisher>() == "Microsoft");
        REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == "MSIX SDK");
        REQUIRE(manifest.DefaultLocalization.Get<Localization::License>() == "MIT License");
        REQUIRE(manifest.DefaultLocalization.Get<Localization::Description>() == "The MSIX SDK project is an effort to enable developers");
        REQUIRE(manifest.DefaultLocalization.Get<Localization::ShortDescription>() == "This is MSIX SDK");
        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_5 })
        {
            REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().size() == 1);

            if (shadowInfo.shadowDefaultLocale)
            {
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Url == "https://shadowIcon-default");
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).FileType == IconFileTypeEnum::Ico);
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Resolution == IconResolutionEnum::Custom);
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Theme == IconThemeEnum::Default);
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Sha256 == SHA256::ConvertToBytes("1111111111111111111111111111111111111111111111111111111111111111"));
            }
            else
            {
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().size() == 1);
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Url == "https://testIcon-en-US");
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).FileType == IconFileTypeEnum::Ico);
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Resolution == IconResolutionEnum::Custom);
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Theme == IconThemeEnum::Default);
                REQUIRE(manifest.DefaultLocalization.Get<Localization::Icons>().at(0).Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8123"));
            }
        }

        // Localization
        if (manifestVer >= ManifestVer{ s_ManifestVersionV1_5 })
        {
            REQUIRE(manifest.Localizations.size() == 3);

            bool foundEnGbLocale = false;
            bool foundfrFrLocale = false;
            for (auto const& localization : manifest.Localizations)
            {
                if (localization.Locale == "en-GB")
                {
                    REQUIRE(localization.Get<Localization::Description>() == "The MSIX SDK project is an effort to enable developers UK");
                    if (shadowInfo.shadowEnGbLocale)
                    {
                        REQUIRE(localization.Get<Localization::Icons>().size() == 1);
                        REQUIRE(localization.Get<Localization::Icons>().at(0).Url == "https://shadowIcon-en-GB");
                        REQUIRE(localization.Get<Localization::Icons>().at(0).FileType == IconFileTypeEnum::Png);
                        REQUIRE(localization.Get<Localization::Icons>().at(0).Resolution == IconResolutionEnum::Square32);
                        REQUIRE(localization.Get<Localization::Icons>().at(0).Theme == IconThemeEnum::Light);
                        REQUIRE(localization.Get<Localization::Icons>().at(0).Sha256 == SHA256::ConvertToBytes("2222222222222222222222222222222222222222222222222222222222222222"));
                    }
                    else
                    {
                        REQUIRE(localization.Get<Localization::Icons>().size() == 1);
                        REQUIRE(localization.Get<Localization::Icons>().at(0).Url == "https://localeTestIcon-en-GB");
                        REQUIRE(localization.Get<Localization::Icons>().at(0).FileType == IconFileTypeEnum::Png);
                        REQUIRE(localization.Get<Localization::Icons>().at(0).Resolution == IconResolutionEnum::Square32);
                        REQUIRE(localization.Get<Localization::Icons>().at(0).Theme == IconThemeEnum::Light);
                        REQUIRE(localization.Get<Localization::Icons>().at(0).Sha256 == SHA256::ConvertToBytes("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8321"));
                    }

                    foundEnGbLocale = true;
                }
                else if (localization.Locale == "fr-FR")
                {
                    REQUIRE(localization.Get<Localization::Icons>().size() == 1);
                    REQUIRE(localization.Get<Localization::Icons>().at(0).Url == "https://shadowIcon-fr-FR");
                    REQUIRE(localization.Get<Localization::Icons>().at(0).FileType == IconFileTypeEnum::Jpeg);
                    REQUIRE(localization.Get<Localization::Icons>().at(0).Resolution == IconResolutionEnum::Square20);
                    REQUIRE(localization.Get<Localization::Icons>().at(0).Theme == IconThemeEnum::Dark);
                    REQUIRE(localization.Get<Localization::Icons>().at(0).Sha256 == SHA256::ConvertToBytes("3333333333333333333333333333333333333333333333333333333333333333"));
                    foundfrFrLocale = true;
                }
                else
                {
                    REQUIRE(localization.Locale == "es-MX");
                    REQUIRE(localization.Get<Localization::Description>() == "The MSIX SDK project is an effort to enable developers MX");
                    REQUIRE(localization.Get<Localization::Icons>().size() == 1);
                    REQUIRE(localization.Get<Localization::Icons>().at(0).Url == "https://localeTestIcon-es-MX");
                    REQUIRE(localization.Get<Localization::Icons>().at(0).FileType == IconFileTypeEnum::Png);
                    REQUIRE(localization.Get<Localization::Icons>().at(0).Resolution == IconResolutionEnum::Square32);
                    REQUIRE(localization.Get<Localization::Icons>().at(0).Theme == IconThemeEnum::Light);
                    REQUIRE(localization.Get<Localization::Icons>().at(0).Sha256 == SHA256::ConvertToBytes("4444444444444444444444444444444444444444444444444444444444444444"));
                }
            }

            REQUIRE(foundEnGbLocale);
            REQUIRE(foundfrFrLocale);
        }
    }
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
    REQUIRE(manifest.DefaultInstallerInfo.BaseInstallerType == InstallerTypeEnum::Exe);
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
    REQUIRE(installer1.BaseInstallerType == InstallerTypeEnum::Exe);
    REQUIRE(installer1.Scope == ScopeEnum::User);
    REQUIRE(installer1.PackageFamilyName == "");
    REQUIRE(installer1.ProductCode == "{Foo}");
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
    REQUIRE(installer2.BaseInstallerType == InstallerTypeEnum::Exe);
    REQUIRE(installer2.Scope == ScopeEnum::User);
    REQUIRE(installer2.PackageFamilyName == "");
    REQUIRE(installer2.ProductCode == "{Foo}");
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
    std::ifstream stream(manifestFile.GetPath(), std::ios_base::in | std::ios_base::binary);
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

TEST_CASE("ReadGoodManifests", "[ManifestValidation]")
{
    ManifestTestCase TestCases[] =
    {
        { "Manifest-Good-InstallerTypeExeRoot-Silent.yaml" },
        { "Manifest-Good-InstallerTypeExeRoot-SilentRoot.yaml" },
        { "Manifest-Good-InstallerTypeExe-Silent.yaml" },
        { "Manifest-Good-InstallerTypeExe-SilentRoot.yaml" },
        { "Manifest-Good-InstallerUniqueness-DefaultLang.yaml" },
        { "Manifest-Good-InstallerUniqueness-DiffLangs.yaml" },
        { "Manifest-Good-InstallerUniqueness-DiffScope.yaml" },
        { "Manifest-Good-Minimum.yaml" },
        { "Manifest-Good-Minimum-InstallerType.yaml" },
        { "Manifest-Good-Switches.yaml" },
        { "Manifest-Good-DefaultExpectedReturnCodeInInstallerSuccessCodes.yaml" },
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
        { "Manifest-Bad-ArchInvalid.yaml", "Invalid field value. [Architecture]" },
        { "Manifest-Bad-ArchMissing.yaml", "Missing required property 'Arch'" },
        { "Manifest-Bad-Channel-NotSupported.yaml", "Field is not supported. [Channel]" },
        { "Manifest-Bad-DifferentCase-camelCase.yaml", "All field names should be PascalCased. [installerType]" },
        { "Manifest-Bad-DifferentCase-lower.yaml", "All field names should be PascalCased. [installertype]" },
        { "Manifest-Bad-DifferentCase-UPPER.yaml", "All field names should be PascalCased. [INSTALLERTYPE]" },
        { "Manifest-Bad-DuplicateKey.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-DuplicateKey-DifferentCase.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-DuplicateKey-DifferentCase-lower.yaml", "Duplicate field found in the manifest." },
        { "Manifest-Bad-DuplicateReturnCode-ExpectedCodes.yaml", "Duplicate installer return code found." },
        { "Manifest-Bad-DuplicateReturnCode-SuccessCodes.yaml", "Duplicate installer return code found." },
        { "Manifest-Bad-IdInvalid.yaml", "Failed to validate against schema associated with property name 'Id'" },
        { "Manifest-Bad-IdMissing.yaml", "Missing required property 'Id'" },
        { "Manifest-Bad-InstallersMissing.yaml", "Missing required property 'Installers'" },
        { "Manifest-Bad-InstallerTypeExe-NoSilent.yaml", "Silent and SilentWithProgress switches are not specified for InstallerType exe.", true },
        { "Manifest-Bad-InstallerTypeExe-NoSilentRoot.yaml", "Silent and SilentWithProgress switches are not specified for InstallerType exe.", true },
        { "Manifest-Bad-InstallerTypeExeRoot-NoSilent.yaml", "Silent and SilentWithProgress switches are not specified for InstallerType exe.", true },
        { "Manifest-Bad-InstallerTypeExeRoot-NoSilentRoot.yaml", "Silent and SilentWithProgress switches are not specified for InstallerType exe.", true },
        { "Manifest-Bad-InstallerTypeInvalid.yaml", "Invalid field value. [InstallerType]" },
        { "Manifest-Bad-InstallerTypeMissing.yaml", "Invalid field value. [InstallerType]" },
        { "Manifest-Bad-InstallerTypePortable-InvalidAppsAndFeatures.yaml", "Only zero or one entry for Apps and Features may be specified for InstallerType portable." },
        { "Manifest-Bad-InstallerTypePortable-InvalidCommands.yaml", "Only zero or one value for Commands may be specified for InstallerType portable." },
        { "Manifest-Bad-InstallerTypePortable-InvalidScope.yaml", "Scope is not supported for InstallerType portable." },
        { "Manifest-Bad-InstallerTypeZip-DuplicateCommandAlias.yaml", "Duplicate portable command alias found." },
        { "Manifest-Bad-InstallerTypeZip-DuplicateRelativeFilePath.yaml", "Duplicate relative file path found." },
        { "Manifest-Bad-InstallerTypeZip-InvalidRelativeFilePath.yaml", "Relative file path must not point to a location outside of archive directory" },
        { "Manifest-Bad-InstallerTypeZip-MissingRelativeFilePath.yaml", "Required field missing. [RelativeFilePath]" },
        { "Manifest-Bad-InstallerTypeZip-MultipleNestedInstallers.yaml", "Only one entry for NestedInstallerFiles can be specified for non-portable InstallerTypes." },
        { "Manifest-Bad-InstallerTypeZip-NoNestedInstallerFile.yaml", "Required field missing. [NestedInstallerFiles]" },
        { "Manifest-Bad-InstallerTypeZip-NoNestedInstallerType.yaml", "Required field missing. [NestedInstallerType]" },
        { "Manifest-Bad-InstallerUniqueness.yaml", "Duplicate installer entry found." },
        { "Manifest-Bad-InstallerUniqueness-DefaultScope.yaml", "Duplicate installer entry found." },
        { "Manifest-Bad-InstallerUniqueness-DefaultValues.yaml", "Duplicate installer entry found." },
        { "Manifest-Bad-InstallerUniqueness-SameLang.yaml", "Duplicate installer entry found." },
        { "Manifest-Bad-LicenseMissing.yaml", "Missing required property 'License'" },
        { "Manifest-Bad-NameMissing.yaml", "Missing required property 'Name'" },
        { "Manifest-Bad-PublisherMissing.yaml", "Missing required property 'Publisher'" },
        { "Manifest-Bad-Sha256Invalid.yaml", "Failed to validate against schema associated with property name 'Sha256'" },
        { "Manifest-Bad-Sha256Missing.yaml", "Required field missing. [InstallerSha256]" },
        { "Manifest-Bad-SwitchInvalid.yaml", "Unknown field. [NotASwitch]", true },
        { "Manifest-Bad-UnknownProperty.yaml", "Unknown field. [Fake]", true },
        { "Manifest-Bad-UnsupportedVersion.yaml", "Unsupported ManifestVersion" },
        { "Manifest-Bad-UrlInvalid.yaml", "Invalid field value. [InstallerUrl]" },
        { "Manifest-Bad-UrlMissing.yaml", "Required field missing. [InstallerUrl]" },
        { "Manifest-Bad-VersionInvalid.yaml", "Failed to validate against schema associated with property name 'Version'" },
        { "Manifest-Bad-VersionMissing.yaml", "Missing required property 'Version'" },
        { "Manifest-Bad-InvalidManifestVersionValue.yaml", "Failed to validate against schema associated with property name 'ManifestVersion'" },
        { "InstallFlowTest_MSStore.yaml", "Field value is not supported. [InstallerType] Value: msstore" },
        { "Manifest-Bad-PackageFamilyNameOnMSI.yaml", "The specified installer type does not support PackageFamilyName. [InstallerType] Value: msi", true },
        { "Manifest-Bad-ProductCodeOnMSIX.yaml", "The specified installer type does not support ProductCode. [InstallerType] Value: msix" },
        { "Manifest-Bad-InvalidUpdateBehavior.yaml", "Invalid field value. [UpgradeBehavior]" },
        { "Manifest-Bad-InvalidLocale.yaml", "The locale value is not a well formed bcp47 language tag." },
        { "Manifest-Bad-AppsAndFeaturesEntriesOnMSIX.yaml", "The specified installer type does not write to Apps and Features entry." },
        { "InstallFlowTest_LicenseAgreement.yaml", "Field usage requires verified publishers. [Agreement]", true },
        { "InstallFlowTest_LicenseAgreement.yaml", "Field usage requires verified publishers. [Agreement]", false, GetTestManifestValidateOption(false, true) },
        { "Manifest-Bad-ApproximateVersionInPackageVersion.yaml", "Approximate version not allowed. [PackageVersion]" },
        { "Manifest-Bad-ApproximateVersionInArpVersion.yaml", "Approximate version not allowed. [DisplayVersion]" },
    };

    for (auto const& testCase : TestCases)
    {
        TestManifest(testCase.TestFile, testCase.ExpectedMessage, testCase.IsWarningOnly, testCase.ValidateOption);
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
        Manifest manifest = YamlParser::CreateFromPath(TestDataFile(testCase.TestFile), GetTestManifestValidateOption());
        REQUIRE(manifest.DefaultLocalization.Get<Localization::PackageName>() == u8"MSIX SDK\xA9");
    }
}

TEST_CASE("ComplexSystemReference", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-SystemReferenceComplex.yaml"));

    REQUIRE(manifest.Installers.size() == 4);

    // MSIX installer does inherit
    auto& installer = manifest.Installers[0];
    REQUIRE(installer.BaseInstallerType == InstallerTypeEnum::Msix);
    REQUIRE(installer.Arch == Architecture::X86);
    REQUIRE(installer.PackageFamilyName == "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe");
    REQUIRE(installer.ProductCode == "");

    // MSI installer does inherit
    auto& installer1 = manifest.Installers[1];
    REQUIRE(installer1.BaseInstallerType == InstallerTypeEnum::Msi);
    REQUIRE(installer1.Arch == Architecture::X86);
    REQUIRE(installer1.PackageFamilyName == "");
    REQUIRE(installer1.ProductCode == "{Foo}");

    // MSIX installer with override
    auto& installer2 = manifest.Installers[2];
    REQUIRE(installer2.BaseInstallerType == InstallerTypeEnum::Msix);
    REQUIRE(installer2.Arch == Architecture::X64);
    REQUIRE(installer2.PackageFamilyName == "Override_8wekyb3d8bbwe");
    REQUIRE(installer2.ProductCode == "");

    // MSI installer with override
    auto& installer3 = manifest.Installers[3];
    REQUIRE(installer3.BaseInstallerType == InstallerTypeEnum::Msi);
    REQUIRE(installer3.Arch == Architecture::X64);
    REQUIRE(installer3.PackageFamilyName == "");
    REQUIRE(installer3.ProductCode == "Override");
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

TEST_CASE("ValidateV1GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1-MultiFile-Version.yaml",
        "ManifestV1-MultiFile-Installer.yaml",
        "ManifestV1-MultiFile-DefaultLocale.yaml",
        "ManifestV1-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false);

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false);
}

TEST_CASE("ValidateV1_1GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_1-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true, ManifestVer{ s_ManifestVersionV1_1 });

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_1-MultiFile-Version.yaml",
        "ManifestV1_1-MultiFile-Installer.yaml",
        "ManifestV1_1-MultiFile-DefaultLocale.yaml",
        "ManifestV1_1-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false, ManifestVer{ s_ManifestVersionV1_1 });

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false, ManifestVer{ s_ManifestVersionV1_1 });
}

TEST_CASE("ValidateV1_2GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_2-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true, ManifestVer{ s_ManifestVersionV1_2 });

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_2-MultiFile-Version.yaml",
        "ManifestV1_2-MultiFile-Installer.yaml",
        "ManifestV1_2-MultiFile-DefaultLocale.yaml",
        "ManifestV1_2-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false, ManifestVer{ s_ManifestVersionV1_2 });

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false, ManifestVer{ s_ManifestVersionV1_2 });
}

TEST_CASE("ValidateV1_4GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_4-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true, ManifestVer{ s_ManifestVersionV1_4 });

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_4-MultiFile-Version.yaml",
        "ManifestV1_4-MultiFile-Installer.yaml",
        "ManifestV1_4-MultiFile-DefaultLocale.yaml",
        "ManifestV1_4-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false, ManifestVer{ s_ManifestVersionV1_4 });

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false, ManifestVer{ s_ManifestVersionV1_4 });
}

TEST_CASE("ValidateV1_5GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_5-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true, ManifestVer{ s_ManifestVersionV1_5 });

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_5-MultiFile-Version.yaml",
        "ManifestV1_5-MultiFile-Installer.yaml",
        "ManifestV1_5-MultiFile-DefaultLocale.yaml",
        "ManifestV1_5-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false, ManifestVer{ s_ManifestVersionV1_5 });

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false, ManifestVer{ s_ManifestVersionV1_5 });
}

TEST_CASE("ValidateV1_6GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_6-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true, ManifestVer{ s_ManifestVersionV1_6 });

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_6-MultiFile-Version.yaml",
        "ManifestV1_6-MultiFile-Installer.yaml",
        "ManifestV1_6-MultiFile-DefaultLocale.yaml",
        "ManifestV1_6-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false, ManifestVer{ s_ManifestVersionV1_6 });

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false, ManifestVer{ s_ManifestVersionV1_6 });
}

TEST_CASE("ValidateV1_7GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_7-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true, ManifestVer{ s_ManifestVersionV1_7 });

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_7-MultiFile-Version.yaml",
        "ManifestV1_7-MultiFile-Installer.yaml",
        "ManifestV1_7-MultiFile-DefaultLocale.yaml",
        "ManifestV1_7-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false, ManifestVer{ s_ManifestVersionV1_7 });

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false, ManifestVer{ s_ManifestVersionV1_7 });
}

TEST_CASE("ValidateV1_9GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_9-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true, ManifestVer{ s_ManifestVersionV1_9 });

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_9-MultiFile-Version.yaml",
        "ManifestV1_9-MultiFile-Installer.yaml",
        "ManifestV1_9-MultiFile-DefaultLocale.yaml",
        "ManifestV1_9-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false, ManifestVer{ s_ManifestVersionV1_9 });

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false, ManifestVer{ s_ManifestVersionV1_9 });
}

TEST_CASE("ValidateV1_10GoodManifestAndVerifyContents", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_10-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory, validateOption);
    VerifyV1ManifestContent(singletonManifest, true, ManifestVer{ s_ManifestVersionV1_10 });

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_10-MultiFile-Version.yaml",
        "ManifestV1_10-MultiFile-Installer.yaml",
        "ManifestV1_10-MultiFile-DefaultLocale.yaml",
        "ManifestV1_10-MultiFile-Locale.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContent(multiFileManifest, false, ManifestVer{ s_ManifestVersionV1_10 });

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContent(mergedManifest, false, ManifestVer{ s_ManifestVersionV1_10 });
}

TEST_CASE("WriteV1SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_1-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1-MultiFile-Version.yaml",
        "ManifestV1-MultiFile-Installer.yaml",
        "ManifestV1-MultiFile-DefaultLocale.yaml",
        "ManifestV1-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1 }, true);
}

TEST_CASE("WriteV1_1SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_1-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1_1 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_1-MultiFile-Version.yaml",
        "ManifestV1_1-MultiFile-Installer.yaml",
        "ManifestV1_1-MultiFile-DefaultLocale.yaml",
        "ManifestV1_1-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1_1 }, true);
}

TEST_CASE("WriteV1_2SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_2-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1_2 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_2-MultiFile-Version.yaml",
        "ManifestV1_2-MultiFile-Installer.yaml",
        "ManifestV1_2-MultiFile-DefaultLocale.yaml",
        "ManifestV1_2-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1_2 }, true);
}

TEST_CASE("WriteV1_4SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_4-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1_4 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_4-MultiFile-Version.yaml",
        "ManifestV1_4-MultiFile-Installer.yaml",
        "ManifestV1_4-MultiFile-DefaultLocale.yaml",
        "ManifestV1_4-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1_4 }, true);
}

TEST_CASE("WriteV1_5SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_5-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1_5 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_5-MultiFile-Version.yaml",
        "ManifestV1_5-MultiFile-Installer.yaml",
        "ManifestV1_5-MultiFile-DefaultLocale.yaml",
        "ManifestV1_5-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1_5 }, true);
}

TEST_CASE("WriteV1_6SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_6-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1_6 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_6-MultiFile-Version.yaml",
        "ManifestV1_6-MultiFile-Installer.yaml",
        "ManifestV1_6-MultiFile-DefaultLocale.yaml",
        "ManifestV1_6-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1_6 }, true);
}

TEST_CASE("WriteV1_7SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_7-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1_7 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_7-MultiFile-Version.yaml",
        "ManifestV1_7-MultiFile-Installer.yaml",
        "ManifestV1_7-MultiFile-DefaultLocale.yaml",
        "ManifestV1_7-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1_7 }, true);
}

TEST_CASE("WriteV1_9SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_9-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1_9 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_9-MultiFile-Version.yaml",
        "ManifestV1_9-MultiFile-Installer.yaml",
        "ManifestV1_9-MultiFile-DefaultLocale.yaml",
        "ManifestV1_9-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1_9 }, true);
}

TEST_CASE("WriteV1_10SingletonManifestAndVerifyContents", "[ManifestCreation]")
{
    TempDirectory singletonDirectory{ "SingletonManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_10-Singleton.yaml" }, singletonDirectory);
    Manifest singletonManifest = YamlParser::CreateFromPath(singletonDirectory);

    TempDirectory exportedSingletonDirectory{ "exportedSingleton" };
    std::filesystem::path generatedSingletonManifestPath = exportedSingletonDirectory.GetPath() / "testSingletonManifest.yaml";
    YamlWriter::OutputYamlFile(singletonManifest, singletonManifest.Installers[0], generatedSingletonManifestPath);

    REQUIRE(std::filesystem::exists(generatedSingletonManifestPath));
    Manifest generatedSingletonManifest = YamlParser::CreateFromPath(exportedSingletonDirectory);
    VerifyV1ManifestContent(generatedSingletonManifest, true, ManifestVer{ s_ManifestVersionV1_10 }, true);

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_10-MultiFile-Version.yaml",
        "ManifestV1_10-MultiFile-Installer.yaml",
        "ManifestV1_10-MultiFile-DefaultLocale.yaml",
        "ManifestV1_10-MultiFile-Locale.yaml" }, multiFileDirectory);

    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory);
    TempDirectory exportedMultiFileDirectory{ "exportedMultiFile" };
    std::filesystem::path generatedMultiFileManifestPath = exportedMultiFileDirectory.GetPath() / "testMultiFileManifest.yaml";
    YamlWriter::OutputYamlFile(multiFileManifest, multiFileManifest.Installers[0], generatedMultiFileManifestPath);

    REQUIRE(std::filesystem::exists(generatedMultiFileManifestPath));
    Manifest generatedMultiFileManifest = YamlParser::CreateFromPath(exportedMultiFileDirectory);
    VerifyV1ManifestContent(generatedMultiFileManifest, false, ManifestVer{ s_ManifestVersionV1_10 }, true);
}

// Since Authentication is not supported in community repo and will cause manifest validation failure,
// we are not adding Authentication in v1_10 manifests. Instead a separate test is created for Authentication.
TEST_CASE("ReadWriteValidateV1_10ManifestWithInstallerAuthentication", "[ManifestValidation]")
{
    // Read manifest
    TempDirectory testDirectory{ "TestManifest" };
    CopyTestDataFilesToFolder({ "ManifestV1_10-InstallerAuthentication.yaml" }, testDirectory);
    Manifest testManifest = YamlParser::CreateFromPath(testDirectory);

    // Verify content
    REQUIRE(testManifest.ManifestVersion == AppInstaller::Manifest::ManifestVer{ s_ManifestVersionV1_10 });
    REQUIRE(testManifest.DefaultInstallerInfo.AuthInfo.Type == AppInstaller::Authentication::AuthenticationType::MicrosoftEntraId);
    REQUIRE(testManifest.DefaultInstallerInfo.AuthInfo.MicrosoftEntraIdInfo);
    REQUIRE(testManifest.DefaultInstallerInfo.AuthInfo.MicrosoftEntraIdInfo->Resource == "TestResource");
    REQUIRE(testManifest.DefaultInstallerInfo.AuthInfo.MicrosoftEntraIdInfo->Scope == "TestScope");
    REQUIRE(testManifest.Installers.size() == 1);
    REQUIRE(testManifest.Installers[0].AuthInfo.Type == AppInstaller::Authentication::AuthenticationType::MicrosoftEntraIdForAzureBlobStorage);
    REQUIRE(testManifest.Installers[0].AuthInfo.MicrosoftEntraIdInfo);
    REQUIRE(testManifest.Installers[0].AuthInfo.MicrosoftEntraIdInfo->Resource == "https://storage.azure.com/");
    REQUIRE(testManifest.Installers[0].AuthInfo.MicrosoftEntraIdInfo->Scope.empty());

    // Manifest Validation. Only error is "Authentication not supported".
    auto errors = ValidateManifest(testManifest, true);
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].GetErrorMessage() == "Field is not supported.");
    REQUIRE(errors[0].Context == "Authentication");

    // Write manifest
    TempDirectory exportedDirectory{ "ExportedManifest" };
    std::filesystem::path exportedManifestPath = exportedDirectory.GetPath() / "ExportedManifest.yaml";
    YamlWriter::OutputYamlFile(testManifest, testManifest.Installers[0], exportedManifestPath);

    // Read back and validate content
    REQUIRE(std::filesystem::exists(exportedManifestPath));
    Manifest exportedManifest = YamlParser::CreateFromPath(exportedDirectory);
    REQUIRE(testManifest.ManifestVersion == AppInstaller::Manifest::ManifestVer{ s_ManifestVersionV1_10 });
    REQUIRE(exportedManifest.Installers.size() == 1);
    REQUIRE(exportedManifest.Installers[0].AuthInfo.Type == AppInstaller::Authentication::AuthenticationType::MicrosoftEntraIdForAzureBlobStorage);
    REQUIRE(exportedManifest.Installers[0].AuthInfo.MicrosoftEntraIdInfo);
    REQUIRE(exportedManifest.Installers[0].AuthInfo.MicrosoftEntraIdInfo->Resource == "https://storage.azure.com/");
    REQUIRE(exportedManifest.Installers[0].AuthInfo.MicrosoftEntraIdInfo->Scope.empty());
}

TEST_CASE("WriteManifestWithMultipleLocale", "[ManifestCreation]")
{
    Manifest multiLocaleManifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-MultiLocale.yaml"));
    TempDirectory exportedDirectory{ "exported" };
    std::filesystem::path generatedManifestPath = exportedDirectory.GetPath() / "testManifestWithMultipleLocale.yaml";
    YamlWriter::OutputYamlFile(multiLocaleManifest, multiLocaleManifest.Installers[0], generatedManifestPath);

    REQUIRE(std::filesystem::exists(generatedManifestPath));
    Manifest generatedManifest = YamlParser::CreateFromPath(generatedManifestPath);
    REQUIRE(generatedManifest.Localizations.size() == 2);
}

TEST_CASE("WriteManifestWithMSStoreInstaller", "[ManifestCreation]")
{
    Manifest msstoreManifest = YamlParser::CreateFromPath(TestDataFile("DownloadFlowTest_MSStore.yaml"));
    TempDirectory exportedDirectory{ "exported" };
    std::filesystem::path generatedManifestPath = exportedDirectory.GetPath() / "testManifestWithMultipleLocale.yaml";
    msstoreManifest.ManifestVersion = ManifestVer{ "1.1.0" };
    YamlWriter::OutputYamlFile(msstoreManifest, msstoreManifest.Installers[0], generatedManifestPath);

    REQUIRE(std::filesystem::exists(generatedManifestPath));
    Manifest generatedManifest = YamlParser::CreateFromPath(generatedManifestPath);
    REQUIRE(generatedManifest.Installers[0].BaseInstallerType == InstallerTypeEnum::MSStore);
    REQUIRE(!generatedManifest.Installers[0].ProductId.empty());
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
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest should not contain file with the particular ManifestType. [ManifestType] Value: singleton"));
    }

    {
        // More than 1 version manifest
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1VersionManifest, v1InstallerManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest should contain only one file with the particular ManifestType. [ManifestType] Value: version"));
    }

    {
        // More than 1 installer manifest
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1InstallerManifest, v1InstallerManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest should contain only one file with the particular ManifestType. [ManifestType] Value: installer"));
    }

    {
        // More than 1 default locale manifest
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1InstallerManifest, v1DefaultLocaleManifest, v1DefaultLocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest should contain only one file with the particular ManifestType. [ManifestType] Value: defaultLocale"));
    }

    {
        // Duplicate locales
        std::vector<YamlManifestInfo> input = { v1VersionManifest, v1InstallerManifest, v1DefaultLocaleManifest, v1LocaleManifest, v1LocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest contains duplicate PackageLocale. [PackageLocale] Value: en-GB"));
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
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest has inconsistent field values. [PackageIdentifier] Value: Another.Identifier"));
    }

    {
        // Package Version does not match
        auto installerManifestCopy = v1InstallerManifest;
        installerManifestCopy.Root["PackageVersion"].SetScalar("Another.Version");
        std::vector<YamlManifestInfo> input = { v1VersionManifest, installerManifestCopy, v1DefaultLocaleManifest, v1LocaleManifest };
        REQUIRE_THROWS_MATCHES(YamlParser::ParseManifest(input), ManifestException, ManifestExceptionMatcher("The multi file manifest has inconsistent field values. [PackageVersion] Value: Another.Version"));
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

TEST_CASE("ManifestLocalizationValidation", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-MultiLocale.yaml"));

    // Set 1 locale to bad value
    manifest.Localizations.at(0).Locale = "Invalid";

    // Full validation should detect as error
    auto errors = ValidateManifest(manifest, true);
    REQUIRE(errors.size() == 1);
    REQUIRE(errors.at(0).ErrorLevel == ValidationError::Level::Error);

    // Not full validation should detect as warning
    errors = ValidateManifest(manifest, false);
    REQUIRE(errors.size() == 1);
    REQUIRE(errors.at(0).ErrorLevel == ValidationError::Level::Warning);
}

TEST_CASE("ReadManifestAndValidateMsixInstallers_Success", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Good-MsixInstaller.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(0 == errors.size());
}

TEST_CASE("ReadManifestAndValidateMsixInstallers_InconsistentFields", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Bad-InconsistentMsixInstallerFields.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(4 == errors.size());

    ValidateError(errors[0], ValidationError::Level::Error, ManifestError::MsixSignatureHashFailed);
    ValidateError(errors[1], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageFamilyName", "FakeInstallerForTesting_125rzkzqaqjwj");
    ValidateError(errors[2], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageVersion", "43690.48059.52428.56797");
    ValidateError(errors[3], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "MinimumOSVersion", "10.0.0.0");
}

TEST_CASE("ReadManifestAndValidateMsixInstallers_NoSupportedPlatforms", "[ManifestValidation]")
{
    auto testFileName = "Manifest-Bad-NoSupportedPlatforms.yaml";
    TestDataFile testFile(testFileName);
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(1 == errors.size());

    ValidateError(errors[0], ValidationError::Level::Error, ManifestError::NoSupportedPlatforms, "InstallerUrl", manifest.Installers.front().Url);
}

TEST_CASE("ReadManifestAndValidateMsixInstallers_PackageVersionNotUINT64", "[ManifestValidation]")
{
    Manifest manifest = YamlParser::CreateFromPath(TestDataFile("Manifest-Bad-MsixInstaller-PackageVersion.yaml"));

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(1 == errors.size());

    ValidateError(errors[0], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageVersion", "43690.48059.52428.56797");
}

TEST_CASE("ReadManifestAndValidateMsixInstallers_MissingFields", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Bad-MissingMsixInstallerFields.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    for (bool treatErrorAsWarning : { false, true })
    {
        auto errors = ValidateManifestInstallers(manifest, treatErrorAsWarning);
        auto expectedLevel = treatErrorAsWarning ? ValidationError::Level::Warning : ValidationError::Level::Error;
        REQUIRE(2 == errors.size());

        ValidateError(errors[0], expectedLevel, ManifestError::OptionalFieldMissing, "PackageFamilyName", "FakeInstallerForTesting_125rzkzqaqjwj");
        ValidateError(errors[1], expectedLevel, ManifestError::OptionalFieldMissing, "MinimumOSVersion", "10.0.0.0");
    }
}

TEST_CASE("ReadManifestAndValidateMsixInstallers_Signed_Success", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Good-SignedMsixInstaller.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(0 == errors.size());
}

TEST_CASE("ReadManifestAndValidateMsixInstallers_Signed_InconsistentFields", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Bad-InconsistentSignedMsixInstallerFields.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(4 == errors.size());

    ValidateError(errors[0], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "SignatureSha256", "50562001202c8dad456474d3f20903138d0a15c44ee497c3d4f82e85edbf2f97");
    ValidateError(errors[1], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageFamilyName", "FakeInstallerForTesting_125rzkzqaqjwj");
    ValidateError(errors[2], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageVersion", "43690.48059.52428.56797");
    ValidateError(errors[3], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "MinimumOSVersion", "10.0.0.0");
}

TEST_CASE("ReadManifestAndValidateMsixBundleInstallers_Success", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Good-MsixBundleInstaller.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(0 == errors.size());
}

TEST_CASE("ReadManifestAndValidateMsixBundleInstallers_WithStub_Success", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Good-MsixBundleInstaller-WithStub.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(0 == errors.size());
}

TEST_CASE("ReadManifestAndValidateMsixBundleInstallers_InconsistentFields", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Bad-InconsistentMsixBundleInstallerFields.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(7 == errors.size());

    ValidateError(errors[0], ValidationError::Level::Error, ManifestError::MsixSignatureHashFailed);

    // Validate errors for the first msix package in the msix bundle
    ValidateError(errors[1], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageFamilyName", "FakeInstallerForTesting_125rzkzqaqjwj");
    ValidateError(errors[2], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageVersion", "43690.48059.52428.56797");
    ValidateError(errors[3], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "MinimumOSVersion", "10.0.16299.0");

    // Validate errors for the second msix package in the msix bundle
    ValidateError(errors[4], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageFamilyName", "FakeInstallerForTesting_125rzkzqaqjwj");
    ValidateError(errors[5], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageVersion", "43690.48059.52428.56797");
    ValidateError(errors[6], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "MinimumOSVersion", "10.0.16299.0");
}

TEST_CASE("ReadManifestAndValidateMsixBundleInstallers_Signed_Success", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Good-SignedMsixBundleInstaller.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(0 == errors.size());
}

TEST_CASE("ReadManifestAndValidateMsixBundleInstallers_Signed_InconsistentFields", "[ManifestValidation]")
{
    TestDataFile testFile("Manifest-Bad-InconsistentSignedMsixBundleInstallerFields.yaml");
    Manifest manifest = YamlParser::CreateFromPath(testFile);

    // Update the installer path for testing
    REQUIRE(1 == manifest.Installers.size());
    TestDataFile msixFile(manifest.Installers[0].Url.c_str());
    manifest.Installers[0].Url = msixFile.GetPath().u8string();

    auto errors = ValidateManifestInstallers(manifest);
    REQUIRE(7 == errors.size());

    ValidateError(errors[0], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "SignatureSha256", "d70bd623f87b6ce4ddba4506c6000cf43ef3af4ab1207f5579ec43400de1623f");

    // Validate errors for the first msix package in the msix bundle
    ValidateError(errors[1], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageFamilyName", "FakeInstallerForTesting_125rzkzqaqjwj");
    ValidateError(errors[2], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageVersion", "43690.48059.52428.56797");
    ValidateError(errors[3], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "MinimumOSVersion", "10.0.16299.0");

    // Validate errors for the second msix package in the msix bundle
    ValidateError(errors[4], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageFamilyName", "FakeInstallerForTesting_125rzkzqaqjwj");
    ValidateError(errors[5], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "PackageVersion", "43690.48059.52428.56797");
    ValidateError(errors[6], ValidationError::Level::Error, ManifestError::InstallerMsixInconsistencies, "MinimumOSVersion", "10.0.16299.0");
}

TEST_CASE("ManifestArpVersionRange", "[ManifestValidation]")
{
    Manifest manifestNoArp = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-NoArpVersionDeclared.yaml"));
    REQUIRE(manifestNoArp.GetArpVersionRange().IsEmpty());

    Manifest manifestSingleArp = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-SingleArpVersionDeclared.yaml"));
    auto arpRangeSingleArp = manifestSingleArp.GetArpVersionRange();
    REQUIRE(arpRangeSingleArp.GetMinVersion().ToString() == "11.0");
    REQUIRE(arpRangeSingleArp.GetMaxVersion().ToString() == "11.0");

    Manifest manifestMultiArp = YamlParser::CreateFromPath(TestDataFile("Manifest-Good-MultipleArpVersionDeclared.yaml"));
    auto arpRangeMultiArp = manifestMultiArp.GetArpVersionRange();
    REQUIRE(arpRangeMultiArp.GetMinVersion().ToString() == "12.0");
    REQUIRE(arpRangeMultiArp.GetMaxVersion().ToString() == "13.0");
}

TEST_CASE("ManifestV1_10_SchemaHeaderValidations", "[ManifestValidation]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;

    // Schema header not found
    REQUIRE_THROWS_MATCHES(YamlParser::CreateFromPath(TestDataFile("ManifestV1_10-Bad-SchemaHeaderNotFound.yaml"),validateOption), ManifestException, ManifestExceptionMatcher("Schema header not found"));

    // Schema header not valid
    REQUIRE_THROWS_MATCHES(YamlParser::CreateFromPath(TestDataFile("ManifestV1_10-Bad-SchemaHeaderInvalid.yaml"), validateOption), ManifestException, ManifestExceptionMatcher("The schema header is invalid. Please verify that the schema header is present and formatted correctly."));

    // Schema header URL does not match the expected schema URL
    REQUIRE_THROWS_MATCHES(YamlParser::CreateFromPath(TestDataFile("ManifestV1_10-Bad-SchemaHeaderURLPatternMismatch.yaml"), validateOption), ManifestException, ManifestExceptionMatcher("The schema header URL does not match the expected pattern."));

    // Schema header ManifestType does not match the expected value
    REQUIRE_THROWS_MATCHES(YamlParser::CreateFromPath(TestDataFile("ManifestV1_10-Bad-SchemaHeaderManifestTypeMismatch.yaml"), validateOption), ManifestException, ManifestExceptionMatcher("The manifest type in the schema header does not match the ManifestType property value in the manifest."));

    // Schema header version does not match the expected version
    REQUIRE_THROWS_MATCHES(YamlParser::CreateFromPath(TestDataFile("ManifestV1_10-Bad-SchemaHeaderManifestVersionMismatch.yaml"), validateOption), ManifestException, ManifestExceptionMatcher("The manifest version in the schema header does not match the ManifestVersion property value in the manifest."));
}

TEST_CASE("ShadowManifest", "[ShadowManifest]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    validateOption.AllowShadowManifest = true;

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_5-MultiFile-Version.yaml",
        "ManifestV1_5-Shadow-Installer.yaml",
        "ManifestV1_5-Shadow-DefaultLocale.yaml",
        "ManifestV1_5-Shadow-Locale.yaml",
        "ManifestV1_5-Shadow-Locale2.yaml",
        "ManifestV1_5-Shadow-Shadow.yaml" }, multiFileDirectory);

    auto shadowInfo = ManifestShadowTestInfo{};
    shadowInfo.shadowDefaultLocale = true;
    shadowInfo.shadowEnGbLocale = true;

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContentCreatedWithShadow(multiFileManifest, shadowInfo);

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContentCreatedWithShadow(mergedManifest, shadowInfo);
}

TEST_CASE("ShadowManifest_SkipShadowDefaultLocale", "[ShadowManifest]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    validateOption.AllowShadowManifest = true;

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_5-MultiFile-Version.yaml",
        "ManifestV1_5-Shadow-Installer.yaml",
        "ManifestV1_5-MultiFile-DefaultLocale.yaml",
        "ManifestV1_5-Shadow-Locale.yaml",
        "ManifestV1_5-Shadow-Locale2.yaml",
        "ManifestV1_5-Shadow-Shadow.yaml" }, multiFileDirectory);

    auto shadowInfo = ManifestShadowTestInfo{};
    shadowInfo.shadowDefaultLocale = false;
    shadowInfo.shadowEnGbLocale = true;

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContentCreatedWithShadow(multiFileManifest, shadowInfo);

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContentCreatedWithShadow(mergedManifest, shadowInfo);
}

TEST_CASE("ShadowManifest_SkipShadowLocalizationLocale", "[ShadowManifest]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    validateOption.AllowShadowManifest = true;

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_5-MultiFile-Version.yaml",
        "ManifestV1_5-Shadow-Installer.yaml",
        "ManifestV1_5-Shadow-DefaultLocale.yaml",
        "ManifestV1_5-MultiFile-Locale.yaml",
        "ManifestV1_5-Shadow-Locale2.yaml",
        "ManifestV1_5-Shadow-Shadow.yaml" }, multiFileDirectory);

    auto shadowInfo = ManifestShadowTestInfo{};
    shadowInfo.shadowDefaultLocale = true;
    shadowInfo.shadowEnGbLocale = false;

    TempFile mergedManifestFile{ "merged.yaml" };
    Manifest multiFileManifest = YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile);
    VerifyV1ManifestContentCreatedWithShadow(multiFileManifest, shadowInfo);

    // Read from merged manifest should have the same content as multi file manifest
    Manifest mergedManifest = YamlParser::CreateFromPath(mergedManifestFile);
    VerifyV1ManifestContentCreatedWithShadow(mergedManifest, shadowInfo);
}

TEST_CASE("ShadowManifest_ShadowNotAllowed", "[ShadowManifest]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    validateOption.AllowShadowManifest = false;

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_5-MultiFile-Version.yaml",
        "ManifestV1_5-Shadow-Installer.yaml",
        "ManifestV1_5-Shadow-DefaultLocale.yaml",
        "ManifestV1_5-Shadow-Locale.yaml",
        "ManifestV1_5-Shadow-Locale2.yaml",
        "ManifestV1_5-Shadow-Shadow.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    REQUIRE_THROWS_MATCHES(YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile), ManifestException, ManifestExceptionMatcher("Shadow manifest is not allowed. [ManifestType] Value: shadow File: ManifestV1_5-Shadow-Shadow.yaml"));
}

TEST_CASE("ShadowManifest_TwoShadowFiles", "[ShadowManifest]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    validateOption.AllowShadowManifest = true;

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_5-MultiFile-Version.yaml",
        "ManifestV1_5-Shadow-Installer.yaml",
        "ManifestV1_5-Shadow-DefaultLocale.yaml",
        "ManifestV1_5-Shadow-Shadow.yaml",
        "ManifestV1_5-Shadow-Shadow2.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    REQUIRE_THROWS_MATCHES(YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile), ManifestException, ManifestExceptionMatcher("The multi file manifest should contain only one file with the particular ManifestType. [ManifestType] Value: shadow File: ManifestV1_5-Shadow-Shadow2.yaml"));
}

TEST_CASE("ShadowManifest_NotVerifiedPublisher", "[ShadowManifest]")
{
    ManifestValidateOption validateOption;
    validateOption.FullValidation = true;
    validateOption.AllowShadowManifest = true;
    validateOption.ErrorOnVerifiedPublisherFields = true;

    TempDirectory multiFileDirectory{ "MultiFileManifest" };
    CopyTestDataFilesToFolder({
        "ManifestV1_5-MultiFile-Version.yaml",
        "ManifestV1_5-Shadow-Installer.yaml",
        "ManifestV1_5-Shadow-DefaultLocale.yaml",
        "ManifestV1_5-Shadow-Locale.yaml",
        "ManifestV1_5-Shadow-Locale2.yaml",
        "ManifestV1_5-Shadow-Shadow.yaml" }, multiFileDirectory);

    TempFile mergedManifestFile{ "merged.yaml" };
    REQUIRE_THROWS_MATCHES(YamlParser::CreateFromPath(multiFileDirectory, validateOption, mergedManifestFile), ManifestException, ManifestExceptionMatcher("Field usage requires verified publishers. [Icons]"));
}
