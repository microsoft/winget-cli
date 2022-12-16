// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/RepositorySource.h>
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
    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo, ::AppInstaller::Repository::Source sourceReference)
    {
        m_info = packageCatalogInfo;
        m_sourceReference = std::move(sourceReference);
    }
    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions options)
    {
        m_compositePackageCatalogOptions = options;
    }
    bool PackageCatalogReference::IsComposite()
    {
        return (m_compositePackageCatalogOptions != nullptr);
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
            ::AppInstaller::Repository::Source source;
            if (m_compositePackageCatalogOptions)
            {
                std::vector<::AppInstaller::Repository::Source> remoteSources;

                for (uint32_t i = 0; i < m_compositePackageCatalogOptions.Catalogs().Size(); ++i)
                {
                    auto catalog = m_compositePackageCatalogOptions.Catalogs().GetAt(i);
                    winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference* catalogImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>(catalog);
                    auto copy = catalogImpl->m_sourceReference;
                    copy.Open(progress);
                    remoteSources.emplace_back(std::move(copy));
                }

                // Create the aggregated source.
                source = ::AppInstaller::Repository::Source{ remoteSources };

                // Create composite with installed source if needed.
                ::AppInstaller::Repository::CompositeSearchBehavior searchBehavior = GetRepositoryCompositeSearchBehavior(m_compositePackageCatalogOptions.CompositeSearchBehavior());

                // Check if search behavior indicates that the caller does not want to do local correlation.
                if (m_compositePackageCatalogOptions.CompositeSearchBehavior() != Microsoft::Management::Deployment::CompositeSearchBehavior::RemotePackagesFromRemoteCatalogs)
                {
                    ::AppInstaller::Repository::Source installedSource;
                    auto manifestInstalledScope = GetManifestScope(m_compositePackageCatalogOptions.InstalledScope()).first;
                    if (manifestInstalledScope == ::AppInstaller::Manifest::ScopeEnum::User)
                    {
                        installedSource = ::AppInstaller::Repository::Source{ ::AppInstaller::Repository::PredefinedSource::InstalledUser };
                    }
                    else if (manifestInstalledScope == ::AppInstaller::Manifest::ScopeEnum::Machine)
                    {
                        installedSource = ::AppInstaller::Repository::Source{ ::AppInstaller::Repository::PredefinedSource::InstalledMachine };
                    }
                    else
                    {
                        installedSource = ::AppInstaller::Repository::Source{ ::AppInstaller::Repository::PredefinedSource::Installed };
                    }

                    installedSource.Open(progress);
                    source = ::AppInstaller::Repository::Source{ installedSource, source, searchBehavior };
                }
            }
            else
            {
                source = m_sourceReference;
                source.Open(progress);
            }

            if (!source)
            {
                // If source is null, return the error. There's no way to get the hresult that caused the error right now.
                return GetConnectCatalogErrorResult();
            }

            // Have to make another package catalog info because source->GetDetails has more fields than m_info does.
            // Specifically, Rest sources do not have the Ids filled in m_info since they only get the id from the rest server after being Opened.
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(source.GetDetails());
            auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
            auto packageCatalog = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalog>>();
            packageCatalog->Initialize(*packageCatalogInfo, source, (m_compositePackageCatalogOptions != nullptr));
            connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::Ok, *packageCatalog);
            return *connectResult;
        }
        catch (...)
        {
        }
        return GetConnectCatalogErrorResult();
    }

    hstring PackageCatalogReference::AdditionalPackageCatalogArguments()
    {
        if (!IsComposite())
        {
            if (m_additionalPackageCatalogArguments.has_value())
            {
                return winrt::to_hstring(m_additionalPackageCatalogArguments.value());
            }
        }

        return {};
    }
    void PackageCatalogReference::AdditionalPackageCatalogArguments(hstring const& value)
    {
        if (IsComposite())
        {
            // Can't set AdditionalPackageCatalogArguments on a composite. Callers should set it on each non-composite PackageCatalogReference in the composite.
            throw winrt::hresult_illegal_state_change();
        }
        else
        {
            m_additionalPackageCatalogArguments = ::AppInstaller::Utility::ConvertToUTF8(value);
            m_sourceReference.SetCustomHeader(m_additionalPackageCatalogArguments);
        }
    }
}
