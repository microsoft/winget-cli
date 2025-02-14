// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CompositeSource.h"
#include <winget/ExperimentalFeature.h>

using namespace AppInstaller::Settings;

namespace AppInstaller::Repository
{
    using namespace std::string_view_literals;

    namespace anon
    {
        Utility::VersionAndChannel GetVACFromVersion(IPackageVersion* packageVersion)
        {
            return {
                Utility::Version(packageVersion->GetProperty(PackageVersionProperty::Version)),
                Utility::Channel(packageVersion->GetProperty(PackageVersionProperty::Channel))
            };
        }

        // Returns true for fields that provide a strong match; one that is not based on a heuristic.
        bool IsStrongMatchField(PackageMatchField field)
        {
            switch (field)
            {
            case AppInstaller::Repository::PackageMatchField::PackageFamilyName:
            case AppInstaller::Repository::PackageMatchField::ProductCode:
            case AppInstaller::Repository::PackageMatchField::UpgradeCode:
                return true;
            }

            return false;
        }

        // Gets the only available package from the composite, ensuring this fact in test contexts.
        std::shared_ptr<IPackage> OnlyAvailable(const std::shared_ptr<ICompositePackage>& composite)
        {
            std::vector<std::shared_ptr<IPackage>> availablePackages = composite->GetAvailable();

#ifndef AICLI_DISABLE_TEST_HOOKS
            THROW_HR_IF(E_UNEXPECTED, availablePackages.size() != 1);
#endif

            return std::move(availablePackages.front());
        }

        // Move returns if there is only one package in the matches that is strong; otherwise returns an empty value.
        std::shared_ptr<ICompositePackage> FindOnlyStrongMatchFieldResult(std::vector<ResultMatch>& matches)
        {
            std::shared_ptr<ICompositePackage> result;

            for (auto&& match : matches)
            {
                AICLI_LOG(Repo, Info, << "  Checking match with package id: " << match.Package->GetProperty(PackageProperty::Id));

                if (IsStrongMatchField(match.MatchCriteria.Field))
                {
                    if (!result)
                    {
                        result = std::move(match.Package);
                    }
                    else
                    {
                        AICLI_LOG(Repo, Info, << "  Found multiple packages with strong match fields");
                        result.reset();
                        break;
                    }
                }
            }

            return result;
        }

        // Gets a single matching package from the results
        template <typename MultipleIntro, typename Indeterminate>
        std::shared_ptr<ICompositePackage> GetMatchingPackage(std::vector<ResultMatch>& matches, MultipleIntro&& multipleIntro, Indeterminate&& indeterminate)
        {
            if (matches.empty())
            {
                return {};
            }
            else if (matches.size() == 1)
            {
                return std::move(matches[0].Package);
            }
            else
            {
                multipleIntro();

                auto result = FindOnlyStrongMatchFieldResult(matches);

                if (!result)
                {
                    indeterminate();
                }

                return result;
            }
        }

        // For a given package from a tracking catalog, get the latest write time.
        // Look at all versions rather than just the latest to account for the potential of downgrading.
        std::chrono::system_clock::time_point GetLatestTrackingWriteTime(
            const std::shared_ptr<IPackage>& trackingPackage)
        {
            std::chrono::system_clock::time_point result{};

            for (const auto& key : trackingPackage->GetVersionKeys())
            {
                auto version = trackingPackage->GetVersion(key);
                if (version)
                {
                    auto metadata = version->GetMetadata();
                    auto itr = metadata.find(PackageVersionMetadata::TrackingWriteTime);
                    if (itr != metadata.end())
                    {
                        std::int64_t unixEpoch = 0;
                        try
                        {
                            unixEpoch = std::stoll(itr->second);
                        }
                        CATCH_LOG();

                        std::chrono::system_clock::time_point versionTime = Utility::ConvertUnixEpochToSystemClock(unixEpoch);

                        if (versionTime > result)
                        {
                            result = versionTime;
                        }
                    }
                }
            }

            return result;
        }

        // An installed package's version reported in ARP does not necessarily match the versions used for the manifest.
        // This function uses the data in the manifest to map the installed version string to the version used by the manifest.
        //
        // TODO: Note: Currently this function assumes the all versions in the available package is from one source.
        // Even though a composite package can have available packages from multiple sources, we only call this function
        // for the default (first) available package. If we ever need to consider other sources, this function needs to be revisited.
        std::string GetMappedInstalledVersion(const std::string& installedVersion, const std::shared_ptr<IPackage>& availablePackage)
        {
            // Perform an initial check to see if the latest version has a mapping; if it does not, do not attempt any more.
            auto latestVersion = availablePackage->GetLatestVersion();
            if (latestVersion)
            {
                auto version = latestVersion->GetProperty(PackageVersionProperty::Version);
                auto arpMinVersion = latestVersion->GetProperty(PackageVersionProperty::ArpMinVersion);
                auto arpMaxVersion = latestVersion->GetProperty(PackageVersionProperty::ArpMaxVersion);

                if ((arpMinVersion.empty() || arpMinVersion == version) && (arpMaxVersion.empty() || arpMaxVersion == version))
                {
                    return installedVersion;
                }
            }

            // Stores raw versions value strings to run a preliminary check whether version mapping is needed.
            std::vector<std::tuple<std::string, std::string, std::string>> rawVersionValues;
            auto versionKeys = availablePackage->GetVersionKeys();
            bool shouldTryPerformMapping = false;

            for (auto const& versionKey : versionKeys)
            {
                auto availableVersion = availablePackage->GetVersion(versionKey);
                std::string arpMinVersion = availableVersion->GetProperty(PackageVersionProperty::ArpMinVersion);
                std::string arpMaxVersion = availableVersion->GetProperty(PackageVersionProperty::ArpMaxVersion);

                if (!arpMinVersion.empty() && !arpMaxVersion.empty())
                {
                    std::string manifestVersion = versionKey.Version;

                    if (!shouldTryPerformMapping && (arpMinVersion != manifestVersion || arpMaxVersion != manifestVersion))
                    {
                        shouldTryPerformMapping = true;
                    }

                    rawVersionValues.emplace_back(std::make_tuple(std::move(manifestVersion), std::move(arpMinVersion), std::move(arpMaxVersion)));
                }
            }

            if (!shouldTryPerformMapping)
            {
                return installedVersion;
            }

            // Construct a map between manifest version and arp version range. The map is ordered in descending by package version.
            std::vector<std::pair<Utility::Version, Utility::VersionRange>> arpVersionMap;

            for (auto& tuple : rawVersionValues)
            {
                auto&& [manifestVersion, arpMinVersion, arpMaxVersion] = std::move(tuple);
                Utility::VersionRange arpVersionRange{ Utility::Version(std::move(arpMinVersion)), Utility::Version(std::move(arpMaxVersion)) };
                Utility::Version manifestVer{ std::move(manifestVersion) };
                // Skip mapping to unknown version
                if (!manifestVer.IsUnknown())
                {
                    arpVersionMap.emplace_back(std::make_pair(std::move(manifestVer), std::move(arpVersionRange)));
                }
            }

            // Go through the arp version map and determine what mapping should be performed.
            // shouldPerformMapping is true when at least 1 arp version range is different from the package version.
            bool shouldPerformMapping = false;
            bool isArpVersionRangeInDescendingOrder = true;
            const Utility::VersionRange* previousVersionRange = nullptr;

            for (auto const& pair : arpVersionMap)
            {
                // If arp version range is not same as package version, should perform mapping
                // This check is still needed to account for 1.0 == 1.0.0 cases
                if (!shouldPerformMapping && !pair.second.IsSameAsSingleVersion(pair.first))
                {
                    shouldPerformMapping = true;
                }

                if (!previousVersionRange)
                {
                    // This is the first non empty arp version range
                    previousVersionRange = &pair.second;
                }
                else if (isArpVersionRangeInDescendingOrder)
                {
                    // The arp version range should be less than previous range
                    if (pair.second < *previousVersionRange)
                    {
                        previousVersionRange = &pair.second;
                    }
                    else
                    {
                        isArpVersionRangeInDescendingOrder = false;
                    }
                }
            }

            // Now perform arp version mapping
            if (shouldPerformMapping)
            {
                Utility::Version installed{ installedVersion };
                for (auto const& pair : arpVersionMap)
                {
                    // If the installed version is in the arp version range
                    if (pair.second.ContainsVersion(installed))
                    {
                        return pair.first.ToString();
                    }
                }

                // At this point, no mapping found. Perform approximate mapping if applicable.
                // We'll start from end of the vector because we try to find closest less than version if possible.
                if (isArpVersionRangeInDescendingOrder)
                {
                    const Utility::Version* lastGreaterThanVersion = nullptr;
                    auto it = arpVersionMap.rbegin();
                    while (it != arpVersionMap.rend())
                    {
                        const auto& pair = *it;
                        if (installed < pair.second.GetMinVersion())
                        {
                            return Utility::Version{ pair.first, Utility::Version::ApproximateComparator::LessThan }.ToString();
                        }
                        else
                        {
                            lastGreaterThanVersion = &pair.first;
                        }
                        
                        it++;
                    }

                    // No approximate less than version found, approximate greater than version will be returned.
                    if (lastGreaterThanVersion)
                    {
                        return Utility::Version{ *lastGreaterThanVersion, Utility::Version::ApproximateComparator::GreaterThan }.ToString();
                    }
                }
            }

            // return the input installed version if no mapping is performed or found.
            return installedVersion;
        }

        // A composite package installed version that allows us to override the source or the version.
        struct CompositeInstalledVersion : public IPackageVersion
        {
            CompositeInstalledVersion(std::shared_ptr<IPackageVersion> baseInstalledVersion, Source trackingSource, std::shared_ptr<IPackageVersion> trackingPackageVersion, std::string overrideVersion = {}) :
                m_baseInstalledVersion(std::move(baseInstalledVersion)), m_trackingSource(std::move(trackingSource)), m_trackingPackageVersion(std::move(trackingPackageVersion)), m_overrideVersion(std::move(overrideVersion))
            {}

            Utility::LocIndString GetProperty(PackageVersionProperty property) const override
            {
                // If there is an override version, use it.
                if (property == PackageVersionProperty::Version && !m_overrideVersion.empty())
                {
                    return Utility::LocIndString{ m_overrideVersion };
                }

                return m_baseInstalledVersion->GetProperty(property);
            }

            std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
            {
                return m_baseInstalledVersion->GetMultiProperty(property);
            }

            Manifest::Manifest GetManifest() override
            {
                return m_baseInstalledVersion->GetManifest();
            }

            Source GetSource() const override
            {
                // If there is a tracking source, use it instead to indicate that it came from there.
                if (m_trackingSource)
                {
                    return m_trackingSource;
                }

                return m_baseInstalledVersion->GetSource();
            }

            Metadata GetMetadata() const override
            {
                auto result = m_baseInstalledVersion->GetMetadata();

                // Populate metadata from tracking package version if not present in base installed version.
                if (m_trackingPackageVersion)
                {
                    auto trackingMetadata = m_trackingPackageVersion->GetMetadata();
                    for (auto metadataItem : { PackageVersionMetadata::InstalledArchitecture, PackageVersionMetadata::InstalledLocale,
                        PackageVersionMetadata::UserIntentArchitecture, PackageVersionMetadata::UserIntentLocale, PackageVersionMetadata::PinnedState })
                    {
                        auto itr = trackingMetadata.find(metadataItem);
                        auto existingItr = result.find(metadataItem);
                        if (itr != trackingMetadata.end() && existingItr == result.end())
                        {
                            result[metadataItem] = itr->second;
                        }
                    }
                }

                return result;
            }

        private:
            std::shared_ptr<IPackageVersion> m_baseInstalledVersion;
            Source m_trackingSource;
            std::string m_overrideVersion;
            std::shared_ptr<IPackageVersion> m_trackingPackageVersion;
        };

        // An IPackage for the installed package of a CompositePackage.
        struct CompositeInstalledPackage : public IPackage
        {
            static constexpr IPackageType PackageType = IPackageType::CompositeInstalledPackage;

            CompositeInstalledPackage(std::shared_ptr<IPackage> package)
            {
                AddPackageAndVersionKeyData(std::move(package));
            }

            Utility::LocIndString GetProperty(PackageProperty property) const override
            {
                THROW_HR_IF(E_UNEXPECTED, m_packages.empty() || m_versionKeyData.empty());

                // Use the highest version for package properties
                return m_packages[m_versionKeyData[0].PackageIndex]->GetProperty(property);
            }

            std::vector<PackageVersionKey> GetVersionKeys() const override
            {
                return { m_versionKeyData.begin(), m_versionKeyData.end() };
            }

            std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey& versionKey) const override
            {
                std::shared_ptr<IPackageVersion> installedVersion;
                std::string overrideVersion;

                for (const VersionKeyData& key : m_versionKeyData)
                {
                    if (key.IsMatch(versionKey))
                    {
                        installedVersion = key.InstalledVersion;
                        overrideVersion = key.Version;
                        break;
                    }
                }

                if (installedVersion)
                {
                    // Get the appropriate tracking version or latest if it is not found.
                    // The tracking package uses the mapped version.
                    std::shared_ptr<IPackageVersion> trackingPackageVersion;
                    if (m_trackingPackage)
                    {
                        // Remove our use of the package id as source
                        PackageVersionKey versionKey_NoSource = versionKey;
                        versionKey_NoSource.SourceId.clear();

                        trackingPackageVersion = m_trackingPackage->GetVersion(versionKey_NoSource);

                        if (!trackingPackageVersion)
                        {
                            trackingPackageVersion = m_trackingPackage->GetLatestVersion();
                        }
                    }

                    return std::make_shared<CompositeInstalledVersion>(std::move(installedVersion), m_trackingSource, std::move(trackingPackageVersion), std::move(overrideVersion));
                }

                return nullptr;
            }

            std::shared_ptr<IPackageVersion> GetLatestVersion() const override
            {
                return GetVersion({});
            }

            Source GetSource() const override
            {
                // If there is a tracking source, use it instead to indicate that it came from there.
                // Otherwise, all of the installed packages should be from the same source.
                return m_trackingSource ? m_trackingSource : m_packages[0]->GetSource();
            }

            bool IsSame(const IPackage* other) const override
            {
                const CompositeInstalledPackage* otherPackage = PackageCast<const CompositeInstalledPackage*>(other);

                if (otherPackage)
                {
                    if (m_packages.size() != otherPackage->m_packages.size())
                    {
                        return false;
                    }

                    for (const auto& subPackage : m_packages)
                    {
                        bool foundSame = false;

                        for (const auto& otherSubPackage : otherPackage->m_packages)
                        {
                            if (subPackage->IsSame(otherSubPackage.get()))
                            {
                                foundSame = true;
                                break;
                            }
                        }

                        if (!foundSame)
                        {
                            return false;
                        }
                    }

                    return true;
                }

                return false;
            }

            const void* CastTo(IPackageType type) const override
            {
                if (type == PackageType)
                {
                    return this;
                }

                return nullptr;
            }

            void SetTracking(
                Source trackingSource,
                std::shared_ptr<IPackage> trackingPackage,
                std::chrono::system_clock::time_point trackingWriteTime)
            {
                m_trackingSource = std::move(trackingSource);
                m_trackingPackage = std::move(trackingPackage);
                m_trackingWriteTime = trackingWriteTime;
            }

            Source GetTrackingSource() const
            {
                return m_trackingSource;
            }

            const std::shared_ptr<IPackage>& GetTrackingPackage() const
            {
                return m_trackingPackage;
            }

            std::chrono::system_clock::time_point GetTrackingPackageWriteTime() const
            {
                return m_trackingWriteTime;
            }

            bool ContainsInstalledPackage(const IPackage* installedPackage) const
            {
                for (const auto& package : m_packages)
                {
                    if (package->IsSame(installedPackage))
                    {
                        return true;
                    }
                }

                return false;
            }

            void FoldInstalledIn(const std::shared_ptr<CompositeInstalledPackage>& other)
            {
                for (const auto& package : other->m_packages)
                {
                    AddPackageAndVersionKeyData(package);
                }
            }

            // Set a version that will override the version string from the installed package
            void SetOverrideInstalledVersion(const std::shared_ptr<IPackage>& availablePackage)
            {
                if (availablePackage)
                {
                    m_availablePackageVersionOverride = availablePackage;

                    for (auto& key : m_versionKeyData)
                    {
                        if (Manifest::DoesInstallerTypeSupportArpVersionRange(key.InstalledType))
                        {
                            key.Version = GetMappedInstalledVersion(key.InstalledVersion->GetProperty(PackageVersionProperty::Version), availablePackage);
                        }
                    }
                }
            }

            bool IsEmpty() const
            {
                return m_versionKeyData.empty();
            }

        private:
            // Contains information about all of the version keys.
            // We use the `SourceId` field to store the installed package identifier so that we can disambiguate keys is they have the same version.
            struct VersionKeyData : public PackageVersionKey
            {
                size_t PackageIndex;
                std::shared_ptr<IPackageVersion> InstalledVersion;
                Manifest::InstallerTypeEnum InstalledType;
                Utility::VersionAndChannel VersionAndChannel;

                bool operator<(const VersionKeyData& other) const
                {
                    return VersionAndChannel < other.VersionAndChannel;
                }
            };

            // Adds the package and version key data to the composite.
            // The version keys are then sorted so that the first (index 0) in the vector has the highest version.
            // Note that it may tied for highest version if, for instance, the same version is installed for different architectures.
            void AddPackageAndVersionKeyData(std::shared_ptr<IPackage> package)
            {
                // We don't want this to happen, but it could. Rather than a crash, we will log it and move on.
                if (!package)
                {
                    AICLI_LOG(Repo, Verbose, << "AddPackageAndVersionKeyData called with an empty package");
                    return;
                }

                size_t packageIndex = m_packages.size();
                std::string packageIdentifier = package->GetProperty(PackageProperty::Id);
                bool versionAdded = false;

                for (const auto& versionKey : package->GetVersionKeys())
                {
                    VersionKeyData keyData{ versionKey };

                    keyData.PackageIndex = packageIndex;
                    keyData.InstalledVersion = package->GetVersion(versionKey);

                    if (!keyData.InstalledVersion)
                    {
                        AICLI_LOG(Repo, Verbose, << "AddPackageAndVersionKeyData: Package [" << packageIdentifier << "] did not return a version for [" << versionKey.Version << "]");
                        continue;
                    }

                    // We use the `SourceId` field to store the installed package identifier so that we can disambiguate keys if they have the same version.
                    keyData.SourceId = packageIdentifier;

                    keyData.InstalledType = Manifest::ConvertToInstallerTypeEnum(keyData.InstalledVersion->GetMetadata()[PackageVersionMetadata::InstalledType]);
                    if (m_availablePackageVersionOverride && Manifest::DoesInstallerTypeSupportArpVersionRange(keyData.InstalledType))
                    {
                        keyData.Version = GetMappedInstalledVersion(keyData.InstalledVersion->GetProperty(PackageVersionProperty::Version), m_availablePackageVersionOverride);
                    }

                    keyData.VersionAndChannel = Utility::VersionAndChannel{ keyData.Version, keyData.Channel };

                    m_versionKeyData.emplace_back(std::move(keyData));
                    versionAdded = true;
                }

                if (versionAdded)
                {
                    m_packages.emplace_back(std::move(package));

                    std::sort(m_versionKeyData.begin(), m_versionKeyData.end());
                }
            }

            std::vector<std::shared_ptr<IPackage>> m_packages;
            std::vector<VersionKeyData> m_versionKeyData;
            Source m_trackingSource;
            std::shared_ptr<IPackage> m_trackingPackage;
            std::chrono::system_clock::time_point m_trackingWriteTime = std::chrono::system_clock::time_point::min();
            std::shared_ptr<IPackage> m_availablePackageVersionOverride;
        };

        // An ICompositePackage for the CompositeSource.
        struct CompositePackage : public ICompositePackage
        {
            // The availablePackage may only contain one available package within it, as it is expected to be the output of a search on a single source.
            CompositePackage(const std::shared_ptr<ICompositePackage>& installedPackage, const std::shared_ptr<ICompositePackage>& availablePackage = {}, bool setPrimary = false)
            {
                if (installedPackage)
                {
                    m_installedPackage = std::make_shared<CompositeInstalledPackage>(installedPackage->GetInstalled());

                    // If the installed package result existed, but didn't actually create any installed versions, drop it.
                    if (m_installedPackage->IsEmpty())
                    {
                        m_installedPackage.reset();
                    }
                }

                AddAvailablePackage(availablePackage, setPrimary);
            }

            Utility::LocIndString GetProperty(PackageProperty property) const override
            {
                IPackage* truth = nullptr;
                if (m_primaryAvailablePackage)
                {
                    truth = m_primaryAvailablePackage.get();
                }
                if (!truth && !m_availablePackages.empty())
                {
                    truth = m_availablePackages[0].get();
                }
                if (!truth)
                {
                    truth = m_installedPackage.get();
                }

                THROW_HR_IF(E_UNEXPECTED, !truth);

                return truth->GetProperty(property);
            }

            std::shared_ptr<IPackage> GetInstalled() override
            {
                return m_installedPackage;
            }

            std::vector<std::shared_ptr<IPackage>> GetAvailable() override
            {
                return m_availablePackages;
            }

            const std::vector<std::shared_ptr<IPackage>>& GetAvailablePackages()
            {
                return m_availablePackages;
            }

            bool IsSameAsAnyAvailable(const IPackage* other) const
            {
                if (other)
                {
                    for (const auto& availablePackage : m_availablePackages)
                    {
                        if (other->IsSame(availablePackage.get()))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            const std::shared_ptr<CompositeInstalledPackage>& GetInstalledPackage() const
            {
                return m_installedPackage;
            }

            bool ContainsInstalledPackage(const IPackage* installedPackage) const
            {
                return m_installedPackage ? m_installedPackage->ContainsInstalledPackage(installedPackage) : false;
            }

            void AddAvailablePackage(const std::shared_ptr<ICompositePackage>& availablePackage, bool setPrimary = false)
            {
                if (availablePackage)
                {
                    m_availablePackages.emplace_back(OnlyAvailable(availablePackage));

                    if (setPrimary)
                    {
                        m_primaryAvailablePackage = m_availablePackages.back();
                    }

                    // Set override for primary or with the first available version found
                    if (setPrimary || m_availablePackages.size() == 1)
                    {
                        TrySetOverrideInstalledVersion(m_availablePackages.back());
                    }
                }
            }

            std::shared_ptr<IPackage>& GetPrimaryAvailablePackage()
            {
                return m_primaryAvailablePackage;
            }

            Source GetTrackingSource() const
            {
                return m_installedPackage ? m_installedPackage->GetTrackingSource() : Source{};
            }

            std::shared_ptr<IPackage> GetTrackingPackage() const
            {
                return m_installedPackage ? m_installedPackage->GetTrackingPackage() : std::shared_ptr<IPackage>{};
            }

            std::chrono::system_clock::time_point GetTrackingPackageWriteTime() const
            {
                return m_installedPackage ? m_installedPackage->GetTrackingPackageWriteTime() : std::chrono::system_clock::time_point::min();
            }

            void SetTracking(
                Source trackingSource,
                std::shared_ptr<IPackage> trackingPackage,
                std::chrono::system_clock::time_point trackingWriteTime)
            {
                if (m_installedPackage)
                {
                    m_installedPackage->SetTracking(std::move(trackingSource), std::move(trackingPackage), trackingWriteTime);
                }
            }

            void FoldInstalledIn(const std::shared_ptr<CompositePackage>& other)
            {
                if (other->m_installedPackage)
                {
                    if (m_installedPackage)
                    {
                        m_installedPackage->FoldInstalledIn(other->m_installedPackage);
                    }
                    else
                    {
                        m_installedPackage = other->m_installedPackage;
                    }
                }
            }

        private:
            // Try to set a version that will override the version string from the installed package
            void TrySetOverrideInstalledVersion(const std::shared_ptr<IPackage>& availablePackage)
            {
                if (m_installedPackage && availablePackage)
                {
                    m_installedPackage->SetOverrideInstalledVersion(availablePackage);
                }
            }

            std::shared_ptr<CompositeInstalledPackage> m_installedPackage;
            std::shared_ptr<IPackage> m_primaryAvailablePackage;
            std::vector<std::shared_ptr<IPackage>> m_availablePackages;
        };

        // The comparator compares the ResultMatch by MatchType first, then Field in a predefined order.
        struct ResultMatchComparator
        {
            template <typename U, typename V>
            bool operator() (
                const U& match1,
                const V& match2)
            {
                if (match1.MatchCriteria.Type != match2.MatchCriteria.Type)
                {
                    return match1.MatchCriteria.Type < match2.MatchCriteria.Type;
                }

                if (match1.MatchCriteria.Field != match2.MatchCriteria.Field)
                {
                    return match1.MatchCriteria.Field < match2.MatchCriteria.Field;
                }

                return false;
            }
        };

        template <typename T>
        void SortResultMatches(std::vector<T>& matches)
        {
            std::stable_sort(matches.begin(), matches.end(), ResultMatchComparator());
        }

        // A copy of the standard match that holds a CompositePackage instead.
        struct CompositeResultMatch
        {
            std::shared_ptr<CompositePackage> Package;
            PackageMatchFilter MatchCriteria;

            CompositeResultMatch(std::shared_ptr<CompositePackage> p, PackageMatchFilter f) : Package(std::move(p)), MatchCriteria(std::move(f)) {}
        };

        // Stores data to enable correlation between installed and available packages.
        struct CompositeResult
        {
            // A system reference string.
            struct SystemReferenceString
            {
                SystemReferenceString(PackageMatchField field, Utility::LocIndString string) :
                    Field(field), String1(Utility::FoldCase(string)) {}

                SystemReferenceString(PackageMatchField field, Utility::LocIndString string1, Utility::LocIndString string2) :
                    Field(field), String1(Utility::FoldCase(string1)), String2(Utility::FoldCase(string2)) {}

                bool operator<(const SystemReferenceString& other) const
                {
                    if (Field != other.Field)
                    {
                        return Field < other.Field;
                    }

                    if (String1 != other.String1)
                    {
                        return String1 < other.String1;
                    }

                    return String2 < other.String2;
                }

                bool operator==(const SystemReferenceString& other) const
                {
                    return Field == other.Field && String1 == other.String1 && String2 == other.String2;
                }

                void AddToFilters(
                    std::vector<PackageMatchFilter>& filters) const
                {
                    switch (Field)
                    {
                    case PackageMatchField::NormalizedNameAndPublisher:
                        filters.emplace_back(PackageMatchFilter(Field, MatchType::Exact, String1.get(), String2.get()));
                        break;

                    default:
                        filters.emplace_back(PackageMatchFilter(Field, MatchType::Exact, String1.get()));
                    }
                }

            private:
                PackageMatchField Field;
                Utility::LocIndString String1;
                Utility::LocIndString String2;
            };

            // Data relevant to correlation for a package.
            struct PackageData
            {
                std::set<SystemReferenceString> SystemReferenceStrings;

                void AddIfNotPresent(SystemReferenceString&& srs)
                {
                    if (SystemReferenceStrings.find(srs) == SystemReferenceStrings.end())
                    {
                        SystemReferenceStrings.emplace(std::move(srs));
                    }
                }

                SearchRequest CreateInclusionsSearchRequest(SearchPurpose searchPurpose) const
                {
                    SearchRequest result;
                    for (const auto& srs : SystemReferenceStrings)
                    {
                        srs.AddToFilters(result.Inclusions);
                    }
                    result.Purpose = searchPurpose;
                    return result;
                }

                std::shared_ptr<IPackage> AddSystemReferenceStringsFromTrackingPackage(const PackageTrackingCatalog& trackingCatalog, const Utility::LocIndString& identifier, std::string_view sourceIdentifier)
                {
                    SearchRequest trackingRequest;
                    trackingRequest.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, identifier.get());

                    SearchResult trackingResult = trackingCatalog.Search(trackingRequest);

                    if (trackingResult.Matches.size() == 1)
                    {
                        std::shared_ptr<IPackage> result = OnlyAvailable(trackingResult.Matches[0].Package);
                        AddSystemReferenceStrings(result.get());
                        return result;
                    }
                    else
                    {
                        AICLI_LOG(Repo, Warning, << "Found multiple results for Id [" << identifier << "] in tracking catalog for: " << sourceIdentifier);
                        return {};
                    }
                }

                void AddSystemReferenceStrings(IPackage* package)
                {
                    for (auto const& versionKey : package->GetVersionKeys())
                    {
                        auto version = package->GetVersion(versionKey);
                        AddSystemReferenceStrings(version.get());
                    }
                }

                void AddSystemReferenceStrings(IPackageVersion* version)
                {
                    GetSystemReferenceStrings(
                        version,
                        PackageVersionMultiProperty::PackageFamilyName,
                        PackageMatchField::PackageFamilyName);

                    GetSystemReferenceStrings(
                        version,
                        PackageVersionMultiProperty::ProductCode,
                        PackageMatchField::ProductCode);

                    GetSystemReferenceStrings(
                        version,
                        PackageVersionMultiProperty::UpgradeCode,
                        PackageMatchField::UpgradeCode);

                    GetNameAndPublisher(
                        version);
                }

                void AddSystemReferenceStringsFromManifest(const Manifest::Manifest& manifest)
                {
                    for (const auto& pfn : manifest.GetPackageFamilyNames())
                    {
                        AddIfNotPresent(SystemReferenceString{ PackageMatchField::PackageFamilyName, Utility::LocIndString{ pfn } });
                    }
                    for (const auto& productCode : manifest.GetProductCodes())
                    {
                        AddIfNotPresent(SystemReferenceString{ PackageMatchField::ProductCode, Utility::LocIndString{ productCode } });
                    }
                    for (const auto& upgradeCode : manifest.GetUpgradeCodes())
                    {
                        AddIfNotPresent(SystemReferenceString{ PackageMatchField::UpgradeCode, Utility::LocIndString{ upgradeCode } });
                    }
                    for (const auto& name : manifest.GetPackageNames())
                    {
                        for (const auto& publisher : manifest.GetPublishers())
                        {
                            AddIfNotPresent(SystemReferenceString{
                                PackageMatchField::NormalizedNameAndPublisher,
                                Utility::LocIndString{ name },
                                Utility::LocIndString{ publisher } });
                        }
                    }
                }

            private:
                void GetSystemReferenceStrings(
                    IPackageVersion* installedVersion,
                    PackageVersionMultiProperty prop,
                    PackageMatchField field)
                {
                    for (auto&& string : installedVersion->GetMultiProperty(prop))
                    {
                        AddIfNotPresent(SystemReferenceString{ field, std::move(string) });
                    }
                }

                void GetNameAndPublisher(
                    IPackageVersion* installedVersion)
                {
                    // Unfortunately the names and publishers are unique and not tied to each other strictly, so we need
                    // to go broad on the matches. Future work can hopefully make name and publisher operate more as a unit,
                    // but for now we have to search for the cartesian of these...
                    auto names = installedVersion->GetMultiProperty(PackageVersionMultiProperty::Name);
                    auto publishers = installedVersion->GetMultiProperty(PackageVersionMultiProperty::Publisher);

                    for (size_t i = 0; i < names.size(); ++i)
                    {
                        for (size_t j = 0; j < publishers.size(); ++j)
                        {
                            AddIfNotPresent(SystemReferenceString{
                                PackageMatchField::NormalizedNameAndPublisher,
                                names[i],
                                publishers[j] });
                        }
                    }
                }
            };

            // For a given package, prepares the results for it.
            PackageData GetSystemReferenceStrings(IPackage* package)
            {
                PackageData result;
                result.AddSystemReferenceStrings(package);
                return result;
            }

            // Check for a package already in the result that should have been correlated already.
            // If we find one, see if we should upgrade it's match criteria.
            // If we don't, return package data for further use.
            //     downloadManifests: when creating system reference strings, also download manifests to get more data.
            std::optional<PackageData> CheckForExistingResultFromAvailablePackageMatch(const ResultMatch& availableMatch, bool downloadManifests)
            {
                std::shared_ptr<IPackage> availablePackage = OnlyAvailable(availableMatch.Package);

                for (auto& match : Matches)
                {
                    if (match.Package->IsSameAsAnyAvailable(availablePackage.get()))
                    {
                        if (ResultMatchComparator{}(availableMatch, match))
                        {
                            match.MatchCriteria = availableMatch.MatchCriteria;
                        }

                        return {};
                    }
                }

                PackageData result;
                constexpr int c_downloadManifestsLimit = 3;
                int manifestsDownloaded = 0;
                for (auto const& versionKey : availablePackage->GetVersionKeys())
                {
                    auto packageVersion = availablePackage->GetVersion(versionKey);
                    result.AddSystemReferenceStrings(packageVersion.get());

                    if (downloadManifests && manifestsDownloaded < c_downloadManifestsLimit)
                    {
                        auto manifest = packageVersion->GetManifest();
                        result.AddSystemReferenceStringsFromManifest(manifest);
                        manifestsDownloaded++;
                    }
                }
                return result;
            }

            // Determines if the results contain the given installed package.
            bool ContainsInstalledPackage(const IPackage* installedPackage) const 
            {
                for (auto& match : Matches)
                {
                    if (match.Package->ContainsInstalledPackage(installedPackage))
                    {
                        return true;
                    }
                }

                return false;
            }

            // Determines if the results contain the given installed package.
            std::shared_ptr<CompositePackage> FindInstalledPackage(const IPackage* installedPackage) const
            {
                for (auto& match : Matches)
                {
                    if (match.Package->ContainsInstalledPackage(installedPackage))
                    {
                        return match.Package;
                    }
                }

                return {};
            }

            // *Destructively* converts the result to the standard variant.
            SearchResult ConvertToSearchResult()
            {
                FoldResults();

                SearchResult result;

                result.Matches.reserve(Matches.size());
                for (auto& match : Matches)
                {
                    result.Matches.emplace_back(std::move(match.Package), std::move(match.MatchCriteria));
                }

                result.Truncated = Truncated;

                result.Failures = std::move(Failures);

                return result;
            }

            bool AddFailureIfSourceNotPresent(SearchResult::Failure&& failure)
            {
                auto itr = std::find_if(Failures.begin(), Failures.end(),
                    [&failure](const SearchResult::Failure& present) {
                        return present.SourceName == failure.SourceName;
                    });

                if (itr == Failures.end())
                {
                    Failures.emplace_back(std::move(failure));
                    return true;
                }

                return false;
            }

            SearchResult SearchAndHandleFailures(const Source& source, const SearchRequest& request)
            {
                SearchResult result;

                try
                {
                    result = source.Search(request);
                }
                catch (...)
                {
                    if (AddFailureIfSourceNotPresent({ source.GetDetails().Name, std::current_exception() }))
                    {
                        LOG_CAUGHT_EXCEPTION();
                        AICLI_LOG(Repo, Warning, << "Failed to search source for correlation: " << source.GetDetails().Name);
                    }
                }

                // Move failures into the result
                for (SearchResult::Failure& failure : result.Failures)
                {
                    AddFailureIfSourceNotPresent(std::move(failure));
                }

                return result;
            }

            // Group results in an attempt to have a single result that covers all installed versions.
            // This is expected to be called immediately after the installed search portion,
            // when each result will contain a single installed version and some number of available packages.
            // 
            // The folds that happen are:
            //  1. When results have the same primary available package (the primary available package is set due to tracking data)
            //  2. When a result has no primary available package, but another result does have a primary that matches one of the available
            //      a. Choose the latest primary if there are multiple
            //  3. When multiple results have no primary available package and share the same available package set
            //      a. There are many potential additional rules that could be made here, but we will start with the simplest version.
            //
            // Potential improvements:
            //  1. Attempting correlation of non-primary available packages to allow folding in more complex cases
            //      a. For example, if installed A has {source1:package1, source2:package2} and installed B has {source1:package1}, can we
            //          make sure that source1:package1 and source2:package2 are in fact "the same" to confidently say that installed A and B
            //          are side by side versions.
            //  2. Attempt correlation by installed data only
            //      a. We can potentially detect multiple instances of the same installed item with the same correlation logic turned back on
            //          the installed source.  This would allow for folding even when the package is not in any available source.
            void FoldResults()
            {
                // The key to uniquely identify the package in the map
                struct InstalledResultFoldKey
                {
                    InstalledResultFoldKey() = default;

                    InstalledResultFoldKey(const std::shared_ptr<IPackage>& package)
                    {
                        std::shared_ptr<IPackageVersion> latestAvailable = package->GetLatestVersion();
                        if (latestAvailable)
                        {
                            SourceIdentifier = latestAvailable->GetSource().GetIdentifier();
                            PackageIdentifier = latestAvailable->GetProperty(PackageVersionProperty::Id);
                        }
                    }

                    // Hash operation
                    size_t operator()(const InstalledResultFoldKey& value) const noexcept
                    {
                        std::hash<std::string> hashString;
                        return hashString(value.SourceIdentifier) ^ (hashString(value.PackageIdentifier) << 1);
                    }

                    bool operator==(const InstalledResultFoldKey& other) const noexcept
                    {
                        // Treat both empty as invalid and never equal
                        if (SourceIdentifier.empty() && PackageIdentifier.empty())
                        {
                            return false;
                        }

                        return SourceIdentifier == other.SourceIdentifier && PackageIdentifier == other.PackageIdentifier;
                    }

                    std::string SourceIdentifier;
                    std::string PackageIdentifier;
                };

                // The data for a package in the map
                struct InstalledResultFoldData
                {
                    InstalledResultFoldData() = default;
                    explicit InstalledResultFoldData(size_t primaryPackageIndex) : PrimaryPackageIndex(primaryPackageIndex) {}

                    std::optional<size_t> PrimaryPackageIndex;
                    std::vector<size_t> NonPrimaryPackageIndices;
                };

                std::unordered_map<InstalledResultFoldKey, InstalledResultFoldData, InstalledResultFoldKey> foldData;

                // Attempt to fold all primary package matches first.
                // Packages without primaries will still be indexed into the hash table.
                for (size_t i = 0; i < Matches.size(); ++i)
                {
                    CompositeResultMatch& currentMatch = Matches[i];

                    // Check current match for fold target
                    if (currentMatch.Package->GetPrimaryAvailablePackage())
                    {
                        InstalledResultFoldKey key{ currentMatch.Package->GetPrimaryAvailablePackage() };

                        auto itr = foldData.find(key);
                        if (itr != foldData.end())
                        {
                            if (itr->second.PrimaryPackageIndex)
                            {
                                Matches[itr->second.PrimaryPackageIndex.value()].Package->FoldInstalledIn(currentMatch.Package);
                                currentMatch.Package.reset();
                            }
                            else
                            {
                                itr->second.PrimaryPackageIndex = i;
                            }
                        }
                        else
                        {
                            foldData[key] = InstalledResultFoldData{ i };
                        }
                    }
                    else
                    {
                        for (const auto& availablePackage : currentMatch.Package->GetAvailablePackages())
                        {
                            InstalledResultFoldKey key{ availablePackage };

                            auto itr = foldData.find(key);
                            if (itr == foldData.end())
                            {
                                itr = foldData.insert({ key, {} }).first;
                            }

                            itr->second.NonPrimaryPackageIndices.emplace_back(i);
                        }
                    }
                }

                // After primary matches are folded, attempt to fold results without primary matches.
                // The latest primary match will be preferred.
                for (size_t i = 0; i < Matches.size(); ++i)
                {
                    CompositeResultMatch& currentMatch = Matches[i];

                    // Skip any matches that we have already folded
                    if (!currentMatch.Package)
                    {
                        continue;
                    }

                    if (!currentMatch.Package->GetPrimaryAvailablePackage())
                    {
                        InstalledResultFoldData* latestPrimaryAvailable = nullptr;
                        std::vector<InstalledResultFoldData*> availableFoldData;

                        for (const auto& availablePackage : currentMatch.Package->GetAvailablePackages())
                        {
                            auto& packageFoldData = foldData.at(availablePackage);

                            if (packageFoldData.PrimaryPackageIndex)
                            {
                                if (!latestPrimaryAvailable ||
                                    Matches[latestPrimaryAvailable->PrimaryPackageIndex.value()].Package->GetTrackingPackageWriteTime() < Matches[packageFoldData.PrimaryPackageIndex.value()].Package->GetTrackingPackageWriteTime())
                                {
                                    latestPrimaryAvailable = &packageFoldData;
                                }
                            }
                            else
                            {
                                availableFoldData.emplace_back(&packageFoldData);
                            }
                        }

                        if (latestPrimaryAvailable)
                        {
                            Matches[latestPrimaryAvailable->PrimaryPackageIndex.value()].Package->FoldInstalledIn(currentMatch.Package);
                            currentMatch.Package.reset();

                            // If the result with the primary is later, move it forward
                            if (latestPrimaryAvailable->PrimaryPackageIndex.value() > i)
                            {
                                currentMatch.Package = std::move(Matches[latestPrimaryAvailable->PrimaryPackageIndex.value()].Package);
                                Matches[latestPrimaryAvailable->PrimaryPackageIndex.value()].Package.reset();
                                latestPrimaryAvailable->PrimaryPackageIndex = i;
                            }
                            continue;
                        }

                        // First, find the intersection of all results that contain all of the packages from this result.
                        std::vector<size_t> candidateMatches;
                        for (size_t j = 0; j < availableFoldData.size(); ++j)
                        {
                            InstalledResultFoldData* packageFoldData = availableFoldData[j];

                            if (j == 0)
                            {
                                candidateMatches = packageFoldData->NonPrimaryPackageIndices;
                            }
                            else
                            {
                                std::vector<size_t> temp;
                                std::set_intersection(
                                    candidateMatches.begin(), candidateMatches.end(),
                                    packageFoldData->NonPrimaryPackageIndices.begin(), packageFoldData->NonPrimaryPackageIndices.end(),
                                    std::back_inserter(temp));
                                candidateMatches = std::move(temp);
                            }
                        }

                        // Now exclude both our own result and any that have a different (larger) number of available packages
                        candidateMatches.erase(std::remove_if(candidateMatches.begin(), candidateMatches.end(),
                            [&](size_t index) { return index == i || Matches[index].Package->GetAvailablePackages().size() != currentMatch.Package->GetAvailablePackages().size(); }),
                            candidateMatches.end());

                        // All of these remaining values should be folded in to our result
                        for (size_t foldTarget : candidateMatches)
                        {
                            currentMatch.Package->FoldInstalledIn(Matches[foldTarget].Package);
                            Matches[foldTarget].Package.reset();
                        }
                    }
                }

                // Get rid of the folded results; we reset the Package to indicate that it is no longer valid
                Matches.erase(std::remove_if(Matches.begin(), Matches.end(), [&](const CompositeResultMatch& match) { return !match.Package; }), Matches.end());
            }

            std::vector<CompositeResultMatch> Matches;
            bool Truncated = false;
            std::vector<SearchResult::Failure> Failures;
        };

        std::shared_ptr<ICompositePackage> GetTrackedPackageFromAvailableSource(CompositeResult& result, const Source& source, const Utility::LocIndString& identifier)
        {
            SearchRequest directRequest;
            directRequest.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, identifier.get());

            SearchResult directResult = result.SearchAndHandleFailures(source, directRequest);

            if (directResult.Matches.empty())
            {
                AICLI_LOG(Repo, Warning, << "Did not find Id [" << identifier << "] in tracked source: " << source.GetDetails().Name);
            }
            else if (directResult.Matches.size() == 1)
            {
                return directResult.Matches[0].Package;
            }
            else
            {
                AICLI_LOG(Repo, Warning, << "Found multiple results for Id [" << identifier << "] in tracked source: " << source.GetDetails().Name);
            }

            return {};
        }
    }

    using namespace anon;

    CompositeSource::CompositeSource(std::string identifier)
    {
        m_details.Identifier = std::move(identifier);
    }

    const SourceDetails& CompositeSource::GetDetails() const
    {
        return m_details;
    }

    const std::string& CompositeSource::GetIdentifier() const
    {
        return m_details.Identifier;
    }

    // The composite search needs to take several steps to get results, and due to the
    // potential for different information spread across multiple sources, base searches
    // need to be performed in both installed and available.
    //
    // If an installed source is present, then the searches should only return packages
    // that are installed. This means that the base searches against available sources
    // will only return results where a match is found in the installed source.
    SearchResult CompositeSource::Search(const SearchRequest& request) const
    {
        if (m_installedSource)
        {
            return SearchInstalled(request);
        }
        else
        {
            return SearchAvailable(request);
        }
    }

    void* CompositeSource::CastTo(ISourceType type)
    {
        if (type == SourceType)
        {
            return this;
        }

        return nullptr;
    }

    void CompositeSource::AddAvailableSource(const Source& source)
    {
        m_availableSources.emplace_back(source);
    }

    void CompositeSource::SetInstalledSource(Source source, CompositeSearchBehavior searchBehavior)
    {
        m_installedSource = std::move(source);
        m_searchBehavior = searchBehavior;
    }

    // An installed search first finds all installed packages that match the request, then correlates with available sources.
    // Next the search is performed against the available sources and correlated with the installed source. A result will only
    // be added if there exists an installed package that was not found by the initial search.
    // This allows for search terms to find installed packages by their available metadata, as well as the local values.
    //
    // Search flow:
    //  Installed :: Search incoming request
    //  For each result
    //      For each available source
    //          Tracking :: Search system references
    //      If tracking found
    //          Available :: Search tracking ID
    //      If no available, for each available source
    //          Available :: Search system references
    // 
    //  For each available source
    //      Tracking :: Search incoming request
    //      For each result
    //          Installed :: Search system references
    //          If found
    //              Available :: Search tracking ID
    //      Available :: Search incoming request
    //      For each result
    //          Installed :: Search system references
    SearchResult CompositeSource::SearchInstalled(const SearchRequest& request) const
    {
        CompositeResult result;

        // If the search behavior is for AllPackages or Installed then the result can contain packages that are
        // only in the Installed source, but do not have an AvailableVersion.
        if (m_searchBehavior == CompositeSearchBehavior::AllPackages || m_searchBehavior == CompositeSearchBehavior::Installed)
        {
            // Search installed source (allow exceptions out as we own the installed source)
            SearchResult installedResult = m_installedSource.Search(request);
            result.Truncated = installedResult.Truncated;

            for (auto&& match : installedResult.Matches)
            {
                if (!match.Package)
                {
                    // Ensure that the crash from installedVersion below is not from the actual package being null.
                    AICLI_LOG(Repo, Warning, << "CompositeSource: The match of the package (matched on " <<
                        ToString(match.MatchCriteria.Field) << " => '" << match.MatchCriteria.Value <<
                        "') was null and is being dropped from the results.");
                    continue;
                }

                std::shared_ptr<CompositePackage> compositePackage = std::make_shared<CompositePackage>(match.Package);
                auto installedPackage = compositePackage->GetInstalled();

                if (!installedPackage)
                {
                    // One would think that the installed package coming directly from our own installed source
                    // would never be null, but it is sometimes. Rather than making users suffer through crashes
                    // that break their entire experience, lets log a few things and then ignore this match.
                    AICLI_LOG(Repo, Warning, << "CompositeSource: The installed version of the package '" <<
                        match.Package->GetProperty(PackageProperty::Id) << "' was null and is being dropped from the results.");
                    continue;
                }

                auto installedPackageData = result.GetSystemReferenceStrings(installedPackage.get());

                // Create a search request to run against all available sources
                if (!installedPackageData.SystemReferenceStrings.empty())
                {
                    SearchRequest systemReferenceSearch = installedPackageData.CreateInclusionsSearchRequest(SearchPurpose::CorrelationToAvailable);
                    AICLI_LOG(Repo, Verbose, << "Finding available package from installed package using system reference search: " << systemReferenceSearch.ToString());

                    // Search sources and add to result
                    for (const auto& source : m_availableSources)
                    {
                        AICLI_LOG(Repo, Verbose, << " ... searching source: " << source.GetDetails().Name << " [" << source.GetIdentifier() << ']');

                        // Find the tracking result with the latest timestamp.
                        auto trackingCatalog = source.GetTrackingCatalog();
                        SearchResult trackingResult = trackingCatalog.Search(systemReferenceSearch);

                        std::shared_ptr<IPackage> trackingPackage;
                        std::chrono::system_clock::time_point trackingPackageTime;
                        bool trackingSet = false;

                        for (const auto& trackingMatch : trackingResult.Matches)
                        {
                            auto candidateTime = GetLatestTrackingWriteTime(OnlyAvailable(trackingMatch.Package));

                            if (!trackingPackage || candidateTime > trackingPackageTime)
                            {
                                trackingPackage = OnlyAvailable(trackingMatch.Package);
                                trackingPackageTime = candidateTime;
                            }
                        }

                        if (trackingPackage && trackingPackageTime > compositePackage->GetTrackingPackageWriteTime())
                        {
                            AICLI_LOG(Repo, Verbose, << " ... setting latest tracking package to: " << trackingPackage->GetProperty(PackageProperty::Id));
                            compositePackage->SetTracking(source, trackingPackage, trackingPackageTime);
                            trackingSet = true;
                        }

                        // Attempt to correlate local packages against this source if supported.
                        SearchResult availableResult;
                        if (source.GetDetails().SupportInstalledSearchCorrelation)
                        {
                            availableResult = result.SearchAndHandleFailures(source, systemReferenceSearch);
                        }

                        auto availablePackage = GetMatchingPackage(availableResult.Matches,
                            [&]() {
                                AICLI_LOG(Repo, Info,
                                << "Found multiple matches for installed package [" << installedPackage->GetProperty(PackageProperty::Id) <<
                                "] in source [" << source.GetIdentifier() << "] when searching for [" << systemReferenceSearch.ToString() << "]");
                            }, [&] {
                                AICLI_LOG(Repo, Warning, << "  Appropriate available package could not be determined");
                            });

                        if (trackingPackage)
                        {
                            auto trackingIdentifier = trackingPackage->GetProperty(PackageProperty::Id);

                            // We always want to take the available search result if it exists as the package may have been updated.
                            if (availablePackage)
                            {
                                auto availableIdentifier = availablePackage->GetProperty(PackageProperty::Id);
                                if (!Utility::ICUCaseInsensitiveEquals(availableIdentifier, trackingIdentifier))
                                {
                                    AICLI_LOG(Repo, Verbose, << " ... overriding tracking package (" << trackingIdentifier << ") with available package (" << availableIdentifier << ")");
                                }
                            }
                            else
                            {
                                AICLI_LOG(Repo, Verbose, << " ... using tracking package: " << trackingIdentifier);
                                availablePackage = GetTrackedPackageFromAvailableSource(result, source, trackingIdentifier);
                            }
                        }

                        if (availablePackage)
                        {
                            AICLI_LOG(Repo, Verbose, << " ... adding available package: " << availablePackage->GetProperty(PackageProperty::Id));
                            compositePackage->AddAvailablePackage(availablePackage, trackingSet);
                        }
                    }
                }

                // Move the installed result into the composite result
                result.Matches.emplace_back(std::move(compositePackage), std::move(match.MatchCriteria));
            }

            // Optimization for the "everything installed" case, no need to allow for reverse correlations
            if (request.IsForEverything() && m_searchBehavior == CompositeSearchBehavior::Installed)
            {
                return result.ConvertToSearchResult();
            }
        }

        // Search available sources
        for (const auto& source : m_availableSources)
        {
            auto trackingCatalog = source.GetTrackingCatalog();

            SearchResult availableResult = result.SearchAndHandleFailures(source, request);
            bool downloadManifests = source.QueryFeatureFlag(SourceFeatureFlag::ManifestMayContainAdditionalSystemReferenceStrings);

            for (auto&& match : availableResult.Matches)
            {
                // Check for the package already in the result.
                // In cases that PackageData will be created, also download manifests for system reference strings
                // when search result is small (currently limiting to 1).
                auto packageData = result.CheckForExistingResultFromAvailablePackageMatch(match, downloadManifests && availableResult.Matches.size() == 1);

                // If found existing package in the result, continue
                if (!packageData)
                {
                    continue;
                }

                // Use data from the tracking catalog as it can potentially get better correlations
                auto trackingPackage = packageData->AddSystemReferenceStringsFromTrackingPackage(trackingCatalog, match.Package->GetProperty(PackageProperty::Id), source.GetDetails().Name);

                // If no package was found that was already in the results, do a correlation lookup with the installed
                // source to create a new composite package entry if we find any packages there.
                bool foundInstalledMatch = false;
                if (!packageData->SystemReferenceStrings.empty())
                {
                    // Create a search request to run against the installed source
                    SearchRequest systemReferenceSearch = packageData->CreateInclusionsSearchRequest(SearchPurpose::CorrelationToInstalled);

                    AICLI_LOG(Repo, Verbose, << "Finding installed package from available package using system reference search: " << systemReferenceSearch.ToString());
                    // Correlate against installed (allow exceptions out as we own the installed source)
                    SearchResult installedCrossRef = m_installedSource.Search(systemReferenceSearch);

                    for (const auto& installedMatch : installedCrossRef.Matches)
                    {
                        if (!IsStrongMatchField(installedMatch.MatchCriteria.Field))
                        {
                            // For weak correlations, do an installed -> available check to ensure that there are no other strong correlations.
                            SearchResult correlationConfirmation;
                            if (source.GetDetails().SupportInstalledSearchCorrelation)
                            {
                                correlationConfirmation = result.SearchAndHandleFailures(source, result.GetSystemReferenceStrings(installedMatch.Package->GetInstalled().get()).CreateInclusionsSearchRequest(SearchPurpose::CorrelationToAvailable));
                            }

                            if (correlationConfirmation.Matches.empty())
                            {
                                // We probably made the correlation due to tracking data, keep it.
                            }
                            else if (correlationConfirmation.Matches.size() > 1)
                            {
                                // There is contention for the correlation.
                                AICLI_LOG(Repo, Verbose, << " ... installed package [" << installedMatch.Package->GetProperty(PackageProperty::Id) <<
                                    "] had multiple correlations and is being ignored as a match for [" << match.Package->GetProperty(PackageProperty::Id) << "]");
                                continue;
                            }
                            else if (!OnlyAvailable(correlationConfirmation.Matches[0].Package)->IsSame(OnlyAvailable(match.Package).get()))
                            {
                                // The only correlation is not to the current package.
                                AICLI_LOG(Repo, Verbose, << " ... installed package [" << installedMatch.Package->GetProperty(PackageProperty::Id) <<
                                    "] was found through available package [" << match.Package->GetProperty(PackageProperty::Id) << "], but only correlated to [" <<
                                    correlationConfirmation.Matches[0].Package->GetProperty(PackageProperty::Id) << "] and is being ignored");
                                continue;
                            }
                        }

                        // Now that we know we need to add this available package, determine how exactly
                        std::shared_ptr<CompositePackage> resultPackage = result.FindInstalledPackage(installedMatch.Package->GetInstalled().get());

                        if (resultPackage)
                        {
                            // Check for a package from the same source already present on the result package.
                            bool foundSameSource = false;

                            for (const auto& availablePackage : resultPackage->GetAvailablePackages())
                            {
                                if (availablePackage->GetSource() == source)
                                {
                                    // TODO: May need to add more data so that we can choose the proper correlation, but it may also be very difficult to get through
                                    //       the gauntlet of other checks and arrive in this situation.
                                    AICLI_LOG(Repo, Verbose, << " ... found [" << availablePackage->GetProperty(PackageProperty::Id) <<
                                        "] already correlated to [" << installedMatch.Package->GetProperty(PackageProperty::Id) << "] from the same source [" <<
                                        source.GetDetails().Name << "] as [" << match.Package->GetProperty(PackageProperty::Id) << "]; ignoring the second correlation");
                                    foundSameSource = true;
                                }
                            }

                            if (foundSameSource)
                            {
                                continue;
                            }
                        }
                        else
                        {
                            result.Matches.emplace_back(std::make_shared<CompositePackage>(installedMatch.Package), match.MatchCriteria);
                            resultPackage = result.Matches.back().Package;
                        }

                        bool setPrimary = false;
                        if (trackingPackage)
                        {
                            auto trackingPackageTime = GetLatestTrackingWriteTime(trackingPackage);

                            if (trackingPackageTime > resultPackage->GetTrackingPackageWriteTime())
                            {
                                resultPackage->SetTracking(source, std::move(trackingPackage), trackingPackageTime);
                                setPrimary = true;
                            }
                        }

                        resultPackage->AddAvailablePackage(std::move(match.Package), setPrimary);

                        foundInstalledMatch = true;
                    }
                }

                // If there was no correlation for this package, add it without one.
                if ((m_searchBehavior == CompositeSearchBehavior::AllPackages || m_searchBehavior == CompositeSearchBehavior::AvailablePackages) && !foundInstalledMatch)
                {
                    result.Matches.emplace_back(std::make_shared<CompositePackage>(std::shared_ptr<ICompositePackage>{}, std::move(match.Package)), match.MatchCriteria);
                }
            }
        }

        SortResultMatches(result.Matches);

        if (request.MaximumResults > 0 && result.Matches.size() > request.MaximumResults)
        {
            result.Truncated = true;
            result.Matches.erase(result.Matches.begin() + request.MaximumResults, result.Matches.end());
        }

        return result.ConvertToSearchResult();
    }

    // An available search goes through each source, searching individually and then sorting the full result set.
    SearchResult CompositeSource::SearchAvailable(const SearchRequest& request) const
    {
        SearchResult result;

        // Search available sources
        for (const auto& source : m_availableSources)
        {
            SearchResult oneSourceResult;

            try
            {
                oneSourceResult = source.Search(request);
            }
            catch (...)
            {
                LOG_CAUGHT_EXCEPTION();
                AICLI_LOG(Repo, Warning, << "Failed to search source: " << source.GetDetails().Name);
                result.Failures.emplace_back(SearchResult::Failure{ source.GetDetails().Name, std::current_exception() });
            }

            // Move into the single result
            std::move(oneSourceResult.Matches.begin(), oneSourceResult.Matches.end(), std::back_inserter(result.Matches));
            std::move(oneSourceResult.Failures.begin(), oneSourceResult.Failures.end(), std::back_inserter(result.Failures));
        }

        SortResultMatches(result.Matches);

        if (request.MaximumResults > 0 && result.Matches.size() > request.MaximumResults)
        {
            result.Truncated = true;
            result.Matches.erase(result.Matches.begin() + request.MaximumResults, result.Matches.end());
        }

        return result;
    }
}
