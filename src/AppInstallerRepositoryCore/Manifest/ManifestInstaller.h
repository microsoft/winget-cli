// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerArchitecture.h>
#include <AppInstallerStrings.h>
#include <yaml-cpp/yaml.h>

#include <string>
#include <map>

#include "ManifestValidation.h"

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

        // If present, has more precedence than root
        InstallerTypeEnum InstallerType;

        // If present, has more precedence than root
        std::map<InstallerSwitchType, string_t> Switches;

        static InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

        static std::map<InstallerSwitchType, string_t> GetDefaultKnownSwitches(InstallerTypeEnum installerType);

        // Populates InstallerSwitches
        // The value declared in the manifest takes precedence, then value in the manifest root, then default known values.
        static std::vector<ValidationError> PopulateSwitchesFields(
            const YAML::Node& switchesNode,
            std::map<InstallerSwitchType, string_t>& switches,
            bool fullValidation,
            ManifestVer manifestVersion);

        // Populates ManifestInstaller
        // defaultInstaller: if an optional field is not found in the YAML node, the field will be populated with value from defaultInstaller.
        std::vector<ValidationError> PopulateInstallerFields(
            const YAML::Node& installerNode,
            const ManifestInstaller& defaultInstaller,
            bool fullValidation,
            ManifestVer manifestVersion);

        static std::string InstallerTypeToString(InstallerTypeEnum installerType);
    };
}