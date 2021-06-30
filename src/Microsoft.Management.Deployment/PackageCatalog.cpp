// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <mutex>
#include <AppInstallerRepositorySource.h>
#include "Workflows/WorkflowBase.h"
#include "Converters.h"
#include "PackageCatalog.h"
#include "PackageCatalog.g.cpp"
#include "PackageCatalogInfo.h"
#include "FindPackagesResult.h"
#include "MatchResult.h"
#include "CatalogPackage.h"
#include "Commands/RootCommand.h"
#include "Helpers.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageMatchFilter.h"
#pragma warning( pop )
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include <winget/GroupPolicy.h>
#include <AppInstallerErrors.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageCatalog::Initialize(
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo info,
        std::shared_ptr<const ::AppInstaller::Repository::ISource> source,
        bool isComposite)
    {
        m_info = info;
        m_source = std::move(source);
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

    HRESULT PopulateSearchRequestFromVector(
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
                    if (searchRequest->Query.has_value())
                    {
                        // CatalogDefault match field can't be used twice.
                        return E_INVALIDARG;
                    }
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
                    return E_INVALIDARG;
                }
                auto matchFilter = ::AppInstaller::Repository::PackageMatchFilter(matchField, packageFieldMatchOption, winrt::to_string(filter.Value()));
                searchRequest->Filters.emplace_back(matchFilter);
            }
        }
        return S_OK;
    }

    HRESULT PopulateSearchRequest(
        ::AppInstaller::Repository::SearchRequest* searchRequest,
        winrt::Microsoft::Management::Deployment::FindPackagesOptions const& options)
    {
        RETURN_IF_FAILED(PopulateSearchRequestFromVector(searchRequest, options.Filters(), false));
        RETURN_IF_FAILED(PopulateSearchRequestFromVector(searchRequest, options.Selectors(), true));
        return S_OK;
    }

    winrt::Microsoft::Management::Deployment::FindPackagesResult GetFindPackagesResult(HRESULT hr, bool isTruncated, Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::MatchResult> matches)
    {
        auto findPackagesResult = winrt::make_self<wil::details::module_count_wrapper<
            winrt::Microsoft::Management::Deployment::implementation::FindPackagesResult>>();
        // TODO: Add search timeout and error code.
        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus status = FindPackagesResultStatus(hr);
        findPackagesResult->Initialize(status, isTruncated, matches);
        return *findPackagesResult;
    }

    winrt::Microsoft::Management::Deployment::FindPackagesResult PackageCatalog::FindPackages(winrt::Microsoft::Management::Deployment::FindPackagesOptions const& options)
    {
        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::Ok;
        bool isTruncated = false;
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::MatchResult> matches{ winrt::single_threaded_vector<Microsoft::Management::Deployment::MatchResult>() };
        ::AppInstaller::Repository::SearchRequest searchRequest;

        HRESULT hr = S_OK;
        try
        {
            // No need to check for caller capability again since packageQuery was required in order to get the PackageCatalog object through Connect

            if (FAILED(hr = PopulateSearchRequest(&searchRequest, options)))
            {
                return GetFindPackagesResult(hr, isTruncated, matches);
            }
        
            searchRequest.MaximumResults = options.ResultLimit();
            auto searchResult = m_source->Search(searchRequest);

            // Build the result object from the searchResult
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
            isTruncated = searchResult.Truncated;
        }
        WINGET_CATCH_STORE(hr);

        return GetFindPackagesResult(hr, isTruncated, matches);
    }
}
