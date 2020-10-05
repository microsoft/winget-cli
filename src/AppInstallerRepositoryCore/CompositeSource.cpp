// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CompositeSource.h"

namespace AppInstaller::Repository
{
    namespace
    {
        // Holds a PackageVersionKey and the VersionAndChannel used to sort it.
        struct SortablePackageVersionKey
        {
            SortablePackageVersionKey(PackageVersionKey&& key) : \
                m_packageVersionKey(std::move(key)),
                m_versionAndChannel(Utility::Version(m_packageVersionKey.Version), Utility::Channel(m_packageVersionKey.Channel)) {}

            bool operator<(const SortablePackageVersionKey& other) const
            {
                return m_versionAndChannel < other.m_versionAndChannel;
            }

            PackageVersionKey Detach()
            {
                return std::move(m_packageVersionKey);
            }

        private:
            PackageVersionKey m_packageVersionKey;
            Utility::VersionAndChannel m_versionAndChannel;
        };

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
            CompositePackage(const std::string& sourceIdentifier) : m_sourceIdentifier(sourceIdentifier) {}

            CompositePackage(const std::string& sourceIdentifier, std::shared_ptr<IPackage> installedPackage) :
                m_sourceIdentifier(sourceIdentifier), m_installedPackage(std::move(installedPackage)) {}

            const std::string& GetSourceIdentifier() const
            {
                return m_sourceIdentifier;
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
                if (m_availablePackages.empty())
                {
                    return {};
                }

                if (m_availablePackages.size() == 1)
                {
                    return m_availablePackages[0]->GetAvailableVersionKeys();
                }

                std::vector<SortablePackageVersionKey> sortableResults;

                for (const auto& availablePackage : m_availablePackages)
                {
                    auto availableVersions = availablePackage->GetAvailableVersionKeys();
                    std::copy(std::move_iterator(availableVersions.begin()), std::move_iterator(availableVersions.end()), std::back_inserter(sortableResults));
                }

                // Stable sort so that equal versions will maintain package addition order.
                std::stable_sort(sortableResults.begin(), sortableResults.end());

                std::vector<PackageVersionKey> result;

                for (auto& key : sortableResults)
                {
                    result.emplace_back(key.Detach());
                }

                return result;
            }

            std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
            {
                if (m_availablePackages.empty())
                {
                    return {};
                }

                if (m_availablePackages.size() == 1)
                {
                    return m_availablePackages[0]->GetLatestAvailableVersion();
                }

                std::shared_ptr<IPackageVersion> result;
                Utility::VersionAndChannel resultVersion;

                for (const auto& availablePackage : m_availablePackages)
                {
                    std::shared_ptr<IPackageVersion> latest = availablePackage->GetLatestAvailableVersion();

                    if (!result)
                    {
                        result = std::move(latest);
                        resultVersion = GetVACFromVersion(result.get());
                        continue;
                    }

                    Utility::VersionAndChannel latestVersion = GetVACFromVersion(latest.get());

                    // Only swap in this one if the current is less than, keeps equal versions stable.
                    if (resultVersion < latestVersion)
                    {
                        result = std::move(latest);
                        resultVersion = GetVACFromVersion(result.get());
                    }
                }

                return result;
            }

            std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const override
            {
                if (m_availablePackages.empty())
                {
                    return {};
                }

                if (m_availablePackages.size() == 1)
                {
                    return m_availablePackages[0]->GetAvailableVersion(versionKey);
                }

                for (const auto& availablePackage : m_availablePackages)
                {
                    std::shared_ptr<IPackageVersion> result = availablePackage->GetAvailableVersion(versionKey);
                    if (result)
                    {
                        return result;
                    }
                }

                return {};
            }

            bool IsUpdateAvailable() const override
            {
                auto installed = GetInstalledVersion();
                auto latest = GetLatestAvailableVersion();

                return (installed && latest && (GetVACFromVersion(installed.get()) < GetVACFromVersion(latest.get())));
            }

            void AddAvailablePackage(std::shared_ptr<IPackage> package)
            {
                m_availablePackages.emplace_back(std::move(package));
            }

        private:
            const std::string& m_sourceIdentifier;
            std::shared_ptr<IPackage> m_installedPackage;
            std::vector<std::shared_ptr<IPackage>> m_availablePackages;
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

    // The composite search needs to take several steps to merge results, and due to the
    // potential for different information spread across multiple sources, base searches
    // need to be performed in both installed and available.
    //
    // If an installed source is present, then the searches should only return packages
    // that are installed. This means that the base searches against available sources
    // will only return results where a match is found in the installed source.
    //
    // Available sources need to be merged with each other as well.
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
        // The result is our primary list, while the maps allow us to look up results by system reference string.
        SearchResult result;

        // Non-owning pointers to objects that live in result.
        struct CompositeResultReference
        {
            size_t ResultIndex;
            CompositePackage* Package;
        };

        std::map<Utility::LocIndString, CompositeResultReference> packageFamilyNameMap;
        std::map<Utility::LocIndString, CompositeResultReference> productCodeMap;

        // Search installed source
        SearchResult installedResult = m_installedSource->Search(request);
        result.Truncated = installedResult.Truncated;

        for (auto&& match : installedResult.Matches)
        {
            auto compositePackage = std::make_shared<CompositePackage>(m_identifier, std::move(match.Package));
            CompositeResultReference compositeResultRef{ result.Matches.size(), compositePackage.get() };

            // Create a search request to run against all available sources
            // TODO: Determine if we should create a single search or one for each installed package.
            SearchRequest systemReferenceSearch;

            auto installedVersion = compositePackage->GetInstalledVersion();

            // While we don't expect more than one, allow for it anyway.
            for (auto&& packageFamilyName : installedVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName))
            {
                if (packageFamilyNameMap.find(packageFamilyName) != packageFamilyNameMap.end())
                {
                    AICLI_LOG(Repo, Error, << "Multiple installed packages found with package family name [" << packageFamilyName << "], ignoring secondary packages for correlation.");
                }
                else
                {
                    systemReferenceSearch.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::PackageFamilyName, MatchType::Exact, packageFamilyName));
                    packageFamilyNameMap.emplace(std::move(packageFamilyName), compositeResultRef);
                }
            }

            for (auto&& productCode : installedVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode))
            {
                if (productCodeMap.find(productCode) != productCodeMap.end())
                {
                    AICLI_LOG(Repo, Error, << "Multiple installed packages found with product code [" << productCode << "], ignoring secondary packages for correlation.");
                }
                else
                {
                    systemReferenceSearch.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, productCode));
                    productCodeMap.emplace(std::move(productCode), compositeResultRef);
                }
            }

            if (!systemReferenceSearch.Inclusions.empty())
            {
                // Search sources and pile into result
                for (const auto& source : m_availableSources)
                {
                    SearchResult availableResult = source->Search(systemReferenceSearch);

                    for (auto&& available : availableResult.Matches)
                    {
                        compositePackage->AddAvailablePackage(std::move(available.Package));
                    }
                }
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
            // Check for a package already in the result that should have been correlated already
            auto latestVersion = match.Package->GetLatestAvailableVersion();
            auto packageFamilyNames = latestVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
            auto productCodes = latestVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
            bool foundExistingPackage = false;

            for (const auto& packageFamilyName : packageFamilyNames)
            {
                auto itr = packageFamilyNameMap.find(packageFamilyName);
                if (itr != packageFamilyNameMap.end())
                {
                    foundExistingPackage = true;

                    if (ResultMatchComparator{}(match, result.Matches[itr->second.ResultIndex]))
                    {
                        result.Matches[itr->second.ResultIndex].MatchCriteria = match.MatchCriteria;
                    }
                }
            }

            for (const auto& productCode : productCodes)
            {
                auto itr = productCodeMap.find(productCode);
                if (itr != productCodeMap.end())
                {
                    foundExistingPackage = true;

                    if (ResultMatchComparator{}(match, result.Matches[itr->second.ResultIndex]))
                    {
                        result.Matches[itr->second.ResultIndex].MatchCriteria = match.MatchCriteria;
                    }
                }
            }

            // If no package was found that was already in the results, do a correlation lookup with the installed
            // source to create a new composite package entry if we find any packages there.
            if (!foundExistingPackage)
            {
                // Create a search request to run against the installed source
                SearchRequest systemReferenceSearch;

                for (auto&& packageFamilyName : packageFamilyNames)
                {
                    systemReferenceSearch.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::PackageFamilyName, MatchType::Exact, packageFamilyName));
                }

                for (auto&& productCode : productCodes)
                {
                    systemReferenceSearch.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, productCode));
                }

                if (!systemReferenceSearch.Inclusions.empty())
                {
                    SearchResult installedCrossRef = m_installedSource->Search(systemReferenceSearch);

                    for (auto&& crossRef : installedCrossRef.Matches)
                    {
                        auto compositePackage = std::make_shared<CompositePackage>(m_identifier, std::move(crossRef.Package));
                        compositePackage->AddAvailablePackage(match.Package);

                        result.Matches.emplace_back(std::move(compositePackage), match.MatchCriteria);
                    }
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

    // An available search goes through each source, searching individually and then correlating with
    // any existing results. If no existing results are found, a new result is added to the return values.
    SearchResult CompositeSource::SearchAvailable(const SearchRequest& request) const
    {
        // The result is our primary list, while the maps allow us to look up results by system reference string.
        SearchResult result;

        // Non-owning pointers to objects that live in result.
        struct CompositeResultReference
        {
            size_t ResultIndex;
            CompositePackage* Package;
        };

        std::map<Utility::LocIndString, CompositeResultReference> packageFamilyNameMap;
        std::map<Utility::LocIndString, CompositeResultReference> productCodeMap;

        // Search available sources
        for (const auto& source : m_availableSources)
        {
            auto oneSourceResult = source->Search(request);

            for (auto&& match : oneSourceResult.Matches)
            {
                // Check for a package already in the list to correlate with
                auto latestVersion = match.Package->GetLatestAvailableVersion();
                auto packageFamilyNames = latestVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
                auto productCodes = latestVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
                bool foundExistingPackage = false;

                for (const auto& packageFamilyName : packageFamilyNames)
                {
                    auto itr = packageFamilyNameMap.find(packageFamilyName);
                    if (itr != packageFamilyNameMap.end())
                    {
                        foundExistingPackage = true;
                        itr->second.Package->AddAvailablePackage(match.Package);

                        if (ResultMatchComparator{}(match, result.Matches[itr->second.ResultIndex]))
                        {
                            result.Matches[itr->second.ResultIndex].MatchCriteria = match.MatchCriteria;
                        }
                    }
                }

                for (const auto& productCode : productCodes)
                {
                    auto itr = productCodeMap.find(productCode);
                    if (itr != productCodeMap.end())
                    {
                        foundExistingPackage = true;
                        itr->second.Package->AddAvailablePackage(match.Package);

                        if (ResultMatchComparator{}(match, result.Matches[itr->second.ResultIndex]))
                        {
                            result.Matches[itr->second.ResultIndex].MatchCriteria = match.MatchCriteria;
                        }
                    }
                }

                // If not an installed package search and no existing package found to correlate with, add this package to the result
                if (!foundExistingPackage)
                {
                    auto compositePackage = std::make_shared<CompositePackage>(m_identifier);
                    compositePackage->AddAvailablePackage(std::move(match.Package));
                    CompositeResultReference compositeResultRef{ result.Matches.size(), compositePackage.get() };

                    for (auto&& packageFamilyName : packageFamilyNames)
                    {
                        packageFamilyNameMap.emplace(std::move(packageFamilyName), compositeResultRef);
                    }

                    for (auto&& productCode : productCodes)
                    {
                        productCodeMap.emplace(std::move(productCode), compositeResultRef);
                    }

                    result.Matches.emplace_back(std::move(compositePackage), std::move(match.MatchCriteria));
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

    void CompositeSource::SortResultMatches(std::vector<ResultMatch>& matches)
    {
        std::stable_sort(matches.begin(), matches.end(), ResultMatchComparator());
    }
}

