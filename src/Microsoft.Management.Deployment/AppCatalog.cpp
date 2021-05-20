#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "AppCatalog.h"
#include "AppCatalog.g.cpp"
#include "AppCatalogInfo.h"
#include "FindPackagesResult.h"
#include "MatchResult.h"
#include "CatalogPackage.h"
#include "PackageMatchFilter.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void AppCatalog::Initialize(winrt::Microsoft::Management::Deployment::AppCatalogInfo info, std::shared_ptr<::AppInstaller::Repository::ISource> source)
    {
        m_info = info;
        m_source = source;
    }
    bool AppCatalog::IsComposite()
    {
        return m_source->IsComposite();
    }
    winrt::Microsoft::Management::Deployment::AppCatalogInfo AppCatalog::Info()
    {
        auto appCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalogInfo>>();
        appCatalogInfo->Initialize(m_source->GetDetails());
        return *appCatalogInfo;
    }
    winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::FindPackagesResult> AppCatalog::FindPackagesAsync(winrt::Microsoft::Management::Deployment::FindPackagesOptions options)
    {
        co_return FindPackages(options);
    }
    void Populate(::AppInstaller::Repository::SearchRequest* searchRequest, Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> filters, bool isAdditive)
    {
        for (uint32_t i = 0; i < filters.Size(); ++i)
        {
            Microsoft::Management::Deployment::PackageMatchFilter filter = filters.GetAt(i);

            if (filter.Value().size() == 0)
            {
                continue;
            }

            ::AppInstaller::Repository::MatchType matchType = ::AppInstaller::Repository::MatchType::Exact;
            switch (filter.Type())
            {
            case Microsoft::Management::Deployment::MatchType::CaseInsensitive:
                matchType = ::AppInstaller::Repository::MatchType::CaseInsensitive;
                break;
            case Microsoft::Management::Deployment::MatchType::Exact:
                matchType = ::AppInstaller::Repository::MatchType::Exact;
                break;
            case Microsoft::Management::Deployment::MatchType::Fuzzy:
                matchType = ::AppInstaller::Repository::MatchType::Fuzzy;
                break;
            case Microsoft::Management::Deployment::MatchType::FuzzySubstring:
                matchType = ::AppInstaller::Repository::MatchType::FuzzySubstring;
                break;
            case Microsoft::Management::Deployment::MatchType::StartsWith:
                matchType = ::AppInstaller::Repository::MatchType::StartsWith;
                break;
            case Microsoft::Management::Deployment::MatchType::Substring:
                matchType = ::AppInstaller::Repository::MatchType::Substring;
                break;
            case Microsoft::Management::Deployment::MatchType::Wildcard:
                matchType = ::AppInstaller::Repository::MatchType::Wildcard;
                break;
            default:
                matchType = ::AppInstaller::Repository::MatchType::Exact;
                break;
            }

            ::AppInstaller::Repository::PackageMatchField matchField = ::AppInstaller::Repository::PackageMatchField::Id;
            switch (filter.Field())
            {
            case Microsoft::Management::Deployment::PackageMatchField::Command:
                matchField = ::AppInstaller::Repository::PackageMatchField::Command;
                break;
            case Microsoft::Management::Deployment::PackageMatchField::Id:
                matchField = ::AppInstaller::Repository::PackageMatchField::Id;
                break;
            case Microsoft::Management::Deployment::PackageMatchField::Moniker:
                matchField = ::AppInstaller::Repository::PackageMatchField::Moniker;
                break;
            case Microsoft::Management::Deployment::PackageMatchField::Name:
                matchField = ::AppInstaller::Repository::PackageMatchField::Name;
                break;
            case Microsoft::Management::Deployment::PackageMatchField::Tag:
                matchField = ::AppInstaller::Repository::PackageMatchField::Tag;
                break;
            default:
                matchField = ::AppInstaller::Repository::PackageMatchField::Id;
                break;
            }

            if (isAdditive)
            {
                if (filter.Field() == Microsoft::Management::Deployment::PackageMatchField::AppCatalogDefined)
                {
                    searchRequest->Query = ::AppInstaller::Repository::RequestMatch(matchType, winrt::to_string(filter.Value()));
                }
                else
                {
                    auto inclusion = ::AppInstaller::Repository::PackageMatchFilter(matchField, matchType, winrt::to_string(filter.Value()));
                    searchRequest->Inclusions.emplace_back(inclusion);
                }
            }
            else
            {
                searchRequest->Filters.emplace_back(::AppInstaller::Repository::PackageMatchFilter(matchField, matchType, winrt::to_string(filter.Value())));
            }
        }

    }
    winrt::Microsoft::Management::Deployment::FindPackagesResult AppCatalog::FindPackages(winrt::Microsoft::Management::Deployment::FindPackagesOptions const& options)
    {
        ::AppInstaller::Repository::SearchRequest searchRequest;
        
        Populate(&searchRequest, options.Filters(), false);
        Populate(&searchRequest, options.Selectors(), true);
        searchRequest.MaximumResults = options.ResultLimit();
        auto searchResult = m_source->Search(searchRequest);

        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::MatchResult> matches{ winrt::single_threaded_vector<Microsoft::Management::Deployment::MatchResult>() };

        for (size_t i = 0; i < searchResult.Matches.size(); ++i)
        {
            auto match = searchResult.Matches[i];
            auto catalogPackage = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::CatalogPackage>>();
            catalogPackage->Initialize(m_source, match.Package);

            auto packageMatchFilter = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageMatchFilter>>();

            auto matchResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::MatchResult>>();
            matchResult->Initialize(*catalogPackage, *packageMatchFilter);

            matches.Append(*matchResult);
        }
        auto findPackagesResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::FindPackagesResult>>();
        findPackagesResult->Initialize(matches);

        return *findPackagesResult;
    }
}
