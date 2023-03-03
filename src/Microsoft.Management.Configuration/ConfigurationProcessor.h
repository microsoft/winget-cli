// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationProcessor.g.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include "ConfigThreadGlobals.h"

#include <string_view>
#include <functional>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationProcessor : ConfigurationProcessorT<ConfigurationProcessor>
    {
        using ConfigurationSet = Configuration::ConfigurationSet;
        using ConfigurationSetChangeData = Configuration::ConfigurationSetChangeData;
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using DiagnosticInformation = Configuration::DiagnosticInformation;
        using ApplyConfigurationSetResult = Configuration::ApplyConfigurationSetResult;
        using TestConfigurationSetResult = Configuration::TestConfigurationSetResult;
        using TestConfigurationUnitResult = Configuration::TestConfigurationUnitResult;
        using GetConfigurationUnitSettingsResult = Configuration::GetConfigurationUnitSettingsResult;

        ConfigurationProcessor(const IConfigurationProcessorFactory& factory);

        event_token Diagnostics(const Windows::Foundation::EventHandler<DiagnosticInformation>& handler);
        void Diagnostics(const event_token& token) noexcept;

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

        void GetSetDetails(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel);
        Windows::Foundation::IAsyncAction GetSetDetailsAsync(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel);

        void GetUnitDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel);
        Windows::Foundation::IAsyncAction GetUnitDetailsAsync(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel);

        ApplyConfigurationSetResult ApplySet(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);
        Windows::Foundation::IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> ApplySetAsync(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);

        TestConfigurationSetResult TestSet(const ConfigurationSet& configurationSet);
        Windows::Foundation::IAsyncOperationWithProgress<TestConfigurationSetResult, TestConfigurationUnitResult> TestSetAsync(const ConfigurationSet& configurationSet);

        GetConfigurationUnitSettingsResult GetUnitSettings(const ConfigurationUnit& unit);
        Windows::Foundation::IAsyncOperation<GetConfigurationUnitSettingsResult> GetUnitSettingsAsync(const ConfigurationUnit& unit);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        // Sends diagnostics objects to the event.
        void Diagnostics(DiagnosticLevel level, std::string_view message);

    private:
        IConfigurationProcessorFactory m_factory = nullptr;
        event<Windows::Foundation::EventHandler<DiagnosticInformation>> m_diagnostics;
        event<Windows::Foundation::TypedEventHandler<ConfigurationSet, ConfigurationChangeData>> m_configurationChange;
        ConfigThreadGlobals m_threadGlobals;
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
