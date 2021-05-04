#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "AppCatalog.h"
#include "AppCatalog.g.cpp"
#include "AppCatalogInfo.h"
#include "FindPackagesResult.h"
#include "ResultMatch.h"
#include "CatalogPackage.h"
#include "PackageMatchFilter.h"

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
        else
        {
            m_source = ::AppInstaller::Repository::OpenSource(winrt::to_string(m_catalogId), progress).Source;
        }
        co_return;
    }
    Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::FindPackagesResult> AppCatalog::FindPackagesAsync(Microsoft::Management::Deployment::FindPackagesOptions options)
    {
        ::AppInstaller::Repository::MatchType matchType = ::AppInstaller::Repository::MatchType::StartsWith;

        ::AppInstaller::Repository::SearchRequest searchRequest;
        hstring queryValue = options.Filters().GetAt(0).Value();
        std::string query = winrt::to_string(queryValue);
        searchRequest.Inclusions.emplace_back(::AppInstaller::Repository::PackageMatchFilter(::AppInstaller::Repository::PackageMatchField::Id, matchType, query));

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
