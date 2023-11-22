// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>
#include "DscConfigurationSetProcessorFactory.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct DscConfigurationSetProcessor : winrt::implements<DscConfigurationSetProcessor, winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor>
    {
        DscConfigurationSetProcessor(winrt::Microsoft::Management::Configuration::ConfigurationSet const& configurationSet, winrt::weak_ref<DscConfigurationSetProcessorFactory> const& weakFactory);

        winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails GetUnitProcessorDetails(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit,
            winrt::Microsoft::Management::Configuration::ConfigurationUnitDetailFlags detailFlags);

        winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessor CreateUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationSet m_configurationSet;
        winrt::weak_ref<DscConfigurationSetProcessorFactory> m_weakFactory;
    };
}
