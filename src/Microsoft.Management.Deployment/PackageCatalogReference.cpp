#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "PackageCatalogReference.h"
#include "PackageCatalogReference.g.cpp"
#include "PackageCatalogInfo.h"
#include "PackageCatalog.h"
#include "ConnectResult.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo)
    {
        m_catalogId = packageCatalogInfo.Name();
        m_info = packageCatalogInfo;
    }
    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog predefinedPackageCatalog,
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo)
    {
        m_isPredefinedSource = true;
        m_predefinedPackageCatalog = predefinedPackageCatalog;
        m_info = packageCatalogInfo;
    }
    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::LocalPackageCatalog localPackageCatalog,
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo)
    {
        m_isLocalSource = true;
        m_localPackageCatalog = localPackageCatalog;
        m_info = packageCatalogInfo;
    }
    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions options)
    {
        m_isCompositeSource = true;
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
        if (m_isPredefinedSource)
        {
            switch (m_predefinedPackageCatalog)
            {
            case Microsoft::Management::Deployment::PredefinedPackageCatalog::OpenWindowsCatalog:
                // TODO - creating source by enum or id is not supported yet.
                m_source = ::AppInstaller::Repository::OpenSource(winrt::to_string(L"winget"), progress).Source;
                break;
            default:
                throw hresult_invalid_argument();
            }
        }
        if (m_isLocalSource)
        {
            ::AppInstaller::Repository::PredefinedSource predefinedSource = ::AppInstaller::Repository::PredefinedSource::Installed;
            switch (m_localPackageCatalog)
            {
            case Microsoft::Management::Deployment::LocalPackageCatalog::InstalledPackages:
                predefinedSource = ::AppInstaller::Repository::PredefinedSource::Installed;
                m_source = ::AppInstaller::Repository::OpenPredefinedSource(predefinedSource, progress);
                break;
            case Microsoft::Management::Deployment::LocalPackageCatalog::InstallingPackages:
                // TODO - installing source does not exist yet.
                throw hresult_not_implemented();
            default:
                throw hresult_invalid_argument();
            }
        }
        else if (m_compositePackageCatalogOptions)
        {
            std::shared_ptr<::AppInstaller::Repository::ISource> nonLocalSource;
            for (uint32_t i = 0; i < m_compositePackageCatalogOptions.Catalogs().Size(); ++i)
            {
                auto catalog = m_compositePackageCatalogOptions.Catalogs().GetAt(i);
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
            switch (m_compositePackageCatalogOptions.CompositeSearchBehavior())
            {
            case Microsoft::Management::Deployment::CompositeSearchBehavior::AllLocalPackages:
                // TODO - installing source does not exist yet.
                throw hresult_not_implemented();
            case Microsoft::Management::Deployment::CompositeSearchBehavior::InstalledPackages:
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

            // TODO: Checks in the PackageInstaller.cpp enforce one external source and one Installed source. Need to call AddAdditionalSource to enable multiple external sources.
            std::shared_ptr<::AppInstaller::Repository::ISource> installedSource = ::AppInstaller::Repository::OpenPredefinedSource(::AppInstaller::Repository::PredefinedSource::Installed, progress);
            // Create the composite source from the two.
            m_source = ::AppInstaller::Repository::CreateCompositeSource(installedSource, nonLocalSource, searchBehavior);
        }
        else
        {
            m_source = ::AppInstaller::Repository::OpenSource(winrt::to_string(m_catalogId), progress).Source;
        }

        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        auto packageCatalog = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalog>>();
        packageCatalog->Initialize(Info(), m_source);
        connectResult->Initialize(S_OK, *packageCatalog);
        return *connectResult;
    }
}
