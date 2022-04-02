// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <functional>
#include <map>
#include <set>
#include <string_view>

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

    // V1.1 manifest version
    constexpr std::string_view s_ManifestVersionV1_1 = "1.1.0"sv;

    // V1.2 manifest version
    constexpr std::string_view s_ManifestVersionV1_2 = "1.2.0"sv;

    // The manifest extension for the MS Store
    constexpr std::string_view s_MSStoreExtension = "msstore"sv;

    struct ManifestValidateOption
    {
        bool SchemaValidationOnly = false;
        bool ErrorOnVerifiedPublisherFields = false;

        // Options not exposed in winget util
        bool FullValidation = false;
        bool ThrowOnWarning = false;
    };

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
        Portable,
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

    enum class ExpectedReturnCodeEnum
    {
        Unknown,
        PackageInUse,
        InstallInProgress,
        FileInUse,
        MissingDependency,
        DiskFull,
        InsufficientMemory,
        NoNetwork,
        ContactSupport,
        RebootRequiredToFinish,
        RebootRequiredForInstall,
        RebootInitiated,
        CancelledByUser,
        AlreadyInstalled,
        Downgrade,
        BlockedByPolicy,
        Custom,
    };

    enum class PlatformEnum
    {
        Unknown,
        Universal,
        Desktop,
    };

    enum class ElevationRequirementEnum
    {
        Unknown,
        ElevationRequired,
        ElevationProhibited,
        ElevatesSelf,
    };

    enum class UnsupportedArgumentEnum
    {
        Unknown,
        Log,
        Location
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

    enum class DependencyType
    {
        WindowsFeature,
        WindowsLibrary,
        Package,
        External
    };

    struct ExpectedReturnCode
    {
        DWORD InstallerReturnCode = 0;
        ExpectedReturnCodeEnum ReturnResponse = ExpectedReturnCodeEnum::Unknown;
        string_t ReturnResponseUrl;
    };

    struct Dependency
    {
        DependencyType Type;
        string_t Id;
        std::optional<Utility::Version> MinVersion;

        Dependency(DependencyType type, string_t id, string_t minVersion) : Type(type), Id(std::move(id)), MinVersion(Utility::Version(minVersion)), m_foldedId(FoldCase(Id)) {}
        Dependency(DependencyType type, string_t id) : Type(type), Id(std::move(id)), m_foldedId(FoldCase(Id)){}
        Dependency(DependencyType type) : Type(type) {}

        bool operator==(const Dependency& rhs) const {
            return Type == rhs.Type && m_foldedId == rhs.m_foldedId && MinVersion == rhs.MinVersion;
        }

        bool operator <(const Dependency& rhs) const
        {
            return m_foldedId < rhs.m_foldedId;
        }

        bool IsVersionOk(Utility::Version version)
        {
            return MinVersion <= Utility::Version(version);
        }

    private:
        std::string m_foldedId;
    };

    struct DependencyList
    {
        void Add(const Dependency& newDependency);
        void Add(const DependencyList& otherDependencyList);
        bool HasAny() const;
        bool HasAnyOf(DependencyType type) const;
        Dependency* HasDependency(const Dependency& dependencyToSearch);
        void ApplyToType(DependencyType type, std::function<void(const Dependency&)> func) const;
        void ApplyToAll(std::function<void(const Dependency&)> func) const;
        bool Empty() const;
        void Clear();
        bool HasExactDependency(DependencyType type, string_t id, string_t minVersion = "");
        size_t Size();

    private:
        std::vector<Dependency> m_dependencies;
    };

    struct AppsAndFeaturesEntry
    {
        string_t DisplayName;
        string_t Publisher;
        string_t DisplayVersion;
        string_t ProductCode;
        string_t UpgradeCode;
        InstallerTypeEnum InstallerType = InstallerTypeEnum::Unknown;
    };

    struct MarketsInfo
    {
        std::vector<string_t> AllowedMarkets;
        std::vector<string_t> ExcludedMarkets;
    };

    InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

    UpdateBehaviorEnum ConvertToUpdateBehaviorEnum(const std::string& in);

    ScopeEnum ConvertToScopeEnum(std::string_view in);

    InstallModeEnum ConvertToInstallModeEnum(const std::string& in);

    PlatformEnum ConvertToPlatformEnum(const std::string& in);

    ElevationRequirementEnum ConvertToElevationRequirementEnum(const std::string& in);

    UnsupportedArgumentEnum ConvertToUnsupportedArgumentEnum(const std::string& in);

    ManifestTypeEnum ConvertToManifestTypeEnum(const std::string& in);

    ExpectedReturnCodeEnum ConvertToExpectedReturnCodeEnum(const std::string& in);

    std::string_view InstallerTypeToString(InstallerTypeEnum installerType);

    std::string_view ScopeToString(ScopeEnum scope);

    // Gets a value indicating whether the given installer type uses the PackageFamilyName system reference.
    bool DoesInstallerTypeUsePackageFamilyName(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer type uses the ProductCode system reference.
    bool DoesInstallerTypeUseProductCode(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer type writes ARP entry.
    bool DoesInstallerTypeWriteAppsAndFeaturesEntry(InstallerTypeEnum installerType);

    // Checks whether 2 installer types are compatible. E.g. inno and exe are update compatible
    bool IsInstallerTypeCompatible(InstallerTypeEnum type1, InstallerTypeEnum type2);

    // Get a list of default switches for known installer types
    std::map<InstallerSwitchType, Utility::NormalizedString> GetDefaultKnownSwitches(InstallerTypeEnum installerType);

    // Get a list of default return codes for known installer types
    std::map<DWORD, ExpectedReturnCodeEnum> GetDefaultKnownReturnCodes(InstallerTypeEnum installerType);
}