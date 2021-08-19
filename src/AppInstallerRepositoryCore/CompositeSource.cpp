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
                return true;
            }

            return false;
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
                    auto installedVersion = m_installedPackage->GetInstalledVersion();
                    if (installedVersion)
                    {
                        m_installedChannel = installedVersion->GetProperty(PackageVersionProperty::Channel);
                    }
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

            const std::shared_ptr<IPackage>& GetInstalledPackage()
            {
                return m_installedPackage;
            }

            const std::shared_ptr<IPackage>& GetAvailablePackage()
            {
                return m_availablePackage;
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

                return result;
            }

            std::vector<CompositeResultMatch> Matches;
            bool Truncated = false;

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
                auto names = installedVersion->GetMultiProperty(PackageVersionMultiProperty::Name);
                auto publishers = installedVersion->GetMultiProperty(PackageVersionMultiProperty::Publisher);

                for (size_t i = 0; i < names.size() && i < publishers.size(); ++i)
                {
                    data.AddIfNotPresent(SystemReferenceString{
                        PackageMatchField::NormalizedNameAndPublisher,
                        std::move(names[i]),
                        std::move(publishers[i]) });
                }
            }
        };
    }

    CompositeSource::CompositeSource(std::string identifier)
    {
        m_details.Name = "CompositeSource";
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

    void CompositeSource::AddAvailableSource(std::shared_ptr<ISource> source)
    {
        m_availableSources.emplace_back(std::move(source));
    }

    void CompositeSource::SetInstalledSource(std::shared_ptr<ISource> source, CompositeSearchBehavior searchBehavior)
    {
        m_installedSource = std::move(source);
        m_searchBehavior = searchBehavior;
    }

    // An installed search first finds all installed packages that match the request, then correlates with available sources.
    // Next the search is performed against the available sources and correlated with the installed source. A result will only
    // be added if there exists an installed package that was not found by the initial search.
    // This allows for search terms to find installed packages by their available metadata, as well as the local values.
    SearchResult CompositeSource::SearchInstalled(const SearchRequest& request) const
    {
        CompositeResult result;

        // If the search behavior is for AllPackages or Installed then the result can contain packages that are
        // only in the Installed source, but do not have an AvailableVersion.
        if (m_searchBehavior == CompositeSearchBehavior::AllPackages || m_searchBehavior == CompositeSearchBehavior::Installed)
        {
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
                auto installedPackageData = result.GetSystemReferenceStrings(installedVersion.get());

                if (!installedPackageData.SystemReferenceStrings.empty())
                {
                    for (const auto& srs : installedPackageData.SystemReferenceStrings)
                    {
                        srs.AddToFilters(systemReferenceSearch.Inclusions);
                    }

                    std::shared_ptr<IPackage> availablePackage;

                    // Search sources and add to result
                    for (const auto& source : m_availableSources)
                    {
                        SearchResult availableResult = source->Search(systemReferenceSearch);

                        if (availableResult.Matches.empty())
                        {
                            continue;
                        }

                        if (availableResult.Matches.size() == 1)
                        {
                            availablePackage = std::move(availableResult.Matches[0].Package);
                        }
                        else // availableResult.Matches.size() > 1
                        {
                            AICLI_LOG(Repo, Info,
                                << "Found multiple matches for installed package [" << installedVersion->GetProperty(PackageVersionProperty::Id) <<
                                "] in source [" << source->GetIdentifier() << "] when searching for [" << systemReferenceSearch.ToString() << "]");

                            // More than one match found for the system reference; run some heuristics to check for a match
                            for (auto&& availableMatch : availableResult.Matches)
                            {
                                AICLI_LOG(Repo, Info, << "  Checking match with package id: " <<
                                    availableMatch.Package->GetLatestAvailableVersion()->GetProperty(PackageVersionProperty::Id));

                                if (IsStrongMatchField(availableMatch.MatchCriteria.Field))
                                {
                                    if (!availablePackage)
                                    {
                                        availablePackage = std::move(availableMatch.Package);
                                    }
                                    else
                                    {
                                        AICLI_LOG(Repo, Info, << "  Found multiple packages with strong match fields");
                                        availablePackage.reset();
                                        break;
                                    }
                                }
                            }

                            if (!availablePackage)
                            {
                                AICLI_LOG(Repo, Warning, << "  Appropriate available package could not be determined");
                            }
                        }

                        // We found some matching packages here, don't keep going
                        break;
                    }

                    compositePackage->SetAvailablePackage(std::move(availablePackage));
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
            auto availableResult = source->Search(request);

            for (auto&& match : availableResult.Matches)
            {
                // Check for a package already in the result that should have been correlated already.
                auto packageData = result.CheckForExistingResultFromAvailablePackageMatch(match);

                // If no package was found that was already in the results, do a correlation lookup with the installed
                // source to create a new composite package entry if we find any packages there.
                bool foundInstalledMatch = false;
                if (packageData && !packageData->SystemReferenceStrings.empty())
                {
                    // Create a search request to run against the installed source
                    SearchRequest systemReferenceSearch;
                    for (const auto& srs : packageData->SystemReferenceStrings)
                    {
                        srs.AddToFilters(systemReferenceSearch.Inclusions);
                    }

                    SearchResult installedCrossRef = m_installedSource->Search(systemReferenceSearch);

                    for (auto&& crossRef : installedCrossRef.Matches)
                    {
                        if (!result.ContainsInstalledPackage(crossRef.Package.get()))
                        {
                            foundInstalledMatch = true;
                            result.Matches.emplace_back(std::make_shared<CompositePackage>(std::move(crossRef.Package), std::move(match.Package)), match.MatchCriteria);
                        }
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
}
