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
    // Forward declaration
    struct ManifestInstaller;

    using string_t = Utility::NormalizedString;
    using namespace std::string_view_literals;
    using Utility::RawVersion;

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

    // V1.4 manifest version
    constexpr std::string_view s_ManifestVersionV1_4 = "1.4.0"sv;

    // V1.5 manifest version
    constexpr std::string_view s_ManifestVersionV1_5 = "1.5.0"sv;

    // V1.6 manifest version
    constexpr std::string_view s_ManifestVersionV1_6 = "1.6.0"sv;

    // V1.7 manifest version
    constexpr std::string_view s_ManifestVersionV1_7 = "1.7.0"sv;

    // V1.9 manifest version
    constexpr std::string_view s_ManifestVersionV1_9 = "1.9.0"sv;

    // V1.10 manifest version
    constexpr std::string_view s_ManifestVersionV1_10 = "1.10.0"sv;

    // The manifest extension for the MS Store
    constexpr std::string_view s_MSStoreExtension = "msstore"sv;

    struct ManifestValidateOption
    {
        bool SchemaValidationOnly = false;
        bool ErrorOnVerifiedPublisherFields = false;
        bool InstallerValidation = false;

        // Options not exposed in winget util
        bool FullValidation = false;
        bool ThrowOnWarning = false;
        bool AllowShadowManifest = false;
        bool SchemaHeaderValidationAsWarning = false;
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
        std::vector<RawVersion> m_extensions;
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
        Deny,
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
        Repair,
    };

    enum class RepairBehaviorEnum
    {
        Unknown,
        Modify,
        Installer,
        Uninstaller,
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
        PackageInUseByApplication,
        InstallInProgress,
        FileInUse,
        MissingDependency,
        DiskFull,
        InsufficientMemory,
        InvalidParameter,
        NoNetwork,
        ContactSupport,
        RebootRequiredToFinish,
        RebootRequiredForInstall,
        RebootInitiated,
        CancelledByUser,
        AlreadyInstalled,
        Downgrade,
        BlockedByPolicy,
        SystemNotSupported,
        Custom,
    };

    enum class PlatformEnum
    {
        Unknown,
        Universal,
        Desktop,
        IoT,
        Team,
        Holographic,
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

    enum class InstalledFileTypeEnum
    {
        Unknown,
        Launch,
        Uninstall,
        Other,
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
        Shadow,
    };

    enum class DependencyType
    {
        WindowsFeature,
        WindowsLibrary,
        Package,
        External
    };

    enum class IconFileTypeEnum
    {
        Unknown,
        Jpeg,
        Png,
        Ico,
    };

    enum class IconThemeEnum
    {
        Unknown,
        Default,
        Light,
        Dark,
        HighContrast,
    };

    // Icon resolutions from https://learn.microsoft.com/en-us/windows/apps/design/style/iconography/app-icon-construction#app-icon
    enum class IconResolutionEnum
    {
        Unknown,
        Custom,
        Square16,
        Square20,
        Square24,
        Square30,
        Square32,
        Square36,
        Square40,
        Square48,
        Square60,
        Square64,
        Square72,
        Square80,
        Square96,
        Square256,
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
        const string_t& Id() const { return m_id; };
        std::optional<Utility::Version> MinVersion;

        Dependency(DependencyType type, string_t id, string_t minVersion) : Type(type), m_id(std::move(id)), MinVersion(Utility::Version(std::move(minVersion))), m_foldedId(FoldCase(m_id)) {}
        Dependency(DependencyType type, string_t id) : Type(type), m_id(std::move(id)), m_foldedId(FoldCase(m_id)){}
        Dependency(DependencyType type) : Type(type) {}

        bool operator ==(const Dependency& rhs) const
        {
            return Type == rhs.Type && m_foldedId == rhs.m_foldedId && MinVersion == rhs.MinVersion;
        }

        bool operator <(const Dependency& rhs) const
        {
            return m_foldedId < rhs.m_foldedId;
        }

        bool IsVersionOk(const Utility::Version& version)
        {
            return MinVersion <= version;
        }

        // m_foldedId should be set whenever Id is set
        void SetId(string_t id)
        {
            m_id = std::move(id);
            m_foldedId = FoldCase(m_id);
        }

    private:
        string_t m_id;
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
        bool HasExactDependency(DependencyType type, const string_t& id, const string_t& minVersion = "");
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

    struct NestedInstallerFile
    {
        string_t RelativeFilePath;
        string_t PortableCommandAlias;
    };

    struct InstalledFile
    {
        string_t RelativeFilePath;
        std::vector<BYTE> FileSha256;
        InstalledFileTypeEnum FileType = InstalledFileTypeEnum::Other;
        string_t InvocationParameter;
        string_t DisplayName;
    };

    struct InstallationMetadataInfo
    {
        string_t DefaultInstallLocation;
        std::vector<InstalledFile> Files;

        // Checks if there are any installation metadata available.
        bool HasData() const { return !DefaultInstallLocation.empty() || !Files.empty(); }

        void Clear() { DefaultInstallLocation.clear(); Files.clear(); }
    };

    InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

    UpdateBehaviorEnum ConvertToUpdateBehaviorEnum(const std::string& in);

    ScopeEnum ConvertToScopeEnum(std::string_view in);

    InstallModeEnum ConvertToInstallModeEnum(const std::string& in);

    PlatformEnum ConvertToPlatformEnum(std::string_view in);

    PlatformEnum ConvertToPlatformEnumForMSStoreDownload(std::string_view in);

    ElevationRequirementEnum ConvertToElevationRequirementEnum(const std::string& in);

    UnsupportedArgumentEnum ConvertToUnsupportedArgumentEnum(const std::string& in);

    ManifestTypeEnum ConvertToManifestTypeEnum(const std::string& in);

    ExpectedReturnCodeEnum ConvertToExpectedReturnCodeEnum(const std::string& in);

    InstalledFileTypeEnum ConvertToInstalledFileTypeEnum(const std::string& in);

    IconFileTypeEnum ConvertToIconFileTypeEnum(std::string_view in);

    IconThemeEnum ConvertToIconThemeEnum(std::string_view in);

    IconResolutionEnum ConvertToIconResolutionEnum(std::string_view in);

    RepairBehaviorEnum ConvertToRepairBehaviorEnum(std::string_view in);

    std::string_view InstallerTypeToString(InstallerTypeEnum installerType);

    std::string_view InstallerSwitchTypeToString(InstallerSwitchType installerSwitchType);

    std::string_view ElevationRequirementToString(ElevationRequirementEnum elevationRequirement);

    std::string_view InstallModeToString(InstallModeEnum installMode);

    std::string_view UnsupportedArgumentToString(UnsupportedArgumentEnum unsupportedArgument);

    std::string_view UpdateBehaviorToString(UpdateBehaviorEnum updateBehavior);

    std::string_view RepairBehaviorToString(RepairBehaviorEnum repairBehavior);

    // Short string representation does not contain "Windows."
    std::string_view PlatformToString(PlatformEnum platform, bool shortString = false);

    std::string_view ScopeToString(ScopeEnum scope);

    std::string_view InstalledFileTypeToString(InstalledFileTypeEnum installedFileType);

    std::string_view IconFileTypeToString(IconFileTypeEnum iconFileType);

    std::string_view IconThemeToString(IconThemeEnum iconTheme);

    std::string_view IconResolutionToString(IconResolutionEnum iconResolution);

    std::string_view ManifestTypeToString(ManifestTypeEnum manifestType);

    std::string_view ExpectedReturnCodeToString(ExpectedReturnCodeEnum expectedReturnCode);

    // Gets a value indicating whether the given installer uses the PackageFamilyName system reference.
    bool DoesInstallerTypeUsePackageFamilyName(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer uses the ProductCode system reference.
    bool DoesInstallerTypeUseProductCode(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer writes ARP entry.
    bool DoesInstallerTypeWriteAppsAndFeaturesEntry(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer type supports ARP version range.
    bool DoesInstallerTypeSupportArpVersionRange(InstallerTypeEnum installer);

    // Gets a value indicating whether the given installer ignores the Scope value from the manifest.
    bool DoesInstallerTypeIgnoreScopeFromManifest(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer requires admin for install.
    bool DoesInstallerTypeRequireAdminForMachineScopeInstall(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer requires RepairBehavior for repair.
    bool DoesInstallerTypeRequireRepairBehaviorForRepair(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer type is an archive.
    bool IsArchiveType(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given installer type is a portable.
    bool IsPortableType(InstallerTypeEnum installerType);

    // Gets a value indicating whether the given nested installer type is supported.
    bool IsNestedInstallerTypeSupported(InstallerTypeEnum nestedInstallerType);

    // Checks whether 2 installer types are compatible. E.g. inno and exe are update compatible
    bool IsInstallerTypeCompatible(InstallerTypeEnum type1, InstallerTypeEnum type2);

    // Get a list of default switches for known installer types
    std::map<InstallerSwitchType, Utility::NormalizedString> GetDefaultKnownSwitches(InstallerTypeEnum installerType);

    // Get a list of default return codes for known installer types
    std::map<DWORD, ExpectedReturnCodeEnum> GetDefaultKnownReturnCodes(InstallerTypeEnum installerType);
}
