// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>

#include <map>

namespace AppInstaller::Manifest
{
    using string_t = Utility::NormalizedString;
    using namespace std::string_view_literals;

    // The maximum supported major version known about by this code.
    constexpr uint64_t s_MaxSupportedMajorVersion = 1;

    // The default manifest version assigned to manifests without a ManifestVersion field.
    constexpr std::string_view s_DefaultManifestVersion = "0.1.0"sv;

    // V1 manifest version for GA
    constexpr std::string_view s_ManifestVersionV1 = "1.0.0"sv;

    // The manifest extension for the MS Store
    constexpr std::string_view s_MSStoreExtension = "msstore"sv;

    // ManifestVer is inherited from Utility::Version and is a more restricted version.
    // ManifestVer is used to specify the version of app manifest itself.
    // ManifestVer is a 3 part version in the format of [0-65535].[0-65535].[0-65535]
    // and optionally a following tag in the format of -[SomeString] for experimental purpose.
    struct ManifestVer : public Utility::Version
    {
        ManifestVer() = default;

        ManifestVer(std::string_view version);

        uint64_t Major() const { return m_parts.size() > 0 ? m_parts[0].Integer : 0; }
        uint64_t Minor() const { return m_parts.size() > 1 ? m_parts[1].Integer : 0; }
        uint64_t Patch() const { return m_parts.size() > 2 ? m_parts[2].Integer : 0; }

        bool HasExtension() const;

        bool HasExtension(std::string_view extension) const;

    private:
        std::vector<Version> m_extensions;
    };

    enum class InstallerTypeEnum
    {
        Unknown,
        Inno,
        Wix,
        Msi,
        Nullsoft,
        Zip,
        Msix,
        Exe,
        Burn,
        MSStore,
    };

    enum class UpdateBehaviorEnum
    {
        Unknown,
        Install,
        UninstallPrevious,
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
        Update,
    };

    enum class ScopeEnum
    {
        Unknown,
        User,
        Machine,
    };

    enum class InstallModeEnum
    {
        Unknown,
        Interactive,
        Silent,
        SilentWithProgress,
    };

    enum class PlatformEnum
    {
        Unknown,
        Universal,
        Desktop,
    };

    enum class ManifestTypeEnum
    {
        Singleton,
        Version,
        Installer,
        DefaultLocale,
        Locale,
        Merged,
        Preview,
    };

    struct PackageDependency
    {
        string_t Id;
        string_t MinVersion;
    };

    struct Dependency
    {
        std::vector<string_t> WindowsFeatures;
        std::vector<string_t> WindowsLibraries;
        std::vector<PackageDependency> PackageDependencies;
        std::vector<string_t> ExternalDependencies;
    };


    InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

    UpdateBehaviorEnum ConvertToUpdateBehaviorEnum(const std::string& in);

    ScopeEnum ConvertToScopeEnum(const std::string& in);

    InstallModeEnum ConvertToInstallModeEnum(const std::string& in);

    PlatformEnum ConvertToPlatformEnum(const std::string& in);

    ManifestTypeEnum ConvertToManifestTypeEnum(const std::string& in);

    std::string_view InstallerTypeToString(InstallerTypeEnum installerType);

    std::string_view ScopeToString(ScopeEnum scope);

    // Gets a value indicating whether the given installer type uses the PackageFamilyName system reference.
    bool DoesInstallerTypeUsePackageFamilyName(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer type uses the ProductCode system reference.
    bool DoesInstallerTypeUseProductCode(InstallerTypeEnum installerType);

    // Checks whether 2 installer types are compatible. E.g. inno and exe are update compatible
    bool IsInstallerTypeCompatible(InstallerTypeEnum type1, InstallerTypeEnum type2);

    // Get a list of default switches for known installer types
    std::map<InstallerSwitchType, Utility::NormalizedString> GetDefaultKnownSwitches(InstallerTypeEnum installerType);
}