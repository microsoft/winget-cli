// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationProcessor.g.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include "ConfigThreadGlobals.h"
#include "ConfigurationStatus.h"
#include "Database/ConfigurationDatabase.h"
#include <winget/AsyncTokens.h>
#include <winget/ILifetimeWatcher.h>

#include <functional>
#include <mutex>
#include <string_view>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationProcessor : ConfigurationProcessorT<ConfigurationProcessor, winrt::cloaked<AppInstaller::WinRT::ILifetimeWatcher>>, AppInstaller::WinRT::LifetimeWatcherBase
    {
        using ConfigurationSetChangeData = Configuration::ConfigurationSetChangeData;
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using ApplyConfigurationSetResult = Configuration::ApplyConfigurationSetResult;
        using TestConfigurationSetResult = Configuration::TestConfigurationSetResult;
        using TestConfigurationUnitResult = Configuration::TestConfigurationUnitResult;
        using GetConfigurationUnitSettingsResult = Configuration::GetConfigurationUnitSettingsResult;
        using GetAllConfigurationUnitSettingsResult = Configuration::GetAllConfigurationUnitSettingsResult;
        using GetConfigurationSetDetailsResult = Configuration::GetConfigurationSetDetailsResult;
        using GetConfigurationUnitDetailsResult = Configuration::GetConfigurationUnitDetailsResult;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        ConfigurationProcessor();
#endif

        ConfigurationProcessor(const IConfigurationSetProcessorFactory& factory);

        event_token Diagnostics(const Windows::Foundation::EventHandler<IDiagnosticInformation>& handler);
        void Diagnostics(const event_token& token) noexcept;

        DiagnosticLevel MinimumLevel();
        void MinimumLevel(DiagnosticLevel value);

        hstring Caller() const;
        void Caller(hstring value);

        guid ActivityIdentifier();
        void ActivityIdentifier(const guid& value);

        bool GenerateTelemetryEvents();
        void GenerateTelemetryEvents(bool value);

        event_token ConfigurationChange(const Windows::Foundation::TypedEventHandler<Configuration::ConfigurationSet, Configuration::ConfigurationChangeData>& handler);
        void ConfigurationChange(const event_token& token) noexcept;

        Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet> GetConfigurationHistory();
        Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet>> GetConfigurationHistoryAsync();

        Configuration::OpenConfigurationSetResult OpenConfigurationSet(const Windows::Storage::Streams::IInputStream& stream);
        Windows::Foundation::IAsyncOperation<Configuration::OpenConfigurationSetResult> OpenConfigurationSetAsync(const Windows::Storage::Streams::IInputStream& stream);

        Windows::Foundation::Collections::IVector<ConfigurationConflict> CheckForConflicts(
            const Windows::Foundation::Collections::IVectorView<Configuration::ConfigurationSet>& configurationSets,
            bool includeConfigurationHistory);
        Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVector<ConfigurationConflict>> CheckForConflictsAsync(
            const Windows::Foundation::Collections::IVectorView<Configuration::ConfigurationSet>& configurationSets,
            bool includeConfigurationHistory);

        GetConfigurationSetDetailsResult GetSetDetails(const Configuration::ConfigurationSet& configurationSet, ConfigurationUnitDetailFlags detailFlags);
        Windows::Foundation::IAsyncOperationWithProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult> GetSetDetailsAsync(const Configuration::ConfigurationSet& configurationSet, ConfigurationUnitDetailFlags detailFlags);

        GetConfigurationUnitDetailsResult GetUnitDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags);
        Windows::Foundation::IAsyncOperation<GetConfigurationUnitDetailsResult> GetUnitDetailsAsync(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags);

        ApplyConfigurationSetResult ApplySet(const Configuration::ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);
        Windows::Foundation::IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> ApplySetAsync(const Configuration::ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);

        TestConfigurationSetResult TestSet(const Configuration::ConfigurationSet& configurationSet);
        Windows::Foundation::IAsyncOperationWithProgress<TestConfigurationSetResult, TestConfigurationUnitResult> TestSetAsync(const Configuration::ConfigurationSet& configurationSet);

        GetConfigurationUnitSettingsResult GetUnitSettings(const ConfigurationUnit& unit);
        Windows::Foundation::IAsyncOperation<GetConfigurationUnitSettingsResult> GetUnitSettingsAsync(const ConfigurationUnit& unit);

        GetAllConfigurationUnitSettingsResult GetAllUnitSettings(const ConfigurationUnit& unit);
        Windows::Foundation::IAsyncOperation<GetAllConfigurationUnitSettingsResult> GetAllUnitSettingsAsync(const ConfigurationUnit& unit);

        HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void ConfigurationSetProcessorFactory(const IConfigurationSetProcessorFactory& value);

        // Sends diagnostics objects to the event.
        void SendDiagnostics(DiagnosticLevel level, std::string_view message);

        // Sends diagnostics objects to the event.
        void SendDiagnostics(const IDiagnosticInformation& information);

        // Temporary entry point to enable experimental schema support.
        void SetSupportsSchema03(bool value);

        // Indicate a configuration change occurred.
        void ConfigurationChange(const Configuration::ConfigurationSet& set, const Configuration::ConfigurationChangeData& data);

    private:
        Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet> GetConfigurationHistoryImpl(AppInstaller::WinRT::AsyncCancellation cancellation = {});

        GetConfigurationSetDetailsResult GetSetDetailsImpl(
            const Configuration::ConfigurationSet& configurationSet,
            ConfigurationUnitDetailFlags detailFlags,
            AppInstaller::WinRT::AsyncProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult> progress = {});

        GetConfigurationUnitDetailsResult GetUnitDetailsImpl(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags);

        ApplyConfigurationSetResult ApplySetImpl(
            const Configuration::ConfigurationSet& configurationSet,
            ApplyConfigurationSetFlags flags,
            AppInstaller::WinRT::AsyncProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> progress = {});

        TestConfigurationSetResult TestSetImpl(
            const Configuration::ConfigurationSet& configurationSet,
            AppInstaller::WinRT::AsyncProgress<TestConfigurationSetResult, TestConfigurationUnitResult> progress = {});

        GetConfigurationUnitSettingsResult GetUnitSettingsImpl(const ConfigurationUnit& unit, AppInstaller::WinRT::AsyncCancellation cancellation = {});
        
        GetAllConfigurationUnitSettingsResult GetAllUnitSettingsImpl(const ConfigurationUnit& unit, AppInstaller::WinRT::AsyncCancellation cancellation = {});

        IConfigurationGroupProcessor GetSetGroupProcessor(const Configuration::ConfigurationSet& configurationSet);

        void SendDiagnosticsImpl(const IDiagnosticInformation& information);

        IConfigurationSetProcessorFactory m_factory = nullptr;
        event<Windows::Foundation::EventHandler<IDiagnosticInformation>> m_diagnostics;
        event<Windows::Foundation::TypedEventHandler<Configuration::ConfigurationSet, Configuration::ConfigurationChangeData>> m_configurationChange;
        ConfigThreadGlobals m_threadGlobals;
        IConfigurationSetProcessorFactory::Diagnostics_revoker m_factoryDiagnosticsEventRevoker;
        DiagnosticLevel m_minimumLevel = DiagnosticLevel::Informational;
        std::recursive_mutex m_diagnosticsMutex;
        ConfigurationDatabase m_database;
        bool m_isHandlingDiagnostics = false;
        // Temporary value to enable experimental schema support.
        bool m_supportSchema03 = true;
        std::shared_ptr<ConfigurationStatus::ChangeRegistration> m_changeRegistration;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ConfigurationProcessor : ConfigurationProcessorT<ConfigurationProcessor, implementation::ConfigurationProcessor>
    {
    };
}
#endif
