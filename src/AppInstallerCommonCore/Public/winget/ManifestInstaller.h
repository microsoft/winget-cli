// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerArchitecture.h>
#include <AppInstallerStrings.h>
#include <winget\ManifestCommon.h>

#include <map>
#include <string>

namespace AppInstaller::Manifest
{
    using namespace std::string_view_literals;

    // Token specified in installer args will be replaced by proper value.
    static constexpr std::string_view ARG_TOKEN_LOGPATH = "<LOGPATH>"sv;
    static constexpr std::string_view ARG_TOKEN_INSTALLPATH = "<INSTALLPATH>"sv;

    struct ManifestInstaller
    {
        using string_t = Utility::NormalizedString;

        AppInstaller::Utility::Architecture Arch = AppInstaller::Utility::Architecture::Unknown;

        string_t Url;

        std::vector<BYTE> Sha256;

        // Optional. Only used by appx/msix type. If provided, Appinstaller will
        // validate appx/msix signature and perform streaming install.
        std::vector<BYTE> SignatureSha256;

        // Store Product Id
        string_t ProductId;

        string_t Locale;

        std::vector<PlatformEnum> Platform;

        string_t MinOSVersion;

        // If present, has more precedence than root
        InstallerTypeEnum BaseInstallerType = InstallerTypeEnum::Unknown;

        InstallerTypeEnum NestedInstallerType = InstallerTypeEnum::Unknown;

        InstallerTypeEnum EffectiveInstallerType() const
        {
            return IsArchiveType(BaseInstallerType) ? NestedInstallerType : BaseInstallerType;
        }

        std::vector<NestedInstallerFile> NestedInstallerFiles;

        ScopeEnum Scope = ScopeEnum::Unknown;

        std::vector<InstallModeEnum> InstallModes;

        // If present, has more precedence than root
        std::map<InstallerSwitchType, string_t> Switches;

        std::vector<DWORD> InstallerSuccessCodes;

        struct ExpectedReturnCodeInfo
        {
            ExpectedReturnCodeEnum ReturnResponseEnum = ExpectedReturnCodeEnum::Unknown;
            string_t ReturnResponseUrl;
        };

        std::map<DWORD, ExpectedReturnCodeInfo> ExpectedReturnCodes;

        UpdateBehaviorEnum UpdateBehavior = UpdateBehaviorEnum::Install;

        std::vector<string_t> Commands;

        std::vector<string_t> Protocols;

        std::vector<string_t> FileExtensions;

        // Package family name for MSIX packaged installers.
        string_t PackageFamilyName;

        // Product code for ARP (Add/Remove Programs) installers.
        string_t ProductCode;

        // For msix only
        std::vector<string_t> Capabilities;

        // For msix only
        std::vector<string_t> RestrictedCapabilities;

        DependencyList Dependencies;

        bool InstallerAbortsTerminal = false;

        string_t ReleaseDate;

        bool InstallLocationRequired = false;

        bool RequireExplicitUpgrade = false;

        bool DisplayInstallWarnings = false;

        std::vector<UnsupportedArgumentEnum> UnsupportedArguments;

        std::vector<AppInstaller::Utility::Architecture> UnsupportedOSArchitectures;

        std::vector<AppsAndFeaturesEntry> AppsAndFeaturesEntries;

        ElevationRequirementEnum ElevationRequirement = ElevationRequirementEnum::Unknown;

        MarketsInfo Markets;

        InstallationMetadataInfo InstallationMetadata;
    };
}