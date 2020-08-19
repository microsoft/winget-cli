// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerArchitecture.h>
#include <AppInstallerStrings.h>

#include <string>
#include <map>

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
            Unknown
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

        // If present, has more precedence than root
        InstallerTypeEnum InstallerType;

        // If present, has more precedence than root
        std::map<InstallerSwitchType, string_t> Switches;

        static InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

        static std::string InstallerTypeToString(InstallerTypeEnum installerType);
    };
}