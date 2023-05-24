// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationProcessor.g.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include "ConfigThreadGlobals.h"
#include <winget/AsyncTokens.h>
#include <winget/ILifetimeWatcher.h>

#include <string_view>
#include <functional>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationProcessor : ConfigurationProcessorT<ConfigurationProcessor, AppInstaller::WinRT::ILifetimeWatcher>, AppInstaller::WinRT::LifetimeWatcherBase
    {
        using ConfigurationSet = Configuration::ConfigurationSet;
        using ConfigurationSetChangeData = Configuration::ConfigurationSetChangeData;
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using ApplyConfigurationSetResult = Configuration::ApplyConfigurationSetResult;
        using TestConfigurationSetResult = Configuration::TestConfigurationSetResult;
        using TestConfigurationUnitResult = Configuration::TestConfigurationUnitResult;
        using GetConfigurationUnitSettingsResult = Configuration::GetConfigurationUnitSettingsResult;
        using GetConfigurationSetDetailsResult = Configuration::GetConfigurationSetDetailsResult;
        using GetConfigurationUnitDetailsResult = Configuration::GetConfigurationUnitDetailsResult;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        ConfigurationProcessor() = default;
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

        event_token ConfigurationChange(const Windows::Foundation::TypedEventHandler<ConfigurationSet, ConfigurationChangeData>& handler);
        void ConfigurationChange(const event_token& token) noexcept;

        Windows::Foundation::Collections::IVector<ConfigurationSet> GetConfigurationHistory();
        Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVector<ConfigurationSet>> GetConfigurationHistoryAsync();

        Configuration::OpenConfigurationSetResult OpenConfigurationSet(const Windows::Storage::Streams::IInputStream& stream);
        Windows::Foundation::IAsyncOperation<Configuration::OpenConfigurationSetResult> OpenConfigurationSetAsync(const Windows::Storage::Streams::IInputStream& stream);

        Windows::Foundation::Collections::IVector<ConfigurationConflict> CheckForConflicts(
            const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
            bool includeConfigurationHistory);
        Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVector<ConfigurationConflict>> CheckForConflictsAsync(
            const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
            bool includeConfigurationHistory);

        GetConfigurationSetDetailsResult GetSetDetails(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel);
        Windows::Foundation::IAsyncOperationWithProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult> GetSetDetailsAsync(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel);

        void GetUnitDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel);
        Windows::Foundation::IAsyncAction GetUnitDetailsAsync(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel);

        ApplyConfigurationSetResult ApplySet(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);
        Windows::Foundation::IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> ApplySetAsync(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);

        TestConfigurationSetResult TestSet(const ConfigurationSet& configurationSet);
        Windows::Foundation::IAsyncOperationWithProgress<TestConfigurationSetResult, TestConfigurationUnitResult> TestSetAsync(const ConfigurationSet& configurationSet);

        GetConfigurationUnitSettingsResult GetUnitSettings(const ConfigurationUnit& unit);
        Windows::Foundation::IAsyncOperation<GetConfigurationUnitSettingsResult> GetUnitSettingsAsync(const ConfigurationUnit& unit);

        HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void ConfigurationSetProcessorFactory(const IConfigurationSetProcessorFactory& value);

        // Sends diagnostics objects to the event.
        void Diagnostics(DiagnosticLevel level, std::string_view message);

    private:
        GetConfigurationSetDetailsResult GetSetDetailsImpl(
            const ConfigurationSet& configurationSet,
            ConfigurationUnitDetailLevel detailLevel,
            AppInstaller::WinRT::AsyncProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult> progress = {});

        void GetUnitDetailsImpl(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel);

        ApplyConfigurationSetResult ApplySetImpl(
            const ConfigurationSet& configurationSet,
            ApplyConfigurationSetFlags flags,
            AppInstaller::WinRT::AsyncProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> progress = {});

        TestConfigurationSetResult TestSetImpl(
            const ConfigurationSet& configurationSet,
            AppInstaller::WinRT::AsyncProgress<TestConfigurationSetResult, TestConfigurationUnitResult> progress = {});

        GetConfigurationUnitSettingsResult GetUnitSettingsImpl(const ConfigurationUnit& unit, AppInstaller::WinRT::AsyncCancellation cancellation = {});

        IConfigurationSetProcessorFactory m_factory = nullptr;
        event<Windows::Foundation::EventHandler<IDiagnosticInformation>> m_diagnostics;
        event<Windows::Foundation::TypedEventHandler<ConfigurationSet, ConfigurationChangeData>> m_configurationChange;
        ConfigThreadGlobals m_threadGlobals;
        IConfigurationSetProcessorFactory::Diagnostics_revoker m_factoryDiagnosticsEventRevoker;
        DiagnosticLevel m_minimumLevel = DiagnosticLevel::Informational;
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
