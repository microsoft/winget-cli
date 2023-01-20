// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationProcessor.g.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationProcessor : ConfigurationProcessorT<ConfigurationProcessor>
    {
        using ConfigurationSet = Configuration::ConfigurationSet;
        using ConfigurationUnit = Configuration::ConfigurationUnit;

        ConfigurationProcessor(const IConfigurationProcessorFactory& factory);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize();
#endif

        event_token Diagnostics(const Windows::Foundation::EventHandler<DiagnosticInformation>& handler);
        void Diagnostics(const event_token& token) noexcept;

        event_token ConfigurationChange(const Windows::Foundation::TypedEventHandler<ConfigurationSet, ConfigurationChangeData>& handler);
        void ConfigurationChange(const event_token& token) noexcept;

        Windows::Foundation::Collections::IVector<ConfigurationSet> GetConfigurationHistory();

        Configuration::OpenConfigurationSetResult OpenConfigurationSet(Windows::Storage::Streams::IInputStream stream);

        Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVectorView<ConfigurationConflict>> CheckForConflictsAsync(
            const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
            bool includeConfigurationHistory);

        Windows::Foundation::IAsyncAction GetSetDetailsAsync(const ConfigurationSet& configurationSet, const ConfigurationUnitDetailLevel& detailLevel);

        Windows::Foundation::IAsyncAction GetUnitDetailsAsync(const ConfigurationUnit& unit, const ConfigurationUnitDetailLevel& detailLevel);

        Windows::Foundation::IAsyncOperation<ApplyConfigurationSetResult> ApplyAsync(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags);

        Windows::Foundation::IAsyncOperation<TestConfigurationSetResult> TestAsync(const ConfigurationSet& configurationSet);

        Windows::Foundation::IAsyncOperation<GetConfigurationUnitSettingsResult> GetSettingsAsync(const ConfigurationUnit& unit);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        IConfigurationProcessorFactory m_factory = nullptr;
        event<Windows::Foundation::EventHandler<DiagnosticInformation>> m_diagnostics;
        event<Windows::Foundation::TypedEventHandler<ConfigurationSet, ConfigurationChangeData>> m_configurationChange;
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
