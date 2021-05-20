#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "AppCatalogReference.h"
#include "AppCatalogReference.g.cpp"
#include "AppCatalogInfo.h"
#include "AppCatalog.h"
#include "ConnectResult.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void AppCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::AppCatalogInfo appCatalogInfo)
    {
        m_catalogId = appCatalogInfo.Name();
        m_info = appCatalogInfo;
    }
    void AppCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::PredefinedAppCatalog predefinedAppCatalog,
        winrt::Microsoft::Management::Deployment::AppCatalogInfo appCatalogInfo)
    {
        m_isPredefinedSource = true;
        m_predefinedAppCatalog = predefinedAppCatalog;
        m_info = appCatalogInfo;
    }
    void AppCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::LocalAppCatalog localAppCatalog,
        winrt::Microsoft::Management::Deployment::AppCatalogInfo appCatalogInfo)
    {
        m_isLocalSource = true;
        m_localAppCatalog = localAppCatalog;
        m_info = appCatalogInfo;
    }
    void AppCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::CreateCompositeAppCatalogOptions options)
    {
        m_isCompositeSource = true;
        m_compositeAppCatalogOptions = options;
    }
    winrt::Microsoft::Management::Deployment::AppCatalogInfo AppCatalogReference::Info()
    {
        return m_info;
    }
    winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> AppCatalogReference::ConnectAsync()
    {
        co_return Connect();
    }
    winrt::Microsoft::Management::Deployment::ConnectResult AppCatalogReference::Connect()
    {
        ::AppInstaller::ProgressCallback progress;
        if (m_isPredefinedSource)
        {
            switch (m_predefinedAppCatalog)
            {
            case Microsoft::Management::Deployment::PredefinedAppCatalog::OpenWindowsCatalog:
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
            switch (m_localAppCatalog)
            {
            case Microsoft::Management::Deployment::LocalAppCatalog::InstalledPackages:
                predefinedSource = ::AppInstaller::Repository::PredefinedSource::Installed;
                m_source = ::AppInstaller::Repository::OpenPredefinedSource(predefinedSource, progress);
                break;
            case Microsoft::Management::Deployment::LocalAppCatalog::InstallingPackages:
                // TODO - installing source does not exist yet.
                throw hresult_not_implemented();
            default:
                throw hresult_invalid_argument();
            }
        }
        else if (m_compositeAppCatalogOptions)
        {
            std::shared_ptr<::AppInstaller::Repository::ISource> nonLocalSource;
            for (uint32_t i = 0; i < m_compositeAppCatalogOptions.Catalogs().Size(); ++i)
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

        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        auto appCatalog = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>>();
        appCatalog->Initialize(Info(), m_source);
        connectResult->Initialize(S_OK, *appCatalog);
        return *connectResult;
    }
}
