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

        // A composite package for the CompositeSource.
        struct CompositePackage : public IPackage
        {
            CompositePackage(std::shared_ptr<IPackage> installedPackage, std::shared_ptr<IPackage> availablePackage = {}) :
                m_installedPackage(std::move(installedPackage)), m_availablePackage(std::move(availablePackage))
            {
                // Grab the installed version's channel to allow for filtering in calls to get available info.
                if (m_installedPackage)
                {
                    m_installedChannel = m_installedPackage->GetInstalledVersion()->GetProperty(PackageVersionProperty::Channel);
                }
            }

            Utility::LocIndString GetProperty(PackageProperty property) const override
            {
                std::shared_ptr<IPackageVersion> truth = GetLatestAvailableVersion();
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
                    return m_installedPackage->GetInstalledVersion();
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

            void SetAvailablePackage(std::shared_ptr<IPackage> availablePackage)
            {
                m_availablePackage = std::move(availablePackage);
            }

        private:
            std::shared_ptr<IPackage> m_installedPackage;
            Utility::LocIndString m_installedChannel;
            std::shared_ptr<IPackage> m_availablePackage;
        };

        // A sentinel package with an unknown version.
        struct UnknownAvailablePackage : public IPackage
        {
            static constexpr std::string_view Version = "Unknown"sv;

            struct UnknownAvailablePackageVersion : public IPackageVersion
            {
                Utility::LocIndString GetProperty(PackageVersionProperty property) const override
                {
                    switch (property)
                    {
                    case AppInstaller::Repository::PackageVersionProperty::Version:
                        return Utility::LocIndString{ Version };
                    default:
                        return {};
                    }
                }

                std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty) const override
                {
                    return {};
                };

                Manifest::Manifest GetManifest() const override
                {
                    return {};
                }

                std::shared_ptr<const ISource> GetSource() const override
                {
                    return {};
                }

                IPackageVersion::Metadata GetMetadata() const override
                {
                    return {};
                }
            };

            Utility::LocIndString GetProperty(PackageProperty) const override
            {
                return {};
            }

            std::shared_ptr<IPackageVersion> GetInstalledVersion() const override
            {
                return {};
            }

            std::vector<PackageVersionKey> GetAvailableVersionKeys() const override
            {
                return { { {}, Version, {} } };
            }

            std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
            {
                return std::make_shared<UnknownAvailablePackageVersion>();
            }

            std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey&) const override
            {
                return std::make_shared<UnknownAvailablePackageVersion>();
            }

            bool IsUpdateAvailable() const override
            {
                // Lie here so that list and upgrade will carry on to be able to output the diagnostic information.
                return true;
            }
        };

        // The comparator compares the ResultMatch by MatchType first, then Field in a predefined order.
        struct ResultMatchComparator
        {
            bool operator() (
                const ResultMatch& match1,
                const ResultMatch& match2)
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

        // Stores data to enable correlation between installed and available packages.
        struct CompositeResult : public SearchResult
        {
            // A system reference string.
            struct SystemReferenceString
            {
                SystemReferenceString(PackageMatchField field, Utility::LocIndString string) :
                    Field(field), String(string) {}

                bool operator<(const SystemReferenceString& other) const
                {
                    if (Field != other.Field)
                    {
                        return Field < other.Field;
                    }

                    return String < other.String;
                }

                PackageMatchField Field;
                Utility::LocIndString String;
            };

            // Data relevant to correlation for a package.
            struct PackageData
            {
                std::vector<SystemReferenceString> SystemReferenceStrings;
            };

            // Data relevant to correlation for an installed package.
            struct InstalledPackageData : public PackageData
            {
                size_t MatchIndex;
            };

            // For a given package version, prepares the results for it.
            InstalledPackageData ReserveInstalledPackageSlot(IPackageVersion* installedVersion)
            {
                InstalledPackageData result;
                result.MatchIndex = Matches.size();

                HandleSystemReferenceStringTypeForReserveInstalledPackageSlot(
                    installedVersion,
                    PackageVersionMultiProperty::PackageFamilyName,
                    PackageMatchField::PackageFamilyName,
                    "package family name"sv,
                    result);

                HandleSystemReferenceStringTypeForReserveInstalledPackageSlot(
                    installedVersion,
                    PackageVersionMultiProperty::ProductCode,
                    PackageMatchField::ProductCode,
                    "product code"sv,
                    result);

                return result;
            }

            // Check for a package already in the result that should have been correlated already.
            // If we find one, see if we should upgrade it's match criteria.
            // If we don't, return package data for further use.
            std::optional<PackageData> CheckForExistingResultFromAvailablePackageMatch(const ResultMatch& match)
            {
                bool foundExistingPackage = false;
                PackageData result;

                auto latestVersion = match.Package->GetLatestAvailableVersion();

                foundExistingPackage = HandleSystemReferenceStringTypeForCheckForExistingResultFromAvailablePackageMatch(
                    match,
                    latestVersion.get(),
                    PackageVersionMultiProperty::PackageFamilyName,
                    PackageMatchField::PackageFamilyName,
                    "package family name"sv,
                    result);

                foundExistingPackage = HandleSystemReferenceStringTypeForCheckForExistingResultFromAvailablePackageMatch(
                    match,
                    latestVersion.get(),
                    PackageVersionMultiProperty::ProductCode,
                    PackageMatchField::ProductCode,
                    "product code"sv,
                    result) || foundExistingPackage;

                if (foundExistingPackage)
                {
                    return {};
                }
                else
                {
                    return result;
                }
            }

        private:
            void HandleSystemReferenceStringTypeForReserveInstalledPackageSlot(
                IPackageVersion* installedVersion,
                PackageVersionMultiProperty prop,
                PackageMatchField field,
                std::string_view logType,
                InstalledPackageData& data)
            {
                for (auto&& string : installedVersion->GetMultiProperty(prop))
                {
                    SystemReferenceString srs(field, std::move(string));

                    if (m_systemReferenceMap.find(srs) != m_systemReferenceMap.end())
                    {
                        AICLI_LOG(Repo, Warning, << "Multiple installed packages found with " << logType << " [" << srs.String << "], ignoring secondary packages for correlation.");
                    }
                    else
                    {
                        data.SystemReferenceStrings.emplace_back(srs);
                        m_systemReferenceMap.emplace(std::move(srs), data.MatchIndex);
                    }
                }
            }

            bool HandleSystemReferenceStringTypeForCheckForExistingResultFromAvailablePackageMatch(
                const ResultMatch& match,
                IPackageVersion* availableVersion,
                PackageVersionMultiProperty prop,
                PackageMatchField field,
                std::string_view logType,
                PackageData& data)
            {
                bool foundExistingPackage = false;

                for (auto&& string : availableVersion->GetMultiProperty(prop))
                {
                    SystemReferenceString srs(field, std::move(string));

                    auto itr = m_systemReferenceMap.find(srs);
                    if (itr != m_systemReferenceMap.end())
                    {
                        foundExistingPackage = true;

                        if (ResultMatchComparator{}(match, Matches[itr->second]))
                        {
                            AICLI_LOG(Repo, Verbose, << "Found existing result by " << logType << " [" << srs.String << "], increasing match criteria.");
                            Matches[itr->second].MatchCriteria = match.MatchCriteria;
                        }
                    }

                    data.SystemReferenceStrings.emplace_back(std::move(srs));
                }

                return foundExistingPackage;
            }

            // Maps for storing quick references to results based on their system reference string.
            std::map<SystemReferenceString, size_t> m_systemReferenceMap;
        };
    }

    CompositeSource::CompositeSource(std::string identifier) :
        m_identifier(identifier)
    {
        m_details.Name = "CompositeSource";
    }

    const SourceDetails& CompositeSource::GetDetails() const
    {
        return m_details;
    }

    const std::string& CompositeSource::GetIdentifier() const
    {
        return m_identifier;
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

    void CompositeSource::AddAvailableSource(std::shared_ptr<ISource> source)
    {
        m_availableSources.emplace_back(std::move(source));
    }

    void CompositeSource::SetInstalledSource(std::shared_ptr<ISource> source)
    {
        m_installedSource = std::move(source);
    }

    // An installed search first finds all installed packages that match the request, then correlates with available sources.
    // Next the search is performed against the available sources and correlated with the installed source. A result will only
    // be added if there exists an installed package that was not found by the initial search.
    // This allows for search terms to find installed packages by their available metadata, as well as the local values.
    SearchResult CompositeSource::SearchInstalled(const SearchRequest& request) const
    {
        CompositeResult result;

        // Search installed source
        SearchResult installedResult = m_installedSource->Search(request);
        result.Truncated = installedResult.Truncated;

        for (auto&& match : installedResult.Matches)
        {
            auto compositePackage = std::make_shared<CompositePackage>(std::move(match.Package));

            // Create a search request to run against all available sources
            // TODO: Determine if we should create a single search or one for each installed package.
            SearchRequest systemReferenceSearch;

            auto installedVersion = compositePackage->GetInstalledVersion();
            auto installedPackageData = result.ReserveInstalledPackageSlot(installedVersion.get());

            if (!installedPackageData.SystemReferenceStrings.empty())
            {
                for (const auto& srs : installedPackageData.SystemReferenceStrings)
                {
                    systemReferenceSearch.Inclusions.emplace_back(PackageMatchFilter(srs.Field, MatchType::Exact, srs.String));
                }

                std::shared_ptr<IPackage> availablePackage;

                // Search sources and add to result
                for (const auto& source : m_availableSources)
                {
                    // See if a previous iteration found a package
                    if (availablePackage)
                    {
                        break;
                    }

                    SearchResult availableResult = source->Search(systemReferenceSearch);

                    if (availableResult.Matches.empty())
                    {
                        continue;
                    }

                    if (availableResult.Matches.size() == 1)
                    {
                        availablePackage = std::move(availableResult.Matches[0].Package);
                        break;
                    }
                    else // availableResult.Matches.size() > 1
                    {
                        auto id = installedVersion->GetProperty(PackageVersionProperty::Id);

                        AICLI_LOG(Repo, Info, 
                            << "Found multiple matches for installed package [" << id << "] in source [" << source->GetIdentifier() << "] when searching for [" << systemReferenceSearch.ToString() << "]");

                        // More than one match found for the system reference; run some heuristics to check for a match
                        for (auto&& availableMatch : availableResult.Matches)
                        {
                            auto matchId = availableMatch.Package->GetLatestAvailableVersion()->GetProperty(PackageVersionProperty::Id);

                            AICLI_LOG(Repo, Info, << "  Checking system reference match with package id: " << matchId);

                            if (Utility::ICUCaseInsensitiveEquals(id, matchId))
                            {
                                availablePackage = std::move(availableMatch.Package);
                                break;
                            }
                        }

                        // We did not find an exact match on Id in the results
                        if (!availablePackage)
                        {
                            AICLI_LOG(Repo, Warning, << "  Appropriate available package could not be determined, setting availability state to unknown");
                            availablePackage = std::make_shared<UnknownAvailablePackage>();
                        }
                    }
                }

                compositePackage->SetAvailablePackage(std::move(availablePackage));
            }

            // Move the installed result into the composite result
            result.Matches.emplace_back(std::move(compositePackage), std::move(match.MatchCriteria));
        }

        // Optimization for the "everything installed" case, no need to allow for reverse correlations
        if (request.IsForEverything())
        {
            return result;
        }

        // Search available sources
        auto availableResult = SearchAvailable(request);

        for (auto&& match : availableResult.Matches)
        {
            // Check for a package already in the result that should have been correlated already.
            auto packageData = result.CheckForExistingResultFromAvailablePackageMatch(match);

            // If no package was found that was already in the results, do a correlation lookup with the installed
            // source to create a new composite package entry if we find any packages there.
            if (packageData && !packageData->SystemReferenceStrings.empty())
            {
                // Create a search request to run against the installed source
                SearchRequest systemReferenceSearch;
                for (const auto& srs : packageData->SystemReferenceStrings)
                {
                    systemReferenceSearch.Inclusions.emplace_back(PackageMatchFilter(srs.Field, MatchType::Exact, srs.String));
                }

                SearchResult installedCrossRef = m_installedSource->Search(systemReferenceSearch);

                for (auto&& crossRef : installedCrossRef.Matches)
                {
                    // Ensure that we don't pick up the same package from two available sources by recording it in the map.
                    auto installedVersion = crossRef.Package->GetInstalledVersion();
                    auto installedPackageData = result.ReserveInstalledPackageSlot(installedVersion.get());

                    result.Matches.emplace_back(std::make_shared<CompositePackage>(std::move(crossRef.Package), std::move(match.Package)), match.MatchCriteria);
                }
            }
        }

        SortResultMatches(result.Matches);

        if (request.MaximumResults > 0 && result.Matches.size() > request.MaximumResults)
        {
            result.Truncated = true;
            result.Matches.erase(result.Matches.begin() + request.MaximumResults, result.Matches.end());
        }

        return result;
    }

    // An available search goes through each source, searching individually and then sorting the full result set.
    SearchResult CompositeSource::SearchAvailable(const SearchRequest& request) const
    {
        SearchResult result;

        // Search available sources
        for (const auto& source : m_availableSources)
        {
            auto oneSourceResult = source->Search(request);

            // Move all matches into the single result
            for (auto&& match : oneSourceResult.Matches)
            {
                result.Matches.emplace_back(std::move(match));
            }
        }

        SortResultMatches(result.Matches);

        if (request.MaximumResults > 0 && result.Matches.size() > request.MaximumResults)
        {
            result.Truncated = true;
            result.Matches.erase(result.Matches.begin() + request.MaximumResults, result.Matches.end());
        }

        return result;
    }

    void CompositeSource::SortResultMatches(std::vector<ResultMatch>& matches)
    {
        std::stable_sort(matches.begin(), matches.end(), ResultMatchComparator());
    }
}
