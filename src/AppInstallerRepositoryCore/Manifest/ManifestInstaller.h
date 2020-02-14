// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <map>
#include <AppInstallerArchitecture.h>

namespace AppInstaller::Manifest
{
    using namespace std::string_view_literals;

    // Token specified in installer args will be replaced by proper value.
    static constexpr std::string_view ARG_TOKEN_LOGPATH = "<LOGPATH>"sv;
    static constexpr std::string_view ARG_TOKEN_INSTALLPATH = "<INSTALLPATH>"sv;

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

        // If present, has more precedence than root
        InstallerTypeEnum InstallerType;

        // If present, has more precedence than root
        std::map<InstallerSwitchType, std::string> Switches;

        static InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

        static std::map<InstallerSwitchType, std::string> GetDefaultArgs(InstallerTypeEnum installerType);

        // Populates InstallerSwitches
        // The value declared in the manifest takes precedence, then value in the manifest root, then default known values.
        static void PopulateSwitchesFields(
            const YAML::Node* switchesNode,
            std::map<InstallerSwitchType, std::string>& switches,
            const std::map<InstallerSwitchType, std::string>* manifestRootSwitches = nullptr,
            const std::map<InstallerSwitchType, std::string>* defaultKnownSwitches = nullptr);

        // Populates one Installer Switch
        // The value declared in the manifest takes precedence, then value in the manifest root, then default known values.
        static void PopulateOneSwitchField(
            const YAML::Node* switchesNode,
            const std::string& switchName,
            InstallerSwitchType switchType,
            std::map<InstallerSwitchType, std::string>& switches,
            const std::map<InstallerSwitchType, std::string>* manifestRootSwitches,
            const std::map<InstallerSwitchType, std::string>* defaultKnownSwitches);

        // Populates ManifestInstaller
        // defaultInstaller: if an optional field is not found in the YAML node, the field will be populated with value from defaultInstaller.
        void PopulateInstallerFields(const YAML::Node& installerNode, const ManifestInstaller& defaultInstaller);
    };

    std::ostream& operator<<(std::ostream& out, const ManifestInstaller::InstallerTypeEnum& installerType);
}