#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "PackageCatalogReference.h"
#include "PackageCatalogReference.g.cpp"
#include "PackageCatalogInfo.h"
#include "PackageCatalog.h"
#include "ConnectResult.h"
#include "Converters.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageCatalogReference::Initialize(
        ::AppInstaller::Repository::SourceDetails sourceDetails,
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo)
    {
        m_sourceDetails = sourceDetails;
        m_info = packageCatalogInfo;
    }
    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions options)
    {
        m_compositePackageCatalogOptions = options;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogInfo PackageCatalogReference::Info()
    {
        return m_info;
    }
    winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> PackageCatalogReference::ConnectAsync()
    {
        co_return Connect();
    }
    winrt::Microsoft::Management::Deployment::ConnectResult PackageCatalogReference::Connect()
    {
        ::AppInstaller::ProgressCallback progress;
        if (m_compositePackageCatalogOptions)
        {
            std::vector<std::shared_ptr<::AppInstaller::Repository::ISource>> remoteSources;

            for (uint32_t i = 0; i < m_compositePackageCatalogOptions.Catalogs().Size(); ++i)
            {
                auto catalog = m_compositePackageCatalogOptions.Catalogs().GetAt(i);
                std::shared_ptr<::AppInstaller::Repository::ISource> remoteSource = ::AppInstaller::Repository::OpenSourceByIdentifier(winrt::to_string(catalog.Info().Id()), progress).Source;
                if (!remoteSource)
                {
                    // In the time since the catalog reference was given back the source must have been deleted.
                    auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
                    connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::CatalogNotFound, nullptr);
                    return *connectResult;
                }
                remoteSources.emplace_back(remoteSource);
            }
            ::AppInstaller::Repository::CompositeSearchBehavior searchBehavior = GetRepositoryCompositeSearchBehavior(m_compositePackageCatalogOptions.CompositeSearchBehavior());
           
            std::shared_ptr<::AppInstaller::Repository::ISource> installedSource;
            if (m_compositePackageCatalogOptions.LocalPackageCatalog())
            {
                // Only one type of local package catalog is currently allowed, so if it got past the argument checks it must be the Installed catalog.
                // TODO: this will need to be updated when adding the Installing catalog.
                installedSource = ::AppInstaller::Repository::OpenPredefinedSource(::AppInstaller::Repository::PredefinedSource::Installed, progress);
            }
            
            // Create the composite source.
            m_source = ::AppInstaller::Repository::CreateCompositeSource(installedSource, remoteSources, searchBehavior);
        }
        else
        {
            m_source = ::AppInstaller::Repository::OpenSource(winrt::to_string(m_info.Name()), progress).Source;
        }

        if (!m_source)
        {
            // In the time since the catalog reference was given back the source must have been deleted.
            auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
            connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::CatalogNotFound, nullptr);
            return *connectResult;
        }

        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        auto packageCatalog = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalog>>();
        packageCatalog->Initialize(m_info, m_source);
        connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::Ok, *packageCatalog);
        return *connectResult;
    }
}
