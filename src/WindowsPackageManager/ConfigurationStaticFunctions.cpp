// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
#include <Unknwn.h>
#include <wil\cppwinrt_wrl.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <winrt/Microsoft.Management.Deployment.h>
#include <ComClsids.h>
#include <AppInstallerErrors.h>
#include <AppInstallerFileLogger.h>
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerStrings.h>
#include <winget/ConfigurationSetProcessorHandlers.h>
#include <ConfigurationSetProcessorFactoryRemoting.h>
#include <winget/ILifetimeWatcher.h>
#include <winget/IConfigurationStaticsInternals.h>
#include <winget/GroupPolicy.h>
#include <winget/Security.h>
#include <winget/ThreadGlobals.h>
#include <winget/SelfManagement.h>
#include <winget/MSStore.h>
#include <winget/ExperimentalFeature.h>

using namespace AppInstaller::SelfManagement;
using namespace winrt::Microsoft::Management::Deployment;
using namespace winrt::Windows::Foundation::Collections;

namespace ConfigurationShim
{
    namespace
    {
        static std::atomic_bool s_canBeCreated{ true };
    }

    CLSID CLSID_ConfigurationObjectLifetimeWatcher = { 0x89a8f1d4,0x1e24,0x46a4,{0x9f,0x6c,0x65,0x78,0xb0,0x47,0xf2,0xf7} };

    struct
    DECLSPEC_UUID("89a8f1d4-1e24-46a4-9f6c-6578b047f2f7")
    ConfigurationObjectLifetimeWatcher : winrt::implements<ConfigurationObjectLifetimeWatcher, IUnknown>
    {
    };

    struct
    DECLSPEC_UUID(WINGET_OUTOFPROC_COM_CLSID_ConfigurationStaticFunctions)
    ConfigurationStaticFunctionsShim : winrt::implements<ConfigurationStaticFunctionsShim,
        winrt::Microsoft::Management::Configuration::IConfigurationStatics,
        winrt::Microsoft::Management::Configuration::IConfigurationStatics2>
    {
        ConfigurationStaticFunctionsShim()
        {
            auto threadGlobalsRestore = m_threadGlobals.SetForCurrentThread();
            auto& diagnosticsLogger = m_threadGlobals.GetDiagnosticLogger();
            diagnosticsLogger.EnableChannel(AppInstaller::Logging::Channel::All);
            diagnosticsLogger.SetLevel(AppInstaller::Logging::Level::Verbose);
            diagnosticsLogger.AddLogger(std::make_unique<AppInstaller::Logging::FileLogger>("ConfigStatics"sv));

            if (IsConfigurationAvailable())
            {
                m_statics = winrt::Microsoft::Management::Configuration::ConfigurationStaticFunctions().as<winrt::Microsoft::Management::Configuration::IConfigurationStatics2>();

                // Forward the current feature state to the internal statics
                using namespace AppInstaller;
                using Flags = WinRT::ConfigurationStaticsInternalsStateFlags;

                Flags flags = Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Configuration03) ? Flags::Configuration03 : Flags::None;
                m_statics.as<AppInstaller::WinRT::IConfigurationStaticsInternals>()->SetExperimentalState(ToIntegral(flags));
            }
        }

        winrt::Microsoft::Management::Configuration::ConfigurationUnit CreateConfigurationUnit()
        {
            THROW_HR_IF(CO_E_CLASS_DISABLED, !s_canBeCreated);

            if (!m_statics)
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_PACKAGE_IS_STUB);
            }

            auto result = m_statics.CreateConfigurationUnit();
            result.as<AppInstaller::WinRT::ILifetimeWatcher>()->SetLifetimeWatcher(CreateLifetimeWatcher());
            return result;
        }

        winrt::Microsoft::Management::Configuration::ConfigurationSet CreateConfigurationSet()
        {
            THROW_HR_IF(CO_E_CLASS_DISABLED, !s_canBeCreated);

            if (!m_statics)
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_PACKAGE_IS_STUB);
            }

            auto result = m_statics.CreateConfigurationSet();
            result.as<AppInstaller::WinRT::ILifetimeWatcher>()->SetLifetimeWatcher(CreateLifetimeWatcher());
            return result;
        }

        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory> CreateConfigurationSetProcessorFactoryAsync(winrt::hstring const& handler)
        {
            THROW_HR_IF(CO_E_CLASS_DISABLED, !s_canBeCreated);

            auto strong_this{ get_strong() };
            std::wstring lowerHandler = AppInstaller::Utility::ToLower(handler);

            co_await winrt::resume_background();

            auto threadGlobalsRestore = m_threadGlobals.SetForCurrentThread();
            winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory result;

            if (lowerHandler == AppInstaller::Configuration::PowerShellHandlerIdentifier)
            {
                result = AppInstaller::CLI::ConfigurationRemoting::CreateOutOfProcessFactory();
            }
            else if (lowerHandler == AppInstaller::Configuration::DynamicRuntimeHandlerIdentifier)
            {
                result = AppInstaller::CLI::ConfigurationRemoting::CreateDynamicRuntimeFactory();
            }

            if (result)
            {
                // Objects returned here *must* implement ILifetimeWatcher for now.
                // If we create OOP objects implemented elsewhere in the future, decide then how to exempt those while still ensuring we
                // don't accidentally create a lifetime bug by basing it solely off the QI result.
                result.as<AppInstaller::WinRT::ILifetimeWatcher>()->SetLifetimeWatcher(CreateLifetimeWatcher());
                co_return result;
            }

            AICLI_LOG(Config, Error, << "Unknown handler in CreateConfigurationSetProcessorFactory: " << AppInstaller::Utility::ConvertToUTF8(handler));
            THROW_HR(E_NOT_SET);
        }

        winrt::Microsoft::Management::Configuration::ConfigurationProcessor CreateConfigurationProcessor(winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory const& factory)
        {
            THROW_HR_IF(CO_E_CLASS_DISABLED, !s_canBeCreated);

            if (!m_statics)
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_PACKAGE_IS_STUB);
            }

            auto result = m_statics.CreateConfigurationProcessor(factory);
            result.as<AppInstaller::WinRT::ILifetimeWatcher>()->SetLifetimeWatcher(CreateLifetimeWatcher());
            return result;
        }

        bool IsConfigurationAvailable()
        {
            return !IsStubPackage();
        }

        winrt::Windows::Foundation::IAsyncActionWithProgress<uint32_t> EnsureConfigurationAvailableAsync()
        {
            THROW_HR_IF(CO_E_CLASS_DISABLED, !s_canBeCreated);

            if (IsConfigurationAvailable())
            {
                return;
            }

            auto strong_this{ get_strong() };
            co_await winrt::resume_background();

            SetStubPreferred(false);

            PackageManager packageManager;
            PackageCatalogReference catalogRef{ packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog::MicrosoftStore) };
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR, !catalogRef);

            ConnectResult connectResult = catalogRef.Connect();
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED, connectResult.Status() != ConnectResultStatus::Ok);

            PackageCatalog catalog = connectResult.PackageCatalog();

            FindPackagesOptions findPackagesOptions;
            PackageMatchFilter filter;
            filter.Field(PackageMatchField::Id);
            filter.Option(PackageFieldMatchOption::Equals);
            filter.Value(AppInstaller::MSStore::s_AppInstallerProductId);
            findPackagesOptions.Filters().Append(filter);

            FindPackagesResult findPackagesResult{ catalog.FindPackages(findPackagesOptions) };

            auto matches = findPackagesResult.Matches();
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_MISSING_PACKAGE, matches.Size() == 0);
            auto catalogPackage = matches.GetAt(0).CatalogPackage();

            InstallOptions installOptions;
            installOptions.AcceptPackageAgreements(true);
            installOptions.AllowUpgradeToUnknownVersion(true);
            installOptions.Force(true);

            auto progress = co_await winrt::get_progress_token();

            // Don't use UpgradePackageAsync, we don't support upgrade for packages from the msstore
            // it has to be install and internally we know is an update.
            auto installTask = packageManager.InstallPackageAsync(catalogPackage, installOptions);
            installTask.Progress([progress](auto const&, InstallProgress installProgress)
            {
                if (installProgress.State == PackageInstallProgressState::Downloading && installProgress.BytesRequired != 0)
                {
                    progress((uint32_t)(installProgress.DownloadProgress * 80));
                }
                else if (installProgress.State == PackageInstallProgressState::Installing)
                {
                    progress(((uint32_t)installProgress.InstallationProgress * 20) + 80);
                }
            });

            co_await installTask;
            s_canBeCreated = false;
        }

        winrt::Microsoft::Management::Configuration::ConfigurationParameter CreateConfigurationParameter()
        {
            THROW_HR_IF(CO_E_CLASS_DISABLED, !s_canBeCreated);

            if (!m_statics)
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_PACKAGE_IS_STUB);
            }

            auto result = m_statics.CreateConfigurationParameter();
            result.as<AppInstaller::WinRT::ILifetimeWatcher>()->SetLifetimeWatcher(CreateLifetimeWatcher());
            return result;
        }

    private:
        // Returns a lifetime watcher object that is currently *unowned*.
        IUnknown* CreateLifetimeWatcher()
        {
            ::Microsoft::WRL::ComPtr<IClassFactory> factory;
            THROW_IF_FAILED(::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc>::GetModule().GetClassObject(CLSID_ConfigurationObjectLifetimeWatcher, IID_PPV_ARGS(&factory)));
            winrt::com_ptr<IUnknown> out;
            THROW_IF_FAILED(factory->CreateInstance(nullptr, __uuidof(IUnknown), out.put_void()));
            return out.detach();
        }

        winrt::Microsoft::Management::Configuration::IConfigurationStatics2 m_statics = nullptr;
        AppInstaller::ThreadLocalStorage::WingetThreadGlobals m_threadGlobals;
    };

    // Enable custom code to run before creating any object through the factory.
    template <typename TCppWinRTClass>
    class ConfigurationFactory : public ::wil::wrl_factory_for_winrt_com_class<TCppWinRTClass>
    {
    public:
        IFACEMETHODIMP CreateInstance(_In_opt_::IUnknown* unknownOuter, REFIID riid, _COM_Outptr_ void** object) noexcept try
        {
            *object = nullptr;
            RETURN_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, !::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::WinGet));
            RETURN_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, !::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::Configuration));
            RETURN_HR_IF(E_ACCESSDENIED, !::AppInstaller::Security::IsCOMCallerSameUserAndIntegrityLevel());

            RETURN_HR_IF(CO_E_CLASS_DISABLED, !s_canBeCreated);

            return ::wil::wrl_factory_for_winrt_com_class<TCppWinRTClass>::CreateInstance(unknownOuter, riid, object);
        }
        CATCH_RETURN()
    };

#define CoCreatableMicrosoftManagementConfigurationClass(className) \
    CoCreatableClassWithFactory(className, ::ConfigurationShim::ConfigurationFactory<className>)

    // Disable 6388 as it seems to be falsely warning
#pragma warning(push)
#pragma warning(disable : 6388)
    CoCreatableCppWinRtClass(ConfigurationObjectLifetimeWatcher);
    CoCreatableMicrosoftManagementConfigurationClass(ConfigurationStaticFunctionsShim);
#pragma warning(pop)
}
