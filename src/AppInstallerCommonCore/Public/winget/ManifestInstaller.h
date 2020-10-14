// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerArchitecture.h>
#include <AppInstallerStrings.h>

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

        enum class InstallerTypeEnum
        {
            Inno,
            Wix,
            Msi,
            Nullsoft,
            Zip,
            Msix,
            Exe,
            Burn,
            MSStore,
            Unknown,
        };

        enum class UpdateBehaviorEnum
        {
            Install,
            UninstallPrevious,
            Unknown,
        };

        enum class InstallerSwitchType
        {
            Custom,
            Silent,
            SilentWithProgress,
            Interactive,
            Language,
            Log,
            InstallLocation,
            Update
        };

        // Required. Values: x86, x64, arm, arm64, all.
        AppInstaller::Utility::Architecture Arch;

        // Required
        string_t Url;

        // Required
        std::vector<BYTE> Sha256;

        // Optional. Only used by appx/msix type. If provided, Appinstaller will
        // validate appx/msix signature and perform streaming install.
        std::vector<BYTE> SignatureSha256;

        // Empty means default
        string_t Language;

        // Name TBD
        string_t Scope;

        // Store Product Id
        string_t ProductId;

        // Package family name for MSIX packaged installers.
        string_t PackageFamilyName;

        // Product code for ARP (Add/Remove Programs) installers.
        string_t ProductCode;

        // If present, has more precedence than root
        InstallerTypeEnum InstallerType;

        // Default is Install if not specified
        UpdateBehaviorEnum UpdateBehavior;

        // If present, has more precedence than root
        std::map<InstallerSwitchType, string_t> Switches;

        static InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

        static UpdateBehaviorEnum ConvertToUpdateBehaviorEnum(const std::string& in);

        static std::string InstallerTypeToString(InstallerTypeEnum installerType);

        // Gets a value indicating whether the given installer type uses the PackageFamilyName system reference.
        static bool DoesInstallerTypeUsePackageFamilyName(InstallerTypeEnum installerType);

        // Gets a value indicating whether the given installer type uses the ProductCode system reference.
        static bool DoesInstallerTypeUseProductCode(InstallerTypeEnum installerType);

        // Checks whether 2 installer types are compatible. E.g. inno and exe are update compatible
        static bool IsInstallerTypeCompatible(InstallerTypeEnum type1, InstallerTypeEnum type2);
    };
}