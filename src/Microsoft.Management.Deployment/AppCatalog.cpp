#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "AppCatalog.h"
#include "AppCatalog.g.cpp"
#include "AppCatalogInfo.h"
#include "FindPackagesResult.h"
#include "ResultMatch.h"
#include "CatalogPackage.h"
#include "PackageMatchFilter.h"
#include "Microsoft\PredefinedInstalledSourceFactory.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    AppCatalog::AppCatalog(hstring const& catalogId)
    {
        m_catalogId = catalogId;
    }
    AppCatalog::AppCatalog(Microsoft::Management::Deployment::PredefinedAppCatalog predefinedAppCatalog)
    {
        m_isPredefinedSource = true;
        m_predefinedAppCatalog = predefinedAppCatalog;
    }
    AppCatalog::AppCatalog(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions options)
    {
        m_compositeAppCatalogOptions = options;
    }
    bool AppCatalog::IsComposite()
    {
        return m_source->IsComposite();
    }
    Microsoft::Management::Deployment::AppCatalogInfo AppCatalog::Info()
    {
        if (!m_source.get())
        {
            throw hresult_illegal_method_call();
        }
        winrt::Microsoft::Management::Deployment::AppCatalogInfo appCatalogInfo{ nullptr };
        appCatalogInfo = winrt::make<winrt::Microsoft::Management::Deployment::implementation::AppCatalogInfo>(m_source->GetDetails());
        return appCatalogInfo;
    }
    Windows::Foundation::IAsyncAction AppCatalog::OpenAsync()
    {
        ::AppInstaller::ProgressCallback progress;
        if (m_isPredefinedSource)
        {
            ::AppInstaller::Repository::PredefinedSource predefinedSource = ::AppInstaller::Repository::PredefinedSource::Installed;
            switch (m_predefinedAppCatalog)
            {
            case Microsoft::Management::Deployment::PredefinedAppCatalog::InstalledPackages :
                predefinedSource = ::AppInstaller::Repository::PredefinedSource::Installed;
                m_source = ::AppInstaller::Repository::OpenPredefinedSource(predefinedSource, progress);
                break;
            case Microsoft::Management::Deployment::PredefinedAppCatalog::InstallingPackages :
                // TODO - installing source does not exist yet.
                throw hresult_not_implemented();
            case Microsoft::Management::Deployment::PredefinedAppCatalog::OpenWindowsCatalog :
                // TODO - creating source by enum or id is not supported yet.
                m_source = ::AppInstaller::Repository::OpenSource(winrt::to_string(L"winget"), progress).Source;
                break;
            }
        }
        else if (m_compositeAppCatalogOptions)
        {
            std::shared_ptr<::AppInstaller::Repository::ISource> nonLocalSource;
            bool includeInstalledCatalog = false;
            for (int i = 0; i < m_compositeAppCatalogOptions.Catalogs().Size(); ++i)
            {
                auto catalog = m_compositeAppCatalogOptions.Catalogs().GetAt(i);
                if (catalog.IsComposite())
                {
                    throw hresult_invalid_argument();
                }
                if (winrt::to_string(catalog.Info().Name()).compare(::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Type()) != 0)
                {
                    nonLocalSource = ::AppInstaller::Repository::OpenSource(winrt::to_string(catalog.Info().Name()), progress).Source;
                }
            }

            ::AppInstaller::Repository::CompositeSearchBehavior searchBehavior = ::AppInstaller::Repository::CompositeSearchBehavior::AllPackages;
            switch (m_compositeAppCatalogOptions.CompositeSearchBehavior())
            {
            case Microsoft::Management::Deployment::CompositeSearchBehavior::AllLocalPackages:
                // TODO - installing source does not exist yet.
                throw hresult_not_implemented();
            case Microsoft::Management::Deployment::CompositeSearchBehavior::InstalledPackages:
                if (!includeInstalledCatalog)
                {
                    throw hresult_invalid_argument();
                }
                searchBehavior = ::AppInstaller::Repository::CompositeSearchBehavior::Installed;
                break;
            case Microsoft::Management::Deployment::CompositeSearchBehavior::InstallingPackages:
                // TODO - installing source does not exist yet.
                throw hresult_not_implemented();
            case Microsoft::Management::Deployment::CompositeSearchBehavior::AllPackages:
            default:
                searchBehavior = ::AppInstaller::Repository::CompositeSearchBehavior::AllPackages;
                break;
            }

            // TODO: Checks in the AppInstaller.cpp enforce one external source and one Installed source. Need to call AddAdditionalSource to enable multiple external sources.
            std::shared_ptr<::AppInstaller::Repository::ISource> installedSource = ::AppInstaller::Repository::OpenPredefinedSource(::AppInstaller::Repository::PredefinedSource::Installed, progress);
            // Create the composite source from the two.
            m_source = ::AppInstaller::Repository::CreateCompositeSource(installedSource, nonLocalSource, searchBehavior);
        }
        else
        {
            m_source = ::AppInstaller::Repository::OpenSource(winrt::to_string(m_catalogId), progress).Source;
        }
        co_return;
    }
    Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::FindPackagesResult> AppCatalog::FindPackagesAsync(Microsoft::Management::Deployment::FindPackagesOptions options)
    {
        ::AppInstaller::Repository::SearchRequest searchRequest;
        for (int i = 0; i < options.Filters().Size(); ++i)
        {
            Microsoft::Management::Deployment::PackageMatchFilter filter = options.Filters().GetAt(0);

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

            if (filter.Field() == Microsoft::Management::Deployment::PackageMatchField::AppCatalogDefined)
            {
                searchRequest.Query = ::AppInstaller::Repository::RequestMatch(matchType, winrt::to_string(filter.Value()));
            }
            else if (filter.IsAdditive())
            {
                searchRequest.Inclusions.emplace_back(::AppInstaller::Repository::PackageMatchFilter(matchField, matchType, winrt::to_string(filter.Value())));
            }
            else 
            {
                searchRequest.Filters.emplace_back(::AppInstaller::Repository::PackageMatchFilter(matchField, matchType, winrt::to_string(filter.Value())));
            }

        }
        auto searchResult = m_source->Search(searchRequest);

        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::ResultMatch> matches{ winrt::single_threaded_vector<Microsoft::Management::Deployment::ResultMatch>() };

        for (size_t i = 0; i < searchResult.Matches.size(); ++i)
        {
            auto match = searchResult.Matches[i]; 
            Microsoft::Management::Deployment::CatalogPackage catalogPackage{ nullptr };
            catalogPackage = winrt::make<winrt::Microsoft::Management::Deployment::implementation::CatalogPackage>(match.Package);

            winrt::Microsoft::Management::Deployment::PackageMatchFilter packageMatchFilter{ nullptr };
            packageMatchFilter = winrt::make<winrt::Microsoft::Management::Deployment::implementation::PackageMatchFilter>();

            winrt::Microsoft::Management::Deployment::ResultMatch resultMatch{ nullptr };
            resultMatch = winrt::make<winrt::Microsoft::Management::Deployment::implementation::ResultMatch>(catalogPackage, packageMatchFilter);

            matches.Append(resultMatch);
        }
        winrt::Microsoft::Management::Deployment::FindPackagesResult findPackagesResult{ nullptr };
        findPackagesResult = winrt::make<winrt::Microsoft::Management::Deployment::implementation::FindPackagesResult>(matches);

        co_return findPackagesResult;
    }
}
