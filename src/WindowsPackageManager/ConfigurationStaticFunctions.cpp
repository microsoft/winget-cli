// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
#include <Unknwn.h>
#include <wil\cppwinrt_wrl.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <ComClsids.h>
#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
#include <winget/ConfigurationSetProcessorHandlers.h>
#include <ConfigurationSetProcessorFactoryRemoting.h>
#include <winget/ILifetimeWatcher.h>
#include <winget/GroupPolicy.h>
#include <winget/Security.h>

namespace ConfigurationShim
{
    CLSID CLSID_ConfigurationObjectLifetimeWatcher = { 0x89a8f1d4,0x1e24,0x46a4,{0x9f,0x6c,0x65,0x78,0xb0,0x47,0xf2,0xf7} };

    struct
    DECLSPEC_UUID("89a8f1d4-1e24-46a4-9f6c-6578b047f2f7")
    ConfigurationObjectLifetimeWatcher : winrt::implements<ConfigurationObjectLifetimeWatcher, IUnknown>
    {
    };

    struct
    DECLSPEC_UUID(WINGET_OUTOFPROC_COM_CLSID_ConfigurationStaticFunctions)
    ConfigurationStaticFunctionsShim : winrt::implements<ConfigurationStaticFunctionsShim, winrt::Microsoft::Management::Configuration::IConfigurationStatics>
    {
        ConfigurationStaticFunctionsShim() = default;

        winrt::Microsoft::Management::Configuration::ConfigurationUnit CreateConfigurationUnit()
        {
            auto result = m_statics.CreateConfigurationUnit();
            result.as<AppInstaller::WinRT::ILifetimeWatcher>()->SetLifetimeWatcher(CreateLifetimeWatcher());
            return result;
        }

        winrt::Microsoft::Management::Configuration::ConfigurationSet CreateConfigurationSet()
        {
            auto result = m_statics.CreateConfigurationSet();
            result.as<AppInstaller::WinRT::ILifetimeWatcher>()->SetLifetimeWatcher(CreateLifetimeWatcher());
            return result;
        }

        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory> CreateConfigurationSetProcessorFactoryAsync(winrt::hstring const& handler)
        {
            auto strong_this{ get_strong() };
            std::wstring lowerHandler = AppInstaller::Utility::ToLower(handler);

            co_await winrt::resume_background();

            winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory result;

            if (lowerHandler == AppInstaller::Configuration::PowerShellHandlerIdentifier)
            {
                result = AppInstaller::CLI::ConfigurationRemoting::CreateOutOfProcessFactory();
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
            auto result = m_statics.CreateConfigurationProcessor(factory);
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

        winrt::Microsoft::Management::Configuration::ConfigurationStaticFunctions m_statics;
    };

    // Enable custom code to run before creating any object through the factory.
    template <typename TCppWinRTClass>
    class ConfigurationFactory : public ::wil::wrl_factory_for_winrt_com_class<TCppWinRTClass>
    {
    public:
        IFACEMETHODIMP CreateInstance(_In_opt_::IUnknown* unknownOuter, REFIID riid, _COM_Outptr_ void** object) noexcept try
        {
            *object = nullptr;
            // TODO: Review of policies for configuration
            RETURN_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, !::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::WinGet));
            // TODO: Review of security for configuration OOP
            RETURN_HR_IF(E_ACCESSDENIED, !::AppInstaller::Security::IsCOMCallerSameUserAndIntegrityLevel());

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
