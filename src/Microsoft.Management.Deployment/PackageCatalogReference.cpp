// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
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
        m_isComposite = true;
    }
    bool PackageCatalogReference::IsComposite()
    {
        return m_isComposite;
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
                // TODO, cast this to the internal representation so that it can get the sourceDetails directly and use OpenSourceFromDetails.
                auto catalog = m_compositePackageCatalogOptions.Catalogs().GetAt(i);
                std::shared_ptr<::AppInstaller::Repository::ISource> remoteSource = ::AppInstaller::Repository::OpenSource(winrt::to_string(catalog.Info().Name()), progress).Source;
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
           
            std::shared_ptr<::AppInstaller::Repository::ISource> installedSource{};
            // Check if search behavior indicates that the caller does not want to do local correlation.
            if (m_compositePackageCatalogOptions.CompositeSearchBehavior() != Microsoft::Management::Deployment::CompositeSearchBehavior::RemotePackagesFromRemoteCatalogs)
            {
                installedSource = ::AppInstaller::Repository::OpenPredefinedSource(::AppInstaller::Repository::PredefinedSource::Installed, progress);
            }
            
            // Create the composite source.
            m_source = ::AppInstaller::Repository::CreateCompositeSource(installedSource, remoteSources, searchBehavior);
        }
        else
        {
            m_source = ::AppInstaller::Repository::OpenSourceFromDetails(m_sourceDetails, progress).Source;
        }

        // Have to make another package catalog info because source->GetDetails has more fields than m_info does.
        // Specifically, Rest sources do not have the Ids filled in m_info since they only get the id from the rest server after being Opened.
        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(m_source->GetDetails());
        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        auto packageCatalog = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalog>>();
        packageCatalog->Initialize(m_info, m_source, (m_compositePackageCatalogOptions != nullptr));
        connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::Ok, *packageCatalog);
        return *connectResult;
    }
}
