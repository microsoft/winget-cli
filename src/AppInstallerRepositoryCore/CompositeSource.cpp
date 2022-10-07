// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CompositeSource.h"

namespace AppInstaller::Repository
{
    using namespace std::string_view_literals;

    namespace
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

        // Move returns if there is only one package in the matches that is strong; otherwise returns an empty value.
        std::shared_ptr<IPackage> FindOnlyStrongMatchFieldResult(std::vector<ResultMatch>& matches)
        {
            std::shared_ptr<IPackage> result;

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
        std::shared_ptr<IPackage> GetMatchingPackage(std::vector<ResultMatch>& matches, MultipleIntro&& multipleIntro, Indeterminate&& indeterminate)
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

        // For a given package from a tracking catalog, get the latest write time and its corresponding package version.
        // Look at all versions rather than just the latest to account for the potential of downgrading.
        std::pair<std::chrono::system_clock::time_point, std::shared_ptr<IPackageVersion>> GetLatestTrackingWriteTimeAndPackageVersion(
            const std::shared_ptr<IPackage>& trackingPackage)
        {
            std::chrono::system_clock::time_point resultTime{};
            std::shared_ptr<IPackageVersion> resultVersion;

            for (const auto& key : trackingPackage->GetAvailableVersionKeys())
            {
                auto version = trackingPackage->GetAvailableVersion(key);
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

                        if (versionTime > resultTime)
                        {
                            resultTime = versionTime;
                            resultVersion = version;
                        }
                    }
                }
            }

            return { resultTime, std::move(resultVersion) };
        }

        // TODO: Note: Currently this function assumes the all versions in the available package is from one source.
        // If one day we start adding support for available package from multiple sources, this function needs to be revisited.
        std::string GetMappedInstalledVersion(const std::string& installedVersion, const std::shared_ptr<IPackage>& availablePackage)
        {
            // Stores raw versions value strings to run a preliminary check whether version mapping is needed.
            std::vector<std::tuple<std::string, std::string, std::string>> rawVersionValues;
            auto versionKeys = availablePackage->GetAvailableVersionKeys();
            bool shouldTryPerformMapping = false;

            for (auto const& versionKey : versionKeys)
            {
                auto availableVersion = availablePackage->GetAvailableVersion(versionKey);
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

        // A composite package for the CompositeSource.
        struct CompositePackage : public IPackage
        {
            CompositePackage(std::shared_ptr<IPackage> installedPackage, std::shared_ptr<IPackage> availablePackage = {}) :
                m_installedPackage(std::move(installedPackage)), m_availablePackage(std::move(availablePackage))
            {
                // Grab the installed version's channel to allow for filtering in calls to get available info.
                if (m_installedPackage)
                {
                    auto installedVersion = m_installedPackage->GetInstalledVersion();
                    if (installedVersion)
                    {
                        m_installedChannel = installedVersion->GetProperty(PackageVersionProperty::Channel);
                    }
                }

                TrySetOverrideInstalledVersion();
            }

            Utility::LocIndString GetProperty(PackageProperty property) const override
            {
                std::shared_ptr<IPackageVersion> truth = GetLatestAvailableVersion();
                if (!truth)
                {
                    truth = m_trackingPackageVersion;
                }
                if (!truth)
                {
                    truth = GetInstalledVersion();
                }

                switch (property)
                {
                case PackageProperty::Id:
                    return truth->GetProperty(PackageVersionProperty::Id);
                case PackageProperty::Name:
                    return truth->GetProperty(PackageVersionProperty::Name);
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }

            std::shared_ptr<IPackageVersion> GetInstalledVersion() const override
            {
                if (m_installedPackage)
                {
                    auto installedVersion = m_installedPackage->GetInstalledVersion();
                    if (installedVersion)
                    {
                        return std::make_shared<CompositeInstalledVersion>(std::move(installedVersion), m_trackingSource, m_trackingPackageVersion, m_overrideInstalledVersion);
                    }
                }

                return {};
            }

            std::vector<PackageVersionKey> GetAvailableVersionKeys() const override
            {
                if (m_availablePackage)
                {
                    std::vector<PackageVersionKey> result = m_availablePackage->GetAvailableVersionKeys();
                    std::string_view channel = m_installedChannel;

                    // Remove all elements whose channel does not match the installed package.
                    result.erase(
                        std::remove_if(result.begin(), result.end(), [&](const PackageVersionKey& pvk) { return !Utility::ICUCaseInsensitiveEquals(pvk.Channel, channel); }),
                        result.end());

                    return result;
                }

                return {};
            }

            std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
            {
                return GetAvailableVersion({ "", "", m_installedChannel.get() });
            }

            std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const override
            {
                if (m_availablePackage)
                {
                    return m_availablePackage->GetAvailableVersion(versionKey);
                }

                return {};
            }

            bool IsUpdateAvailable() const override
            {
                auto installed = GetInstalledVersion();

                if (!installed)
                {
                    return false;
                }

                auto latest = GetLatestAvailableVersion();

                return (latest && (GetVACFromVersion(installed.get()).IsUpdatedBy(GetVACFromVersion(latest.get()))));
            }

            bool IsSame(const IPackage* other) const override
            {
                const CompositePackage* otherComposite = dynamic_cast<const CompositePackage*>(other);

                if (!otherComposite ||
                    static_cast<bool>(m_installedPackage) != static_cast<bool>(otherComposite->m_installedPackage) ||
                    (m_installedPackage && !m_installedPackage->IsSame(otherComposite->m_installedPackage.get())) ||
                    static_cast<bool>(m_availablePackage) != static_cast<bool>(otherComposite->m_availablePackage) ||
                    (m_availablePackage && !m_availablePackage->IsSame(otherComposite->m_availablePackage.get())))
                {
                    return false;
                }

                return true;
            }

            const std::shared_ptr<IPackage>& GetInstalledPackage() const
            {
                return m_installedPackage;
            }

            const std::shared_ptr<IPackage>& GetAvailablePackage() const
            {
                return m_availablePackage;
            }

            const std::shared_ptr<IPackage>& GetTrackingPackage() const
            {
                return m_trackingPackage;
            }

            void SetAvailablePackage(std::shared_ptr<IPackage> availablePackage)
            {
                m_availablePackage = std::move(availablePackage);
                TrySetOverrideInstalledVersion();
            }

            void SetTracking(Source trackingSource, std::shared_ptr<IPackage> trackingPackage, std::shared_ptr<IPackageVersion> trackingPackageVersion)
            {
                m_trackingSource = std::move(trackingSource);
                m_trackingPackage = std::move(trackingPackage);
                m_trackingPackageVersion = std::move(trackingPackageVersion);
            }

        private:
            void TrySetOverrideInstalledVersion()
            {
                if (m_installedPackage && m_availablePackage)
                {
                    auto installedVersion = m_installedPackage->GetInstalledVersion();
                    if (installedVersion)
                    {
                        auto installedType = Manifest::ConvertToInstallerTypeEnum(installedVersion->GetMetadata()[PackageVersionMetadata::InstalledType]);
                        if (Manifest::DoesInstallerTypeSupportArpVersionRange(installedType))
                        {
                            m_overrideInstalledVersion = GetMappedInstalledVersion(installedVersion->GetProperty(PackageVersionProperty::Version), m_availablePackage);
                        }
                    }
                }
            }

            std::shared_ptr<IPackage> m_installedPackage;
            Utility::LocIndString m_installedChannel;
            std::shared_ptr<IPackage> m_availablePackage;
            Source m_trackingSource;
            std::shared_ptr<IPackage> m_trackingPackage;
            std::shared_ptr<IPackageVersion> m_trackingPackageVersion;
            std::string m_overrideInstalledVersion;
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
                    Field(field), String1(string) {}

                SystemReferenceString(PackageMatchField field, Utility::LocIndString string1, Utility::LocIndString string2) :
                    Field(field), String1(string1), String2(string2) {}

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

                void AddToFilters(std::vector<PackageMatchFilter>& filters) const
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

                SearchRequest CreateInclusionsSearchRequest() const
                {
                    SearchRequest result;
                    for (const auto& srs : SystemReferenceStrings)
                    {
                        srs.AddToFilters(result.Inclusions);
                    }
                    return result;
                }
            };

            // For a given package version, prepares the results for it.
            PackageData GetSystemReferenceStrings(IPackageVersion* version)
            {
                PackageData result;
                AddSystemReferenceStrings(version, result);
                return result;
            }

            // Check for a package already in the result that should have been correlated already.
            // If we find one, see if we should upgrade it's match criteria.
            // If we don't, return package data for further use.
            std::optional<PackageData> CheckForExistingResultFromAvailablePackageMatch(const ResultMatch& availableMatch)
            {
                for (auto& match : Matches)
                {
                    const std::shared_ptr<IPackage>& availablePackage = match.Package->GetAvailablePackage();
                    if (availablePackage && availablePackage->IsSame(availableMatch.Package.get()))
                    {
                        if (ResultMatchComparator{}(availableMatch, match))
                        {
                            match.MatchCriteria = availableMatch.MatchCriteria;
                        }

                        return {};
                    }
                }

                PackageData result;
                for (auto const& versionKey : availableMatch.Package->GetAvailableVersionKeys())
                {
                    auto packageVersion = availableMatch.Package->GetAvailableVersion(versionKey);
                    AddSystemReferenceStrings(packageVersion.get(), result);
                }
                return result;
            }

            // Check for a package already in the result that should have been correlated already.
            // If we find one, see if we should upgrade it's match criteria.
            // If we don't, return package data for further use.
            std::optional<PackageData> CheckForExistingResultFromTrackingPackageMatch(const ResultMatch& trackingMatch)
            {
                for (auto& match : Matches)
                {
                    const std::shared_ptr<IPackage>& trackingPackage = match.Package->GetTrackingPackage();
                    if (trackingPackage && trackingPackage->IsSame(trackingMatch.Package.get()))
                    {
                        if (ResultMatchComparator{}(trackingMatch, match))
                        {
                            match.MatchCriteria = trackingMatch.MatchCriteria;
                        }

                        return {};
                    }
                }

                PackageData result;
                for (auto const& versionKey : trackingMatch.Package->GetAvailableVersionKeys())
                {
                    auto packageVersion = trackingMatch.Package->GetAvailableVersion(versionKey);
                    AddSystemReferenceStrings(packageVersion.get(), result);
                }
                return result;
            }

            // Determines if the results contain the given installed package.
            bool ContainsInstalledPackage(const IPackage* installedPackage)
            {
                for (auto& match : Matches)
                {
                    const std::shared_ptr<IPackage>& matchPackage = match.Package->GetInstalledPackage();
                    if (matchPackage && matchPackage->IsSame(installedPackage))
                    {
                        return true;
                    }
                }

                return false;
            }

            // Destructively converts the result to the standard variant.
            operator SearchResult() &&
            {
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

            std::vector<CompositeResultMatch> Matches;
            bool Truncated = false;
            std::vector<SearchResult::Failure> Failures;

        private:
            void AddSystemReferenceStrings(IPackageVersion* version, PackageData& data)
            {
                GetSystemReferenceStrings(
                    version,
                    PackageVersionMultiProperty::PackageFamilyName,
                    PackageMatchField::PackageFamilyName,
                    data);

                GetSystemReferenceStrings(
                    version,
                    PackageVersionMultiProperty::ProductCode,
                    PackageMatchField::ProductCode,
                    data);

                GetSystemReferenceStrings(
                    version,
                    PackageVersionMultiProperty::UpgradeCode,
                    PackageMatchField::UpgradeCode,
                    data);

                GetNameAndPublisher(
                    version,
                    data);
            }

            void GetSystemReferenceStrings(
                IPackageVersion* installedVersion,
                PackageVersionMultiProperty prop,
                PackageMatchField field,
                PackageData& data)
            {
                for (auto&& string : installedVersion->GetMultiProperty(prop))
                {
                    data.AddIfNotPresent(SystemReferenceString{ field, std::move(string) });
                }
            }

            void GetNameAndPublisher(
                IPackageVersion* installedVersion,
                PackageData& data)
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
                        data.AddIfNotPresent(SystemReferenceString{
                            PackageMatchField::NormalizedNameAndPublisher,
                            names[i],
                            publishers[j] });
                    }
                }
            }
        };

        std::shared_ptr<IPackage> GetTrackedPackageFromAvailableSource(CompositeResult& result, const Source& source, const Utility::LocIndString& identifier)
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
                return std::move(directResult.Matches[0].Package);
            }
            else
            {
                AICLI_LOG(Repo, Warning, << "Found multiple results for Id [" << identifier << "] in tracked source: " << source.GetDetails().Name);
            }

            return {};
        }
    }

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

                auto compositePackage = std::make_shared<CompositePackage>(match.Package);

                auto installedVersion = compositePackage->GetInstalledVersion();

                if (!installedVersion)
                {
                    // One would think that the installed version coming directly from our own installed source
                    // would never be null, but it is sometimes. Rather than making users suffer through crashes
                    // that break their entire experience, lets log a few things and then ignore this match.
                    AICLI_LOG(Repo, Warning, << "CompositeSource: The installed version of the package '" <<
                        match.Package->GetProperty(PackageProperty::Id) << "' was null and is being dropped from the results.");
                    continue;
                }

                auto installedPackageData = result.GetSystemReferenceStrings(installedVersion.get());

                // Create a search request to run against all available sources
                if (!installedPackageData.SystemReferenceStrings.empty())
                {
                    SearchRequest systemReferenceSearch = installedPackageData.CreateInclusionsSearchRequest();

                    Source trackedSource;
                    std::shared_ptr<IPackage> trackingPackage;
                    std::shared_ptr<IPackageVersion> trackingPackageVersion;
                    std::chrono::system_clock::time_point trackingPackageTime;
                    std::shared_ptr<IPackage> availablePackage;

                    // Check the tracking catalog first to see if there is a correlation there.
                    // TODO: When the issue with support for multiple available packages is fixed, this should move into
                    //       the below available sources loop as we will check all sources at that point.
                    for (const auto& source : m_availableSources)
                    {
                        auto trackingCatalog = source.GetTrackingCatalog();
                        SearchResult trackingResult = trackingCatalog.Search(systemReferenceSearch);

                        std::shared_ptr<IPackage> candidatePackage = GetMatchingPackage(trackingResult.Matches,
                            [&]() {
                                AICLI_LOG(Repo, Info,
                                    << "Found multiple matches for installed package [" << installedVersion->GetProperty(PackageVersionProperty::Id) <<
                                    "] in tracking catalog for source [" << source.GetIdentifier() << "] when searching for [" << systemReferenceSearch.ToString() << "]");
                            }, [&] {
                                AICLI_LOG(Repo, Warning, << "  Appropriate tracking package could not be determined");
                            });

                        // Determine the candidate package with the latest install time
                        if (candidatePackage)
                        {
                            auto [candidateTime, candidateVersion] = GetLatestTrackingWriteTimeAndPackageVersion(candidatePackage);

                            if (!trackingPackage || candidateTime > trackingPackageTime)
                            {
                                trackedSource = source;
                                trackingPackage = std::move(candidatePackage);
                                trackingPackageVersion = std::move(candidateVersion);
                                trackingPackageTime = candidateTime;
                            }
                        }
                    }

                    // Directly search for the available package from tracking information.
                    if (trackingPackage)
                    {
                        availablePackage = GetTrackedPackageFromAvailableSource(result, trackedSource, trackingPackage->GetProperty(PackageProperty::Id));
                    }

                    if (!availablePackage)
                    {
                        // Search sources and add to result
                        for (const auto& source : m_availableSources)
                        {
                            // Do not attempt to correlate local packages against this source
                            if (!source.GetDetails().SupportInstalledSearchCorrelation)
                            {
                                continue;
                            }

                            SearchResult availableResult = result.SearchAndHandleFailures(source, systemReferenceSearch);

                            if (availableResult.Matches.empty())
                            {
                                continue;
                            }

                            availablePackage = GetMatchingPackage(availableResult.Matches,
                                [&]() {
                                    AICLI_LOG(Repo, Info,
                                        << "Found multiple matches for installed package [" << installedVersion->GetProperty(PackageVersionProperty::Id) <<
                                        "] in source [" << source.GetIdentifier() << "] when searching for [" << systemReferenceSearch.ToString() << "]");
                                }, [&] {
                                    AICLI_LOG(Repo, Warning, << "  Appropriate available package could not be determined");
                                });

                            // We found some matching packages here, don't keep going
                            break;
                        }
                    }

                    compositePackage->SetAvailablePackage(std::move(availablePackage));
                    compositePackage->SetTracking(std::move(trackedSource), std::move(trackingPackage), std::move(trackingPackageVersion));
                }

                // Move the installed result into the composite result
                result.Matches.emplace_back(std::move(compositePackage), std::move(match.MatchCriteria));
            }

            // Optimization for the "everything installed" case, no need to allow for reverse correlations
            if (request.IsForEverything() && m_searchBehavior == CompositeSearchBehavior::Installed)
            {
                return std::move(result);
            }
        }

        // Search available sources
        for (const auto& source : m_availableSources)
        {
            // Search the tracking catalog as it can potentially get better correlations
            auto trackingCatalog = source.GetTrackingCatalog();
            SearchResult trackingResult = trackingCatalog.Search(request);

            for (auto&& match : trackingResult.Matches)
            {
                // Check for a package already in the result that should have been correlated already.
                auto packageData = result.CheckForExistingResultFromTrackingPackageMatch(match);

                // If found existing package in the result, continue
                if (!packageData)
                {
                    continue;
                }

                // If no package was found that was already in the results, do a correlation lookup with the installed
                // source to create a new composite package entry if we find any packages there.
                if (packageData && !packageData->SystemReferenceStrings.empty())
                {
                    // Create a search request to run against the installed source
                    SearchRequest systemReferenceSearch = packageData->CreateInclusionsSearchRequest();

                    // Correlate against installed (allow exceptions out as we own the installed source)
                    SearchResult installedCrossRef = m_installedSource.Search(systemReferenceSearch);

                    std::shared_ptr<IPackage> installedPackage = GetMatchingPackage(installedCrossRef.Matches,
                        [&]() {
                            AICLI_LOG(Repo, Info,
                                << "Found multiple matches for tracking package [" << match.Package->GetProperty(PackageProperty::Id) <<
                                "] in source [" << source.GetIdentifier() << "] when searching for [" << systemReferenceSearch.ToString() << "]");
                        }, [&] {
                            AICLI_LOG(Repo, Warning, << "  Appropriate installed package could not be determined");
                        });

                    if (installedPackage && !result.ContainsInstalledPackage(installedPackage.get()))
                    {
                        auto compositePackage = std::make_shared<CompositePackage>(
                            std::move(installedPackage),
                            GetTrackedPackageFromAvailableSource(result, source, match.Package->GetProperty(PackageProperty::Id)));

                        auto [writeTime, trackingPackageVersion] = GetLatestTrackingWriteTimeAndPackageVersion(match.Package);

                        compositePackage->SetTracking(source, std::move(match.Package), std::move(trackingPackageVersion));

                        result.Matches.emplace_back(std::move(compositePackage), match.MatchCriteria);
                    }
                }
            }

            SearchResult availableResult = result.SearchAndHandleFailures(source, request);

            for (auto&& match : availableResult.Matches)
            {
                // Check for a package already in the result that should have been correlated already.
                auto packageData = result.CheckForExistingResultFromAvailablePackageMatch(match);

                // If found existing package in the result, continue
                if (!packageData)
                {
                    continue;
                }

                // If no package was found that was already in the results, do a correlation lookup with the installed
                // source to create a new composite package entry if we find any packages there.
                bool foundInstalledMatch = false;
                if (packageData && !packageData->SystemReferenceStrings.empty())
                {
                    // Create a search request to run against the installed source
                    SearchRequest systemReferenceSearch = packageData->CreateInclusionsSearchRequest();

                    // Correlate against installed (allow exceptions out as we own the installed source)
                    SearchResult installedCrossRef = m_installedSource.Search(systemReferenceSearch);

                    std::shared_ptr<IPackage> installedPackage = GetMatchingPackage(installedCrossRef.Matches,
                        [&]() {
                            AICLI_LOG(Repo, Info,
                                << "Found multiple matches for available package [" << match.Package->GetProperty(PackageProperty::Id) <<
                                "] in source [" << source.GetIdentifier() << "] when searching for [" << systemReferenceSearch.ToString() << "]");
                        }, [&] {
                            AICLI_LOG(Repo, Warning, << "  Appropriate installed package could not be determined");
                        });

                    if (installedPackage && !result.ContainsInstalledPackage(installedPackage.get()))
                    {
                        // TODO: Needs a whole separate change to fix the fact that we don't support multiple available packages and what the different search behaviors mean
                        foundInstalledMatch = true;
                        result.Matches.emplace_back(std::make_shared<CompositePackage>(std::move(installedPackage), std::move(match.Package)), match.MatchCriteria);
                    }
                }

                // If there was no correlation for this package, add it without one.
                if ((m_searchBehavior == CompositeSearchBehavior::AllPackages || m_searchBehavior == CompositeSearchBehavior::AvailablePackages) && !foundInstalledMatch)
                {
                    result.Matches.emplace_back(std::make_shared<CompositePackage>(std::shared_ptr<IPackage>{}, std::move(match.Package)), match.MatchCriteria);
                }
            }
        }

        SortResultMatches(result.Matches);

        if (request.MaximumResults > 0 && result.Matches.size() > request.MaximumResults)
        {
            result.Truncated = true;
            result.Matches.erase(result.Matches.begin() + request.MaximumResults, result.Matches.end());
        }

        return std::move(result);
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
