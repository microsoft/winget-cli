// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "PackageCatalogReference.h"
#include "PackageCatalogReference.g.cpp"
#include "PackageCatalogInfo.h"
#include "PackageCatalog.h"
#include "ConnectResult.h"
#include "Workflows/WorkflowBase.h"
#include "Converters.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include <wil\cppwinrt_wrl.h>
#include <winget/GroupPolicy.h>
#include <AppInstallerErrors.h>
#include <Helpers.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo)
    {
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
    winrt::Microsoft::Management::Deployment::ConnectResult GetConnectCatalogErrorResult()
    {
        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::CatalogError, nullptr);
        return *connectResult;
    }
    winrt::Microsoft::Management::Deployment::ConnectResult PackageCatalogReference::Connect()
    {
        try
        {
            if (FAILED(EnsureComCallerHasCapability(Capability::PackageQuery)))
            {
                // TODO: When more error codes are added, this should go back as something other than CatalogError.
                return GetConnectCatalogErrorResult();
            }

            ::AppInstaller::ProgressCallback progress;
            std::shared_ptr<::AppInstaller::Repository::ISource> source;
            if (m_compositePackageCatalogOptions)
            {
                std::vector<std::shared_ptr<::AppInstaller::Repository::ISource>> remoteSources;

                for (uint32_t i = 0; i < m_compositePackageCatalogOptions.Catalogs().Size(); ++i)
                {
                    auto catalog = m_compositePackageCatalogOptions.Catalogs().GetAt(i);
                    winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo* catalogInfoImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>(catalog.Info());
                    ::AppInstaller::Repository::SourceDetails sourceDetails = catalogInfoImpl->GetSourceDetails();
                    std::shared_ptr<::AppInstaller::Repository::ISource> remoteSource = ::AppInstaller::Repository::OpenSourceFromDetails(sourceDetails, progress).Source;
                    if (!remoteSource)
                    {
                        // If source is null, return the error. There's no way to get the hresult that caused the error right now.
                        return GetConnectCatalogErrorResult();
                    }
                    remoteSources.emplace_back(std::move(remoteSource));
                }
                ::AppInstaller::Repository::CompositeSearchBehavior searchBehavior = GetRepositoryCompositeSearchBehavior(m_compositePackageCatalogOptions.CompositeSearchBehavior());

                std::shared_ptr<::AppInstaller::Repository::ISource> installedSource;
                // Check if search behavior indicates that the caller does not want to do local correlation.
                if (m_compositePackageCatalogOptions.CompositeSearchBehavior() != Microsoft::Management::Deployment::CompositeSearchBehavior::RemotePackagesFromRemoteCatalogs)
                {
                    installedSource = ::AppInstaller::Repository::OpenPredefinedSource(::AppInstaller::Repository::PredefinedSource::Installed, progress);
                }

                // Create the composite source.
                source = ::AppInstaller::Repository::CreateCompositeSource(installedSource, remoteSources, searchBehavior);
            }
            else
            {
                winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo* catalogInfoImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>(m_info);
                ::AppInstaller::Repository::SourceDetails sourceDetails = catalogInfoImpl->GetSourceDetails();
                source = ::AppInstaller::Repository::OpenSourceFromDetails(sourceDetails, progress).Source;
            }

            if (!source)
            {
                // If source is null, return the error. There's no way to get the hresult that caused the error right now.
                return GetConnectCatalogErrorResult();
            }

            // Have to make another package catalog info because source->GetDetails has more fields than m_info does.
            // Specifically, Rest sources do not have the Ids filled in m_info since they only get the id from the rest server after being Opened.
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(source->GetDetails());
            auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
            auto packageCatalog = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalog>>();
            packageCatalog->Initialize(*packageCatalogInfo, source, (m_compositePackageCatalogOptions != nullptr));
            connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::Ok, *packageCatalog);
            return *connectResult;
        }
        catch (...)
        {
        }
        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::CatalogError, nullptr);
        return *connectResult;
    }
}
