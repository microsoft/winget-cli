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
    };

    struct ExpectedReturnCode
    {
        DWORD InstallerReturnCode;
        ExpectedReturnCodeEnum ReturnResponse;
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

    struct Dependency
    {
        DependencyType Type;
        string_t Id;
        std::optional<AppInstaller::Utility::Version> MinVersion;

        Dependency(DependencyType type, string_t id, string_t minVersion) : Type(type), Id(std::move(id)), MinVersion(AppInstaller::Utility::Version(minVersion)) {}
        Dependency(DependencyType type, string_t id) : Type(type), Id(std::move(id)) {}
        Dependency(DependencyType type) : Type(type) {}

        bool operator==(const Dependency& rhs) const {
            return Type == rhs.Type && ICUCaseInsensitiveEquals(Id, rhs.Id);
        }

        bool operator <(const Dependency& rhs) const
        {
            return Id < rhs.Id;
        }

        bool IsVersionOk(string_t version)
        {
            return MinVersion <= AppInstaller::Utility::Version(version);
        }
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

    struct DependencyList
    {
        DependencyList() = default;

        void Add(const Dependency& newDependency)
        { 
            Dependency* existingDependency = HasDependency(newDependency);

            if (existingDependency != NULL) {
                if (newDependency.MinVersion) 
                {
                    if (existingDependency->MinVersion)
                    {
                        if (newDependency.MinVersion.value() > existingDependency->MinVersion.value())
                        {
                            existingDependency->MinVersion.value() = newDependency.MinVersion.value();
                        }
                    }
                    else
                    {
                        existingDependency->MinVersion.value() = newDependency.MinVersion.value();
                    }
                }
            }
            else 
            {
                dependencies.push_back(newDependency); 
            }
        }

        void Add(const DependencyList& otherDependencyList)
        {
            for (const auto& dependency : otherDependencyList.dependencies)
            {
                Add(dependency);
            }
        }

        bool HasAny() const { return !dependencies.empty(); }
        bool HasAnyOf(DependencyType type) const 
        {
            for (const auto& dependency : dependencies)
            {
                if (dependency.Type == type) return true;
            };
            return false;
        }

        Dependency* HasDependency(const Dependency& dependencyToSearch)
        {
            for (auto& dependency : dependencies) {
                if (dependency == dependencyToSearch)
                {
                    return &dependency;
                }
            }
            return nullptr;
        }

        void ApplyToType(DependencyType type, std::function<void(const Dependency&)> func) const
        {
            for (const auto& dependency : dependencies)
            {
                if (dependency.Type == type) func(dependency);
            }
        }

        void ApplyToAll(std::function<void(const Dependency&)> func) const
        {
            for (const auto& dependency : dependencies)
            {
                func(dependency);
            }
        }

        bool Empty() const
        {
            return dependencies.empty();
        }

        void Clear() { dependencies.clear(); }

        // for testing purposes
        bool HasExactDependency(DependencyType type, string_t id, string_t minVersion = "")
        {
            for (const auto& dependency : dependencies)
            {
                if (dependency.Type == type && Utility::ICUCaseInsensitiveEquals(dependency.Id, id))
                {
                    if (dependency.MinVersion) {
                        if (dependency.MinVersion.value() == AppInstaller::Utility::Version(minVersion))
                        {
                            return true;
                        }
                    }
                    else {
                        return true;
                    }
                }
            }
            return false;
        }

        size_t Size()
        {
            return dependencies.size();
        }

    private:
        std::vector<Dependency> dependencies;
    };

    struct DependencyGraph
    {
        // this constructor was intented for use during installation flow (we already have installer dependencies and there's no need to search the source again)
        DependencyGraph(const Dependency& root, const DependencyList& rootDependencies,
            std::function<const DependencyList(const Dependency&)> infoFunction) : m_root(root), getDependencies(infoFunction)
        {
            m_adjacents[m_root] = std::set<Dependency>();
            m_toCheck = std::vector<Dependency>();
            rootDependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                {
                    m_toCheck.push_back(dependency);
                    AddNode(dependency);
                    AddAdjacent(root, dependency);
                });
        }

        DependencyGraph(const Dependency& root, std::function<const DependencyList(const Dependency&)> infoFunction) : m_root(root), getDependencies(infoFunction)
        {
            m_adjacents[m_root] = std::set<Dependency>();
            m_toCheck = std::vector<Dependency>();

            const DependencyList& rootDependencies = getDependencies(root);
            rootDependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                {
                    m_toCheck.push_back(dependency);
                    AddNode(dependency);
                    AddAdjacent(root, dependency);
                });
        }

        void BuildGraph()
        {
            if (m_toCheck.empty())
            {
                return;
            }

            for (unsigned int i = 0; i < m_toCheck.size(); ++i)
            {
                auto node = m_toCheck.at(i);

                const auto& nodeDependencies = getDependencies(node);
                nodeDependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                    {
                        if (!HasNode(dependency))
                        {
                            m_toCheck.push_back(dependency);
                            AddNode(dependency);
                        }

                        AddAdjacent(node, dependency);
                    });
            }

            CheckForLoopsAndGetOrder();
        }

        void AddNode(const Dependency& node)
        {
            m_adjacents[node] = std::set<Dependency>();
        }

        void AddAdjacent(const Dependency& node,const Dependency& adjacent)
        {
            m_adjacents[node].emplace(adjacent);
        }

        bool HasNode(const Dependency& dependency)
        {
            auto search = m_adjacents.find(dependency);
            return search != m_adjacents.end();
        }

        bool HasLoop()
        {
            return hasLoop;
        }

        void CheckForLoopsAndGetOrder()
        {
            m_installationOrder = std::vector<Dependency>();
            std::set<Dependency> visited;
            hasLoop = HasLoopDFS(visited, m_root);
        }

        std::vector<Dependency> GetInstallationOrder()
        {
            return m_installationOrder;
        }

    private:
        // TODO make this function iterative
        bool HasLoopDFS(std::set<Dependency> visited, const Dependency& node)
        {
            bool loop = false;

            visited.insert(node);
            auto lAdjacents = m_adjacents.at(node);
            for (const auto& adjacent : m_adjacents.at(node))
            {
                auto search = visited.find(adjacent);
                if (search == visited.end()) // if not found
                {
                    if (HasLoopDFS(visited, adjacent))
                    {
                        loop = true;
                        // didn't break the loop to have a complete order at the end (even if a loop exists)
                    }
                }
                else
                {
                    loop = true;
                    // didn't break the loop to have a complete order at the end (even if a loop exists)
                }
            }

            // Adding to have an order even if a loop is present
            if (std::find(m_installationOrder.begin(), m_installationOrder.end(), node) == m_installationOrder.end())
            {
                m_installationOrder.push_back(node);
            }

            return loop;
        }

        const Dependency& m_root;
        std::map<Dependency, std::set<Dependency>> m_adjacents;
        std::function<const DependencyList(const Dependency&)> getDependencies;

        bool hasLoop;
        std::vector<Dependency> m_installationOrder;

        std::vector<Dependency> m_toCheck;
    };

    InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in);

    UpdateBehaviorEnum ConvertToUpdateBehaviorEnum(const std::string& in);

    ScopeEnum ConvertToScopeEnum(std::string_view in);

    InstallModeEnum ConvertToInstallModeEnum(const std::string& in);

    PlatformEnum ConvertToPlatformEnum(const std::string& in);

    ElevationRequirementEnum ConvertToElevationRequirementEnum(const std::string& in);

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