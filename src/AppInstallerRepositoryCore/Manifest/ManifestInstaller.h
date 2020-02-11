// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <optional>
#include <AppInstallerArchitecture.h>
#include "InstallerSwitches.h"

namespace AppInstaller::Manifest
{
    class ManifestInstaller
    {
    public:

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
            InstallShield,
            Unknown
        };

        // Required. Values: x86, x64, arm, arm64, all.
        AppInstaller::Utility::Architecture Arch;

        // Required
        std::string Url;

        // Required
        std::vector<BYTE> Sha256;

        // Optional. Only used by appx/msix type. If provided, Appinstaller will
        // validate appx/msix signature and perform streaming install.
        std::vector<BYTE> SignatureSha256;

        // Empty means default
        std::string Language;

        // Name TBD
        std::string Scope;

        // If present, has more presedence than root
        InstallerTypeEnum InstallerType;

        // If present, has more presedence than root
        std::optional<InstallerSwitches> Switches;

        static InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

        // Populates ManifestInstaller
        // defaultInstaller: if an optional field is not found in the YAML node, the field will be populated with value from defaultInstaller.
        void PopulateInstallerFields(const YAML::Node& installerNode, const ManifestInstaller& defaultInstaller);
    };

    std::ostream& operator<<(std::ostream& out, const ManifestInstaller::InstallerTypeEnum& installerType);
}