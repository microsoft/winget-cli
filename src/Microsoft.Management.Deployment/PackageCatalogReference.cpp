// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/RepositorySource.h>
#include "PackageCatalogReference.h"
#include "PackageCatalogReference.g.cpp"
#include "PackageCatalogInfo.h"
#include "PackageCatalog.h"
#include "SourceAgreement.h"
#include "ConnectResult.h"
#include "AuthenticationInfo.h"
#include "Workflows/WorkflowBase.h"
#include "Converters.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include <wil\cppwinrt_wrl.h>
#include <winget/GroupPolicy.h>
#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
#include <winget/UserSettings.h>
#include <Helpers.h>
#include <ExecutionContext.h>
#include <RefreshPackageCatalogResult.h>
#include <PackageCatalogProgress.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    namespace
    {
        winrt::Microsoft::Management::Deployment::RefreshPackageCatalogResult GetRefreshPackageCatalogResult(winrt::hresult terminationStatus)
        {
            winrt::Microsoft::Management::Deployment::RefreshPackageCatalogStatus status = GetPackageCatalogOperationStatus<RefreshPackageCatalogStatus>(terminationStatus);
            auto updateResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::RefreshPackageCatalogResult>>();
            updateResult->Initialize(status, terminationStatus);
            return *updateResult;
        }
    }

    void PackageCatalogReference::Initialize(winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo, ::AppInstaller::Repository::Source sourceReference)
    {
        m_info = packageCatalogInfo;
        m_sourceReference = std::move(sourceReference);
        m_packageCatalogBackgroundUpdateInterval = ::AppInstaller::Settings::User().Get<::AppInstaller::Settings::Setting::AutoUpdateTimeInMinutes>();

        if (IsBackgroundProcessForPolicy())
        {
            // Delay the default update interval for these background processes
            static constexpr winrt::Windows::Foundation::TimeSpan s_PackageCatalogUpdateIntervalDelay_Base = 168h; //1 week

            // Add a bit of randomness to the default interval time
            std::default_random_engine randomEngine(std::random_device{}());
            std::uniform_int_distribution<long long> distribution(0, 604800);

            m_packageCatalogBackgroundUpdateInterval = s_PackageCatalogUpdateIntervalDelay_Base + std::chrono::seconds(distribution(randomEngine));

            // Prevent any update / data processing by default for these background processes for now
            m_installedPackageInformationOnly = m_sourceReference.IsWellKnownSource(AppInstaller::Repository::WellKnownSource::WinGet);
        }
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
    winrt::Microsoft::Management::Deployment::ConnectResult GetConnectCatalogErrorResult(hresult hr)
    {
        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::CatalogError, nullptr, hr);
        return *connectResult;
    }
    winrt::Microsoft::Management::Deployment::ConnectResult GetConnectSourceAgreementsNotAcceptedErrorResult()
    {
        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::SourceAgreementsNotAccepted, nullptr, APPINSTALLER_CLI_ERROR_SOURCE_AGREEMENTS_NOT_ACCEPTED);
        return *connectResult;
    }
    winrt::Microsoft::Management::Deployment::ConnectResult PackageCatalogReference::Connect() try
    {
        HRESULT hr = EnsureComCallerHasCapability(Capability::PackageQuery);
        if (FAILED(hr))
        {
            // TODO: When more error codes are added, this should go back as something other than CatalogError.
            return GetConnectCatalogErrorResult(hr);
        }

        std::string callerName = GetCallerName();

        ::AppInstaller::ProgressCallback progress;
        ::AppInstaller::Repository::Source source;
        if (m_compositePackageCatalogOptions)
        {
            std::vector<::AppInstaller::Repository::Source> remoteSources;

            for (uint32_t i = 0; i < m_compositePackageCatalogOptions.Catalogs().Size(); ++i)
            {
                auto catalog = m_compositePackageCatalogOptions.Catalogs().GetAt(i);
                if (!catalog.AcceptSourceAgreements() && catalog.SourceAgreements().Size() != 0)
                {
                    return GetConnectSourceAgreementsNotAcceptedErrorResult();
                }

                winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference* catalogImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>(catalog);
                auto copy = catalogImpl->m_sourceReference;
                copy.SetCaller(callerName);
                copy.SetBackgroundUpdateInterval(catalog.PackageCatalogBackgroundUpdateInterval());
                copy.InstalledPackageInformationOnly(catalog.InstalledPackageInformationOnly());
                if (catalog.AuthenticationInfo().AuthenticationType() != winrt::Microsoft::Management::Deployment::AuthenticationType::None)
                {
                    copy.SetAuthenticationArguments(GetAuthenticationArguments(catalog.AuthenticationArguments()));
                }
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
            if (!AcceptSourceAgreements() && SourceAgreements().Size() != 0)
            {
                return GetConnectSourceAgreementsNotAcceptedErrorResult();
            }

            source = m_sourceReference;
            source.SetCaller(callerName);
            source.SetBackgroundUpdateInterval(PackageCatalogBackgroundUpdateInterval());
            source.InstalledPackageInformationOnly(m_installedPackageInformationOnly);
            if (AuthenticationInfo().AuthenticationType() != winrt::Microsoft::Management::Deployment::AuthenticationType::None)
            {
                source.SetAuthenticationArguments(GetAuthenticationArguments(m_authenticationArguments));
            }
            source.Open(progress);
        }

        if (!source)
        {
            // We call `Open` on each individual source above, meaning that they should throw any error that occurs.
            // If the source is still not open at this point it is a bug.
            return GetConnectCatalogErrorResult(E_UNEXPECTED);
        }

        // Have to make another package catalog info because source->GetDetails has more fields than m_info does.
        // Specifically, Rest sources do not have the Ids filled in m_info since they only get the id from the rest server after being Opened.
        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(source.GetDetails());
        auto connectResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::ConnectResult>>();
        auto packageCatalog = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalog>>();
        packageCatalog->Initialize(*packageCatalogInfo, source, (m_compositePackageCatalogOptions != nullptr));
        connectResult->Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus::Ok, *packageCatalog, S_OK);
        return *connectResult;
    }
    catch (...)
    {
        return GetConnectCatalogErrorResult(AppInstaller::CLI::Workflow::HandleException(nullptr, std::current_exception()));
    }

    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::SourceAgreement> PackageCatalogReference::SourceAgreements()
    {
        std::call_once(m_sourceAgreementsOnceFlag,
            [&]()
            {
                if (!IsComposite())
                {
                    for (auto const& agreement : m_sourceReference.GetInformation().SourceAgreements)
                    {
                        auto sourceAgreement = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::SourceAgreement>>();
                        sourceAgreement->Initialize(agreement);
                        m_sourceAgreements.Append(*sourceAgreement);
                    }
                }
            });
        return m_sourceAgreements.GetView();
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
    void PackageCatalogReference::AcceptSourceAgreements(bool value)
    {
        if (IsComposite())
        {
            // Can't set AcceptSourceAgreements on a composite. Callers should set it on each non-composite PackageCatalogReference in the composite.
            throw winrt::hresult_illegal_state_change();
        }
        m_acceptSourceAgreements = value;
    }
    bool PackageCatalogReference::AcceptSourceAgreements()
    {
        return m_acceptSourceAgreements;
    }

    void PackageCatalogReference::PackageCatalogBackgroundUpdateInterval(winrt::Windows::Foundation::TimeSpan const& value)
    {
        if (IsComposite())
        {
            // Can't set PackageCatalogBackgroundUpdateInterval on a composite. Callers should set it on each non-composite PackageCatalogReference in the composite.
            throw winrt::hresult_illegal_state_change();
        }
        m_packageCatalogBackgroundUpdateInterval = value;
    }
    winrt::Windows::Foundation::TimeSpan PackageCatalogReference::PackageCatalogBackgroundUpdateInterval()
    {
        return m_packageCatalogBackgroundUpdateInterval;
    }

    bool PackageCatalogReference::InstalledPackageInformationOnly()
    {
        return m_installedPackageInformationOnly;
    }

    void PackageCatalogReference::InstalledPackageInformationOnly(bool value)
    {
        if (IsComposite())
        {
            throw winrt::hresult_illegal_state_change();
        }

        m_installedPackageInformationOnly = value;
    }
    winrt::Microsoft::Management::Deployment::AuthenticationArguments PackageCatalogReference::AuthenticationArguments()
    {
        return m_authenticationArguments;
    }
    void PackageCatalogReference::AuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments const& value)
    {
        if (IsComposite())
        {
            throw winrt::hresult_illegal_state_change();
        }

        m_authenticationArguments = value;
    }
    winrt::Microsoft::Management::Deployment::AuthenticationInfo PackageCatalogReference::AuthenticationInfo()
    {
        std::call_once(m_authenticationInfoOnceFlag,
            [&]()
            {
                if (!IsComposite())
                {
                    auto authenticationInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AuthenticationInfo>>();
                    authenticationInfo->Initialize(m_sourceReference.GetInformation().Authentication);
                    m_authenticationInfo = *authenticationInfo;
                }
            });
        return m_authenticationInfo;
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::RefreshPackageCatalogResult, double> PackageCatalogReference::RefreshPackageCatalogAsync()
    {
        HRESULT terminationHR = S_OK;
        try {
            // Check for permissions and get caller info for telemetry
            THROW_IF_FAILED(EnsureComCallerHasCapability(Capability::PackageQuery));

            auto strong_this = get_strong();
            auto report_progress{ co_await winrt::get_progress_token() };
            co_await winrt::resume_background();

            auto packageCatalogProgressSink = winrt::Microsoft::Management::Deployment::ProgressSinkFactory::CreatePackageCatalogProgressSink(this->m_sourceReference.GetDetails().Type, report_progress);

            packageCatalogProgressSink->BeginProgress();
            ::AppInstaller::ProgressCallback progress(packageCatalogProgressSink.get());
            this->m_sourceReference.Update(progress);
            packageCatalogProgressSink->EndProgress(false);
        }
        catch (...)
        {
            terminationHR = AppInstaller::CLI::Workflow::HandleException(nullptr, std::current_exception());
        }

        co_return GetRefreshPackageCatalogResult(terminationHR);
    }
}
