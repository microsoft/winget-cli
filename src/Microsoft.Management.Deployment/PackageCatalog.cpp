// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "Converters.h"
#include "PackageCatalog.h"
#include "PackageCatalog.g.cpp"
#include "PackageCatalogInfo.h"
#include "FindPackagesResult.h"
#include "MatchResult.h"
#include "CatalogPackage.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageMatchFilter.h"
#pragma warning( pop )
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageCatalog::Initialize(
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo info, 
        std::shared_ptr<const ::AppInstaller::Repository::ISource> source,
        bool isComposite)
    {
        m_info = info;
        m_source = source;
        m_isComposite = isComposite;
    }
    bool PackageCatalog::IsComposite()
    {
        // Can't use m_source->IsComposite for this because all remote sources are turned into composite sources 
        // behind the scenes when being opened in PackageCatalogReference.cpp so that CatalogPackage.IsInstalled works.
        return m_isComposite;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogInfo PackageCatalog::Info()
    {
        return m_info;
    }
    winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::FindPackagesResult> PackageCatalog::FindPackagesAsync(winrt::Microsoft::Management::Deployment::FindPackagesOptions options)
    {
        co_return FindPackages(options);
    }
    
    void PopulateSearchRequestFromVector(
        ::AppInstaller::Repository::SearchRequest* searchRequest, 
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> vector, 
        bool isSelector)
    {
        // Populates either the Filters vector of a searchRequest (if isSelector is false),
        // or the Inclusions and Query (if true)
        for (uint32_t i = 0; i < vector.Size(); ++i)
        {
            Microsoft::Management::Deployment::PackageMatchFilter filter = vector.GetAt(i);

            if (filter.Value().size() == 0)
            {
                // If the caller did not add a value it can't actually be used to filter or include anything so just ignore it.
                continue;
            }
            ::AppInstaller::Repository::MatchType packageFieldMatchOption = GetRepositoryMatchType(filter.Option());
            ::AppInstaller::Repository::PackageMatchField matchField = GetRepositoryMatchField(filter.Field());

            if (isSelector)
            {
                if (filter.Field() == Microsoft::Management::Deployment::PackageMatchField::CatalogDefault)
                {
                    searchRequest->Query = ::AppInstaller::Repository::RequestMatch(packageFieldMatchOption, winrt::to_string(filter.Value()));
                }
                else
                {
                    auto matchFilter = ::AppInstaller::Repository::PackageMatchFilter(matchField, packageFieldMatchOption, winrt::to_string(filter.Value()));
                    searchRequest->Inclusions.emplace_back(matchFilter);
                }
            }
            else
            {
                if (filter.Field() == Microsoft::Management::Deployment::PackageMatchField::CatalogDefault)
                {
                    // CatalogDefault match fields can't be used in the Filters.
                    throw hresult_invalid_argument();
                }
                auto matchFilter = ::AppInstaller::Repository::PackageMatchFilter(matchField, packageFieldMatchOption, winrt::to_string(filter.Value()));
                searchRequest->Filters.emplace_back(matchFilter);
            }
        }
    }
    winrt::Microsoft::Management::Deployment::FindPackagesResult PackageCatalog::FindPackages(winrt::Microsoft::Management::Deployment::FindPackagesOptions const& options)
    {
        ::AppInstaller::Repository::SearchRequest searchRequest;
        
        // throws if a filter with PackageMatchField::CatalogDefault is included in the options.Filters list
        PopulateSearchRequestFromVector(&searchRequest, options.Filters(), false);
        PopulateSearchRequestFromVector(&searchRequest, options.Selectors(), true);
        searchRequest.MaximumResults = options.ResultLimit();
        auto searchResult = m_source->Search(searchRequest);

        // Build the result object from the searchResult
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::MatchResult> matches{ winrt::single_threaded_vector<Microsoft::Management::Deployment::MatchResult>() };
        for (size_t i = 0; i < searchResult.Matches.size(); ++i)
        {
            auto match = searchResult.Matches[i];
            auto catalogPackage = winrt::make_self<wil::details::module_count_wrapper<
                winrt::Microsoft::Management::Deployment::implementation::CatalogPackage>>();
            catalogPackage->Initialize(m_source, match.Package);

            auto packageMatchFilter = winrt::make_self<wil::details::module_count_wrapper<
                winrt::Microsoft::Management::Deployment::implementation::PackageMatchFilter>>();
            packageMatchFilter->Initialize(match.MatchCriteria);

            auto matchResult = winrt::make_self<wil::details::module_count_wrapper<
                winrt::Microsoft::Management::Deployment::implementation::MatchResult>>();
            matchResult->Initialize(*catalogPackage, *packageMatchFilter);

            matches.Append(*matchResult);
        }
        auto findPackagesResult = winrt::make_self<wil::details::module_count_wrapper<
            winrt::Microsoft::Management::Deployment::implementation::FindPackagesResult>>();
        // TODO: Add search timeout and error code.
        findPackagesResult->Initialize(winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::Ok, searchResult.Truncated, matches);

        return *findPackagesResult;
    }
}
