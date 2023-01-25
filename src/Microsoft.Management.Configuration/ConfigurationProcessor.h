// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationProcessor.g.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include "ConfigThreadGlobals.h"

#include <string_view>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationProcessor : ConfigurationProcessorT<ConfigurationProcessor>
    {
        using ConfigurationSet = Configuration::ConfigurationSet;
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using DiagnosticInformation = Configuration::DiagnosticInformation;
        using ApplyConfigurationSetResult = Configuration::ApplyConfigurationSetResult;
        using TestConfigurationSetResult = Configuration::TestConfigurationSetResult;
        using GetConfigurationUnitSettingsResult = Configuration::GetConfigurationUnitSettingsResult;

        ConfigurationProcessor(const IConfigurationProcessorFactory& factory);

        event_token Diagnostics(const Windows::Foundation::EventHandler<DiagnosticInformation>& handler);
        void Diagnostics(const event_token& token) noexcept;

        event_token ConfigurationChange(const Windows::Foundation::TypedEventHandler<ConfigurationSet, ConfigurationChangeData>& handler);
        void ConfigurationChange(const event_token& token) noexcept;

        Windows::Foundation::Collections::IVector<ConfigurationSet> GetConfigurationHistory();

        Configuration::OpenConfigurationSetResult OpenConfigurationSet(Windows::Storage::Streams::IInputStream stream);

        Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVectorView<ConfigurationConflict>> CheckForConflictsAsync(
            const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
            bool includeConfigurationHistory);

        Windows::Foundation::IAsyncAction GetSetDetailsAsync(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel);

        Windows::Foundation::IAsyncAction GetUnitDetailsAsync(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel);

        Windows::Foundation::IAsyncOperation<ApplyConfigurationSetResult> ApplyAsync(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);

        Windows::Foundation::IAsyncOperation<TestConfigurationSetResult> TestAsync(const ConfigurationSet& configurationSet);

        Windows::Foundation::IAsyncOperation<GetConfigurationUnitSettingsResult> GetSettingsAsync(const ConfigurationUnit& unit);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        // Sends diagnostics objects to the event.
        void Diagnostics(DiagnosticLevel level, std::string_view message);

        Windows::Foundation::Collections::IVectorView<ConfigurationConflict> CheckForConflicts(
            const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
            bool includeConfigurationHistory);

        void GetSetDetails(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel);

        void GetUnitDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel);

        ApplyConfigurationSetResult Apply(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);

        TestConfigurationSetResult Test(const ConfigurationSet& configurationSet);

        GetConfigurationUnitSettingsResult GetSettings(const ConfigurationUnit& unit);

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
